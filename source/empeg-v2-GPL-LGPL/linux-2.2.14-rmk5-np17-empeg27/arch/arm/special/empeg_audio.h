#ifndef EMPEG_AUDIO_H
#define EMPEG_AUDIO_H 1

/* DSP memory: incomplete list of locations, but enough for now */
#define X_modpntr    0x000
#define X_levn	     0x001
#define X_leva	     0x002
#define X_mlta       0x006
#define X_mltflim    0x007
#define X_pltd	     0x00e
#define X_leva_u     0x019
#define X_stepSize   0x120
#define X_counterX   0x121
#define X_plusmax    0x122
#define X_minmax     0x123
#define Y_mod00      0x800 /* Filtered FM level */
#define Y_p1	     0x81d
#define Y_q1	     0x81e
#define Y_c1	     0x821
#define Y_p7	     0x840
#define Y_q7	     0x841
#define Y_minsmtcn   0x842
#define Y_p2	     0x84d
#define Y_q2	     0x84e
#define Y_minsmtc    0x84f
#define Y_compry0st_28	0x856
#define Y_p3	     0x861
#define Y_q3	     0x862
#define Y_E_strnf_str 0x867
#define Y_E_mltp_str 0x868
#define Y_stro	     0x869
#define Y_p5	     0x86d
#define Y_q5	     0x86e
#define Y_E_strnf_rsp 0x872
#define Y_E_mltp_rsp 0x873
#define Y_EMute      0x887
#define Y_VGA        0x8e0
#define Y_KLCl       0x8e1
#define Y_KLCh       0x8e2
#define Y_KLBl       0x8e3
#define Y_KLBh       0x8e4
#define Y_KLA0l      0x8e5
#define Y_KLA0h      0x8e6
#define Y_KLA2l      0x8e7
#define Y_KLA2h      0x8e8
#define Y_KLtre      0x8e9
#define Y_KLbas      0x8ea
#define Y_KLmid      0x8eb
#define Y_VAT        0x8ec
#define Y_SAM	     0x8ed
#define Y_OutSwi     0x8ee
#define Y_SrcScal    0x8f3
#define Y_samCl      0x8f4
#define Y_samCh      0x8f5
#define Y_delta      0x8f6
#define Y_switch     0x8f7
#define Y_louSwi     0x8f9
#define Y_statLou    0x8fa
#define Y_OFFS       0x8fb
#define Y_KPDL       0x8fc
#define Y_KMDL       0x8fd
#define Y_Cllev      0x8fe
#define Y_Ctre       0x8ff
#define Y_EMuteF1    0x90e
#define Y_scalS1_    0x927
#define Y_scalS1     0x928
#define Y_cpyS1      0x92a
#define Y_cpyS1_     0x92b
#define Y_c3sin      0x92e
#define Y_c1sin      0x92f
#define Y_c0sin      0x931
#define Y_c2sin      0x932
#define Y_VLsin      0x933
#define Y_VRsin      0x934
#define Y_IClipAmax  0x935
#define Y_IClipAmin  0x936
#define Y_IcoefAl    0x937
#define Y_IcoefAh    0x938
#define Y_IClipBmax  0x939
#define Y_IClipBmin  0x93a
#define Y_IcoefBL    0x93b
#define Y_IcoefBH    0x93c
#define Y_samDecl    0x93d
#define Y_samDech    0x93e
#define Y_deltaD     0x93f
#define Y_switchD    0x940
#define Y_samAttl    0x941
#define Y_samAtth    0x942
#define Y_deltaA     0x943
#define Y_switchA    0x944
#define Y_iSinusWant 0x946
#define Y_sinusMode  0x947
#define Y_tfnFL	     0x948
#define Y_tfnFR	     0x949
#define Y_tfnBL	     0x94a
#define Y_tfnBR	     0x94b
#define Y_BALL0	     0x8bc
#define Y_BALR0	     0x8bd
#define Y_BALL1      0x8ca
#define Y_BALR1      0x8cb
#define Y_FLcof      0x8ef
#define Y_FRcof      0x8f0
#define Y_RLcof      0x8f1
#define Y_RRcof      0x8f2


/*
 * Empeg I2C support
 */

#define IIC_CLOCK  	EMPEG_I2CCLOCK
#define IIC_DATAOUT	EMPEG_I2CDATA
#define IIC_DATAIN 	EMPEG_I2CDATAIN

static __inline__ void i2c_delay(void)
{
	udelay(15); // as low as revision 4 will go
//	schedule();
}

/*
 * Pulse out a single bit, assumes the current state is clock low
 * and that the data direction is outbound.
 */

static __inline__ void i2c_putdatabit(int bit)
{
	/* First set the data bit (clock low) */
	GPCR=IIC_CLOCK;
	if (bit) {
	    if (GPLR&IIC_DATAOUT) {
		GPCR = IIC_DATAOUT;
		udelay(14);
	    }
	}
	else {
	    if (!(GPLR&IIC_DATAOUT)) {
		GPSR = IIC_DATAOUT;
	    }
	}
	udelay(1);

	/* Now trigger the clock */
	GPSR=IIC_CLOCK;
	udelay(1);
	
	/* Drop the clock */
	GPCR=IIC_CLOCK;
	udelay(1);
}

/*
 * Read in a single bit, assumes the current state is clock low
 * and that the data direction is inbound.
 */

static __inline__ int i2c_getdatabit(void)
{
	int result;

	/* Trigger the clock */
	GPSR=IIC_CLOCK;
	udelay(15);
	
	/* Wait for the slave to give me the data */
	result = !(GPLR & IIC_DATAIN);
//	udelay(1);

	/* Now take the clock low */
	GPCR = IIC_CLOCK;
	udelay(15);
	return result;
}

/* Pulse out the start sequence */

static __inline__ void i2c_startseq(void)
{
	/* Clock low, data high */
	GPCR=IIC_CLOCK|IIC_DATAOUT;
	udelay(15);

	/* Put clock high */
	GPSR=IIC_CLOCK;
	udelay(1);

	/* Put data low */
	GPSR=IIC_DATAOUT;
	udelay(1);

	/* Clock low again */
	GPCR=IIC_CLOCK;
	udelay(15);
}

/* Pulse out the stop sequence */

static __inline__ void i2c_stopseq(void)
{
	/* Data low, clock low */
	GPCR=IIC_CLOCK;
	GPSR=IIC_DATAOUT;
	udelay(15);

	/* Clock high */
	GPSR=IIC_CLOCK;
	udelay(1);
	
	/* Let data float high */
	GPCR=IIC_DATAOUT;
	udelay(15);
}

/*
 * Pulse out a complete byte and receive acknowledge.
 * Returns 0 on success, non-zero on failure.
 */
static __inline__ int i2c_putbyte(int byte)
{
	int i, ack;
#ifdef I2C_LOCK
	unsigned long flags;
	save_flags_cli(flags);
#endif

	/* Clock/data low */
	GPCR=IIC_CLOCK;
	GPSR=IIC_DATAOUT;

	/* Clock out the data */
	for(i = 7; i >= 0; --i)
		i2c_putdatabit(byte & (1 << i));
	
	/* data high (ie, no drive) */
	GPCR = IIC_DATAOUT | IIC_CLOCK;

	i2c_delay();
	
	/* Clock out */
	GPSR = IIC_CLOCK;
	
	/* Wait for ack to arrive */
	i2c_delay();

	ack = !(GPLR & IIC_DATAIN);

	i2c_delay();
	/* Clock low */
	GPCR = IIC_CLOCK;
	
	i2c_delay();

	if (ack) {
		i2c_stopseq();
		{ int a,b=0; for(a=0;a<100000;a++) b+=a; }
		printk(KERN_ERR "i2c: Failed to receive ACK for data!\n");
	}

#ifdef I2C_LOCK
	restore_flags(flags);
#endif

	return ack;
}

/*
 * Read an entire byte and send out acknowledge.
 * Returns byte read.
 */

static __inline__ int i2c_getbyte(unsigned char *byte, int nak)
{
	int i;
#ifdef I2C_LOCK
	unsigned long flags;	
	save_flags_cli(flags);
#endif

	/* Let data line float */
	GPCR = IIC_DATAOUT;
	i2c_delay();

	*byte = 0;
	/* Clock in the data */
	for(i = 7; i >= 0; --i)
		if (i2c_getdatabit())
			(*byte) |= (1 << i);
	
	/* Well, I got it so respond with an ack, or nak */
	
	/* Send data low to indicate success */
	if(!nak) {
		GPSR = IIC_DATAOUT;
	}
	else {
		GPCR = IIC_DATAOUT;
	}
	i2c_delay();

	/* Trigger clock since data is ready */
	GPSR = IIC_CLOCK;
	i2c_delay();

	/* Take clock low */

	GPCR = IIC_CLOCK;
	i2c_delay();

	/* Release data line */
	GPCR = IIC_DATAOUT;
	i2c_delay();

#ifdef I2C_LOCK
	restore_flags(flags);
#endif

	return 0; /* success */
}
	
	

/* Write to one or more I2C registers */

static __inline__ int i2c_write(unsigned char device, unsigned short address,
					 unsigned int *data, unsigned short count)
{
#ifdef I2C_LOCK
	unsigned long flags;
	save_flags_cli(flags);
#endif

	/* Pulse out the start sequence */
	i2c_startseq();

	/* Say who we're talking to */
	if (i2c_putbyte(device & 0xFE)) {
		printk("i2c_write: device select failed\n");
		goto i2c_error;
	}

	/* Set the address (higher then lower) */
	if (i2c_putbyte(address >> 8) || i2c_putbyte(address & 0xFF)) {
		printk("i2c_write: address select failed\n");
		goto i2c_error;
	}

	/* Now send the actual data */
	while(count--)
	{
		if (address<0x200) {
			/* Send out the 24 bit quantity */
			
			/* Mask off the top eight bits in certain situations! */
			if (i2c_putbyte((*data>>16)&0xff)) {
				printk("i2c_write: write first byte failed"
				       ", count:%d\n", count);
				goto i2c_error;
			}
			if (i2c_putbyte((*data >> 8) & 0xFF)) {
				printk("i2c_write: write second byte failed"
				       ", count:%d\n", count);
				goto i2c_error;
			}
			if (i2c_putbyte(*data & 0xFF)) {
				printk("i2c_write: write third byte failed"
				       ", count:%d\n", count);
				goto i2c_error;
			}
		}
		else {
			/* Send out 16 bit quantity */
			/* Mask off the top eight bits in certain situations! */
			if (i2c_putbyte(*data >> 8)) {
				printk("i2c_write: write first byte failed"
				       ", count:%d\n", count);
				goto i2c_error;
			}
			if (i2c_putbyte(*data & 0xFF)) {
				printk("i2c_write: write second byte failed"
				       ", count:%d\n", count);
				goto i2c_error;
			}
		}
		++data;
	}
	
	i2c_stopseq();

#ifdef I2C_LOCK
	restore_flags(flags);
#endif

	/* Complete success */
	return 0;

 i2c_error:
#ifdef I2C_LOCK
	restore_flags(flags);
#endif

	/* Complete failure */
	return -1;
}


static __inline__ int i2c_read(unsigned char device, unsigned short address, unsigned int *data, int count)
{
#ifdef I2C_LOCK
	unsigned long flags;
	save_flags_cli(flags);
#endif

	/* Send start sequence */
	i2c_startseq();

	/* Set the device */
	if (i2c_putbyte(device & 0xFE))
		goto i2c_error;

	/* Set the address (higher then lower) */
	if (i2c_putbyte(address >> 8) || i2c_putbyte(address & 0xFF))
		goto i2c_error;

	/* Repeat the start sequence */
	i2c_startseq();
	
	/* Set the device but this time in read mode */
	if (i2c_putbyte(device | 0x01))
		goto i2c_error;

	/* Now read in the actual data */
	while(count--)
	{
		unsigned char b1, b2, b3;
		if(address < 0x200) {
			if (i2c_getbyte(&b1, 0) ||
			    i2c_getbyte(&b2, 0) ||
			    i2c_getbyte(&b3, 1))
				goto i2c_error;
			*data++ = (b1 << 16) | (b2 << 8) | b3;
		} else {
			/* Receive the 16 bit quantity */
			if (i2c_getbyte(&b1, 0) ||
			    i2c_getbyte(&b2, 1))
				goto i2c_error;
			*data++ = (b1 << 8) | b2;
		}
	}

	/* Now say we don't want any more: NAK (send bit 1) */
	i2c_putdatabit(1);

	i2c_stopseq();	
	
#ifdef I2C_LOCK
	restore_flags(flags);
#endif

	return 0;

 i2c_error:
#ifdef I2C_LOCK
	restore_flags(flags);
#endif

	return -1;
}

static __inline__ int i2c_write1(unsigned char device, unsigned short address, unsigned int data)
{
	return i2c_write(device, address, &data, 1);
}

static __inline__ int i2c_read1(unsigned char device, unsigned short address, unsigned int *data)
{
	return i2c_read(device, address, data, 1);
}

/* THIS SHOULDN'T BE IN HERE, IT'S ONLY VISITING!
   <altman@empeg.com> */

/* Some DSP defines */
#define IICD_DSP 0x38

#define IIC_DSP_SEL 0x0FFA
#define IIC_DSP_SEL_RESERVED0 0x01
#define IIC_DSP_SEL_AUX_FM 0x02
#define IIC_DSP_SEL_AUX_AM_TAPE 0x04
#define IIC_DSP_SEL_AUX_CD_TAPE 0x08
#define IIC_DSP_SEL_RESERVED1 0x10
#define IIC_DSP_SEL_LEV_AMFM 0x20
#define IIC_DSP_SEL_LEV_WIDENARROW 0x40
#define IIC_DSP_SEL_LEV_DEF 0x80
#define IIC_DSP_SEL_BYPASS_PLL 0x100
#define IIC_DSP_SEL_DC_OFFSET 0x200
#define IIC_DSP_SEL_RESERVED2 0x400
#define IIC_DSP_SEL_ADC_SRC 0x800
#define IIC_DSP_SEL_NSDEC 0x1000
#define IIC_DSP_SEL_INV_HOST_WS 0x2000
#define IIC_DSP_SEL_RESERVED3 0x4000
#define IIC_DSP_SEL_ADC_BW_SWITCH 0x8000

#endif
