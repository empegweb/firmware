/*
 * empeg-car.c	Version 0.10
 *
 * (C) 1999-2000 empeg Ltd
 *
 * Authors:
 *   John Ripley <john@empeg.com>
 *
 * USB driver for empeg-car players
 *
 * This driver is derived largely from the SuSE sponsored:
 * USB Abstract Control Model driver for USB modems and ISDN adapters
 *
 * Copyright (c) 1999 Armin Fuerst	<fuerst@in.tum.de>
 * Copyright (c) 1999 Pavel Machek	<pavel@suse.cz>
 * Copyright (c) 1999 Johannes Erdfelt	<jerdfelt@valinux.com>
 * Copyright (c) 1999 Vojtech Pavlik	<vojtech@suse.cz>
 *
 * ChangeLog:
 *      v0.10 John Ripley	- first release, derived from acm.c v0.13
 *	v0.11 John Ripley	- throttling workaround for bugs in uhci/ohci
 *				  drivers, not really needed anyway.
 *	v0.12 John Ripley	- fixed problem with two processes opening
 *				  the device at the same time.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/malloc.h>
#include <linux/fcntl.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/tty.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/usb.h>

/* #define CONFIG_USB_EMPEG_DEBUG */

#ifdef CONFIG_USB_EMPEG_DEBUG
#define empeg_debug(fmt,arg...)	printk("empeg-car: " fmt "\n" , ##arg)
#else
#define empeg_debug(fmt,arg...)	do {} while(0)
#endif

/*
 * empeg-car has the following ID
 */

#define EMPEG_CAR_IDVENDOR	0x084f
#define EMPEG_CAR_IDPRODUCT	0x0001

#define EMPEG_CAR_NPACKETS	64

/*
 * Major and minor numbers. Anyone got 8 players?
 */

#define EMPEG_CAR_TTY_MAJOR		240	/* Documentation/devices.txt */
#define EMPEG_CAR_TTY_MINORS		8	/* says this is local/experimental use */

/*
 * Internal driver structures.
 */

struct empeg_car_t {
	struct usb_device *dev;			/* the coresponding usb device */
	struct usb_config_descriptor *cfg;	/* configuration number on this device */
	struct tty_struct *tty;			/* the corresponding tty */
	unsigned int readsize, writesize;	/* max packet size for bulk endpoints */
	void *readbuf, *writebuf;		/* kmalloc'd buffers */
	struct urb readurb;			/* read urb */
	struct urb writeurb;			/* write urb */
	unsigned int minor;			/* the minor number of this device */
	unsigned int present;			/* this device is connected to the usb bus */
	unsigned int used;			/* someone has this device open */
};

static struct usb_driver empeg_car_driver;
static struct empeg_car_t *empeg_car_table[EMPEG_CAR_TTY_MINORS] = { NULL, /* .... */ };

#define EMPEG_READY(empeg)	(empeg && empeg->present && empeg->used)


static void empeg_read_bulk(struct urb *urb)
{
	struct empeg_car_t *empeg = urb->context;
	struct tty_struct *tty = empeg->tty;
	unsigned char *data = urb->transfer_buffer;
	int i;

	if (!EMPEG_READY(empeg)) return;

	if (urb->status) {
		empeg_debug("nonzero read bulk status received: %d", urb->status);
		return;
	}

	for (i = 0; i < urb->actual_length; i++) {
		/* if we insert more than TTY_FLIPBUF_SIZE characters,
		   we drop them. Yuck. */
		if(tty->flip.count >= TTY_FLIPBUF_SIZE)
			tty_flip_buffer_push(tty);
		
		/* also, this wouldn't actually push the data through unless
		   tty->low_latency is set! */
		tty_insert_flip_char(tty, data[i], 0);
	}
	/* with tty->low_latency set, this goes straight through
	   instead of scheduling. */
	tty_flip_buffer_push(tty);

	if (usb_submit_urb(urb))
		empeg_debug("failed resubmitting read urb");
}

static void empeg_write_bulk(struct urb *urb)
{
	struct empeg_car_t *empeg = (struct empeg_car_t *)urb->context;
	struct tty_struct *tty = empeg->tty;

	if (!EMPEG_READY(empeg)) return;

	if (urb->status)
		empeg_debug("nonzero write bulk status received: %d", urb->status);

	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup)
		(tty->ldisc.write_wakeup)(tty);

	wake_up_interruptible(&tty->write_wait);
}

/*
 * TTY handlers
 */

static int empeg_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct empeg_car_t *empeg = empeg_car_table[MINOR(tty->device)];

	if (!empeg || !empeg->present) return -EINVAL;
	if (empeg->used) return -EBUSY;
	empeg->used++;
	
	tty->driver_data = empeg;
	empeg->tty = tty;

	/* Not quite right, but it achieves the same */
	if (empeg->used == 1)
		MOD_INC_USE_COUNT;

	/* ugly hack otherwise we lose data:
	 *
	 * tty_flip_buffer_push(tty) usually schedules a push, but the
	 * empeg_car_bulk_read function is called many times before
	 * any scheduling can occur - so we lose data after 512 bytes.
	 * A "fix" is to set it low_latency, which performs an
	 * immediate push.
	 */
	
	tty->low_latency = 1;

	if (usb_submit_urb(&empeg->readurb))
		empeg_debug("usb_submit_urb(read bulk) failed");

	return 0;
}

static void empeg_tty_close(struct tty_struct *tty, struct file *filp)
{
	struct empeg_car_t *empeg = tty->driver_data;

	if (!empeg) return;
	empeg->used--;
	if (empeg->used < 0) {
		empeg->used = 0;
		return;
	}
	if (!empeg->used)
		MOD_DEC_USE_COUNT;

	if (empeg->present) {
		usb_unlink_urb(&empeg->writeurb);
		usb_unlink_urb(&empeg->readurb);
		return;
	} else {
		/* Clean up if disconnected while somebody's got it open */
		empeg_car_table[empeg->minor] = NULL;
		kfree(empeg);
	}
}

static int empeg_tty_write(struct tty_struct *tty, int from_user,
			   const unsigned char *buf, int count)
{
	struct empeg_car_t *empeg = tty->driver_data;

	if (!EMPEG_READY(empeg)) return -EINVAL;
	if (empeg->writeurb.status == -EINPROGRESS) return 0;

	count = (count > empeg->writesize) ? empeg->writesize : count;

	if (from_user)
		copy_from_user(empeg->writeurb.transfer_buffer, buf, count);
	else
		memcpy(empeg->writeurb.transfer_buffer, buf, count);

	empeg->writeurb.transfer_buffer_length = count;

	if (usb_submit_urb(&empeg->writeurb))
		empeg_debug("usb_submit_urb(write bulk) failed");

	return count;
}

static int empeg_tty_write_room(struct tty_struct *tty)
{
	struct empeg_car_t *empeg = tty->driver_data;
	if (!EMPEG_READY(empeg)) return -EINVAL;
	return empeg->writeurb.status == -EINPROGRESS
		? 0
		: empeg->writesize;
}

static int empeg_tty_chars_in_buffer(struct tty_struct *tty)
{
	struct empeg_car_t *empeg = tty->driver_data;
	if (!EMPEG_READY(empeg)) return -EINVAL;
	return empeg->writeurb.status == -EINPROGRESS
		? empeg->writeurb.transfer_buffer_length
		: 0;
}

static void empeg_tty_throttle(struct tty_struct *tty)
{
	/* hmm... don't really need to */
}

static void empeg_tty_unthrottle(struct tty_struct *tty)
{
	/* moo */
}

static void empeg_tty_set_termios(struct tty_struct *tty, struct termios *old)
{
	/* nope */
}

/*
 * USB probe and disconnect routines.
 */

static void *empeg_probe(struct usb_device *dev, unsigned int ifnum)
{
	struct empeg_car_t *empeg = NULL;
	struct usb_interface_descriptor *ifdata;
	struct usb_endpoint_descriptor *epread, *epwrite;
	int minor;

	/* Vendor/Product is a pretty good start for a test */

	if ((dev->descriptor.idVendor != EMPEG_CAR_IDVENDOR) &&
	    (dev->descriptor.idProduct != EMPEG_CAR_IDPRODUCT))
		return NULL;

	/* The rest is sanity checks */

	if (dev->descriptor.bDeviceClass != 0 || dev->descriptor.bDeviceSubClass != 0
		|| dev->descriptor.bDeviceProtocol != 0)
		return NULL;

	if (dev->descriptor.bNumConfigurations != 1) return NULL;

	ifdata = dev->config->interface[0].altsetting + 0;
	if (ifdata->bInterfaceClass != 0 || ifdata->bInterfaceSubClass != 0 ||
	    ifdata->bInterfaceProtocol != 0 || ifdata->bNumEndpoints != 3)
		return NULL;
	
	if (usb_interface_claimed(dev->config->interface + 0))
		return NULL;

	epread = ifdata->endpoint + 0;
	epwrite = ifdata->endpoint + 1;
	
	if ((epread->bmAttributes & 3) != 2 || (epwrite->bmAttributes & 3) != 2 ||
	    ((epread->bEndpointAddress & 0x80) ^ (epwrite->bEndpointAddress & 0x80)) != 0x80)
		return NULL;

	/* OK, everything seems to be in order, let's create some structures */

	if ((epread->bEndpointAddress & 0x80) != 0x80) {
		epread = ifdata->endpoint + 1;
		epwrite = ifdata->endpoint + 0;
	}

	usb_set_configuration(dev, dev->config->bConfigurationValue);
	
	for (minor = 0; minor < EMPEG_CAR_TTY_MINORS && empeg_car_table[minor]; minor++);
	if (minor >= EMPEG_CAR_TTY_MINORS) {
		empeg_debug("somebody has way too many empegs");
		return NULL;
	}

	if (!(empeg = kmalloc(sizeof(struct empeg_car_t), GFP_KERNEL))) {
		empeg_debug("can't allocate empeg_car_t structure");
		return NULL;
	}
	memset(empeg, 0, sizeof(struct empeg_car_t));

	empeg_car_table[minor] = empeg;

	empeg->minor = minor;
	empeg->present = 1;	
	empeg->dev = dev;
	empeg->cfg = dev->config;
	empeg->readsize = epread->wMaxPacketSize * EMPEG_CAR_NPACKETS;
	empeg->writesize = epwrite->wMaxPacketSize * EMPEG_CAR_NPACKETS;
	
	empeg_debug("empeg_car probe: read pipe %02x len %d, write pipe %02x len %d\n",
		    epread->bEndpointAddress, empeg->readsize,
		    epwrite->bEndpointAddress, empeg->writesize);

	if (!(empeg->readbuf = kmalloc(empeg->readsize, GFP_KERNEL))) {
		empeg_debug("can't allocate read buffer");
		goto bail_out;
	}
	if (!(empeg->writebuf = kmalloc(empeg->writesize, GFP_KERNEL))) {
		empeg_debug("can't allocate write buffer");
		goto bail_out;
	}

	FILL_BULK_URB(&empeg->readurb, dev,
		      usb_rcvbulkpipe(dev, epread->bEndpointAddress),
		      empeg->readbuf, empeg->readsize, empeg_read_bulk, empeg);
	
	FILL_BULK_URB(&empeg->writeurb, dev,
		      usb_sndbulkpipe(dev, epwrite->bEndpointAddress),
		      empeg->writebuf, empeg->writesize, empeg_write_bulk, empeg);
	
	printk(KERN_INFO "empeg-car%d: USB empeg-car device\n", minor);
	
	usb_driver_claim_interface(&empeg_car_driver, empeg->cfg->interface + 0, empeg);
	usb_driver_claim_interface(&empeg_car_driver, empeg->cfg->interface + 1, empeg);

	return empeg;
	
 bail_out:
	if(empeg) {
		if(empeg->writebuf) kfree(empeg->writebuf);
		if(empeg->readbuf) kfree(empeg->readbuf);
		kfree(empeg);
	}
	return NULL;
}

static void empeg_disconnect(struct usb_device *dev, void *ptr)
{
	struct empeg_car_t *empeg = ptr;

	if (!empeg || !empeg->present) {
		empeg_debug("disconnect on nonexistant interface");
		return;
	}

	empeg->present = 0;

	usb_unlink_urb(&empeg->writeurb);
	usb_unlink_urb(&empeg->readurb);

	kfree(empeg->writebuf);
	kfree(empeg->readbuf);

	usb_driver_release_interface(&empeg_car_driver, empeg->cfg->interface + 0);
	usb_driver_release_interface(&empeg_car_driver, empeg->cfg->interface + 1);

	if (!empeg->used) {
		empeg_car_table[empeg->minor] = NULL;
		kfree(empeg);
	}
}

/*
 * USB driver structure.
 */

static struct usb_driver empeg_car_driver = {
	name:		"empeg-car",
	probe:		empeg_probe,
	disconnect:	empeg_disconnect
};

/*
 * TTY driver structures.
 */

static int empeg_tty_refcount;

static struct tty_struct *empeg_tty_table[EMPEG_CAR_TTY_MINORS];
static struct termios *empeg_tty_termios[EMPEG_CAR_TTY_MINORS];
static struct termios *empeg_tty_termios_locked[EMPEG_CAR_TTY_MINORS];

static struct tty_driver empeg_car_tty_driver = {
	magic:			TTY_DRIVER_MAGIC,
	driver_name:		"usb",
	name:			"empeg-car",
	major:			EMPEG_CAR_TTY_MAJOR,
	minor_start:		0,
	num:			EMPEG_CAR_TTY_MINORS,
	type:			TTY_DRIVER_TYPE_SERIAL,
	subtype:		SERIAL_TYPE_NORMAL,
	flags:			TTY_DRIVER_REAL_RAW,

	refcount:		&empeg_tty_refcount,

	table:			empeg_tty_table,
	termios:		empeg_tty_termios,
	termios_locked:		empeg_tty_termios_locked,

	open:			empeg_tty_open,
	close:			empeg_tty_close,
	write:			empeg_tty_write,
	write_room:		empeg_tty_write_room,
	set_termios:		empeg_tty_set_termios,
	throttle:		empeg_tty_throttle,
	unthrottle:		empeg_tty_unthrottle,
	chars_in_buffer:	empeg_tty_chars_in_buffer
};

/*
 * Init / cleanup.
 */

#ifdef MODULE
void cleanup_module(void)
{
	usb_deregister(&empeg_car_driver);
	tty_unregister_driver(&empeg_car_tty_driver);
}

int init_module(void)
#else
int usb_empeg_init(void)
#endif
{
	empeg_car_tty_driver.init_termios = tty_std_termios;

	/* paranoia, we definitely want raw */
	
	empeg_car_tty_driver.init_termios.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
						       |INLCR|IGNCR|ICRNL|IXON);
	empeg_car_tty_driver.init_termios.c_oflag &= ~OPOST;
	empeg_car_tty_driver.init_termios.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	empeg_car_tty_driver.init_termios.c_cflag &= ~(CSIZE|PARENB);
	empeg_car_tty_driver.init_termios.c_cflag |= CS8;

	if (tty_register_driver(&empeg_car_tty_driver))
		return -1;

	if (usb_register(&empeg_car_driver) < 0) {
		tty_unregister_driver(&empeg_car_tty_driver);
		return -1;
	}

	return 0;
}

