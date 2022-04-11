/* Header for the power-pic device support in the empeg */

#ifndef EMPEG_POWER_H
#define EMPEG_POWER_H

/* There are multiple instances of this structure for the different
   channels provided */
struct power_dev
{
	time_t alarmtime;           /* Current alarm time */
};

/* Declarations */
static int power_ioctl(struct inode*,struct file*,unsigned int,unsigned long);
static int power_open(struct inode*,struct file*);
static int power_release(struct inode*,struct file*);

/* External initialisation */
void empeg_power_init(void);

#endif

