/* empeg-car radio driver
 * (C) 1999-2000 empeg Ltd
 *
 * Based on the radiotrack driver distributed with Linux 2.2.0pre9.
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 */

/* This needs updating to support the new Marvin radio */

#include <linux/module.h>	/* Modules 			*/
#include <linux/init.h>		/* Initdata			*/
#include <linux/ioport.h>	/* check_region, request_region	*/
#include <linux/delay.h>	/* udelay			*/
#include <asm/io.h>		/* outb, outb_p			*/
#include <asm/uaccess.h>	/* copy to/from user		*/
#include <linux/videodev.h>	/* kernel radio structs		*/
#include <linux/config.h>
#include <asm/arch/empeg.h>

#define FLAG_SEARCH (1<<24)
#define FLAG_SEARCH_UP (1<<23)
#define FLAG_SEARCH_DOWN (0)
#define FLAG_MONO (1<<22)
#define FLAG_STEREO (0)
#define FLAG_BAND_FM (0)
#define FLAG_LOCAL (0)
#define FLAG_DX (1<<19)
#define FLAG_SENSITIVITY_HIGH (0)
#define FLAG_SENSITIVITY_MEDIUM (1<<17)
#define FLAG_SENSITIVITY_LOW (1<<16)
#define FLAG_SENSITIVITY_VERYLOW ((1<<16)|(1<<17))
#define FREQUENCY_MASK (0x7fff)

void audio_clear_stereo(void);
void audio_set_stereo(int on);

static int users = 0;

struct empeg_radio_device
{
	/* Eventually we need to put lock strength, stereo/mono
	 * etc. in here. */
	unsigned long freq;
	int mono;
	int dx;
	int sensitivity;
};

/* Sonja has an I2Cesque radio data interface */
#define RADIO_DATAOUT   GPIO_GPIO10
#define RADIO_DATAIN    GPIO_GPIO14
#define RADIO_WRITE     GPIO_GPIO13
#define RADIO_CLOCK     GPIO_GPIO12

static void philips_radio_writebit(int data)
{
	if (data)
		GPCR=EMPEG_RADIODATA;
	else
		GPSR=EMPEG_RADIODATA;
	GPSR=EMPEG_RADIOCLOCK;
	udelay(7);
	GPCR=EMPEG_RADIOCLOCK;
	udelay(7);
}

static void philips_radio_writeword(int data)
{
	int a;
	
	/* Put radio module in write mode */
	GPSR=RADIO_WRITE;
	udelay(10);
	
	/* Clock out a 25-bit frame */
	for(a=24;a>=0;a--) philips_radio_writebit(data&(1<<a));
	
	/* Ensure radio data is floating again */
	GPCR=RADIO_DATAOUT;
	
	/* Out of write mode */
	GPCR=RADIO_WRITE;
}

static int empeg_philips_radio_update(struct empeg_radio_device *dev, int search, int direction)
{
	int word = 0;

	int a, divisor = 102400000;
        unsigned long freq = dev->freq;
	if (dev->mono)
		word |= FLAG_MONO;
	else
		word |= FLAG_STEREO;

	if (dev->dx)
		word |= FLAG_DX;
	else
		word |= FLAG_LOCAL;

	switch (dev->sensitivity)
	{
	case 0:
		word |= FLAG_SENSITIVITY_VERYLOW;
		break;
	case 1:
		word |= FLAG_SENSITIVITY_LOW;
		break;
	case 2:
		word |= FLAG_SENSITIVITY_MEDIUM;
		break;
	case 3:
	default:
		word |= FLAG_SENSITIVITY_HIGH;
		break;
	}
	
	if (search)
	{
		word |= FLAG_SEARCH;
		if (direction)
			word |= FLAG_SEARCH_UP;
		else
			word |= FLAG_SEARCH_DOWN;
	}
	
	/* Add 10.7Mhz IF */
	freq+=10700000;
	
	/* Set bits */
	for(a = 13; a >= 0; a--)
	{
		if (freq>=divisor)
		{
			word|=(1<<a);
			freq-=divisor;
		}
		divisor>>=1;
	}
	
	philips_radio_writeword(word);

	return 0;
}

static void marvin_radio_writeword(int data)
{
	/* All shifted to top end to make PIC code neater */
	unsigned int frame=(data<<7);

	/* Write 4 byte frame to radio */
	Ser1UTDR=1; /* SOH */
	Ser1UTDR=2; /* Tune command */
	Ser1UTDR=(frame    )&0xff;
	Ser1UTDR=(frame>> 8)&0xff;
	Ser1UTDR=(frame>>16)&0xff;
	Ser1UTDR=(frame>>24)&0xff;
}

static int empeg_marvin_radio_update(struct empeg_radio_device *dev, int search, int direction)
{
	int word = 0;
	int a, divisor = 102400000;
        unsigned long freq = dev->freq;
	if (dev->mono)
		word |= FLAG_MONO;
	else
		word |= FLAG_STEREO;

	if (1 /*dev->dx*/)
		word |= FLAG_DX;
	else
		word |= FLAG_LOCAL;

	switch (dev->sensitivity)
	{
	case 0:
		word |= FLAG_SENSITIVITY_VERYLOW;
		break;
	case 1:
		word |= FLAG_SENSITIVITY_LOW;
		break;
	case 2:
		word |= FLAG_SENSITIVITY_MEDIUM;
		break;
	case 3:
	default:
		word |= FLAG_SENSITIVITY_HIGH;
		break;
	}
	
	if (search)
	{
		word |= FLAG_SEARCH;
		if (direction)
			word |= FLAG_SEARCH_UP;
		else
			word |= FLAG_SEARCH_DOWN;
	}
	
	/* Add 10.7Mhz IF */
	freq+=10700000;
	
	/* Set bits */
	for(a = 13; a >= 0; a--)
	{
		if (freq>=divisor)
		{
			word|=(1<<a);
			freq-=divisor;
		}
		divisor>>=1;
	}
	
	marvin_radio_writeword(word);

	return 0;
}

int empeg_philips_radio_getsigstr(struct empeg_radio_device *dev)
{
	unsigned signal = audio_get_fm_level();
	if ((signal < 0) || (signal > 131071)) signal = 0;
	else {
		signal >>= 1;	// 18 bits signed -> 16 bits unsigned
	}
	return (int) signal;
}

int empeg_philips_radio_getstereo(struct empeg_radio_device *dev)
{
	return audio_get_stereo();
}

static int empeg_philips_radio_ioctl(struct video_device *dev, unsigned int cmd, void *arg)
{
	struct empeg_radio_device *radio=dev->priv;
	
	switch(cmd)
	{
	case VIDIOCGCAP:
	{
		struct video_capability v;
		v.type=VID_TYPE_TUNER;
		v.channels=1;
		v.audios=1;
		/* No we don't do pictures */
		v.maxwidth=0;
		v.maxheight=0;
		v.minwidth=0;
		v.minheight=0;
		strcpy(v.name, "empeg-car radio");
		if(copy_to_user(arg,&v,sizeof(v)))
			return -EFAULT;
		return 0;
	}
	case VIDIOCGTUNER:
	{
		struct video_tuner v;
		if(copy_from_user(&v, arg,sizeof(v))!=0) 
			return -EFAULT;
		if(v.tuner)	/* Only 1 tuner */ 
			return -EINVAL;
		v.rangelow = 87500000;
		v.rangehigh = 108000000;
		v.flags = 0;
		if(empeg_philips_radio_getstereo(radio))
			v.flags |= VIDEO_TUNER_STEREO_ON;
		v.mode = VIDEO_MODE_AUTO;
		v.signal = empeg_philips_radio_getsigstr(radio);

		if(copy_to_user(arg,&v, sizeof(v)))
			return -EFAULT;
		return 0;
	}
	case VIDIOCSTUNER:
	{
		struct video_tuner v;
		if(copy_from_user(&v, arg, sizeof(v)))
			return -EFAULT;
		if(v.tuner!=0)
			return -EINVAL;
		/* Only 1 tuner so no setting needed ! */
		return 0;
	}
	case VIDIOCGFREQ:
		if(copy_to_user(arg, &radio->freq, sizeof(radio->freq)))
			return -EFAULT;
		return 0;
	case VIDIOCSFREQ:
		if(copy_from_user(&radio->freq, arg,sizeof(radio->freq)))
			return -EFAULT;
		empeg_philips_radio_update(radio, FALSE, 0);
		audio_clear_stereo(); // reset stereo level to 0
		return 0;
	case VIDIOCGAUDIO:
	{	
		struct video_audio v;
		memset(&v,0, sizeof(v));
		/*v.flags|=VIDEO_AUDIO_MUTABLE|VIDEO_AUDIO_VOLUME; */
		/*v.volume=radio->curvol * 6554;*/
		/*v.step=6554;*/
		strcpy(v.name, "Radio");
		if(copy_to_user(arg,&v, sizeof(v)))
			return -EFAULT;
		return 0;			
	}
	case VIDIOCSAUDIO:
	{
		struct video_audio v;
		if(copy_from_user(&v, arg, sizeof(v))) 
			return -EFAULT;	
		if(v.audio) 
			return -EINVAL;
		
		/* Check the mode for stereo/mono */
		if (v.mode == VIDEO_SOUND_MONO)
			radio->mono = TRUE;
		else
			radio->mono = FALSE;
		empeg_philips_radio_update(radio, FALSE, 0);
		return 0;
	}
	case EMPEG_RADIO_READ_MONO:
		copy_to_user_ret(arg, &(radio->mono), sizeof(radio->mono), -EFAULT);
		return 0;

	case EMPEG_RADIO_WRITE_MONO:
		copy_from_user_ret(&(radio->mono), arg, sizeof(radio->mono), -EFAULT);
		empeg_philips_radio_update(radio, FALSE, 0);
		return 0;

	case EMPEG_RADIO_READ_DX:
		copy_to_user_ret(arg, &(radio->dx), sizeof(radio->dx), -EFAULT);
		return 0;

	case EMPEG_RADIO_WRITE_DX:
		copy_from_user_ret(&(radio->dx), arg, sizeof(radio->dx), -EFAULT);
		empeg_philips_radio_update(radio, FALSE, 0);
		return 0;

	case EMPEG_RADIO_READ_SENSITIVITY:
		copy_to_user_ret(arg, (&radio->sensitivity), sizeof(radio->sensitivity), -EFAULT);
		return 0;

	case EMPEG_RADIO_WRITE_SENSITIVITY:
		copy_from_user_ret(&(radio->sensitivity), arg, sizeof(radio->sensitivity), -EFAULT);
		empeg_philips_radio_update(radio, FALSE, 0);
		return 0;

	case EMPEG_RADIO_SEARCH:
	{
		int direction;
		copy_from_user_ret(&direction, arg, sizeof(direction), -EFAULT);
		empeg_philips_radio_update(radio, TRUE, direction);
		return 0;
	}
	case EMPEG_RADIO_GET_MULTIPATH: {
		unsigned multi = audio_get_multipath();
		if(multi == -1) return -EIO;
		copy_to_user_ret(arg, &multi, sizeof(unsigned), -EFAULT);
		return 0;
	}
	case EMPEG_RADIO_SET_STEREO: {
		int stereo;
		copy_from_user_ret(&stereo, arg, sizeof(int), -EFAULT);
		audio_set_stereo(stereo);
		return 0;
	}
	default:
		return -ENOIOCTLCMD;
	}
}

static int empeg_philips_radio_open(struct video_device *dev, int flags)
{
	if(users)
		return -EBUSY;
	users++;
	MOD_INC_USE_COUNT;
	return 0;
}

static void empeg_philips_radio_close(struct video_device *dev)
{
	users--;
	MOD_DEC_USE_COUNT;
}

static int empeg_marvin_radio_open(struct video_device *dev, int flags)
{
	if (users)
		return -EBUSY;

	users++;
	MOD_INC_USE_COUNT;
	return 0;
}

static void empeg_marvin_radio_close(struct video_device *dev)
{
	users--;
	MOD_DEC_USE_COUNT;
}

static int empeg_marvin_radio_ioctl(struct video_device *dev, unsigned int cmd, void *arg)
{
	struct empeg_radio_device *radio=dev->priv;
	
	/* For now we'll just accept everything */
	switch(cmd)
	{
	case VIDIOCGCAP:
	{
		struct video_capability v;
		v.type=VID_TYPE_TUNER;
		v.channels=1;
		v.audios=1;
		/* No we don't do pictures */
		v.maxwidth=0;
		v.maxheight=0;
		v.minwidth=0;
		v.minheight=0;
		strcpy(v.name, "empeg-car radio");
		if(copy_to_user(arg,&v,sizeof(v)))
			return -EFAULT;
		return 0;
	}
	case VIDIOCGTUNER:
	{
		struct video_tuner v;
		if(copy_from_user(&v, arg,sizeof(v))!=0) 
			return -EFAULT;
		if(v.tuner)	/* Only 1 tuner */ 
			return -EINVAL;
		v.rangelow = 87500000;
		v.rangehigh = 108000000;
		v.flags = 0;
		v.mode = VIDEO_MODE_AUTO;
		v.signal = empeg_philips_radio_getsigstr(radio);

		if(copy_to_user(arg,&v, sizeof(v)))
			return -EFAULT;
		return 0;
	}
	case VIDIOCSTUNER:
	{
		struct video_tuner v;
		if(copy_from_user(&v, arg, sizeof(v)))
			return -EFAULT;
		if(v.tuner!=0)
			return -EINVAL;
		/* Only 1 tuner so no setting needed ! */
		return 0;
	}
	case VIDIOCGFREQ:
		if(copy_to_user(arg, &radio->freq, sizeof(radio->freq)))
			return -EFAULT;
		return 0;
	case VIDIOCSFREQ:
		if(copy_from_user(&radio->freq, arg,sizeof(radio->freq)))
			return -EFAULT;
		empeg_marvin_radio_update(radio, FALSE, 0);
		audio_clear_stereo(); // reset stereo level to 0
		return 0;
	case VIDIOCGAUDIO:
	{	
		struct video_audio v;
		memset(&v,0, sizeof(v));
		/*v.flags|=VIDEO_AUDIO_MUTABLE|VIDEO_AUDIO_VOLUME; */
		/*v.volume=radio->curvol * 6554;*/
		/*v.step=6554;*/
		strcpy(v.name, "Radio");
		if(copy_to_user(arg,&v, sizeof(v)))
			return -EFAULT;
		return 0;			
	}
	case VIDIOCSAUDIO:
	{
		struct video_audio v;
		if(copy_from_user(&v, arg, sizeof(v))) 
			return -EFAULT;	
		if(v.audio) 
			return -EINVAL;
		
		/* Check the mode for stereo/mono */
		if (v.mode == VIDEO_SOUND_MONO)
			radio->mono = TRUE;
		else
			radio->mono = FALSE;
		empeg_marvin_radio_update(radio, FALSE, 0);
		return 0;
	}
	case EMPEG_RADIO_READ_MONO:
		copy_to_user_ret(arg, &(radio->mono), sizeof(radio->mono), -EFAULT);
		return 0;

	case EMPEG_RADIO_WRITE_MONO:
		copy_from_user_ret(&(radio->mono), arg, sizeof(radio->mono), -EFAULT);
		return 0;

	case EMPEG_RADIO_READ_DX:
		copy_to_user_ret(arg, &(radio->dx), sizeof(radio->dx), -EFAULT);
		return 0;

	case EMPEG_RADIO_WRITE_DX:
		copy_from_user_ret(&(radio->dx), arg, sizeof(radio->dx), -EFAULT);
		empeg_marvin_radio_update(radio, FALSE, 0);
		return 0;

	case EMPEG_RADIO_READ_SENSITIVITY:
		copy_to_user_ret(arg, (&radio->sensitivity), sizeof(radio->sensitivity), -EFAULT);
		return 0;

	case EMPEG_RADIO_WRITE_SENSITIVITY:
		copy_from_user_ret(&(radio->sensitivity), arg, sizeof(radio->sensitivity), -EFAULT);
		empeg_marvin_radio_update(radio, FALSE, 0);
		return 0;

	case EMPEG_RADIO_SEARCH:
	{
		int direction;
		copy_from_user_ret(&direction, arg, sizeof(direction), -EFAULT);
		empeg_marvin_radio_update(radio, TRUE, direction);
		return 0;
	}
	case EMPEG_RADIO_GET_MULTIPATH: {
		unsigned multi = audio_get_multipath();
		if(multi == -1) return -EIO;
		copy_to_user_ret(arg, &multi, sizeof(unsigned), -EFAULT);
		return 0;
	}
	case EMPEG_RADIO_SET_STEREO: {
		int stereo;
		copy_from_user_ret(&stereo, arg, sizeof(int), -EFAULT);
		audio_set_stereo(stereo);
		return 0;
	}
	default:
		return -ENOIOCTLCMD;
	}
	
	return 0;
}

static struct empeg_radio_device empeg_unit;

static struct video_device empeg_philips_radio=
{
	"empeg Philips FM radio",
	VID_TYPE_TUNER,
	VID_HARDWARE_RTRACK,
	empeg_philips_radio_open,
	empeg_philips_radio_close,
	NULL,	/* Can't read  (no capture ability) */
	NULL,	/* Can't write */
	NULL,	/* No poll */
	empeg_philips_radio_ioctl,
	NULL,
	NULL
};

static struct video_device empeg_marvin_radio=
{
	"empeg mk2 radio",
	VID_TYPE_TUNER,
	VID_HARDWARE_RTRACK,
	empeg_marvin_radio_open,
	empeg_marvin_radio_close,
	NULL,	/* Can't read  (no capture ability) */
	NULL,	/* Can't write */
	NULL,	/* No poll */
	empeg_marvin_radio_ioctl,
	NULL,
	NULL
};

int __init empeg_radio_init(struct video_init *v)
{
	if (empeg_hardwarerevision() < 6) {
		empeg_philips_radio.priv = &empeg_unit;
		
		empeg_unit.freq = 87500000;
		empeg_unit.dx = 1;
		empeg_unit.mono = 0;
		empeg_unit.sensitivity = 10;
		
		if(video_register_device(&empeg_philips_radio, VFL_TYPE_RADIO)==-1)
			return -EINVAL;
		
		printk(KERN_INFO "empeg FM radio driver (Philips).\n");
		
	} else {
		int dtimeout,data,state,tries=0;
		unsigned char packet[5],checksum;

		/* Private word pointer */
		empeg_marvin_radio.priv = &empeg_unit;

		/* Set up serial port: 8-bit, 4800bps, no IRQs */
		Ser1UTCR0=UTCR0_DSS;
		Ser1UTCR1=0;
		Ser1UTCR2=47;
		Ser1UTCR3=UTCR3_RXE|UTCR3_TXE;

		while(tries<2) {
			//printk("probing, try %d\n",tries);

			/* Flush serial RX */
			while(Ser1UTSR1&UTSR1_RNE) data=Ser1UTDR;
			
			/* Check for radio: some padding then an ID command */
			Ser1UTDR=0;
			Ser1UTDR=0;
			Ser1UTDR=0;
			Ser1UTDR=0;
			Ser1UTDR=1; /* SOH */
			Ser1UTDR=1; /* ID yourself */
			
			/* Wait for response */
			dtimeout=jiffies+(HZ/4);
			state=0;
			while(jiffies<dtimeout && state!=7) {
				/* Any data? */
				if (Ser1UTSR1&UTSR1_RNE) {
					data=Ser1UTDR;
					//printk("got %02x, state=%d\n",data,state);
					switch(state) {
					case 0:
						if (data==1) {
							checksum=0;
							state++;
						}
						break;
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
						packet[state-1]=data;
						checksum+=data;
						state++;
						break;
					case 6:
						if (checksum==data) {
							/* Got good reply */
							state++;
						} else {
							printk("checksum error from radio (calc=%02x, rx=%02x)\n",checksum,data);		  
							state=0;
						}
						break;
					}
				}
				
			}
			
			if (state==7) break;
			
			/* Try again */
			tries++;
		}

		if (state==7) {
			/* Found unit */
			empeg_unit.freq = 87500000;
			empeg_unit.dx = 1;
			empeg_unit.mono = 0;
			empeg_unit.sensitivity = 10;
			
			if(video_register_device(&empeg_marvin_radio, VFL_TYPE_RADIO)==-1)
				return -EINVAL;
			
			printk(KERN_INFO "empeg radio unit found, capabilities %02x\n",packet[0]);
		} else {
			printk(KERN_INFO "no empeg radio unit found\n");
		}
	}

	return 0;
}

#ifdef MODULE

MODULE_AUTHOR("Mike Crowe");
MODULE_DESCRIPTION("A driver for the empeg-car FM radio.");
MODULE_SUPPORTED_DEVICE("radio");

EXPORT_NO_SYMBOLS;

int init_module(void)
{
	return empeg_radio_init(NULL);
}

void cleanup_module(void)
{
	video_unregister_device(&empeg_radio);
}

#endif
