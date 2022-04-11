#define DEBUG_USB
#define NO_ZERO_TERM

#ifdef CONFIG_EMPEG_USB9602
#error Mk1 empeg's have the 9602, Mk2's have the PDIUSBD12 - none have both!
#endif

/* empeg USB support for Philips PDIUSB12
 * Hugo Fiennes <hugo@empeg.com>
 *
 * What is this USB driver thing anyway?
 * -------------------------------------
 * The USB driver in the empeg is very simple: don't confuse it with the highly
 * complex USB host device drivers like the ones which just missed the 2.2
 * kernel. Remember that usually, USB hosts are huge, firebreathing desktop
 * boxes, whereas USB slaves are little meek things like mice and scanners.
 * Ok, in our case we've got a 220Mhz USB slave, but we're atypical. If you
 * know the SA1100, you might be wondering why we're not using the USB slave
 * available on-chip: this is because it doesn't work until Rev G parts, of
 * which none are available :(
 *
 * Original empegs used the Natsemi USBN9602 chip - newer ones use the Philips
 * chip. This is for many reasons including size (the USB12 is available in
 * TSOP), speed, and EMC issues.
 *
 * The mapping of this endpoint is simple: it basically behaves as a device,
 * /dev/usb0 which can be opened, closed, and have reads and writes performed
 * on it. Due to the high bandwidth of the USB (12Mbit) we maintain local
 * buffers to ensure that we don't get starved of data during transmissions -
 * and have a receive buffer to allow the process dealing with USB to read in
 * bigger blocks than the packet size.
 *
 * The implementation is designed to be pretty transparent: this is for a
 * number of reasons, not least of which is that we run basically the same
 * emplode-connection protocol over both the USB and the serial ports on the
 * empeg unit. Implementing the endpoint as a simple 'open/close' device
 * as opposed to a more complex network-style interface also means that we can
 * do froody stuff like run PPP over a 12Mbit usb link (host permitting, of
 * course...). To this end, there is limited control over the way the USB
 * device works - endpoint 0 is handled totally under software control, and
 * only a limited number of events are passed though for the user-side task
 * to worry about (like connection/disconnection of the USB cable).
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

#include "empeg_pdiusb12.h"
#include "empeg_usb.h"

/* Only one USB channel */
struct usb_dev usb_devices[1];

/* We keep buffers to hold the last packet sent, in order that:
   On send: we can retransmit the packet on a NAK */
#define MAXTXPACKET 64
static int usb_txsize;
static int usb_txidle=1;

/* ...and for receive */
#define MAXRXPACKET 64

/* Logging for proc */
static char log[3800];
static int log_size=0;

static inline void LOG(char x)
{
	if (log_size < sizeof(log))
		log[log_size++]=x;
}

static inline void LOGS(const char *x)
{
	while (*x) {
		LOG(*x);
		x++;
	}
}

static inline void LOGN(int n)
{
  char s[32];
  sprintf(s,"%d",n);
  LOGS(s);
}

static struct file_operations usb_fops = {
  NULL, /* usb_lseek */
  usb_read,
  usb_write,
  NULL, /* usb_readdir */
  usb_poll,
  usb_ioctl,
  NULL, /* usb_mmap */
  usb_open,
  NULL, /* usb_flush */
  usb_release,
};

/**********************************************************************/
/* These are the global variables                                     */
/**********************************************************************/
int desc_typ, desc_idx, desc_sze;
unsigned char *desc_ptr=NULL;
int usb_cfg=0;

/* for now the sizes and offsets below need to be hand calculated, until I can
   find a better way to do it for multiple byte values, LSB goes first */
#define DEV_DESC_SIZE 18

unsigned char DEV_DESC[] = {      DEV_DESC_SIZE,     /* length of this desc. */
				  0x01,              /* DEVICE descriptor */
				  0x00,0x01,         /* spec rev level (BCD) */
				  0x00,              /* device class */
				  0x00,              /* device subclass */
				  0x00,              /* device protocol */  
				  0x10,              /* max packet size */   
				  0x4f,0x08,         /* empeg's vendor ID */
				  0x01,0x00,         /* empeg car product ID */
				  0x01,0x00,         /* empeg's revision ID */  
				  1,                 /* index of manuf. string */   
				  2,                 /* index of prod.  string */  
				  0,                 /* index of ser. # string */   
				  0x01               /* number of configs. */ 
};
unsigned char CFG_DESC[] = {      0x09,              /* length of this desc. */ 
				  0x02,              /* CONFIGURATION descriptor */  
				  0x27,0x00,         /* total length returned */ 
				  0x01,              /* number of interfaces */ 
				  0x01,              /* number of this config */ 
				  0x00,              /* index of config. string */  
				  0x40,              /* attr.: self powered */   
				  25,                /* we take no bus power */  
				  
				  0x09,              /* length of this desc. */  
				  0x04,              /* INTERFACE descriptor */  
				  0x00,              /* interface number */
				  0x00,              /* alternate setting */  
				  0x03,              /* # of (non 0) endpoints */ 
				  0x00,              /* interface class */
				  0x00,              /* interface subclass */  
				  0x00,              /* interface protocol */  
				  0x00,              /* index of intf. string */ 
				  
				  /* Pipe 0 */
				  0x07,              /* length of this desc. */   
				  0x05,              /* ENDPOINT descriptor */ 
				  0x82,              /* address (IN) */  
				  0x02,              /* attributes  (BULK) */    
				  (MAXTXPACKET&0xff),/* max packet size */
				  (MAXTXPACKET>>8),
				  0,                 /* interval (ms) */
				  
				  /* Pipe 1 */ 
				  0x07,              /* length of this desc. */   
				  0x05,              /* ENDPOINT descriptor*/ 
				  0x02,              /* address (OUT) */  
				  0x02,              /* attributes  (BULK) */    
				  (MAXRXPACKET&0xff),/* max packet size */
				  (MAXRXPACKET>>8),
				  0,                 /* interval (ms) */
				  
				  /* Pipe 2 */ 
				  0x07,              /* length of this desc. */   
				  0x05,              /* ENDPOINT descriptor*/ 
				  0x05,              /* address (OUT) */  
				  0x02,              /* attributes  (BULK) */    
				  0x40,0x00,         /* max packet size (64) */
				  0};                /* interval (ms) */

#define CFG_DESC_SIZE sizeof(CFG_DESC) 

/* Unicode descriptors for our device description */
unsigned char UNICODE_AVAILABLE[]={ 0x04,0x00,0x09,0x04 };        /* We offer only one language: 0409, US English */
unsigned char UNICODE_MANUFACTURER[]={12,0,'e',0,'m',0,'p',0,'e',0,'g',0};
unsigned char UNICODE_PRODUCT[]={20,0,'e',0,'m',0,'p',0,'e',0,'g',0,'-',0,
                             'c',0,'a',0,'r',0};

/* Predeclarations */
static void tx_data(void);

/* Read/write USB chip
 *
 * The empeg has the PDIUSBD12 on the PCMCIA expansion bus, directly above the
 * second IDE channel:
 *
 * pdiusbd12+128 = data register
 * pdiusbd12+132 = command register
 *
 * The timing loop provides a 540ns delay, which is needed for the inter-access
 * timings of the USB chip.
 * 
 */
static volatile unsigned char *usb_data=(volatile unsigned char*)0xe0000080;
static volatile unsigned char *usb_cmd=(volatile unsigned char*)0xe0000084;

static __inline__ void usb_command(unsigned char cmd)
{
	*usb_cmd=cmd;

	/* Needs loads of time after a command write, it appears... */
	udelay(2);
}

static __inline__ unsigned char usb_cread(void)
{
	unsigned char d;
	d=*usb_data;
	{ int a; for(a=0;a<34;a++); }
	return(d);
}

static __inline__ void usb_cwrite(unsigned char dta)  
{
	*usb_data=dta;
	{ int a; for(a=0;a<34;a++); }
}

/* Check to see if endpoint is full */
static __inline__ int checkendpoint(int endpoint)
{
	usb_command(endpoint);
	return(usb_cread()&SELECTEP_FULL);
}

/* Read a packet out of a FIFO */
static int readendpoint(int endpoint, unsigned char *buffer, int length)
{
	int a,c;

	/* Any data? */
	if (!checkendpoint(endpoint)) return(0);

	/* Read it from the fifo */
	usb_command(CMD_READBUFFER);
	usb_cread(); /* Discard */
	c=usb_cread();

	/* Trim length & read packet */
	if (c>length) c=length;
	for(a=0;a<c;a++) *buffer++=usb_cread();

	/* Now we've read it, clear the buffer */
	usb_command(CMD_CLEARBUFFER);
		
	/* Return bytes read */
	return(c);
}

/* Write a packet into the fifo */
static void writeendpoint(int endpoint, unsigned char *buffer, int length)
{
	int a;

	/* Select the endpoint */
	usb_command(CMD_SELECTEP0+endpoint);

	/* Write the data */
	usb_command(CMD_WRITEBUFFER);
	usb_cwrite(0);
	usb_cwrite(length);
	for(a=0;a<length;a++) usb_cwrite(*buffer++);

	/* Validate the buffer so the chip will send it */
	usb_command(CMD_VALIDATEBUFFER);
}

/* Stall the control endpoint */
static void stall_ep0(void)
{
	usb_command(CMD_SETEPSTATUS0);
	usb_cwrite(SETEPSTATUS_STALLED);
	usb_command(CMD_SETEPSTATUS1);
	usb_cwrite(SETEPSTATUS_STALLED);
}

/* Initialisation of the PDIUSBD12 */
static int init_usb(void)
{
	int id;

	/* Read id */
	usb_command(CMD_READCHIPID);
	id=usb_cread();
	id|=(usb_cread()<<8);

	/* Setup chip: no lazy clock, always keep the clock running,
	   non-isosynchronous mode and lowest output clock (ie, 4Mhz) */
	usb_command(CMD_SETMODE);
	usb_cwrite(SETMODE1_NOLAZYCLOCK|SETMODE1_CLOCKRUNNING|SETMODE1_NONISO);
	usb_cwrite(SETMODE2_SETTOONE|11);

	/* Set DMA mode (no dma) */
	usb_command(CMD_SETDMA);
	usb_command(0);

	/* Set default address, enable EP0 only */
	usb_command(CMD_ENDPOINTENABLE);
	usb_cwrite(0);
	
	/* Default values */
	desc_ptr=NULL;
	desc_idx=0;
	desc_sze=0;

	/* Return the chip ID */
	return(id);
}

/* Actually connect to the bus */
static void init_usb_connect(void)
{
	/* Connect to the bus */
	usb_command(CMD_SETMODE);
	usb_cwrite(SETMODE1_NOLAZYCLOCK|SETMODE1_CLOCKRUNNING|
		  SETMODE1_SOFTCONNECT|SETMODE1_NONISO);
	usb_cwrite(SETMODE2_SETTOONE|11);
}

/* Deal with get_descriptor */
static void get_descriptor(unsigned char *command)
{
	int maxlength;

	/* Store the type requested */
	desc_typ = command[3];
	if (desc_typ==DEVICE) {
		desc_ptr=DEV_DESC;
		desc_sze=DEV_DESC_SIZE;
        } else if (desc_typ==CONFIGURATION) {
		desc_ptr=CFG_DESC;
		desc_sze=CFG_DESC_SIZE;
	} else if (desc_typ==XSTRING) {
		switch(command[2]) {
		case 0:
			desc_ptr=UNICODE_AVAILABLE;
			desc_sze=sizeof(UNICODE_AVAILABLE);
			break;
		case 1:
			desc_ptr=UNICODE_MANUFACTURER;
			desc_sze=sizeof(UNICODE_MANUFACTURER);
			break;

		case 2:
			desc_ptr=UNICODE_PRODUCT;
			desc_sze=sizeof(UNICODE_PRODUCT);
			break;
		}
	} else {
		/* I've never seen this packet in my life, mister */
		stall_ep0();
		return;
	}
	
	/* Get max length that remote end wants */
	maxlength=command[6]|(command[7]<<8);
	if (desc_sze > maxlength) {
#ifdef DEBUG_USB_1
		LOGS("trimming response to ");
		LOGN(maxlength);
		LOGS(" bytes (was ");
		LOGN(desc_sze);
		LOGS(")\n");
#endif
		desc_sze = maxlength;    
	}
	
	/* Queue the first data chunk */
	if (desc_sze>16) {
		/* Send first chunk */
		writeendpoint(1,desc_ptr,16);
		desc_ptr+=16;
		desc_idx=16;
	} else {
		/* Send whole reply */
		writeendpoint(1,desc_ptr,desc_sze);
		desc_idx=desc_sze;
	}

#ifdef DEBUG_USB_1
	LOGS("sent ");
	LOGN(desc_idx);
	LOGS(" of ");
	LOGN(desc_sze);
	LOGS("\n");
#endif
}

/* Deal with get_status() */
static void get_status(unsigned char *command)
{
	unsigned char reply[4];

	/* Find request target */
	switch (command[0]&0x03) {
	case 0:                         /* DEVICE */  
		reply[0]=0;             /* first byte is reserved */
		reply[1]=0;
		break;
		
	case 1:                         /* INTERFACE */  
		reply[0]=0;
		reply[1]=0;
		break;
		
	case 2:                         /* ENDPOINT */  
		/* reply[0] needs to be 1 if the endpoint
		   referred to in command[3] is stalled,
		   otherwise 0 */
		reply[0]=0;
		reply[1]=0;
		break;
		
	default:                        /* UNDEFINED */   
		/* Stall endpoints 0 & 1 */
		stall_ep0();
		return;
	}
	
	/* Write this packet */
	writeendpoint(1,reply,2);
}

/* Make a configuration active */
static void set_configuration(unsigned char *command)
{
	struct usb_dev *dev=usb_devices;

	/* Set the configuration # */  
	usb_cfg=command[2];
	if (usb_cfg==0) {
		/* Unconfigure device - disable all endpoints except 0 */
		writeendpoint(1,0,0);
		usb_command(CMD_ENDPOINTENABLE);
		usb_cwrite(0);
	} else if (usb_cfg==1) {
		/* Configure device */
		writeendpoint(1,0,0);

		usb_command(CMD_ENDPOINTENABLE);
		usb_cwrite(0);
		usb_command(CMD_ENDPOINTENABLE);
		usb_cwrite(EPENABLE_GENERICISOEN);
		
		/* If there's anything in the tx buffer, kick tx */
		if (dev->tx_used>0) tx_data();
	} else {
		/* Panic! */
		stall_ep0();
	}
}

/* Deal with RX */
static __inline__ void rx_data(void)
{
	struct usb_dev *dev=usb_devices;
	int rxstat,bytes;

	/* Get status/clear IRQ */
	usb_command(CMD_LASTTRANSACTION4);
	rxstat=usb_cread();

	/* Any data? If not, return */
	if (!checkendpoint(CMD_SELECTEP4)) return;

	/* Read it from the fifo */
	usb_command(CMD_READBUFFER);
	usb_cread(); /* Discard */
	bytes=usb_cread();

	/* No data? */
	if (bytes==0) return;

	/* Bump counts */
	dev->rx_count+=bytes;
	dev->stats_rxok[1]++;

#ifdef DEBUG_USB_1
	LOG('r');
	LOGN(bytes);
#endif
	/* If there's no room in the buffer, truncate */
	if (bytes>dev->rx_free) {
		bytes=dev->rx_free;
		LOGS("rxfull!\n");
	}

	/* Buffer the data */
	dev->rx_used+=bytes;
	dev->rx_free-=bytes;
	while(bytes--) {
		/* Buffer the data */
		dev->rx_buffer[dev->rx_head++]=usb_cread();
		if (dev->rx_head==USB_RX_BUFFER_SIZE)
			dev->rx_head=0;
	}
		
	/* Now we've read it, clear the buffer */
	usb_command(CMD_CLEARBUFFER);

	/* Wake up anyone that's waiting on read */
	wake_up_interruptible(&dev->rx_wq);		
}

/* TX on the data fifo */
static void tx_data()
{
	struct usb_dev *dev=usb_devices;
	unsigned long flags;
	int a,txstat;

	/* Get status/clear IRQ */
	usb_command(CMD_LASTTRANSACTION5);
	txstat=usb_cread();

#ifdef DEBUG_USB_1
	printk("tx(%02x)\n",txstat);
#endif
	/* Sucessfully sent some stuff: bump counts & reset buffer */
	dev->stats_txok[1]++;

	/* If last packet was short, and there's nothing in the buffer to send,
	   then just stop here with TX disabled */
	if (usb_txsize<MAXTXPACKET && dev->tx_used==0) {
#ifdef NO_ZERO_TERM
		usb_txidle=1;
#else	
		/* Just send zero-length packet */
		usb_txidle=0;
		usb_txsize=0;
		writeendpoint(5,0,0);
#endif
		return;
	}

	/* While we can send stuff... (this will fill both FIFOs) */
	while(checkendpoint(CMD_SELECTEP5)==0 && dev->tx_used>=0) {
		/* Fill local packet buffer from TX buffer: if there's nothing
		   to send (there might be: we need to be able to send zero
		   length packets to terminate a transfer of an exact multiple
		   of the buffer size), then we'll be sending a 0 byte
		   packet */		
		save_flags_cli(flags);

		if ((usb_txsize=dev->tx_used)>MAXTXPACKET)
			usb_txsize=MAXTXPACKET;
		dev->tx_used-=usb_txsize;
		dev->tx_free+=usb_txsize;
		dev->tx_count+=usb_txsize;

#ifdef DEBUG_USB_1
		LOG('t');
		LOGN(usb_txsize);
#endif

		/* Put it into the chip */
		usb_command(CMD_WRITEBUFFER);
		usb_cwrite(0);
		usb_cwrite(a=usb_txsize);
		while(a--) {
			usb_cwrite(dev->tx_buffer[dev->tx_tail++]);
			if (dev->tx_tail==USB_TX_BUFFER_SIZE)
				dev->tx_tail=0;
		}
		restore_flags(flags);

		/* Validate the buffer so the chip will send it */
		usb_command(CMD_VALIDATEBUFFER);

		/* Was this packet less than max length? If so, stop here
		   as that will signal end of write on usb */
		if (usb_txsize<MAXTXPACKET) break;
	}

	/* Wake up anyone that's waiting on write when we've got a decent
	   amount of free space */
	if (dev->tx_free>(USB_TX_BUFFER_SIZE/4))
		wake_up_interruptible(&dev->tx_wq);

#ifdef DEBUG_USB_1
	printk("tx_d() queued %d byte packet\n",usb_txsize);
#endif
}

/* Deal with incoming packet on the control endpoint */
static void rx_command(void)
{
	/* Get receiver status */    
	struct usb_dev *dev=usb_devices;
	unsigned char command[8];
#ifdef DEBUG_USB
	static char *reqtext[]=
	{"GET_STATUS","CLEAR_FEATURE","(2)","SET_FEATURE","(4)","SET_ADDRESS",
	 "GET_DESCRIPTOR","SET_DESCRIPTOR","GET_CONFIGURATION","SET_CONFIGURATION",
	 "GET_INTERFACE","SET_INTERFACE","SYNC_FRAME","(13)","(14)","(15)"};
#endif
	int eplast;

	/* Get last transaction status (clears IRQ) */
	usb_command(CMD_LASTTRANSACTION0);
	eplast=usb_cread();
	
	/* Bump stats */
	dev->stats_rxok[0]++;

	/* Is this a setup packet? */  
	if(eplast&LASTTRANS_SETUP) {
		int i;

		/* Read the packet into our buffer */
		if ((i=readendpoint(0,command,sizeof(command)))!=sizeof(command)) {
			usb_command(CMD_SETEPSTATUS0);
			usb_cwrite(SETEPSTATUS_STALLED);
			usb_command(CMD_SETEPSTATUS1);
			usb_cwrite(SETEPSTATUS_STALLED);
			printk("Short USB control read (%d bytes)!\n",i);
			return;
		}
		
		/* Acknowledge both in/out endpoints */
		usb_command(CMD_SELECTEP0);
		usb_command(CMD_ACKSETUP);
		usb_command(CMD_CLEARBUFFER); /* Not really needed */
		usb_command(CMD_SELECTEP1);
		usb_command(CMD_ACKSETUP);
		
		/* If a standard request? */
		if ((command[0]&0x60)==0x00) {
#ifdef DEBUG_USB
			LOGS("USB: ");
			LOGS(reqtext[command[1]&0xf]);
			LOG('\n');
#endif
			/* Find request target */
			switch (command[1]) {
			case GET_STATUS: 
				get_status(command);   
				break;

			case CLEAR_FEATURE:
			case SET_FEATURE:
				/* Find request target */
				switch (command[0]&0x03) {
				/* DEVICE */  
				case 0:
					if (command[2]==USB_FEATURE_REMOTE_WAKEUP)
						writeendpoint(1,0,0);
					break;
					
				/* INTERFACE */  
				case 1:
					break;
					
				/* ENDPOINT */
				case 2: { 
					/* Find endpoint */
					int ep=command[4]&3;
					int stall=(command[1]==SET_FEATURE)?SETEPSTATUS_STALLED:0;
#ifdef DEBUG_USB_1
					printk("endpoint stall(%d)\n",ep);
#endif
					/* Set/clear endpoint stall flag */
					usb_command(CMD_SETEPSTATUS0+(ep*2)+1);
					usb_cwrite(stall);
					usb_command(CMD_SETEPSTATUS0+(ep*2));
					usb_cwrite(stall);
					writeendpoint(1,0,0);
					break;
				}		
				
				/* UNDEFINED */   
				default:              
					stall_ep0();
					break;
				}
				break;
				
			case SET_ADDRESS:
				/* Set and enable new address for endpoint 0 */
				usb_command(CMD_SETADDRESS);
				usb_cwrite((command[2]&SETADDRESS_ADDRESS)|
					  SETADDRESS_ENABLE);
				writeendpoint(1,0,0);
				break;
				
			case GET_DESCRIPTOR: 
				get_descriptor(command);
				break;
				
			case GET_CONFIGURATION: {
				/* Reply with the configuration */
				char reply[1];

				reply[0]=usb_cfg;
				writeendpoint(1,reply,1);
				break;
			}
				
			case SET_CONFIGURATION:
				set_configuration(command); 
				break;
				
			default:      
				/* Unsupported standard req */  
				break;
			}
		} else {                         
			/* If a non-standard req. */   
#ifdef DEBUG_USB
			printk("USB: Non-standard request\n");
#endif
		}
	} else {   
		/* If not a setup packet, it must be an OUT packet */ 

		/* Exit get_descr mode */
		desc_ptr=NULL;
	}
}

/* TX command */
static void tx_command(void)
{
	struct usb_dev *dev=usb_devices;

	/* Read last status & discard */
	usb_command(CMD_LASTTRANSACTION1);
	usb_cread();

	/* Bump stats */
	dev->stats_txok[0]++;

	LOGS("tx_command ");

	/* In the middle of a transmit? */
	if (desc_ptr!=NULL && desc_idx<=desc_sze) {
		int tosend=(desc_sze-desc_idx);
		if (tosend>16) tosend=16;

		/* Write the next packet */
		writeendpoint(1,desc_ptr,tosend);
		desc_ptr+=tosend;
		desc_idx+=tosend;

#ifdef DEBUG_USB_1
		LOGS("sent ");
		LOGN(desc_idx);
		LOGS(" of ");
		LOGN(desc_sze);
		LOGS("\n");
#endif

		/* Done it all? */
		if (tosend<16) desc_ptr=NULL;
	} else {
#ifdef DEBUG_USB_1
	        LOGS("nothing to do\n");
#endif
	}
}

void usb_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
        /*struct usb_dev *dev=usb_devices;*/
	int evnt;

 again:
	/* Read IRQ status */
	usb_command(CMD_READINTERRUPT);
	evnt=usb_cread();
	evnt|=(usb_cread()<<8);

	/* No irq? */
	if (evnt==0) return;

	if (evnt&IRQ1_MAINOUT) {
		rx_data();
		rx_data();
	}
	if (evnt&IRQ1_MAININ) {
		tx_data();
	}
	if (evnt&IRQ1_CONTROLOUT) {
		rx_command();
	}
	if (evnt&IRQ1_CONTROLIN) {
		tx_command();
	}
	if (evnt&IRQ1_EP1OUT) {
		char temp[16];
		usb_command(CMD_LASTTRANSACTION2);
		usb_cread();
		readendpoint(2,temp,16);
		LOGS("1r");
	}
	if (evnt&IRQ1_EP1IN) {
		usb_command(CMD_LASTTRANSACTION3);
		usb_cread();
		LOGS("1t");
	}
	if (evnt&IRQ1_BUSRESET) {
#if DEBUG_USB_1
		printk("Bus reset\n");
#endif
		init_usb();
		init_usb_connect();
	}
	if (evnt&IRQ1_SUSPENDCHANGE) {
#if DEBUG_USB_1
		printk("Suspend/change\n");
#endif
	}

	goto again;
}

static int usb_read_procmem(char *buf, char **start, off_t offset, int len, int unused)
{
	struct usb_dev *dev = usb_devices;
	int a;
	len = 0;

	len+=sprintf(buf+len,"Control endpoint 0\n");
	len+=sprintf(buf+len,"  %9d RX ok\n",dev->stats_rxok[0]);
	len+=sprintf(buf+len,"  %9d RX error\n",dev->stats_rxerr[0]);
	len+=sprintf(buf+len,"  %9d RX nak\n",dev->stats_rxnak[0]);
	len+=sprintf(buf+len,"  %9d TX ok\n",dev->stats_txok[0]);
	len+=sprintf(buf+len,"  %9d TX error\n\n",dev->stats_txerr[0]);
	
	len+=sprintf(buf+len,"Overall stats\n");
	len+=sprintf(buf+len,"  %9d RX bytes\n",dev->rx_count);
	len+=sprintf(buf+len,"  %9d TX bytes\n\n",dev->tx_count);

	for(a=1;a<2;a++) {
		len+=sprintf(buf+len,"Endpoint %d\n",a);
		len+=sprintf(buf+len,"  %9d RX ok\n",dev->stats_rxok[a]);
		len+=sprintf(buf+len,"  %9d RX error\n",dev->stats_rxerr[a]);
		len+=sprintf(buf+len,"  %9d RX nak\n",dev->stats_rxnak[a]);
		len+=sprintf(buf+len,"  %9d RX overruns\n",dev->stats_rxoverrun[a]);
		len+=sprintf(buf+len,"  %9d TX ok\n",dev->stats_txok[a]);
		len+=sprintf(buf+len,"  %9d TX error\n\n",dev->stats_txerr[a]);
	}

	LOG(0);
	len+=sprintf(buf+len,"Log: %s",log);
	log_size=0;
	
	return len;
}

struct proc_dir_entry usb_proc_entry = {
	0,			/* inode (dynamic) */
	9, "empeg_usb",  	/* length and name */
	S_IFREG | S_IRUGO, 	/* mode */
	1, 0, 0, 		/* links, owner, group */
	0, 			/* size */
	NULL, 			/* use default operations */
	&usb_read_procmem, 	/* function used to read data */
};

/* Device initialisation */
void __init empeg_usb_init(void)
{
        struct usb_dev *dev=usb_devices;
	int result,id;
	
	/* Do chip setup */
	id=init_usb();
	
	/* Allocate buffers */
	dev->tx_buffer=vmalloc(USB_TX_BUFFER_SIZE);
	if (!dev->tx_buffer) {
		printk(KERN_WARNING "Could not allocate memory for USB transmit buffer\n");
		return;
	}

	dev->rx_buffer=vmalloc(USB_RX_BUFFER_SIZE);
	if (!dev->rx_buffer) {
		printk(KERN_WARNING "Could not allocate memory for USB receive buffer\n");
		return;
	}

	/* Initialise buffer bits */
	dev->tx_head=dev->tx_tail=0;
	dev->tx_used=0; dev->tx_free=USB_TX_BUFFER_SIZE;
	dev->rx_head=dev->rx_tail=0;
	dev->rx_used=0; dev->rx_free=USB_RX_BUFFER_SIZE;
	dev->rx_wq = dev->tx_wq = NULL;
	
	/* Claim USB IRQ */
	result=request_irq(EMPEG_IRQ_USBIRQ,usb_interrupt,
			   0,"empeg_usbirq",dev);

	/* Got it ok? */
	if (result==0) {
		/* Enable IRQs on rising edge only (there's an inverter
		   between the USBD12 and the SA) */
		GRER|=EMPEG_USBIRQ;
		GFER&=~EMPEG_USBIRQ;
		
		/* Clear edge detect */
		GEDR=EMPEG_USBIRQ;

		/* Dad's home! */
		printk("empeg usb initialised, PDIUSBD12 id %04x\n",id);
	}
	else {
		printk(KERN_ERR "Can't get empeg USBIRQ IRQ %d.\n",
		       EMPEG_IRQ_USBIRQ);
		return;
	}

	/* Get the device */
	result=register_chrdev(EMPEG_USB_MAJOR,"empeg_usb",&usb_fops);
	if (result<0) {
		printk(KERN_WARNING "empeg USB: Major number %d unavailable.\n",
		       EMPEG_USB_MAJOR);
		return;
	}

#ifdef CONFIG_PROC_FS
	proc_register(&proc_root, &usb_proc_entry);
#endif
	/* Queue a 0-byte write to start with */
        writeendpoint(5,0,0);

	/* If there are pending IRQs, process them as we can only
	   detect edges */
	if (GPLR&EMPEG_USBIRQ) usb_interrupt(1,NULL,NULL);

	/* All ready! */
	init_usb_connect();
}

/* Open and release are very simplistic: by the nature of USB, we don't enable
   and disable the device when we open and close the device (which in this
   case is just an endpoint anyway) - otherwise, we wouldn't be able to see
   stuff for endpoint 0, ie configuration and connection events */
static int usb_open(struct inode *inode, struct file *flip)
{
	struct usb_dev *dev=usb_devices;
	
	MOD_INC_USE_COUNT;
	flip->private_data=dev;
	return 0;
}

static int usb_release(struct inode *inode, struct file *flip)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

/* Read data from USB buffer */
static ssize_t usb_read(struct file *flip, char *dest, size_t count, loff_t *ppos)
{
	struct usb_dev *dev=flip->private_data;
	unsigned long flags;
	size_t bytes;

	while (dev->rx_used==0) {
		if (flip->f_flags & O_NONBLOCK)
			return -EAGAIN;
      
		interruptible_sleep_on(&dev->rx_wq);
		/* If the sleep was terminated by a signal give up */
		if (signal_pending(current))
			return -ERESTARTSYS;
	}

	/* Read as much as we can */
	save_flags_cli(flags);
	if (count > dev->rx_used) count = dev->rx_used;
	dev->rx_used -= count;
	dev->rx_free += count;

	/* Copy the data out with IRQs enabled: this is safe as the tail is
	   only updated by us */
	bytes = count;
	while (bytes--) {
		*dest++ = dev->rx_buffer[dev->rx_tail++];
		if (dev->rx_tail==USB_RX_BUFFER_SIZE)
			dev->rx_tail=0;
	}
	restore_flags(flags);
	
	return count;
}

/* Write data to the USB buffer */
static ssize_t usb_write(struct file *filp, const char *buf, size_t count,
			 loff_t *ppos)
{
	/* This will need heavy mods to cause the device to be kicked */
	struct usb_dev *dev = filp->private_data;
	unsigned long flags;
	size_t bytes;

	while (dev->tx_free == 0) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
      
		interruptible_sleep_on(&dev->tx_wq);

		/* If the sleep was terminated by a signal give up */
		if (signal_pending(current))
			return -ERESTARTSYS;
	}
	
	/* How many bytes can we write? */
	save_flags_cli(flags);
	if (count > dev->tx_free) count = dev->tx_free;
	dev->tx_free -= count;
	dev->tx_used += count;

	/* Copy data into buffer with IRQs enabled: we're the only people who
	   deal with the head, so no problems here */
	bytes = count;
	while (bytes--) {
		dev->tx_buffer[dev->tx_head++] = *buf++;
		if (dev->tx_head == USB_TX_BUFFER_SIZE)
			dev->tx_head=0;
	}

	/* Do we need to kick the TX? */
	if (usb_txidle) {
		/* TX not going, kick the b'stard (while he's down) */
		tx_data();
	}
	restore_flags(flags);

	return count;
}

static unsigned int usb_poll(struct file *filp, poll_table *wait)
{
	struct usb_dev *dev = filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &dev->rx_wq, wait);
	poll_wait(filp, &dev->tx_wq, wait);

	/* Is there stuff in the read buffer? */
	if (dev->rx_used)
		mask |= POLLIN | POLLRDNORM;

	/* Is there room in the write buffer? */
	if (dev->tx_free)
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static int usb_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}
