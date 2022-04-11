/* empeg power-pic support
 *
 * (C)2000 empeg ltd, http://www.empeg.com
 *
 * Author:
 *   Hugo Fiennes, <hugo@empeg.com>
 *
 * The power-pic is a PIC12C508A in the empeg Mk2's power supply. This chip
 * powered by the permanent 12v supply line when the unit is used in-car.
 * With only the power-pic running (and the RTC power capacitor charging), the
 * empeg-car takes around 1mA.
 *
 * In normal circumstances, the power-pic waits for the accessory line of the
 * car to go high, then turns on the power supply to the main system. When
 * accessory goes low, it's up to the main system to turn itself off.
 *
 * Note that it's actually impossible for the main system to turn itself off
 * if the accessory line is high - the power will be turned on instantly again.
 *
 * When the accessory line is low, the power-pic runs a timer which can cause
 * the main system to get powered up at preset intervals. As the pic is running
 * with its internal RC oscillator (vaguely 4Mhz) this power up timer isn't
 * very accurate, and can drift by as much as 30% in either direction. The
 * timer can be set to never wake the main system, or wake it up after 'n' 15
 * second units of time have elapsed since power off (n ranges from 1 to 253,
 * giving up to an hour of powered down time before powering up again).
 *
 * The secret to accurate wakeup alarms is that when the system wakes up, it
 * checks the real time clock, then can set another alarm before powering off
 * again - iteratively approaching the wakeup time.
 *
 * If there are no events pending when the system wakes up, it just powers
 * itself off again. Generally, this brief flash of power lasts no more than
 * 300ms (using 200mA - an average hourly drain of 16uAH).
 *
 * The power pic, being connected to the permanent 5v supply, can keep track of
 * the first power up after main power has been applied: this is useful for
 * (eg) PIN numbers which will only get requested when the unit has been
 * removed from the car and not every time ACC goes high and powers the main
 * system.
 *
 * You can read the current power states from this driver with ioctls, and also
 * from /proc/empeg_power
 *
 * 2000/03/15 HBF First version
 * 2000/05/24 HBF Added ioctls
 * 
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/init.h>

#include <asm/byteorder.h>
#include <asm/irq.h>
#include <asm/fiq.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>

#include "empeg_power.h"

/* Only one power pic */
struct power_dev power_devices[1];

/* First boot state */
static int power_firstboot=0;

static struct file_operations power_fops = {
  NULL, /* power_lseek */
  NULL, /* power_read */
  NULL, /* power_write */
  NULL, /* power_readdir */
  NULL, /* power_poll */
  power_ioctl,
  NULL, /* power_mmap */
  power_open,
  NULL, /* power_flush */
  power_release,
};

/* Actual communication routine */
static void powercontrol(int b)
{
	/* Send a command to the mk2's power control PIC */
	int bit;
	unsigned long flags;

	/* Need to do this with IRQs disabled to preserve timings */
	save_flags_cli(flags);

	/* Starts with line high, plus a delay to ensure the PIC has noticed */
	GPSR=EMPEG_POWERCONTROL;
	udelay(100);

	/* Send 8 bits */
	for(bit=7;bit>=0;bit--) {
		/* Set line low */
		GPCR=EMPEG_POWERCONTROL;

		/* Check the bit */
		if (b&(1<<bit)) {
			/* High - 20us of low */
			udelay(20);
		} else {
			/* Low - 4us of low */
			udelay(4);
		}

		/* Set line high */
		GPSR=EMPEG_POWERCONTROL;

		/* Inter-bit delay */
	        udelay(20);
	}

	/* End of transmission, line low */
	GPCR=EMPEG_POWERCONTROL;

	/* Reenable IRQs */
	restore_flags(flags);
}

/* First boot time? */
int empeg_power_firstboot(void)
{
	return(power_firstboot);
}

/* Device initialisation */
void __init empeg_power_init(void)
{
	unsigned long flags;
	int result;
	
	/* Get the device */
	result=register_chrdev(EMPEG_POWER_MAJOR,"empeg_power",&power_fops);
	if (result<0) {
		printk(KERN_WARNING "empeg power: Major number %d unavailable.\n",
		       EMPEG_POWER_MAJOR);
		return;
	}

	/* Disable IRQs completely here as timing is critical */
	save_flags_cli(flags);

	/* Ask power PIC if this is our first boot */
	powercontrol(2);

	/* Wait to ensure PIC has twiddled accessory sense line correctly */
	udelay(100);

	/* Read reply */
	power_firstboot=(GPLR&EMPEG_ACCSENSE)?0:1;

	/* IRQs back on */
	restore_flags(flags);

	printk("empeg power-pic driver initialised%s\n",power_firstboot?" (first boot)":"");
}

static int power_open(struct inode *inode, struct file *flip)
{
	struct power_dev *dev=power_devices;
	
	MOD_INC_USE_COUNT;
	flip->private_data=dev;

	return 0;
}

static int power_release(struct inode *inode, struct file *flip)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

static int power_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
	case EMPEG_POWER_TURNOFF:
		/* No arguments - turn off now */
		powercontrol(0);
		return 0;

	case EMPEG_POWER_WAKETIME: {
		int waketime;

		/* 0=never wakeup, 1-253=wakeup in n*15 seconds */
		get_user_ret(waketime, (int*)arg, -EFAULT);

		/* Valid time? */
		if (waketime>253) return -EINVAL;

		if (waketime==0) {
			/* Never wakeup */
			powercontrol(1);
		} else {
			/* Wake up in n*15 seconds */
			powercontrol(1+waketime);
		}

		return 0;
	}
	case EMPEG_POWER_READSTATE: {
		/* Build bitset:
		   b0 = 0 ac power, 1 dc power
		   b1 = 0 powerfail disabled, 1 powerfail enabled
		   b2 = 0 accessory low, 1 accessory high
		   b3 = 0 2nd or later boot, 1 first boot
		*/
		int bitset=0;

		if (GPLR&EMPEG_EXTPOWER) bitset|=1;
		if (powerfail_enabled()) bitset|=2;
		if (GPLR&EMPEG_ACCSENSE) bitset|=4;
		if (power_firstboot)     bitset|=8;

		put_user_ret(bitset, (int*)arg, -EFAULT);

		return 0;
	}

	}

	return -EINVAL;
}

