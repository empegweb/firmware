/*
 * empeg-car hardware-specifics
 *
 * (C) 1999/2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Hugo Fiennes, <hugo@empeg.com>
 *   Mike Crowe, <mac@empeg.com>
 *
 */

#ifndef __ASM_ARCH_EMPEG_H
#define __ASM_ARCH_EMPEG_H

/* Revision 3 (Sonja) IO definitions */
#define EMPEG_POWERFAIL      (GPIO_GPIO0)  /* IN  Power fail sense           */
#define EMPEG_USBIRQ         (GPIO_GPIO1)  /* IN  USB IRQ                    */
#define EMPEG_USBDRQ         (GPIO_GPIO2)  /* IN  USB DRQ                    */
#define EMPEG_CRYSTALDRQ     (GPIO_GPIO2)  /* IN  Mk2 CS4231 audio in DRQ    */
#define EMPEG_RDSCLOCK       (GPIO_GPIO3)  /* IN  RDS clock                  */
#define EMPEG_IRINPUT_BIT    4
#define EMPEG_IRINPUT        GPIO_GPIO(EMPEG_IRINPUT_BIT)  /* IN  Frontboard consumer IR     */
#define EMPEG_DSP2OUT        (GPIO_GPIO5)  /* IN  7705's DSP_OUT2 pin        */
#define EMPEG_IDE1IRQ        (GPIO_GPIO6)  /* IN  IDE channel 1 irq (actL)   */
#define EMPEG_IDE2IRQ        (GPIO_GPIO7)  /* IN  IDE channel 2 irq (actL)   */
#define EMPEG_ETHERNETIRQ    (GPIO_GPIO7)  /* IN  Mk2 Ethernet               */
#define EMPEG_I2CCLOCK       (GPIO_GPIO8)  /* OUT I2C clock                  */
#define EMPEG_I2CDATA        (GPIO_GPIO9)  /* OUT I2C data (when high pulls  */
                                           /*     data line low)             */
#define EMPEG_RADIODATA      (GPIO_GPIO10) /* OUT Radio data (when high it   */
					   /*     pulls data line low)       */
#define EMPEG_DISPLAYCONTROL (GPIO_GPIO10) /* OUT Mk2 display control line   */
#define EMPEG_I2CDATAIN      (GPIO_GPIO11) /* IN  I2C data input             */
#define EMPEG_RADIOCLOCK     (GPIO_GPIO12) /* OUT Radio clock                */
#define EMPEG_SIRSPEED0      (GPIO_GPIO12) /* OUT Mk2 SIR endec speed sel 0  */
#define EMPEG_RADIOWR        (GPIO_GPIO13) /* OUT Radio write enable         */
#define EMPEG_ACCSENSE       (GPIO_GPIO13) /* IN  Mk2 Car accessory sense    */
#define EMPEG_RADIODATAIN    (GPIO_GPIO14) /* IN  Radio data input           */
#define EMPEG_POWERCONTROL   (GPIO_GPIO14) /* OUT Mk2 Control for power PIC  */
#define EMPEG_DSPPOM         (GPIO_GPIO15) /* OUT DSP power-on-mute          */
#define EMPEG_IDERESET       (GPIO_GPIO16) /* OUT IDE hard reset (actL)      */
#define EMPEG_RDSDATA        (GPIO_GPIO17) /* IN  RDS datastream             */
#define EMPEG_DISPLAYPOWER   (GPIO_GPIO18) /* OUT Frontboard power           */
#define EMPEG_I2SCLOCK       (GPIO_GPIO19) /* IN  I2S clock (2.8224Mhz)      */
#define EMPEG_FLASHWE        (GPIO_GPIO20) /* OUT Flash write enable (actL)  */
#define EMPEG_SERIALDSR      (GPIO_GPIO21) /* IN  Serial DSR                 */
#define EMPEG_SERIALCTS      (GPIO_GPIO22) /* IN  Serial CTS                 */
#define EMPEG_SERIALDTR      (GPIO_GPIO23) /* OUT Serial DTS (also LED)      */
#define EMPEG_SIRSPEED1      (GPIO_GPIO23) /* OUT Mk2 SIR endec speed sel 1  */
#define EMPEG_SERIALRTS      (GPIO_GPIO24) /* OUT Serial RTS                 */
#define EMPEG_SIRSPEED2      (GPIO_GPIO24) /* OUT Mk2 SIR endec speed sel 2  */
#define EMPEG_EXTPOWER       (GPIO_GPIO25) /* IN  External power sense (0=   */
                                           /*     unit is in-car)            */
#define EMPEG_DS1821         (GPIO_GPIO26) /* I/O DS1821 data line           */
#define EMPEG_SERIALDCD      (GPIO_GPIO27) /* IN  Serial DCD                 */

/* ... and IRQ defintions */
#define EMPEG_IRQ_IR         4
#define EMPEG_IRQ_USBIRQ     1
#define EMPEG_IRQ_IDE1       6
#define EMPEG_IRQ_IDE2       7
#define EMPEG_IRQ_POWERFAIL  0

#define EMPEG_IR_MAJOR 	     (242)
#define EMPEG_USB_MAJOR      (243)
#define EMPEG_DISPLAY_MAJOR  (244)
#define EMPEG_AUDIO_MAJOR    (245)
#define EMPEG_STATE_MAJOR    (246)
#define EMPEG_RDS_MAJOR      (248)
#define EMPEG_AUDIOIN_MAJOR  (249)
#define EMPEG_POWER_MAJOR    (250)

/* Empeg IR ioctl values */
#define EMPEG_IR_MAGIC 'i'

/* Set/get the remote control type */
#define EMPEG_IR_WRITE_TYPE _IOW(EMPEG_IR_MAGIC, 1, int)
#define EMPEG_IR_READ_TYPE _IOR(EMPEG_IR_MAGIC, 2, int)

/* Set/get the delay before repeats are honoured (in microseconds)
 * Cannot be greater than one second. */
#define EMPEG_IR_WRITE_RPTDELAY _IOW(EMPEG_IR_MAGIC, 3, unsigned long)
#define EMPEG_IR_READ_RPTDELAY _IOR(EMPEG_IR_MAGIC, 4, unsigned long)

/* Set/get the interval between repeats (in microseconds)
 * Cannot be greater than one second. */
#define EMPEG_IR_WRITE_RPTINT _IOW(EMPEG_IR_MAGIC, 4, unsigned long)
#define EMPEG_IR_READ_RPTINT _IOR(EMPEG_IR_MAGIC, 5, unsigned long)

/* Set/get the timeout for a repeat occuring. If a repeat code happens
 * after this amount of time without one then it will be ignored. */
#define EMPEG_IR_WRITE_RPTTMOUT _IOW(EMPEG_IR_MAGIC, 6, unsigned long)
#define EMPEG_IR_READ_RPTTMOUT _IOR(EMPEG_IR_MAGIC, 7, unsigned long)

/* Deprecated ioctl values */
#define IR_IOCSTYPE _IOW(EMPEG_IR_MAGIC, 1, int)
#define IR_IOCGTYPE _IOR(EMPEG_IR_MAGIC, 2, int)
#define IR_IOCTTYPE IR_IOCGTYPE /*deprecated*/
#define IR_IOCSRPTDELAY _IOW(EMPEG_IR_MAGIC, 3, unsigned long)
#define IR_IOCGRPTDELAY _IOR(EMPEG_IR_MAGIC, 4, unsigned long)
#define IR_IOCSRPTINT _IOW(EMPEG_IR_MAGIC, 4, unsigned long)
#define IR_IOCGRPTINT _IOR(EMPEG_IR_MAGIC, 5, unsigned long)
#define IR_IOCSRPTTMOUT _IOW(EMPEG_IR_MAGIC, 6, unsigned long)
#define IR_IOCGRPTTMOUT _IOR(EMPEG_IR_MAGIC, 7, unsigned long)

#define IR_TYPE_COUNT 1

#define IR_TYPE_CAPTURE 0
#define IR_TYPE_KENWOOD 1

/* Empeg Display ioctl values */
#define EMPEG_DISPLAY_MAGIC 'd'

/* Deprecated ioctl codes */
#define DIS_IOCREFRESH _IO(EMPEG_DISPLAY_MAGIC, 0)
#define DIS_IOCSPOWER _IOW(EMPEG_DISPLAY_MAGIC, 1, int)
#define DIS_IOCSPALETTE _IOW(EMPEG_DISPLAY_MAGIC, 4, int)
#define DIS_IOCCLEAR _IO(EMPEG_DISPLAY_MAGIC, 5)
#define DIS_IOCENQUEUE _IO(EMPEG_DISPLAY_MAGIC, 6)
#define DIS_IOCPOPQUEUE _IO(EMPEG_DISPLAY_MAGIC, 7)
#define DIS_IOCFLUSHQUEUE _IO(EMPEG_DISPLAY_MAGIC, 8)

/* Should use these ioctl codes instead */
#define EMPEG_DISPLAY_REFRESH _IO(EMPEG_DISPLAY_MAGIC, 0)
#define EMPEG_DISPLAY_POWER _IOW(EMPEG_DISPLAY_MAGIC, 1, int)
#define EMPEG_DISPLAY_WRITE_PALETTE _IOW(EMPEG_DISPLAY_MAGIC, 4, int)
#define EMPEG_DISPLAY_CLEAR _IO(EMPEG_DISPLAY_MAGIC, 5)
#define EMPEG_DISPLAY_ENQUEUE _IO(EMPEG_DISPLAY_MAGIC, 6)
#define EMPEG_DISPLAY_POPQUEUE _IO(EMPEG_DISPLAY_MAGIC, 7)
#define EMPEG_DISPLAY_FLUSHQUEUE _IO(EMPEG_DISPLAY_MAGIC, 8)
#define EMPEG_DISPLAY_QUERYQUEUEFREE _IOR(EMPEG_DISPLAY_MAGIC, 9, int)
#define EMPEG_DISPLAY_SENDCONTROL _IOW(EMPEG_DISPLAY_MAGIC, 10, int)
#define EMPEG_DISPLAY_SETBRIGHTNESS _IOW(EMPEG_DISPLAY_MAGIC, 11, int)

/* Sound IOCTLs */
/* Make use of the bitmasks in soundcard.h, we only support.
 * PCM, RADIO and LINE. */

#define EMPEG_MIXER_MAGIC 'm'
#define EMPEG_DSP_MAGIC 'a'

#define EMPEG_MIXER_READ_SOURCE _IOR(EMPEG_MIXER_MAGIC, 0, int)
#define EMPEG_MIXER_WRITE_SOURCE _IOW(EMPEG_MIXER_MAGIC, 0, int)
#define EMPEG_MIXER_READ_FLAGS _IOR(EMPEG_MIXER_MAGIC, 1, int)
#define EMPEG_MIXER_WRITE_FLAGS _IOW(EMPEG_MIXER_MAGIC, 1, int)
#define EMPEG_MIXER_READ_DB _IOR(EMPEG_MIXER_MAGIC, 2, int)
#define EMPEG_MIXER_WRITE_LOUDNESS _IOW(EMPEG_MIXER_MAGIC, 4, int)
#define EMPEG_MIXER_READ_LOUDNESS _IOR(EMPEG_MIXER_MAGIC, 4, int)
#define EMPEG_MIXER_READ_LOUDNESS_DB _IOR(EMPEG_MIXER_MAGIC, 5, int)
#define EMPEG_MIXER_WRITE_BALANCE _IOW(EMPEG_MIXER_MAGIC, 6, int)
#define EMPEG_MIXER_READ_BALANCE _IOR(EMPEG_MIXER_MAGIC, 6, int)
#define EMPEG_MIXER_READ_BALANCE_DB _IOR(EMPEG_MIXER_MAGIC, 7, int)
#define EMPEG_MIXER_WRITE_FADE _IOW(EMPEG_MIXER_MAGIC, 8, int)
#define EMPEG_MIXER_READ_FADE _IOR(EMPEG_MIXER_MAGIC, 8, int)
#define EMPEG_MIXER_READ_FADE_DB _IOR(EMPEG_MIXER_MAGIC, 9, int)
#define EMPEG_MIXER_SET_EQ _IOW(EMPEG_MIXER_MAGIC, 10, int)
#define EMPEG_MIXER_GET_EQ _IOR(EMPEG_MIXER_MAGIC, 11, int)
#define EMPEG_MIXER_SET_EQ_FOUR_CHANNEL _IOW(EMPEG_MIXER_MAGIC, 12, int)
#define EMPEG_MIXER_GET_EQ_FOUR_CHANNEL _IOR(EMPEG_MIXER_MAGIC, 13, int)
#define EMPEG_MIXER_GET_COMPRESSION _IOR(EMPEG_MIXER_MAGIC, 14, int)
#define EMPEG_MIXER_SET_COMPRESSION _IOW(EMPEG_MIXER_MAGIC, 14, int)
#define EMPEG_MIXER_SET_SAM _IOW(EMPEG_MIXER_MAGIC, 15, int)

/* Retrieve volume level corresponding to 0dB */
#define EMPEG_MIXER_READ_ZERO_LEVEL _IOR(EMPEG_MIXER_MAGIC, 3, int)

#define EMPEG_MIXER_FLAG_MUTE (1<<0)
/*#define EMPEG_MIXER_FLAG_LOUDNESS (1<<1)*/

/* Radio IOCTLs */
/* These are in addition to those provided by the Video4Linux API */
#define EMPEG_RADIO_MAGIC 'r'
#define EMPEG_RADIO_READ_MONO _IOR(EMPEG_RADIO_MAGIC, 73, int)
#define EMPEG_RADIO_WRITE_MONO _IOW(EMPEG_RADIO_MAGIC, 73, int)
#define EMPEG_RADIO_READ_DX _IOR(EMPEG_RADIO_MAGIC, 74, int)
#define EMPEG_RADIO_WRITE_DX _IOW(EMPEG_RADIO_MAGIC, 74, int)
#define EMPEG_RADIO_READ_SENSITIVITY _IOR(EMPEG_RADIO_MAGIC, 75, int)
#define EMPEG_RADIO_WRITE_SENSITIVITY _IOW(EMPEG_RADIO_MAGIC, 75, int)
#define EMPEG_RADIO_SEARCH _IO(EMPEG_RADIO_MAGIC, 76) /* Pass in direction in *arg */
#define EMPEG_RADIO_GET_MULTIPATH _IOR(EMPEG_RADIO_MAGIC, 77, int)
#define EMPEG_RADIO_SET_STEREO _IOW(EMPEG_RADIO_MAGIC, 78, int)

#define EMPEG_DSP_BEEP _IOW(EMPEG_DSP_MAGIC, 0, int)
#define EMPEG_DSP_PURGE _IOR(EMPEG_DSP_MAGIC, 1, int)

/* Audio input IOCTLs */
#define EMPEG_AUDIOIN_MAGIC 'c'
#define EMPEG_AUDIOIN_READ_SAMPLERATE _IOR(EMPEG_AUDIOIN_MAGIC, 0, int)
#define EMPEG_AUDIOIN_WRITE_SAMPLERATE _IOW(EMPEG_AUDIOIN_MAGIC, 1, int)
#define EMPEG_AUDIOIN_READ_CHANNEL _IOR(EMPEG_AUDIOIN_MAGIC, 2, int)
#define EMPEG_AUDIOIN_WRITE_CHANNEL _IOW(EMPEG_AUDIOIN_MAGIC, 3, int)
#define EMPEG_AUDIOIN_READ_STEREO _IOR(EMPEG_AUDIOIN_MAGIC, 4, int)
#define EMPEG_AUDIOIN_WRITE_STEREO _IOW(EMPEG_AUDIOIN_MAGIC, 5, int)
#define EMPEG_AUDIOIN_READ_GAIN _IOR(EMPEG_AUDIOIN_MAGIC, 6, int)
#define EMPEG_AUDIOIN_WRITE_GAIN _IOW(EMPEG_AUDIOIN_MAGIC, 7, int)
#define EMPEG_AUDIOIN_CHANNEL_DSPOUT 0
#define EMPEG_AUDIOIN_CHANNEL_AUXIN  1
#define EMPEG_AUDIOIN_CHANNEL_MIC    2

/* Where flash is mapped in the empeg's kernel memory map */
#define EMPEG_FLASHBASE		0xd0000000

/* Power control IOCTLs */
#define EMPEG_POWER_MAGIC 'p'
#define EMPEG_POWER_TURNOFF _IO(EMPEG_POWER_MAGIC, 0)
#define EMPEG_POWER_WAKETIME _IOW(EMPEG_POWER_MAGIC, 1, int)
#define EMPEG_POWER_READSTATE _IOR(EMPEG_POWER_MAGIC, 2, int)

/* State storage ioctls */
/* Shouldn't need either of these in normal use. */
#define EMPEG_STATE_MAGIC 's'
#define EMPEG_STATE_FORCESTORE _IO(EMPEG_STATE_MAGIC, 74)
#define EMPEG_STATE_FAKEPOWERFAIL _IO(EMPEG_STATE_MAGIC, 75)

#ifndef __ASSEMBLY__
struct empeg_eq_section_t
{
	unsigned int word1;
	unsigned int word2;
};

#ifdef __KERNEL__
extern void audio_emitted_action(void);
extern int audio_get_fm_level(void);
extern int audio_get_stereo(void);
extern int audio_get_multipath(void);
#ifdef CONFIG_EMPEG_STATE
extern void enable_powerfail(int enable);
extern int powerfail_enabled(void);
#else
static inline void enable_powerfail(int enable)
{
	enable = enable;
}
#endif /* CONFIG_EMPEG_STATE */

#ifdef CONFIG_EMPEG_DISPLAY
static inline void display_powerfail_action(void)
{
	/* Mute audio & turn off display */
	GPCR=EMPEG_DSPPOM | EMPEG_DISPLAYPOWER;

	/* Turn off scan */
	LCCR0=0;
}
extern void display_powerreturn_action(void);
#endif

static inline int empeg_hardwarerevision(void)
{
	/* Return hardware revision */
	unsigned int *id=(unsigned int*)(EMPEG_FLASHBASE+0x2000);

	return(id[0]);
}

static inline unsigned int get_empeg_id(void)
{
	unsigned int *flash=(unsigned int*)(EMPEG_FLASHBASE+0x2000);
	return flash[1];
}

#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARCH_EMPEG_H */
