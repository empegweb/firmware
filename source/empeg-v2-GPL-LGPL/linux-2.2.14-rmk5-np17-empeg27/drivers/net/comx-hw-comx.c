/*
 * Hardware-level driver for the COMX and HICOMX cards
 * for Linux kernel 2.2.X
 *
 * Original authors:  Arpad Bakay <bakay.arpad@synergon.hu>,
 *                    Peter Bajan <bajan.peter@synergon.hu>,
 * Rewritten by: Tivadar Szemethy <tiv@itc.hu>
 * Currently maintained by: Gergely Madarasz <gorgo@itc.hu>
 *
 * Copyright (C) 1995-1999 ITConsult-Pro Co. <info@itc.hu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Version 0.80 (99/06/11):
 *		- port back to kernel, add support builtin driver 
 *		- cleaned up the source code a bit
 *
 * Version 0.81 (99/06/22):
 *		- cleaned up the board load functions, no more long reset
 *		  timeouts
 *		- lower modem lines on close
 *		- some interrupt handling fixes
 *
 * Version 0.82 (99/08/24):
 *		- fix multiple board support
 * 
 */

#define VERSION "0.82"

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/init.h>

#include "comx.h"
#include "comxhw.h"

MODULE_AUTHOR("Gergely Madarasz <gorgo@itc.hu>, Tivadar Szemethy <tiv@itc.hu>, Arpad Bakay");
MODULE_DESCRIPTION("Hardware-level driver for the COMX and HICOMX adapters\n");

#define	COMX_readw(dev, offset)	(readw(dev->mem_start + offset + \
	(unsigned int)(((struct comx_privdata *)\
	((struct comx_channel *)dev->priv)->HW_privdata)->channel) \
	* COMX_CHANNEL_OFFSET))

#define COMX_WRITE(dev, offset, value)	(writew(value, dev->mem_start + offset \
	+ (unsigned int)(((struct comx_privdata *) \
	((struct comx_channel *)dev->priv)->HW_privdata)->channel) \
	* COMX_CHANNEL_OFFSET))

#define COMX_CMD(dev, cmd)	(COMX_WRITE(dev, OFF_A_L2_CMD, cmd))

struct comx_firmware {
	int	len;
	unsigned char *data;
};

struct comx_privdata {
	struct comx_firmware *firmware;
	u16	clock;
	char	channel;		// channel no.
	int	memory_size;
	short	io_extent;
	u_long	histogram[5];
};

struct device *memory_used[(COMX_MEM_MAX - COMX_MEM_MIN) / 0x10000];
extern struct comx_hardware hicomx_hw;
extern struct comx_hardware comx_hw;
extern struct comx_hardware cmx_hw;

static void COMX_interrupt(int irq, void *dev_id, struct pt_regs *regs);

static void COMX_board_on(struct device *dev)
{
	outb_p( (byte) (((dev->mem_start & 0xf0000) >> 16) | 
	    COMX_ENABLE_BOARD_IT | COMX_ENABLE_BOARD_MEM), dev->base_addr);
}

static void COMX_board_off(struct device *dev)
{
	outb_p( (byte) (((dev->mem_start & 0xf0000) >> 16) | 
	   COMX_ENABLE_BOARD_IT), dev->base_addr);
}

static void HICOMX_board_on(struct device *dev)
{
	outb_p( (byte) (((dev->mem_start & 0xf0000) >> 12) | 
	   HICOMX_ENABLE_BOARD_MEM), dev->base_addr);
}

static void HICOMX_board_off(struct device *dev)
{
	outb_p( (byte) (((dev->mem_start & 0xf0000) >> 12) | 
	   HICOMX_DISABLE_BOARD_MEM), dev->base_addr);
}

static void COMX_set_clock(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;

	COMX_WRITE(dev, OFF_A_L1_CLKINI, hw->clock);
}

static struct device *COMX_access_board(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct device *ret;
	int mempos = (dev->mem_start - COMX_MEM_MIN) >> 16;
	unsigned long flags;

	save_flags(flags); cli();
	if (memory_used[mempos] == dev) {
		restore_flags(flags);
		return dev;
	}

	if (ch->twin && memory_used[mempos] == ch->twin) {
		memory_used[mempos] = dev;
		ret = ch->twin;
	} else {	
		ret = memory_used[mempos];
		if (ret) ch->HW_board_off(ret);
		memory_used[mempos] = dev;
		ch->HW_board_on(dev);
	}
	restore_flags(flags);
	return ret;
}

static void COMX_release_board(struct device *dev, struct device *savep)
{
	unsigned long flags;
	int mempos = (dev->mem_start - COMX_MEM_MIN) >> 16;
	struct comx_channel *ch = dev->priv;

	save_flags(flags); cli();
	if (memory_used[mempos] == savep) {
		restore_flags(flags);
		return;
	}

	memory_used[mempos] = savep;
	if (!ch->twin || ch->twin != savep) {
		ch->HW_board_off(dev);
		if (savep) ch->HW_board_on(savep);
	}
	restore_flags(flags);
}

static int COMX_txe(struct device *dev) 
{
	struct device *savep;
	struct comx_channel *ch = dev->priv;
	int rc = 0;

	savep = ch->HW_access_board(dev);
	if (COMX_readw(dev,OFF_A_L2_LINKUP) == LINKUP_READY) {
		rc = COMX_readw(dev,OFF_A_L2_TxEMPTY);
	}
	ch->HW_release_board(dev,savep);
	return rc;
}

static int COMX_send_packet(struct device *dev, struct sk_buff *skb)
{
	struct device *savep;
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	int ret = FRAME_DROPPED;

	savep = ch->HW_access_board(dev);	

	if (ch->debug_flags & DEBUG_HW_TX) {
		comx_debug_bytes(dev, skb->data, skb->len,"COMX_send packet");
	}

	if (skb->len > COMX_MAX_TX_SIZE) {
		dev_kfree_skb(skb);
		return FRAME_DROPPED;
	}

	if ((ch->line_status & LINE_UP) && COMX_readw(dev, OFF_A_L2_TxEMPTY)) {
		int lensave = skb->len;
		int dest = COMX_readw(dev, OFF_A_L2_TxBUFP);
		word *data = (word *)skb->data;

		writew((unsigned short)skb->len, dev->mem_start + dest);
		dest += 2;
		while (skb->len > 1) {
			writew(*data++, dev->mem_start + dest);
			dest += 2; skb->len -= 2;
		}
		if (skb->len == 1) {
			writew(*((byte *)data), dev->mem_start + dest);
		}
		writew(0, dev->mem_start + (int)hw->channel * 
		   COMX_CHANNEL_OFFSET + OFF_A_L2_TxEMPTY);
		ch->stats.tx_packets++;	
		ch->stats.tx_bytes += lensave; 
		ret = FRAME_ACCEPTED;
	} else {
		ch->stats.tx_dropped++;
		printk(KERN_INFO "%s: frame dropped\n",dev->name);
	}
	
	ch->HW_release_board(dev, savep);
	dev_kfree_skb(skb);
	return ret;
}

static inline int comx_read_buffer(struct device *dev) 
{
	struct comx_channel *ch = dev->priv;
	word rbuf_offs;
	struct sk_buff *skb;
	word len;
	int i=0;
	word *writeptr;

	i = 0;
	rbuf_offs = COMX_readw(dev, OFF_A_L2_RxBUFP);
	len = readw(dev->mem_start + rbuf_offs);
	if ((skb = dev_alloc_skb(len + 16)) == NULL) {
		ch->stats.rx_dropped++;
		COMX_WRITE(dev, OFF_A_L2_DAV, 0);
		return 0;
	}
	rbuf_offs += 2;
	skb_reserve(skb, 16);
	skb_put(skb, len);
	skb->dev = dev;
	writeptr = (word *)skb->data;
	while (i < len) {
		*writeptr++ = readw(dev->mem_start + rbuf_offs);
		rbuf_offs += 2; 
		i += 2;
	}
	COMX_WRITE(dev, OFF_A_L2_DAV, 0);
	ch->stats.rx_packets++;
	ch->stats.rx_bytes += len;
	if (ch->debug_flags & DEBUG_HW_RX) {
		comx_debug_skb(dev, skb, "COMX_interrupt receiving");
	}
	ch->LINE_rx(dev, skb);
	return 1;
}

static inline char comx_line_change(struct device *dev, char linestat)
{
	struct comx_channel *ch=dev->priv;
	char idle=1;
	
	
	if (linestat & LINE_UP) { /* Vonal fol */
		if (ch->lineup_delay) {
			if (!test_and_set_bit(0, &ch->lineup_pending)) {
				ch->lineup_timer.function = comx_lineup_func;
				ch->lineup_timer.data = (unsigned long)dev;
				ch->lineup_timer.expires = jiffies +
					HZ*ch->lineup_delay;
				add_timer(&ch->lineup_timer);
				idle=0;
			}
		} else {
			idle=0;
			ch->LINE_status(dev, ch->line_status |= LINE_UP);
		}
	} else { /* Vonal le */
		idle=0;
		if (test_and_clear_bit(0, &ch->lineup_pending)) {
			del_timer(&ch->lineup_timer);
		} else {
			ch->line_status &= ~LINE_UP;
			if (ch->LINE_status) {
				ch->LINE_status(dev, ch->line_status);
			}
		}
	}
	return idle;
}



static void COMX_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct device *dev = dev_id;
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	struct device *interrupted;
	unsigned long jiffs;
	char idle = 0;
	int count = 0;
	word tmp;

	if (dev == NULL) {
		printk(KERN_ERR "COMX_interrupt: irq %d for unknown device\n", irq);
		return;
	}

	if (dev->interrupt) {
		printk(KERN_ERR "%s: re-entering interrupt handler!\n", dev->name);
		return;
	}

	printk("%s: entering interrupt handler\n", dev->name);

	jiffs = jiffies;

	dev->interrupt = 1;
	interrupted = ch->HW_access_board(dev);

	while (!idle && count < 5000) {
		char channel = 0;
		idle = 1;

		while (channel < 2) {
			char linestat = 0;
			char buffers_emptied = 0;

			if (channel == 1) {
				if (ch->twin) {
					dev->interrupt = 0;
					dev = ch->twin;
					dev->interrupt = 1;
					ch = dev->priv;
					hw = ch->HW_privdata;
				} else {
					break;
				}
			} else {
				COMX_WRITE(dev, OFF_A_L1_REPENA, 
				    COMX_readw(dev, OFF_A_L1_REPENA) & 0xFF00);
			}
			channel++;

			if ((ch->init_status & (HW_OPEN | LINE_OPEN)) != 
			   (HW_OPEN | LINE_OPEN)) {
				continue;
			}
	
			/* Collect stats */
			tmp = COMX_readw(dev, OFF_A_L1_ABOREC);
			COMX_WRITE(dev, OFF_A_L1_ABOREC, 0);
			ch->stats.rx_missed_errors += (tmp >> 8) & 0xff;
			ch->stats.rx_over_errors += tmp & 0xff;
			tmp = COMX_readw(dev, OFF_A_L1_CRCREC);
			COMX_WRITE(dev, OFF_A_L1_CRCREC, 0);
			ch->stats.rx_crc_errors += (tmp >> 8) & 0xff;
			ch->stats.rx_missed_errors += tmp & 0xff;

			if ((ch->line_status & LINE_UP) && ch->LINE_rx) {
				while (COMX_readw(dev, OFF_A_L2_DAV)) {
					idle=0;
					buffers_emptied+=comx_read_buffer(dev);
				}
			}

			if (COMX_readw(dev, OFF_A_L2_TxEMPTY) && ch->LINE_tx) {
				ch->LINE_tx(dev);
			}

			if (COMX_readw(dev, OFF_A_L1_PBUFOVR) >> 8) {
				linestat &= ~LINE_UP;
			} else {
				linestat |= LINE_UP;
			}

			if ((linestat & LINE_UP) != (ch->line_status & LINE_UP)) {
				ch->stats.tx_carrier_errors++;
				idle &= comx_line_change(dev,linestat);
			}
				
			hw->histogram[(int)buffers_emptied]++;
		}
		count++;
	}

	if(count==5000) {
		printk(KERN_WARNING "%s: interrupt stuck\n",dev->name);
	}

	ch->HW_release_board(dev, interrupted);
	printk("%s: leaving interrupt handler\n", dev->name);
	dev->interrupt = 0;
}

static int COMX_open(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	struct proc_dir_entry *procfile = ch->procdir->subdir;
	unsigned long jiffs;
	struct device *savep;

	if (!dev->base_addr || !dev->irq || !dev->mem_start) {
		return -ENODEV;
	}

	if (!ch->twin || 
	   (!(((struct comx_channel *)(ch->twin->priv))->init_status & HW_OPEN))) {
		if (check_region(dev->base_addr, hw->io_extent)) {
			return -EAGAIN;
		}
		if (request_irq(dev->irq, COMX_interrupt, SA_INTERRUPT, dev->name, 
		   (void *)dev)) {
			printk(KERN_ERR "comx-hw-comx: unable to obtain irq %d\n", dev->irq);
			return -EAGAIN;
		}
		ch->init_status |= IRQ_ALLOCATED;
		request_region(dev->base_addr, hw->io_extent, dev->name);
		if (!ch->HW_load_board || ch->HW_load_board(dev)) {
			release_region(dev->base_addr, hw->io_extent);
			free_irq(dev->irq, (void *)dev);
			ch->init_status &= ~IRQ_ALLOCATED;
			return -ENODEV;
		}
	}

	savep = ch->HW_access_board(dev);
	COMX_WRITE(dev, OFF_A_L2_LINKUP, 0);

	if (ch->HW_set_clock) {
		ch->HW_set_clock(dev);
	}

	COMX_CMD(dev, COMX_CMD_INIT); 
	jiffs = jiffies;
	while (COMX_readw(dev, OFF_A_L2_LINKUP) != 1 && jiffies < jiffs + HZ) {
		schedule_timeout(1);
	}
	
	ch->HW_release_board(dev, savep);
	if (jiffies > jiffs + HZ) {
		printk(KERN_ERR "%s: board timeout on INIT command\n", dev->name);
		release_region(dev->base_addr, hw->io_extent);
		free_irq(dev->irq, (void *)dev);
		return -EIO;
	}

	savep = ch->HW_access_board(dev);

	/* Ide kellene irni, hogy DTE vagy DCE ? */
	COMX_CMD(dev, COMX_CMD_OPEN);

	ch->init_status |= HW_OPEN;
	
	/* Ez eleg ciki, de ilyen a rendszer */
	if (COMX_readw(dev, OFF_A_L1_PBUFOVR) >> 8) {
		ch->line_status &= ~LINE_UP;
	} else {
		ch->line_status |= LINE_UP;
	}
	
	if (ch->LINE_status) {
		ch->LINE_status(dev, ch->line_status);
	}

	ch->HW_release_board(dev, savep);

	for ( ; procfile ; procfile = procfile->next) {
		if (strcmp(procfile->name, FILENAME_IRQ) == 0 
		    || strcmp(procfile->name, FILENAME_IO) == 0
		    || strcmp(procfile->name, FILENAME_MEMADDR) == 0
		    || strcmp(procfile->name, FILENAME_CHANNEL) == 0
		    || strcmp(procfile->name, FILENAME_FIRMWARE) == 0
		    || strcmp(procfile->name, FILENAME_CLOCK) == 0) {
			procfile->mode = S_IFREG | 0444;
		
		}
	}	
	
	return 0;	
}

static int COMX_close(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct proc_dir_entry *procfile = ch->procdir->subdir;
	struct comx_privdata *hw = ch->HW_privdata;
	struct comx_channel *twin_ch;
	struct device *savep;

	savep = ch->HW_access_board(dev);

	COMX_CMD(dev, COMX_CMD_CLOSE);
	udelay(1000);
	COMX_CMD(dev, COMX_CMD_EXIT);

	ch->HW_release_board(dev, savep);

	if (ch->init_status & IRQ_ALLOCATED) {
		free_irq(dev->irq, (void *)dev);
		ch->init_status &= ~IRQ_ALLOCATED;
	}
	release_region(dev->base_addr, hw->io_extent);

	if (ch->twin && (twin_ch = ch->twin->priv) && 
	    (twin_ch->init_status & HW_OPEN)) {
		/* Pass the irq to the twin */
		if (request_irq(dev->irq, COMX_interrupt, 0, ch->twin->name, 
		   (void *)ch->twin) == 0) {
			twin_ch->init_status |= IRQ_ALLOCATED;
		}
	}

	for ( ; procfile ; procfile = procfile->next) {
		if (strcmp(procfile->name, FILENAME_IRQ) == 0 
		    || strcmp(procfile->name, FILENAME_IO) == 0
		    || strcmp(procfile->name, FILENAME_MEMADDR) == 0
		    || strcmp(procfile->name, FILENAME_CHANNEL) == 0
		    || strcmp(procfile->name, FILENAME_FIRMWARE) == 0
		    || strcmp(procfile->name, FILENAME_CLOCK) == 0) {
			procfile->mode = S_IFREG | 0644;
		}
	}
	
	ch->init_status &= ~HW_OPEN;
	return 0;
}

static int COMX_statistics(struct device *dev, char *page)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	struct device *savep;
	int len = 0;

	savep = ch->HW_access_board(dev);

	len += sprintf(page + len, "Board data: %s %s %s %s\nPBUFOVR: %02x, "
		"MODSTAT: %02x, LINKUP: %02x, DAV: %02x\nRxBUFP: %02x, "
		"TxEMPTY: %02x, TxBUFP: %02x\n",
		(ch->init_status & HW_OPEN) ? "HW_OPEN" : "",
		(ch->init_status & LINE_OPEN) ? "LINE_OPEN" : "",
		(ch->init_status & FW_LOADED) ? "FW_LOADED" : "",
		(ch->init_status & IRQ_ALLOCATED) ? "IRQ_ALLOCATED" : "",
		COMX_readw(dev, OFF_A_L1_PBUFOVR) & 0xff,
		(COMX_readw(dev, OFF_A_L1_PBUFOVR) >> 8) & 0xff,
		COMX_readw(dev, OFF_A_L2_LINKUP) & 0xff,
		COMX_readw(dev, OFF_A_L2_DAV) & 0xff,
		COMX_readw(dev, OFF_A_L2_RxBUFP) & 0xff,
		COMX_readw(dev, OFF_A_L2_TxEMPTY) & 0xff,
		COMX_readw(dev, OFF_A_L2_TxBUFP) & 0xff);

	len += sprintf(page + len, "hist[0]: %8lu hist[1]: %8lu hist[2]: %8lu\n"
		"hist[3]: %8lu hist[4]: %8lu\n",hw->histogram[0],hw->histogram[1],
		hw->histogram[2],hw->histogram[3],hw->histogram[4]);

	ch->HW_release_board(dev, savep);

	return len;
}

static int COMX_load_board(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	struct comx_firmware *fw = hw->firmware;
	word board_segment = dev->mem_start >> 16;
	unsigned long jiff;
	unsigned char id1, id2;
	int len;
	byte *COMX_address;

	if (!fw || !fw->len) {
		struct comx_channel *twin_ch = ch->twin ? ch->twin->priv : NULL;
		struct comx_privdata *twin_hw;

		if (!twin_ch || !(twin_hw = twin_ch->HW_privdata)) {
			return -EAGAIN;
		}

		if (!(fw = twin_hw->firmware) || !fw->len) {
			return -EAGAIN;
		}
	}

	id1 = fw->data[OFF_FW_L1_ID]; 
	id2 = fw->data[OFF_FW_L1_ID + 1];

	if (id1 != FW_L1_ID_1 || id2 != FW_L1_ID_2_COMX) {
		printk(KERN_ERR "%s: incorrect firmware, load aborted\n", 
			dev->name);
		return -EAGAIN;
	}

	printk(KERN_INFO "%s: Loading COMX Layer 1 firmware %s\n", dev->name, 
		(char *)(fw->data + OFF_FW_L1_ID + 2));

	id1 = fw->data[OFF_FW_L2_ID]; 
	id2 = fw->data[OFF_FW_L2_ID + 1];
	if (id1 == FW_L2_ID_1 && (id2 == 0xc0 || id2 == 0xc1 || id2 == 0xc2)) {
		printk(KERN_INFO "with Layer 2 code %s\n", 
			(char *)(fw->data + OFF_FW_L2_ID + 2));
	}

	outb_p(board_segment | COMX_BOARD_RESET, dev->base_addr);
	/* 10 usec should be enough here */
	udelay(100);
	outb_p(board_segment | COMX_ENABLE_BOARD_MEM, dev->base_addr);

	writeb(0, dev->mem_start + COMX_JAIL_OFFSET);	
	jiff=jiffies;
	while(jiffies < jiff + HZ && readb(dev->mem_start + COMX_JAIL_OFFSET) 
	    != COMX_JAIL_VALUE) {
		schedule_timeout(1);
	}	
	
	if (readb(dev->mem_start + COMX_JAIL_OFFSET) != COMX_JAIL_VALUE) {
		printk(KERN_ERR "%s: Can't reset board, JAIL value is %02x\n",
			dev->name, readb(dev->mem_start + COMX_JAIL_OFFSET));
		outb_p(board_segment | COMX_DISABLE_ALL, dev->base_addr);
		return -ENODEV;
	}

	writeb(0x55, dev->mem_start + 0x18ff);
	jiff=jiffies;
	while(jiffies < jiff + 3*HZ && readb(dev->mem_start + 0x18ff) != 0) {
		schedule_timeout(1);
	}

	if(readb(dev->mem_start + 0x18ff) != 0) {
		printk(KERN_ERR "%s: Can't reset board, reset timeout\n",
			dev->name);
		outb_p(board_segment | COMX_DISABLE_ALL, dev->base_addr);
		return -ENODEV;
	}		

	len = 0;
	COMX_address = (byte *)dev->mem_start;
	while (fw->len > len) {
		writeb(fw->data[len++], COMX_address++);
	}

	len = 0;
	COMX_address = (byte *)dev->mem_start;
	while (len != fw->len && readb(COMX_address++) == fw->data[len]) {
		len++;
	}

	if (len != fw->len) {
		printk(KERN_ERR "%s: error loading firmware: [%d] is 0x%02x "
			"instead of 0x%02x\n", dev->name, len, 
			readb(COMX_address - 1), fw->data[len]);
		outb_p(board_segment | COMX_DISABLE_ALL, dev->base_addr);
		return -EAGAIN;
	}

	writeb(0, dev->mem_start + COMX_JAIL_OFFSET);

	jiff = jiffies;
	while ( COMX_readw(dev, OFF_A_L2_LINKUP) != 1 && jiffies < jiff + 3*HZ ) {
		schedule_timeout(1);
	}

	if (jiffies > jiff + 3*HZ) {
		printk(KERN_ERR "%s: error starting firmware, linkup word is %04x\n",
			dev->name, COMX_readw(dev, OFF_A_L2_LINKUP));
		outb_p(board_segment | COMX_DISABLE_ALL, dev->base_addr);
		return -EAGAIN;
	}

	outb_p(board_segment | COMX_DISABLE_BOARD_MEM, dev->base_addr);
	ch->init_status |= FW_LOADED;
	return 0;
}

static int CMX_load_board(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	struct comx_firmware *fw = hw->firmware;
	word board_segment = dev->mem_start >> 16;
	unsigned long jiff;
	#if 0
	unsigned char id1, id2;
	#endif
	int len;
	byte *COMX_address;

	if (!fw || !fw->len) {
		struct comx_channel *twin_ch = ch->twin ? ch->twin->priv : NULL;
		struct comx_privdata *twin_hw;

		if (!twin_ch || !(twin_hw = twin_ch->HW_privdata)) {
			return -EAGAIN;
		}

		if (!(fw = twin_hw->firmware) || !fw->len) {
			return -EAGAIN;
		}
	}

	/* Ide kell olyat tenni, hogy ellenorizze az ID-t */

	if (inb_p(dev->base_addr) != CMX_ID_BYTE) {
		printk(KERN_ERR "%s: CMX id byte is invalid(%02x)\n", dev->name,
			inb_p(dev->base_addr));
		return -ENODEV;
	}

	printk(KERN_INFO "%s: Loading CMX Layer 1 firmware %s\n", dev->name, 
		(char *)(fw->data + OFF_FW_L1_ID + 2));

	outb_p(board_segment | COMX_ENABLE_BOARD_MEM | COMX_BOARD_RESET, 
		dev->base_addr);

	len = 0;
	COMX_address = (byte *)dev->mem_start;
	while (fw->len > len) {
		writeb(fw->data[len++], COMX_address++);
	}

	len = 0;
	COMX_address = (byte *)dev->mem_start;
	while (len != fw->len && readb(COMX_address++) == fw->data[len]) {
		len++;
	}

	outb_p(board_segment | COMX_ENABLE_BOARD_MEM, dev->base_addr);

	if (len != fw->len) {
		printk(KERN_ERR "%s: error loading firmware: [%d] is 0x%02x "
			"instead of 0x%02x\n", dev->name, len, 
			readb(COMX_address - 1), fw->data[len]);
		outb_p(board_segment | COMX_DISABLE_ALL, dev->base_addr);
		return -EAGAIN;
	}

	jiff = jiffies;
	while ( COMX_readw(dev, OFF_A_L2_LINKUP) != 1 && jiffies < jiff + 3*HZ ) {
		schedule_timeout(1);
	}

	if (jiffies > jiff + 3*HZ) {
		printk(KERN_ERR "%s: error starting firmware, linkup word is %04x\n",
			dev->name, COMX_readw(dev, OFF_A_L2_LINKUP));
		outb_p(board_segment | COMX_DISABLE_ALL, dev->base_addr);
		return -EAGAIN;
	}

	outb_p(board_segment | COMX_DISABLE_BOARD_MEM, dev->base_addr);
	ch->init_status |= FW_LOADED;
	return 0;
}

static int HICOMX_load_board(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	struct comx_firmware *fw = hw->firmware;
	word board_segment = dev->mem_start >> 12;
	unsigned long jiff;
	unsigned char id1, id2;
	int len;
	word *HICOMX_address;
	char id = 1;

	if (!fw || !fw->len) {
		struct comx_channel *twin_ch = ch->twin ? ch->twin->priv : NULL;
		struct comx_privdata *twin_hw;

		if (!twin_ch || !(twin_hw = twin_ch->HW_privdata)) {
			return -EAGAIN;
		}

		if (!(fw = twin_hw->firmware) || !fw->len) {
			return -EAGAIN;
		}
	}

	while (id != 4) {
		if (inb_p(dev->base_addr + id++) != HICOMX_ID_BYTE) {
			break;
		}
	}

	if (id != 4) {
		printk(KERN_ERR "%s: can't find HICOMX at 0x%04x, id[%d] = %02x\n",
			dev->name, (unsigned int)dev->base_addr, id - 1,
			inb_p(dev->base_addr + id - 1));
		return -1;	
	}

	id1 = fw->data[OFF_FW_L1_ID]; 
	id2 = fw->data[OFF_FW_L1_ID + 1];
	if (id1 != FW_L1_ID_1 || id2 != FW_L1_ID_2_HICOMX) {
		printk(KERN_ERR "%s: incorrect firmware, load aborted\n", dev->name);
		return -EAGAIN;
	}

	printk(KERN_INFO "%s: Loading HICOMX Layer 1 firmware %s\n", dev->name, 
		(char *)(fw->data + OFF_FW_L1_ID + 2));

	id1 = fw->data[OFF_FW_L2_ID]; 
	id2 = fw->data[OFF_FW_L2_ID + 1];
	if (id1 == FW_L2_ID_1 && (id2 == 0xc0 || id2 == 0xc1 || id2 == 0xc2)) {
		printk(KERN_INFO "with Layer 2 code %s\n", 
			(char *)(fw->data + OFF_FW_L2_ID + 2));
	}

	outb_p(board_segment | HICOMX_BOARD_RESET, dev->base_addr);
	udelay(10);	
	outb_p(board_segment | HICOMX_ENABLE_BOARD_MEM, dev->base_addr);
	outb_p(HICOMX_PRG_MEM, dev->base_addr + 1);

	len = 0;
	HICOMX_address = (word *)dev->mem_start;
	while (fw->len > len) {
		writeb(fw->data[len++], HICOMX_address++);
	}

	len = 0;
	HICOMX_address = (word *)dev->mem_start;
	while (len != fw->len && (readw(HICOMX_address++) & 0xff) == fw->data[len]) {
		len++;
	}

	if (len != fw->len) {
		printk(KERN_ERR "%s: error loading firmware: [%d] is 0x%02x "
			"instead of 0x%02x\n", dev->name, len, 
			readw(HICOMX_address - 1) & 0xff, fw->data[len]);
		outb_p(board_segment | HICOMX_DISABLE_ALL, dev->base_addr);
		outb_p(HICOMX_DATA_MEM, dev->base_addr + 1);
		return -EAGAIN;
	}

	outb_p(board_segment | HICOMX_BOARD_RESET, dev->base_addr);
	outb_p(HICOMX_DATA_MEM, dev->base_addr + 1);
	outb_p(board_segment | HICOMX_ENABLE_BOARD_MEM, dev->base_addr);

	jiff = jiffies;
	while ( COMX_readw(dev, OFF_A_L2_LINKUP) != 1 && jiffies < jiff + 3*HZ ) {
		schedule_timeout(1);
	}

	if ( COMX_readw(dev, OFF_A_L2_LINKUP) != 1 ) {
		printk(KERN_ERR "%s: error starting firmware, linkup word is %04x\n",
			dev->name, COMX_readw(dev, OFF_A_L2_LINKUP));
		outb_p(board_segment | HICOMX_DISABLE_ALL, dev->base_addr);
		return -EAGAIN;
	}

	outb_p(board_segment | HICOMX_DISABLE_ALL, dev->base_addr);
	ch->init_status |= FW_LOADED;
	return 0;
}

static struct device *comx_twin_check(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct proc_dir_entry *procfile = ch->procdir->parent->subdir;
	struct comx_privdata *hw = ch->HW_privdata;

	struct device *twin;
	struct comx_channel *ch_twin;
	struct comx_privdata *hw_twin;


	for ( ; procfile ; procfile = procfile->next) {
	
		if(!S_ISDIR(procfile->mode)) {
			continue;
		}
	
		twin=procfile->data;
		ch_twin=twin->priv;
		hw_twin=ch_twin->HW_privdata;


		if (twin != dev && dev->irq && dev->base_addr && dev->mem_start &&
		   dev->irq == twin->irq && dev->base_addr == twin->base_addr &&
	  	   dev->mem_start == twin->mem_start &&
		   hw->channel == (1 - hw_twin->channel) &&
		   ch->hardware == ch_twin->hardware) {
		   	return twin;
		}
	}
	return NULL;
}

static int comxhw_write_proc(struct file *file, const char *buffer, 
	u_long count, void *data)
{
	struct proc_dir_entry *entry = (struct proc_dir_entry *)data;
	struct device *dev = entry->parent->data;
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	char *page;


	if (file->f_dentry->d_inode->i_ino != entry->low_ino) {
		printk(KERN_ERR "comx_write_proc: file <-> data internal error\n");
		return -EIO;
	}

	if(ch->init_status & HW_OPEN) {
		return -EAGAIN;	
	}
	
	if (strcmp(FILENAME_FIRMWARE, entry->name) != 0) {
		if (!(page = (char *)__get_free_page(GFP_KERNEL))) {
			return -ENOMEM;
		}
		copy_from_user(page, buffer, count = (min(count, PAGE_SIZE)));
		if (*(page + count - 1) == '\n') {
			*(page + count - 1) = 0;
		}
	} else {
		byte *tmp;

		if (!hw->firmware) {
			if ((hw->firmware = kmalloc(sizeof(struct comx_firmware), 
			    GFP_KERNEL)) == NULL) {
			    	return -ENOMEM;
			}
			hw->firmware->len = 0;
			hw->firmware->data = NULL;
		}
		
		if ((tmp = kmalloc(count + file->f_pos, GFP_KERNEL)) == NULL) {
			return -ENOMEM;
		}
		
		/* Ha nem 0 a fpos, akkor meglevo file-t irunk. Gyenge trukk. */
		if (hw->firmware && hw->firmware->len && file->f_pos 
		    && hw->firmware->len < count + file->f_pos) {
			memcpy(tmp, hw->firmware->data, hw->firmware->len);
		}
		if (hw->firmware->data) {
			kfree(hw->firmware->data);
		}
		copy_from_user(tmp + file->f_pos, buffer, count);
		hw->firmware->len = entry->size = file->f_pos + count;
		hw->firmware->data = tmp;
		file->f_pos += count;
		return count;
	}

	if (strcmp(entry->name, FILENAME_CHANNEL) == 0) {
		hw->channel = simple_strtoul(page, NULL, 0);
		if (hw->channel >= MAX_CHANNELNO) {
			printk(KERN_ERR "Invalid channel number\n");
			hw->channel = 0;
		}
		if ((ch->twin = comx_twin_check(dev)) != NULL) {
			struct comx_channel *twin_ch = ch->twin->priv;
			twin_ch->twin = dev;
		}
	} else if (strcmp(entry->name, FILENAME_IRQ) == 0) {
		dev->irq = simple_strtoul(page, NULL, 0);
		if (dev->irq == 2) {
			dev->irq = 9;
		}
		if (dev->irq < 3 || dev->irq > 15) {
			printk(KERN_ERR "comxhw: Invalid irq number\n");
			dev->irq = 0;
		}
		if ((ch->twin = comx_twin_check(dev)) != NULL) {
			struct comx_channel *twin_ch = ch->twin->priv;
			twin_ch->twin = dev;
		}
	} else if (strcmp(entry->name, FILENAME_IO) == 0) {
		dev->base_addr = simple_strtoul(page, NULL, 0);
		if ((dev->base_addr & 3) != 0 || dev->base_addr < 0x300 
		   || dev->base_addr > 0x3fc) {
			printk(KERN_ERR "Invalid io value\n");
			dev->base_addr = 0;
		}
		if ((ch->twin = comx_twin_check(dev)) != NULL) {
			struct comx_channel *twin_ch = ch->twin->priv;

			twin_ch->twin = dev;
		}
	} else if (strcmp(entry->name, FILENAME_MEMADDR) == 0) {
		dev->mem_start = simple_strtoul(page, NULL, 0);
		if (dev->mem_start <= 0xf000 && dev->mem_start >= 0xa000) {
			dev->mem_start *= 16;
		}
		if ((dev->mem_start & 0xfff) != 0 || dev->mem_start < COMX_MEM_MIN
		    || dev->mem_start + hw->memory_size > COMX_MEM_MAX) {
			printk(KERN_ERR "Invalid memory page\n");
			dev->mem_start = 0;
		}
		dev->mem_end = dev->mem_start + hw->memory_size;
		if ((ch->twin = comx_twin_check(dev)) != NULL) {
			struct comx_channel *twin_ch = ch->twin->priv;

			twin_ch->twin = dev;
		}
	} else if (strcmp(entry->name, FILENAME_CLOCK) == 0) {
		if (strncmp("ext", page, 3) == 0) {
			hw->clock = 0;
		} else {
			int kbps;

			kbps = simple_strtoul(page, NULL, 0);
			hw->clock = kbps ? COMX_CLOCK_CONST/kbps : 0;
		}
	}

	free_page((unsigned long)page);
	return count;
}

static int comxhw_read_proc(char *page, char **start, off_t off, int count,
	int *eof, void *data)
{
	struct proc_dir_entry *file = (struct proc_dir_entry *)data;
	struct device *dev = file->parent->data;
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;
	int len = 0;


	if (strcmp(file->name, FILENAME_IO) == 0) {
		len = sprintf(page, "0x%03x\n", (unsigned int)dev->base_addr);
	} else if (strcmp(file->name, FILENAME_IRQ) == 0) {
		len = sprintf(page, "0x%02x\n", dev->irq == 9 ? 2 : dev->irq);
	} else if (strcmp(file->name, FILENAME_CHANNEL) == 0) {
		len = sprintf(page, "%01d\n", hw->channel);
	} else if (strcmp(file->name, FILENAME_MEMADDR) == 0) {
		len = sprintf(page, "0x%05x\n", (unsigned int)dev->mem_start);
	} else if (strcmp(file->name, FILENAME_TWIN) == 0) {
		len = sprintf(page, "%s\n", ch->twin ? ch->twin->name : "none");
	} else if (strcmp(file->name, FILENAME_CLOCK) == 0) {
		if (hw->clock) {
			len = sprintf(page, "%-8d\n", COMX_CLOCK_CONST/hw->clock);
		} else {
			len = sprintf(page, "external\n");
		}
	} else if (strcmp(file->name, FILENAME_FIRMWARE) == 0) {
		len = min(FILE_PAGESIZE, min(count, 
			hw->firmware ? (hw->firmware->len - off) :  0));
		if (len < 0) {
			len = 0;
		}
		*start = hw->firmware ? (hw->firmware->data + off) : NULL;
		if (off + len >= (hw->firmware ? hw->firmware->len : 0) || len == 0) {
			*eof = 1;
		}
		return len;
	}	

	if (off >= len) {
		*eof = 1;
		return 0;
	}

	*start = page + off;
	if (count >= len - off) {
		*eof = 1;
	}
	return(min(count, len - off));
}

/* Called on echo comx >boardtype */
static int COMX_init(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw;
	struct proc_dir_entry *new_file;

	if ((ch->HW_privdata = kmalloc(sizeof(struct comx_privdata), 
	    GFP_KERNEL)) == NULL) {
	    	return -ENOMEM;
	}
	memset(hw = ch->HW_privdata, 0, sizeof(struct comx_privdata));

	if (ch->hardware == &comx_hw || ch->hardware == &cmx_hw) {
		hw->memory_size = COMX_MEMORY_SIZE;
		hw->io_extent = COMX_IO_EXTENT;
		dev->base_addr = COMX_DEFAULT_IO;
		dev->irq = COMX_DEFAULT_IRQ;
		dev->mem_start = COMX_DEFAULT_MEMADDR;
		dev->mem_end = COMX_DEFAULT_MEMADDR + COMX_MEMORY_SIZE;
	} else if (ch->hardware == &hicomx_hw) {
		hw->memory_size = HICOMX_MEMORY_SIZE;
		hw->io_extent = HICOMX_IO_EXTENT;
		dev->base_addr = HICOMX_DEFAULT_IO;
		dev->irq = HICOMX_DEFAULT_IRQ;
		dev->mem_start = HICOMX_DEFAULT_MEMADDR;
		dev->mem_end = HICOMX_DEFAULT_MEMADDR + HICOMX_MEMORY_SIZE;
	} else {
		printk(KERN_ERR "SERIOUS INTERNAL ERROR in %s, line %d\n", __FILE__, __LINE__);
	}

	if ((new_file = create_proc_entry(FILENAME_IO, S_IFREG | 0644, ch->procdir))
	    == NULL) {
	    	return -EIO;
	}
	new_file->data = (void *)new_file;
	new_file->read_proc = &comxhw_read_proc;
	new_file->write_proc = &comxhw_write_proc;
	new_file->ops = &comx_normal_inode_ops;
	new_file->size = 6;
	new_file->nlink = 1;

	if ((new_file = create_proc_entry(FILENAME_IRQ, S_IFREG | 0644, ch->procdir))
	    == NULL) {
	    	return -EIO;
	}
	new_file->data = (void *)new_file;
	new_file->read_proc = &comxhw_read_proc;
	new_file->write_proc = &comxhw_write_proc;
	new_file->ops = &comx_normal_inode_ops;
	new_file->size = 5;
	new_file->nlink = 1;

	if ((new_file = create_proc_entry(FILENAME_CHANNEL, S_IFREG | 0644, 
	    ch->procdir)) == NULL) {
	    	return -EIO;
	}
	new_file->data = (void *)new_file;
	new_file->read_proc = &comxhw_read_proc;
	new_file->write_proc = &comxhw_write_proc;
	new_file->ops = &comx_normal_inode_ops;
	new_file->size = 2;		// Ezt tudjuk
	new_file->nlink = 1;

	if (ch->hardware == &hicomx_hw || ch->hardware == &cmx_hw) {
		if ((new_file = create_proc_entry(FILENAME_CLOCK, S_IFREG | 0644, 
		   ch->procdir)) == NULL) {
		   	return -EIO;
		}
		new_file->data = (void *)new_file;
		new_file->read_proc = &comxhw_read_proc;
		new_file->write_proc = &comxhw_write_proc;
		new_file->ops = &comx_normal_inode_ops;
		new_file->size = 9;
		new_file->nlink = 1;
	}

	if ((new_file = create_proc_entry(FILENAME_MEMADDR, S_IFREG | 0644, 
	    ch->procdir)) == NULL) {
	    	return -EIO;
	}
	new_file->data = (void *)new_file;
	new_file->read_proc = &comxhw_read_proc;
	new_file->write_proc = &comxhw_write_proc;
	new_file->ops = &comx_normal_inode_ops;
	new_file->size = 8;
	new_file->nlink = 1;

	if ((new_file = create_proc_entry(FILENAME_TWIN, S_IFREG | 0444, 
	    ch->procdir)) == NULL) {
		return -EIO;
	}
	new_file->data = (void *)new_file;
	new_file->read_proc = &comxhw_read_proc;
	new_file->write_proc = NULL;
	new_file->ops = &comx_normal_inode_ops;
	new_file->nlink = 1;

	if ((new_file = create_proc_entry(FILENAME_FIRMWARE, S_IFREG | 0644, 
	    ch->procdir)) == NULL) {
	    	return -EIO;
	}
	new_file->data = (void *)new_file;
	new_file->read_proc = &comxhw_read_proc;
	new_file->write_proc = &comxhw_write_proc;
	new_file->ops = &comx_normal_inode_ops;
	new_file->nlink = 1;

	if (ch->hardware == &comx_hw) {
		ch->HW_board_on = COMX_board_on;
		ch->HW_board_off = COMX_board_off;
		ch->HW_load_board = COMX_load_board;
	} else if (ch->hardware == &cmx_hw) {
		ch->HW_board_on = COMX_board_on;
		ch->HW_board_off = COMX_board_off;
		ch->HW_load_board = CMX_load_board;
		ch->HW_set_clock = COMX_set_clock;
	} else if (ch->hardware == &hicomx_hw) {
		ch->HW_board_on = HICOMX_board_on;
		ch->HW_board_off = HICOMX_board_off;
		ch->HW_load_board = HICOMX_load_board;
		ch->HW_set_clock = COMX_set_clock;
	} else {
		printk(KERN_ERR "SERIOUS INTERNAL ERROR in %s, line %d\n", __FILE__, __LINE__);
	}

	ch->HW_access_board = COMX_access_board;
	ch->HW_release_board = COMX_release_board;
	ch->HW_txe = COMX_txe;
	ch->HW_open = COMX_open;
	ch->HW_close = COMX_close;
	ch->HW_send_packet = COMX_send_packet;
	ch->HW_statistics = COMX_statistics;

	if ((ch->twin = comx_twin_check(dev)) != NULL) {
		struct comx_channel *twin_ch = ch->twin->priv;

		twin_ch->twin = dev;
	}

	MOD_INC_USE_COUNT;
	return 0;
}

/* Called on echo valami >boardtype */
static int COMX_exit(struct device *dev)
{
	struct comx_channel *ch = dev->priv;
	struct comx_privdata *hw = ch->HW_privdata;

	if (hw->firmware) {
		if (hw->firmware->data) kfree(hw->firmware->data);
		kfree(hw->firmware);
	} if (ch->twin) {
		struct comx_channel *twin_ch = ch->twin->priv;

		twin_ch->twin = NULL;
	}
	
	kfree(ch->HW_privdata);
	remove_proc_entry(FILENAME_IO, ch->procdir);
	remove_proc_entry(FILENAME_IRQ, ch->procdir);
	remove_proc_entry(FILENAME_CHANNEL, ch->procdir);
	remove_proc_entry(FILENAME_MEMADDR, ch->procdir);
	remove_proc_entry(FILENAME_FIRMWARE, ch->procdir);
	remove_proc_entry(FILENAME_TWIN, ch->procdir);
	if (ch->hardware == &hicomx_hw || ch->hardware == &cmx_hw) {
		remove_proc_entry(FILENAME_CLOCK, ch->procdir);
	}

	MOD_DEC_USE_COUNT;
	return 0;
}

static int COMX_dump(struct device *dev)
{
	printk(KERN_INFO "%s: COMX_dump called, why ?\n", dev->name);
	return 0;
}

static struct comx_hardware comx_hw = {
	"comx",
	VERSION,
	COMX_init,
	COMX_exit,
	COMX_dump,
	NULL
};

static struct comx_hardware cmx_hw = {
	"cmx",
	VERSION,
	COMX_init,
	COMX_exit,
	COMX_dump,
	NULL
};

static struct comx_hardware hicomx_hw = {
	"hicomx",
	VERSION,
	COMX_init,
	COMX_exit,
	COMX_dump,
	NULL
};

#ifdef MODULE
#define comx_hw_comx_init init_module
#endif

__initfunc(int comx_hw_comx_init(void))
{
	comx_register_hardware(&comx_hw);
	comx_register_hardware(&cmx_hw);
	comx_register_hardware(&hicomx_hw);
	memset(memory_used, 0, sizeof(memory_used));
	return 0;
}

#ifdef MODULE
void cleanup_module(void)
{
	comx_unregister_hardware("comx");
	comx_unregister_hardware("cmx");
	comx_unregister_hardware("hicomx");
}
#endif
