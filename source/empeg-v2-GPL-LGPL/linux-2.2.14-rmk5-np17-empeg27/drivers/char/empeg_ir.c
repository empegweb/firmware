/*
 * empeg-car Infrared 205/70VR15 support
 *
 * (C) 1999 empeg ltd
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This driver supports infrared remote control devices on the
 * empegCar. It currently has support only for the Kenwood credit card
 * remote control (and presumably other Kenwood remotes) and a capture
 * mode to decipher the format of other remote controls.
 *
 * The repeat handling code is biased towards the Kenwood remote and
 * other controls may need a different approach.
 *
 * 1999/02/11 MAC Code tidied up and symbols made static.
 *
 * 1999/02/11 MAC Comments added. Now limitted to only one open.
 *
 * 1999/02/12 MAC Magic sequence support added. It doesn't actually
 *                work yet so it's disabled by default.
 *
 * 1999/03/01 MAC Added support for a repeat timeout. If a repeat code
 *                is received a long time after the previous repeat
 *                code/button code then it is ignored. The actual
 *                data for the repeat is now stored inside the remote
 *                specific function so that the repeat is for the
 *                correct control.
 *
 * 1999/06/02 MAC Generation of repeats are no longer cancelled in
 *                Kenwood handler if dodgy data is received. So we
 *                are relying on the timeout to stop the wrong code
 *                being repeated.
 *
 * 1999/07/03 MAC Various fixes in the IR handlers to reduce the
 *                chances of missed messages. Widened the
 *                acceptable values for the start sequence on
 *                button presses.
 *
 * 1999/07/04 MAC Added bounds checking on kenwood handler so that
 *                repeat codes are now much less likely to come
 *                from the sky.
 * */

/* Since we now use jiffies for the repeat handling we're assuming
   that the device won't be up for 497 days :-) */

#include <linux/sched.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <asm/ptrace.h>
#include <asm/fiq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/system.h>
#include <asm/arch/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/empeg.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include "empeg_ir.h"

#define MS_TO_JIFFIES(MS) ((MS)/(1000/HZ))
#define JIFFIES_TO_MS(J) (((J)*1000)/HZ)
#define US_TO_TICKS(US) ((368 * (US))/100)
#define TICKS_TO_US(T) ((100 * (T))/368)

#define IR_DEBUG 0

#define USE_TIMING_QUEUE 1

/* The CS4231 in the Mk2 uses FIQs */
#ifndef CONFIG_EMPEG_CS4231
#define USE_TIMING_QUEUE_FIQS 1
#else
#define USE_TIMING_QUEUE_FIQS 0
#endif

#define IR_TYPE_DEFAULT IR_TYPE_KENWOOD

/* Delay before repeating default */
#define IR_RPTDELAY_DEFAULT MS_TO_JIFFIES(300) /* .3 seconds */

/* Delay between repeats default */
#define IR_RPTINT_DEFAULT MS_TO_JIFFIES(100) /*.1 seconds */

/* Timeout for getting a repeat code and still repeating */
#define IR_RPTTMOUT_DEFAULT MS_TO_JIFFIES(500) /* 0.5 seconds */

/* Freeing up the IRQ breaks on Henry due to the IRQ hack. This means
 * that we can't request and free on open and close. Ultimately we
 * should probably do so but it needs testing on Sonja first.
 */   
#define REQUEST_IRQ_ON_OPEN 0

/* Each button press when in one of the control specific modes
 * generates a 4 byte code. When in capture mode each transition
 * received generates a 4 byte code.
 *
 * Therefore, this buffer has room for 256 keypresses or
 * transitions. This should be enough since capturing is only meant as
 * a diagnostic and development feature and therefore should not be
 * attempted if you can't guarantee reading it often enough.
 */
#define IR_BUFFER_SIZE 256

/* The type used for IR codes returned to the user process. */
typedef __u32 ir_code;

/* We only support one device */
struct ir_dev
{
#if USE_TIMING_QUEUE
	volatile int timings_used;	/* Do not move */
        volatile int timings_free;	/* Do not move */
        volatile int timings_head;      /* Do not move */
	volatile int timings_tail;      /* Do not move */
	volatile unsigned long *timings_buffer;     /* Do not move */
#endif
	
	ir_code *buf_start;
	ir_code *buf_end;
	ir_code *buf_wp;
	ir_code *buf_rp;

	struct wait_queue *wq;          /* Blocking queue */
	struct tq_struct timer;         /* Timer queue */
	
	int ir_type;
	unsigned long repeat_delay_jiffies;
	unsigned long repeat_interval_jiffies;
	unsigned long repeat_timeout_jiffies;

	/* Repeat support */
	unsigned long last_repeat_jiffies;
	unsigned long last_new_data_jiffies;

	/* Statistics */
	unsigned long count_valid;
	unsigned long count_repeat;
	unsigned long count_badrepeat;
	unsigned long count_spurious;
	unsigned long count_malformed;
	unsigned long count_missed;
#if USE_TIMING_QUEUE
	unsigned long timings_hwm;
#endif
};

static struct ir_dev ir_devices[1];

/* Used to disallow multiple opens. */
static int users = 0;
static struct fiq_handler fh= { NULL, "empeg_ir", NULL, NULL };

/* Bottom bit must be clear for switch statement */
#define IR_STATE_IDLE 0x00
/*#define IR_STATE_RECOVER 0x02*/
#define IR_STATE_START1 0x04
#define IR_STATE_START2 0x06
#define IR_STATE_START3 0x08
#define IR_STATE_DATA1  0x0a
#define IR_STATE_DATA2 0x0c

static void ir_append_data(struct ir_dev *dev, ir_code data)
{
	/* Now this is called from the bottom half we need to make
	   sure that noone else is fiddling with stuff while we
	   do it. */
	ir_code *new_wp;
	unsigned long flags;
	save_flags_cli(flags);
	
	new_wp = dev->buf_wp + 1;
	if (new_wp == dev->buf_end)
		new_wp = dev->buf_start;
	
	if (new_wp != dev->buf_rp)
	{
		*dev->buf_wp = data;
		dev->buf_wp = new_wp;
		/* Now we've written, wake up anyone who's reading */
		wake_up_interruptible(&dev->wq);
	}
#if IR_DEBUG
	else
		printk("Infra-red buffer is full.\n");
#endif
	restore_flags(flags);
}

static void ir_append_data_repeatable(struct ir_dev *dev,
				      ir_code data)
{
	unsigned long now = jiffies;
	dev->last_new_data_jiffies = now;
	dev->last_repeat_jiffies = 0; /* Make it look like a long time ago */
	
	ir_append_data(dev, data);
#ifdef SUPPORT_MAGIC
	ir_handle_magic(data);
#endif
}

/* Append a repetition of the last data. This is only done if the
 * repetition is long enough after the initial button press and if the
 * last repetition was long enough ago.
 *
 * Also, the repeat must have been within the repeat timeout since the
 * last repeat.
 */
   
static void ir_append_data_repeat(struct ir_dev *dev, ir_code data)
{
	unsigned long now_jiffies = jiffies;

	unsigned long since_new_jiffies;
	unsigned long since_repeat_jiffies;

	if (dev->last_repeat_jiffies)
		since_repeat_jiffies = now_jiffies - dev->last_repeat_jiffies;
	else
		since_repeat_jiffies = 100000; /* A long time ago */

	since_new_jiffies = now_jiffies - dev->last_new_data_jiffies;
	
	if ((since_repeat_jiffies < dev->repeat_timeout_jiffies
	     || since_new_jiffies < dev->repeat_timeout_jiffies)
	    && since_repeat_jiffies > dev->repeat_interval_jiffies) {
		if (since_new_jiffies > dev->repeat_delay_jiffies) {
			dev->last_repeat_jiffies = now_jiffies;
			ir_append_data(dev, data);
#if IR_DEBUG
			printk("Doing repeat. timings are:\n");
			printk("  Time of last new %ld.\n", dev->last_new_data_jiffies);
			printk("  Time of last repeat %ld.\n", dev->last_repeat_jiffies);
			printk("  Since new %ld.\n", since_new_jiffies);
			printk("  Since repeat %ld.\n", since_repeat_jiffies);
#endif
		}
	}
#if IR_DEBUG
	else
		printk(".");
#endif
}

struct recent_entry {
	int state;
	int interval;
};
#define ENTRY_COUNT 32

#if 0
static void dump_entries(struct recent_entry *e, int start, int end)
{
	int i = start;
	printk("State level interval\n");
	while (i != end)
	{
		printk("%5x %5d %8d\n", e[i].state & ~1, e[i].state & 1, e[i].interval);
		++i;
		if (i >= ENTRY_COUNT)
			i = 0;
	}
}
#endif

static inline void ir_buttons_interrupt(struct ir_dev *dev, int level,
					unsigned long span)
{
	static int state = IR_STATE_IDLE;
	static int unit_time = 1; /* not zero in case we accidentally use it */
	static unsigned short bit_position = 0;
	static __u8 data = 0;
	
	static int entry_rp = 0, entry_wp = 0;
	static struct recent_entry entries[ENTRY_COUNT];

	/* Check to see if the last interrupt was so long ago that
	 * we should restart the state machine.
	 */

	if (span >= US_TO_TICKS(40000)) {
		bit_position = 0;
		state = IR_STATE_IDLE;
	}

	entries[entry_wp].state = state | (level ? 1 : 0);
	entries[entry_wp].interval = span;

	entry_wp++;
	if (entry_wp >= ENTRY_COUNT)
		entry_wp = 0;

	if (entry_wp == entry_rp) {
		entry_rp++;
		if (entry_rp >= ENTRY_COUNT)
			entry_rp = 0;
	}

#if 0
#define ON_RECOVER dump_entries(entries, entry_rp, entry_wp)
#define ON_VALID entry_rp = entry_wp = 0
#else
#define ON_RECOVER
#define ON_VALID
#endif
	
	//old_state = state | (level ? 1 : 0);
	/* Now we can actually do something */

#ifdef DEBUG_BUTTONS
	{
		int result = (1<<31);
		if (level)
		    result |= (1<<30);
		result |= ((state & 0xF) << 26);
		result |= (span & 0x3ffff);
		ir_append_data(dev, result);
	}
#endif

			
retry:
	switch(state | (level ? 1 : 0))
	{
	case IR_STATE_IDLE | 1:
		/* Going high in idle doesn't mean anything */
		break;
		
	case IR_STATE_IDLE | 0:
		/* Going low in idle is the start of a start sequence */
		state = IR_STATE_START1;
		break;

	case IR_STATE_START1 | 1:
		/* Going high, that should be after 4T */
		unit_time = span / 4;
		if (unit_time > US_TO_TICKS(50) && unit_time < US_TO_TICKS(550))
		//if (unit_time > US_TO_TICKS(150) && unit_time < US_TO_TICKS(350)) << original
		//if (unit_time > 225 && unit_time < 275)
			state = IR_STATE_START2;
		else
			state = IR_STATE_IDLE; /* There's no point in recovering immediately
						  since this can't be the start of a new
						  sequence. */
		break;

	case IR_STATE_START1 | 0:
		/* Shouldn't ever go low in START1, recover */
		/*state = IR_STATE_IDLE;*/
		/* We jump straight back to START1 since this might still */
		/* be the start of a sequence */
		ON_RECOVER;
		state = IR_STATE_IDLE;
		++dev->count_missed;
		goto retry;

	case IR_STATE_START2 | 1:
		/* Shouldn't ever go high in START2, recover */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;

	case IR_STATE_START2 | 0:
		/* If this forms the end of the start sequence then we
		 * should have been high for around 8T time.
		 */
		if (span > 7 * unit_time && span < 9 * unit_time) {
			/* It's the start of one of the unit button codes. */
			data = 0;
			bit_position = 0;
			state = IR_STATE_DATA1;
		} else {
			/* We're out of bounds. Give up, but this might be the
			   start of a valid start sequence. */
			ON_RECOVER;
			state = IR_STATE_IDLE;
			goto retry; /* try again */
		}
		break;

	case IR_STATE_DATA1 | 0:
		/* This should never happen */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;
		
	case IR_STATE_DATA1 | 1:
		data <<= 1;
		if (span <= 3 * unit_time) {
			/* It's a zero bit */
			bit_position++;
			state = IR_STATE_DATA2;
		} else if (span > 3 * unit_time && span < 5 * unit_time) {
			/* It's a one bit */
			data |= 1;
			bit_position++;
			state = IR_STATE_DATA2;
		} else {
			/* This can't be the start of a start sequence since
			   we're going high, so just give up */
			ON_RECOVER;
			state = IR_STATE_START1;
			goto retry;
		}

		/* Now process the bit */
		if (bit_position > 7) {
			/* Does it pass the validity check */
			if (((data>>4)^(data&0xf))==0xf)
			{
				/* We don't do anything with repeats so go
				   straight to the real code */
				ir_append_data(dev, data >> 4);
				ON_VALID;
			        state = IR_STATE_IDLE;
			}
			else
			{
				/* Report CRC failures */
				//ir_append_data(dev, tv_now, 0xFF00 | data);
				ON_RECOVER;
				state = IR_STATE_START1;
				goto retry;
			}
		}
		break;

	case IR_STATE_DATA2 | 0:
		if (span < 2 * unit_time) {
			/* It's a correct inter-bit gap */
			state = IR_STATE_DATA1;
		} else {
			ON_RECOVER;
			/* It's out of bounds. It might be the start
			   of another valid sequence so try again. */
			state = IR_STATE_IDLE;
			goto retry;
		}
		break;

	case IR_STATE_DATA2 | 1:
		/* Should never get here */
		ON_RECOVER;
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;
		
	default:
#if IR_DEBUG
		printk("Buttons handler got into impossible state. Recovering.\n");
		state = IR_STATE_IDLE;
#endif
		break;
	}
}

static inline void ir_kenwood_interrupt(struct ir_dev *dev, int level,
					unsigned long span)
{
	static int state = IR_STATE_IDLE;
	static int unit_time = 1; /* not zero in case we accidentally use it */
	static unsigned short bit_position = 0;
	static __u32 data = 0;
	static ir_code decoded_data = 0;
	static int repeat_valid = FALSE;
	//static struct timeval tv_last_valid;
	
	/* Check to see if the last interrupt was so long ago that
	 * we should restart the state machine.
	 */

	if (span >= US_TO_TICKS(40000)) {
		bit_position = 0;
		state = IR_STATE_IDLE;
	}

	/* Now we can actually do something */

 retry:
	switch(state | (level ? 1 : 0))
	{
	case IR_STATE_IDLE | 1:
		/* Going high in idle doesn't mean anything */
		break;
		
	case IR_STATE_IDLE | 0:
		/* Going low in idle is the start of a start sequence */
		state = IR_STATE_START1;
		break;

	case IR_STATE_START1 | 1:
		/* Going high, that should be after 8T */
		unit_time = span / 8;
		/* But some bounds on it so we don't start receiving magic
		   codes from the sky */
		if (unit_time > US_TO_TICKS(500) && unit_time < US_TO_TICKS(1500))
			state = IR_STATE_START2;
		else
			state = IR_STATE_IDLE;
		break;

	case IR_STATE_START1 | 0:
		/* Shouldn't ever go low in START1, recover */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;

	case IR_STATE_START2 | 1:
		/* Shouldn't ever go low in START2, recover */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;

	case IR_STATE_START2 | 0:
		/* If this forms the end of the start sequence then we
		 * should have been high for around 4T time.
		 */
		if ((span >= 3 * unit_time) &&
			(span < 5 * unit_time)) {
			state = IR_STATE_START3;
			/*repeat_valid = FALSE;*/
		} else if (span > unit_time && span < 3 * unit_time) {
			/* This means that the last code is repeated - just
			   send out the last code again with the top bit set to indicate
			   a repeat. */
			if (repeat_valid)
			{
				ir_append_data_repeat(dev, (1<<31) | decoded_data);
				++dev->count_repeat;
			}
			else
			{
				++dev->count_badrepeat;
			}
			state = IR_STATE_IDLE;
		} else {
			/* We're out of bounds. Recover */
			state = IR_STATE_IDLE;
			/* But it could be the start of a new sequence
                           so try again */
			++dev->count_spurious;
			goto retry;
		}
		break;

	case IR_STATE_START3 | 0:
		/* Shouldn't happen */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;

	case IR_STATE_START3 | 1:
		/* Data will follow this */
		if (span < unit_time) {
			bit_position = 0;
			data = 0;
			state = IR_STATE_DATA1;
		} else {
			/* We're out of bounds. It might be the start
			   of a new sequence so try it again. */
			state = IR_STATE_IDLE;
			++dev->count_spurious;
			goto retry;
		}
		break;
		
	case IR_STATE_DATA1 | 1:
		/* Shouldn't get this. Recover */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;

	case IR_STATE_DATA1 | 0:
		/* The actual data bit is encoded in the length of this.
		 */
		if (span < unit_time) {
			/* It's a zero */
			bit_position++;
			state = IR_STATE_DATA2;
		} else if (span < 2 * unit_time) {
			/* It's a one */
			data |= (1<<bit_position);
			bit_position++;
			state = IR_STATE_DATA2;
		} else {
			/* Not valid. It might be the start of a new
                           sequence though. */
			state = IR_STATE_IDLE;
			++dev->count_spurious;
			goto retry;
		}
		break;

	case IR_STATE_DATA2 | 1:
		/* This marks the end of the post-data space
		 * It is a consistent length
		 */
		if (span < unit_time) {
			/* It's a valid space */
			if (bit_position >= 32) {
				__u16 data1, data2;
				
				data1 = ((data >> 16) & 0xFF00) | ((data >> 8) & 0xFF);
				data2 = ((data >> 8) & 0xFF00) | ((data & 0xFF));
				
				/* We've finished getting data, confirm
				   it passes the validity check */
				if (data1 == ((__u16)(~data2))) {
					decoded_data = be16_to_cpu(data2);
					ir_append_data_repeatable(dev, decoded_data);
					repeat_valid = TRUE;
					++dev->count_valid;
				} else {
#if IR_DEBUG
					printk("Got an invalid sequence %08lx (%04lx, %04lx)\n",
					       (unsigned long)data, (unsigned long)data1,
					       (unsigned long)data2);
					printk("(%04lx %04lx, %04lx %04lx)\n",
					       (unsigned long)data1, (unsigned long)~data1,
					       (unsigned long)data2, (unsigned long)~data2);
#endif
					++dev->count_malformed;
				}
				state = IR_STATE_IDLE;
			}
			else
				state = IR_STATE_DATA1;
		} else {
			/* It's too long to be valid. Give up and try again. */
			state = IR_STATE_IDLE;
			++dev->count_spurious;
			goto retry;
		}
		break;

	case IR_STATE_DATA2 | 0:
		/* Shouldn't get this. Recover */
		state = IR_STATE_IDLE;
		++dev->count_missed;
		break;

	default:
		state = IR_STATE_IDLE;
		break;
	}
}

static inline void ir_capture_interrupt(struct ir_dev *dev, int level,
					unsigned long span)
{
	unsigned long us = TICKS_TO_US(span);
	/* Just capture it straight to the output buffer */
	if (level)
		ir_append_data(dev, us | (1<<30) | (1<<31));
	else
		ir_append_data(dev, (us & ~(1<<30)) | (1<<31));
}

static inline void ir_transition(struct ir_dev *dev, int level, unsigned long span)
{
	/* Call buttons interrupt handler */
	if (dev->ir_type != IR_TYPE_CAPTURE)
		ir_buttons_interrupt(dev, level, span);
	
	/* Call remote specific interrupt handler */

	switch (dev->ir_type)
	{
	case IR_TYPE_CAPTURE:
		ir_capture_interrupt(dev, level, span);
		break;
	case IR_TYPE_KENWOOD:
		ir_kenwood_interrupt(dev, level, span);
		break;
	default:
		/* Hmm, I wonder what it was supposed to be */
		dev->ir_type = IR_TYPE_CAPTURE;
		break;
	}
}

#if USE_TIMING_QUEUE

#if USE_TIMING_QUEUE_FIQS
static void ir_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/* Not needed if we're running on FIQs */
	printk("BAD!\n");
}
#else
static void ir_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int level;
	unsigned long now; 
	unsigned long entry;
	unsigned long flags;
	struct ir_dev *dev = ir_devices;
	
	save_flags_cli(flags);

	level = (GPLR & EMPEG_IRINPUT)?1:0;
	now = OSCR;

	entry = (now & ~1) | level;
	
	if (dev->timings_free) {
		--dev->timings_free;
		dev->timings_buffer[dev->timings_head] = entry;
		if (++dev->timings_head >= TIMINGS_BUFFER_SIZE)
			dev->timings_head = 0;
		++dev->timings_used;
	}

	restore_flags(flags);
	
}
#endif

static void ir_check_buffer(void *dev_id)
{
	/* This stores the time of the last actual interrupt, not the time this
	 * routine was called
	 */
	static unsigned long last_interrupt;
	
	struct ir_dev *dev = dev_id;

	while (dev->timings_used) {
		unsigned long flags;
		unsigned long entry;
		unsigned long span;
		unsigned long interrupt_time;
		int level;

		/* Keep track of the hwm */
		{
			int used = dev->timings_used;
			if (used > dev->timings_hwm)
				dev->timings_hwm = used;
		}

		/* Safe to do even if an interrupt happens during it. */
		entry = dev->timings_buffer[dev->timings_tail];
		
		/* Disable interrupts while we tidy up the pointers */
		save_flags_clif(flags);
		++dev->timings_free;
		--dev->timings_used;
		restore_flags(flags);

		if (++dev->timings_tail >= TIMINGS_BUFFER_SIZE)
			dev->timings_tail = 0;

		/* Now, we have our entry, go and deal with it. */
		interrupt_time = entry & ~1;
		level = entry & 1;

		span = interrupt_time - last_interrupt;
		last_interrupt = interrupt_time;

		//printk("Transition(%d): %d %5ld\n", dev->timings_tail, level, span);
		ir_transition(dev, level, span);
	}

	/* Requeue me */
	queue_task(&dev->timer, &tq_timer);
}

#else

static inline void ir_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int level;
	static unsigned long last_interrupt;
	unsigned long now;
	unsigned long span;
	struct ir_dev *dev = dev_id;

	level = (GPLR & EMPEG_IRINPUT) > 0;

	now = OSCR;
	span = now - last_interrupt;
	last_interrupt = now;

	ir_transition(dev, level, span);
}
#endif

static int ir_open(struct inode *inode, struct file *filp)
{
	struct ir_dev *dev = ir_devices;

	if (users)
		return -EBUSY;

	users++;
	MOD_INC_USE_COUNT;
	
	/* This shouldn't be necessary, but there's something (IDE, audio?)
	 * that's setting rather than or'ing these and breaking it after
	 * initialisation.
	 */
	GRER|=EMPEG_IRINPUT;
	GFER|=EMPEG_IRINPUT;

	dev->ir_type = IR_TYPE_DEFAULT;
	dev->repeat_delay_jiffies = IR_RPTDELAY_DEFAULT;
	dev->repeat_interval_jiffies = IR_RPTINT_DEFAULT;
	dev->repeat_timeout_jiffies = IR_RPTTMOUT_DEFAULT;
	filp->private_data = dev;
	
	return 0;
}

static int ir_release(struct inode *inode, struct file *filp)
{
	--users;
	MOD_DEC_USE_COUNT;
	return 0;
}

/*
 * Read some bytes from the IR buffer. The count should be a multiple
 * of four bytes - if it isn't then it is rounded down. If the
 * rounding means that it is zero then EINVAL will result.
 */
static ssize_t ir_read(struct file *filp, char *dest, size_t count,
		       loff_t *ppos)
{
	struct ir_dev *dev = filp->private_data;
	int n;

	while (dev->buf_rp == dev->buf_wp) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		interruptible_sleep_on(&dev->wq);
		/* If the sleep was terminated by a signal give up */
		if (signal_pending(current))
			return -ERESTARTSYS;
	}

	/* Only allow reads of a multiple of four bytes. Anything else
           is meaningless and may cause us to get out of sync */
	count >>= 2;

	/* Not sure if this is a valid thing to be doing */
	if (!count)
		return -EINVAL;
	
	n = 0;
	while (count--) {
		ir_code data;

		if (dev->buf_rp == dev->buf_wp)
			return n;

		/* Need to put one byte at a time since there are no
		   alignment requirements on parameters to read. */
		data = *dev->buf_rp;
		__put_user(data & 0xFF, dest++);
		__put_user((data >> 8) & 0xFF, dest++);
		__put_user((data >> 16) & 0xFF, dest++);
		__put_user((data >> 24) & 0xFF, dest++);
		dev->buf_rp++;
		if (dev->buf_rp == dev->buf_end)
			dev->buf_rp = dev->buf_start;

		n += 4;
	}

	return n;
}

unsigned int ir_poll(struct file *filp, poll_table *wait)
{
	struct ir_dev *dev = filp->private_data;

	/* Add ourselves to the wait queue */
	poll_wait(filp, &dev->wq, wait);

	/* Check if we've got data to read */
	if (dev->buf_rp != dev->buf_wp)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

int ir_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
	     unsigned long arg)
{
	struct ir_dev *dev = filp->private_data;

	switch(cmd)
	{
	case EMPEG_IR_WRITE_TYPE:
		if (arg >= IR_TYPE_COUNT)
			return -EINVAL;
		dev->ir_type = arg;
		return 0;
		
	case EMPEG_IR_READ_TYPE:
		return put_user(dev->ir_type, (int *)arg);
		
	case EMPEG_IR_WRITE_RPTDELAY:
		dev->repeat_delay_jiffies = MS_TO_JIFFIES(arg);
		return 0;

	case EMPEG_IR_READ_RPTDELAY:
		return put_user(JIFFIES_TO_MS(dev->repeat_delay_jiffies), (unsigned long *)arg);
		
	case EMPEG_IR_WRITE_RPTINT:
		dev->repeat_interval_jiffies = MS_TO_JIFFIES(arg);
		return 0;

	case EMPEG_IR_READ_RPTINT:
		return put_user(JIFFIES_TO_MS(dev->repeat_interval_jiffies), (unsigned long *)arg);
		
	case EMPEG_IR_WRITE_RPTTMOUT:
		dev->repeat_timeout_jiffies = MS_TO_JIFFIES(arg);
		return 0;

	case EMPEG_IR_READ_RPTTMOUT:
		return put_user(JIFFIES_TO_MS(dev->repeat_timeout_jiffies), (unsigned long *)arg);
		
	default:
		return -EINVAL;
	}
}

static struct file_operations ir_fops = {
	NULL, /* ir_lseek */
	ir_read,
	NULL, /* ir_write */
	NULL, /* ir_readdir */
	ir_poll, /* ir_poll */
	ir_ioctl,
	NULL, /* ir_mmap */
	ir_open,
	NULL, /* ir_flush */
	ir_release,
};

int ir_read_procmem(char *buf, char **start, off_t offset, int len, int unused)
{
	struct ir_dev *dev = ir_devices;
	len = 0;

	len += sprintf(buf+len, "Valid sequences:      %ld\n", dev->count_valid);
	len += sprintf(buf+len, "Repeated sequences:   %ld\n", dev->count_repeat);
	len += sprintf(buf+len, "Unfulfilled repeats:  %ld\n", dev->count_badrepeat);
	len += sprintf(buf+len, "Malformed sequences:  %ld\n", dev->count_malformed);
	len += sprintf(buf+len, "Spurious transitions: %ld\n", dev->count_spurious);
	len += sprintf(buf+len, "Missed interrupts:    %ld\n", dev->count_missed);
	len += sprintf(buf+len, "Timings buffer hwm:   %ld\n", dev->timings_hwm);

	return len;
}

struct proc_dir_entry ir_proc_entry = {
	0,			/* inode (dynamic) */
	8, "empeg_ir",  	/* length and name */
	S_IFREG | S_IRUGO, 	/* mode */
	1, 0, 0, 		/* links, owner, group */
	0, 			/* size */
	NULL, 			/* use default operations */
	&ir_read_procmem, 	/* function used to read data */
};

void __init empeg_ir_init(void)
{
	struct ir_dev *dev = ir_devices;
	int result;
#if USE_TIMING_QUEUE_FIQS
	struct pt_regs regs;
	extern char empeg_ir_fiq, empeg_ir_fiqend;
#endif

	result = register_chrdev(EMPEG_IR_MAJOR, "empeg_ir", &ir_fops);
	if (result < 0) {
		printk(KERN_WARNING "empeg IR: Major number %d unavailable.\n",
			   EMPEG_IR_MAJOR);
		return;
	}

	/* First grab the memory buffer */
	dev->buf_start = vmalloc(IR_BUFFER_SIZE * sizeof(ir_code));
	if (!dev->buf_start) {
		printk(KERN_WARNING "Could not allocate memory buffer for empeg IR.\n");
		return;
	}
	
	dev->buf_end = dev->buf_start + IR_BUFFER_SIZE;
	dev->buf_rp = dev->buf_wp = dev->buf_start;
	dev->wq = NULL;
	dev->ir_type = IR_TYPE_CAPTURE;

	dev->count_valid = 0;
	dev->count_repeat = 0;
	dev->count_badrepeat = 0;
	dev->count_spurious = 0;
	dev->count_malformed = 0;
	dev->count_missed = 0;

#if USE_TIMING_QUEUE
	dev->timings_hwm = 0;

	dev->timings_free = TIMINGS_BUFFER_SIZE;
	dev->timings_used = 0;
	dev->timings_head = 0;
	dev->timings_tail = 0;
	dev->timings_buffer = vmalloc(TIMINGS_BUFFER_SIZE * sizeof(unsigned long));
	
	/* Set up timer routine to check the buffer */
	dev->timer.sync = 0;
	dev->timer.routine = ir_check_buffer;
	dev->timer.data = dev;
	queue_task(&dev->timer, &tq_timer);

#if USE_TIMING_QUEUE_FIQS
	/* Install FIQ handler */
	regs.ARM_r9=(int)dev;
	regs.ARM_r10=(int)&OSCR; 	
	regs.ARM_fp=0; 		/* r11 */
	regs.ARM_ip=0;	 	/* r12 */
	regs.ARM_sp=(int)&GPLR;
	set_fiq_regs(&regs);

	set_fiq_handler(&empeg_ir_fiq,(&empeg_ir_fiqend-&empeg_ir_fiq));
	claim_fiq(&fh);
#endif
#endif

	/* IRQs shouldn't be reenabled, the routine is very fast */
	result = request_irq(EMPEG_IRQ_IR, ir_interrupt, SA_INTERRUPT,
			     "empeg_ir", dev);
	
	if (result != 0) {
		printk(KERN_ERR "Can't get empeg IR IRQ %d.\n", EMPEG_IRQ_IR);
		return;
	}

#if USE_TIMING_QUEUE_FIQS
	/* It's a FIQ not an IRQ */
	ICLR|=EMPEG_IRINPUT;

	/* Enable FIQs: there should be a neater way of doing
	   this... */
	{
		unsigned long flags;
		save_flags(flags);
		flags&=~F_BIT;
		restore_flags(flags);
	}
#endif
  
       	/* We want interrupts on rising and falling */
	GRER|=EMPEG_IRINPUT;
	GFER|=EMPEG_IRINPUT;
	
#ifdef CONFIG_PROC_FS
	proc_register(&proc_root, &ir_proc_entry);
#endif

#if USE_TIMING_QUEUE_FIQS
	printk("empeg infra-red support initialised (Using FIQs).\n");
#else
	printk("empeg infra-red support initialised.\n");
#endif
}

static inline void empeg_ir_cleanup(void)
{
	int result;
	struct ir_dev *dev = ir_devices;

	free_irq(EMPEG_IRQ_IR, dev);

	/* No longer require interrupts */
	GRER&=~(EMPEG_IRINPUT);
	GFER&=~(EMPEG_IRINPUT);

	result = unregister_chrdev(EMPEG_IR_MAJOR, "empeg_ir");
	if (result < 0)
		printk(KERN_WARNING "empeg IR: Unable to unregister device.\n");
	printk("empeg IR cleanup complete.\n");
}

#ifdef MODULE
MODULE_AUTHOR("Mike Crowe");
MODULE_DESCRIPTION("A driver for the empeg Infrared remote control");
MODULE_SUPPORTED_DEVICE("ir");

EXPORT_NO_SYMBOLS

int init_module(void)
{
	return empeg_ir_init();
}

void cleanup_module(void)
{
	empeg_ir_cleanup();
}

#endif /* MODULE */
