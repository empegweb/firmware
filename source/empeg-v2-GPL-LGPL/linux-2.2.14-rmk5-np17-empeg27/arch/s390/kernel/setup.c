/*
 *  arch/s390/kernel/setup.c
 *
 *  S390 version
 *    Copyright (C) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Author(s): Hartmut Penner (hp@de.ibm.com),
 *               Martin Schwidefsky (schwidefsky@de.ibm.com)
 *
 *  Derived from "arch/i386/kernel/setup.c"
 *    Copyright (C) 1995, Linus Torvalds
 */

/*
 * This file handles the architecture-dependent parts of initialization
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/user.h>
#include <linux/a.out.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/config.h>
#include <linux/init.h>
#ifdef CONFIG_BLK_DEV_RAM
#include <linux/blk.h>
#endif
#include <linux/console.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/smp.h>

/*
 * Machine setup..
 */
__u16 boot_cpu_addr;
int cpus_initialized = 0;
unsigned long cpu_initialized = 0;

/*
 * Setup options
 */

#ifdef CONFIG_BLK_DEV_RAM
extern int rd_doload;                  /* 1 = load ramdisk, 0 = don't load */
extern int rd_prompt;           /* 1 = prompt for ramdisk, 0 = don't prompt*/
extern int rd_image_start;             /* starting block # of image        */
#endif

extern int root_mountflags;
extern int _etext, _edata, _end;


/*
 * This is set up by the setup-routine at boot-time
 * for S390 need to find out, what we have to setup
 * using address 0x10400 ...
 */

#include <asm/setup.h>

static char command_line[COMMAND_LINE_SIZE] = { 0, };
       char saved_command_line[COMMAND_LINE_SIZE];

/*
 * cpu_init() initializes state that is per-CPU.
 */
void cpu_init (void)
{
        int nr = smp_processor_id();

        if (test_and_set_bit(nr,&cpu_initialized)) {
                printk("CPU#%d ALREADY INITIALIZED!!!!!!!!!\n", nr);
                for (;;) __sti();
        }
        cpus_initialized++;

        /*
         * Store processor id in lowcore (used e.g. in timer_interrupt)
         */
        asm volatile ("stidp %0": "=m" (S390_lowcore.cpu_data.cpu_id));
        S390_lowcore.cpu_data.cpu_addr = hard_smp_processor_id();
        S390_lowcore.cpu_data.cpu_nr = nr;

        /*
         * Force FPU initialization:
         */
        current->flags &= ~PF_USEDFPU;
        current->used_math = 0;
}

/*
 * Reboot, halt and power_off routines for non SMP.
 */
#ifndef __SMP__
void machine_restart(char * __unused)
{
        if (MACHINE_IS_VM) {
                cpcmd("IPL", NULL, 0);
        } else {
                /* FIXME: how to reipl ? */
                disabled_wait(2);
        }
}

void machine_halt(void)
{
        if (MACHINE_IS_VM) {
                cpcmd("IPL 200 STOP", NULL, 0);
        } else {
                disabled_wait(0);
        }
}

void machine_power_off(void)
{
        if (MACHINE_IS_VM) {
                cpcmd("IPL CMS", NULL, 0);
        } else {
                disabled_wait(0);
        }
}
#endif

/*
 * Waits for 'delay' microseconds using the tod clock
 */
void tod_wait(unsigned long delay)
{
        uint64_t start_cc, end_cc;

	if (delay == 0)
		return;
        asm volatile ("STCK %0" : "=m" (start_cc));
	do {
		asm volatile ("STCK %0" : "=m" (end_cc));
	} while (((end_cc - start_cc)/4096) < delay);
}

/*
 * Setup function called from init/main.c just after the banner
 * was printed.
 */
__initfunc(void setup_arch(char **cmdline_p,
        unsigned long * memory_start_p, unsigned long * memory_end_p))
{
        unsigned long memory_start, memory_end;
        char c = ' ', *to = command_line, *from = COMMAND_LINE;
        static unsigned int smptrap=0;
        unsigned long delay = 0;
        int len = 0;

        if (smptrap)
                return;
        smptrap=1;

        printk("Command line is: %s\n", COMMAND_LINE);

        /*
         * Setup lowcore information for boot cpu
         */
        cpu_init();
        boot_cpu_addr = S390_lowcore.cpu_data.cpu_addr;

        /*
         * print what head.S has found out about the machine 
         */
        if (MACHINE_IS_VM)
                printk("We are running under VM\n");
        else
                printk("We are running native\n");
        if (MACHINE_HAS_IEEE)
                printk("This machine has an IEEE fpu\n");
        else
                printk("This machine has no IEEE fpu\n");

        ROOT_DEV = to_kdev_t(ORIG_ROOT_DEV);
#ifdef CONFIG_BLK_DEV_RAM
        rd_image_start = RAMDISK_FLAGS & RAMDISK_IMAGE_START_MASK;
        rd_prompt = ((RAMDISK_FLAGS & RAMDISK_PROMPT_FLAG) != 0);
        rd_doload = ((RAMDISK_FLAGS & RAMDISK_LOAD_FLAG) != 0);
#endif
	/* nasty stuff with PARMAREAs. we use head.S or parameterline
	  if (!MOUNT_ROOT_RDONLY)
	  root_mountflags &= ~MS_RDONLY;
	*/
        memory_start = (unsigned long) &_end;    /* fixit if use $CODELO etc*/
	memory_end = MEMORY_SIZE;
        init_task.mm->start_code = PAGE_OFFSET;
        init_task.mm->end_code = (unsigned long) &_etext;
        init_task.mm->end_data = (unsigned long) &_edata;
        init_task.mm->brk = (unsigned long) &_end;

        /* Save unparsed command line copy for /proc/cmdline */
        memcpy(saved_command_line, COMMAND_LINE, COMMAND_LINE_SIZE);
        saved_command_line[COMMAND_LINE_SIZE-1] = '\0';

        for (;;) {
                /*
                 * "mem=XXX[kKmM]" sets memsize != 32M
                 * memory size
                 */
                if (c == ' ' && strncmp(from, "mem=", 4) == 0) {
                        if (to != command_line) to--;
                        memory_end = simple_strtoul(from+4, &from, 0);
                        if ( *from == 'K' || *from == 'k' ) {
                                memory_end = memory_end << 10;
                                from++;
                        } else if ( *from == 'M' || *from == 'm' ) {
                                memory_end = memory_end << 20;
                                from++;
                        }
                }
                if (c == ' ' && strncmp(from, "ipldelay=", 9) == 0) {
			if (to != command_line) to--;
                        delay = simple_strtoul(from+9, &from, 0);
			if (*from == 's' || *from == 'S') {
				delay = delay*1000000;
				from++;
			} else if (*from == 'm' || *from == 'M') {
				delay = delay*60*1000000;
				from++;
			}
			tod_wait(delay);
                }
                c = *(from++);
                if (!c)
                        break;
                if (COMMAND_LINE_SIZE <= ++len)
                        break;
                *(to++) = c;
        }
        *to = '\0';
        *cmdline_p = command_line;
        memory_end += PAGE_OFFSET;
        *memory_start_p = memory_start;
        *memory_end_p = memory_end;

#ifdef CONFIG_BLK_DEV_INITRD
        initrd_start = INITRD_START;
        if (initrd_start) {
                initrd_end = initrd_start+INITRD_SIZE;
                printk("Initial ramdisk at: 0x%p (%lu bytes)\n",
                       (void *) initrd_start, INITRD_SIZE);
                initrd_below_start_ok = 1;
                if (initrd_end > *memory_end_p) {
                        printk("initrd extends beyond end of memory "
                               "(0x%08lx > 0x%08lx)\ndisabling initrd\n",
                               initrd_end, (unsigned long) memory_end_p);
                        initrd_start = initrd_end = 0;
                }
        }
#endif

}

void print_cpu_info(struct cpuinfo_S390 *cpuinfo)
{
   printk("cpu %d "
#ifdef __SMP__
           "phys_idx=%d "
#endif
           "vers=%02X ident=%06X machine=%04X unused=%04X\n",
           cpuinfo->cpu_nr,
#ifdef __SMP__
           cpuinfo->cpu_addr,
#endif
           cpuinfo->cpu_id.version,
           cpuinfo->cpu_id.ident,
           cpuinfo->cpu_id.machine,
           cpuinfo->cpu_id.unused);
}

/*
 *	Get CPU information for use by the procfs.
 */

int get_cpuinfo(char * buffer)
{
        struct cpuinfo_S390 *cpuinfo;
        char *p = buffer;
        int i;

        p += sprintf(p,"vendor_id       : IBM/S390\n"
                       "# processors    : %i\n"
                       "bogomips per cpu: %lu.%02lu\n",
                       smp_num_cpus, loops_per_sec/500000,
                       (loops_per_sec/5000)%100);
        for (i = 0; i < smp_num_cpus; i++) {
                cpuinfo = &safe_get_cpu_lowcore(i).cpu_data;
                p += sprintf(p,"processor %i: "
                               "version = %02X,  "
                               "identification = %06X,  "
                               "machine = %04X\n",
                               i, cpuinfo->cpu_id.version,
                               cpuinfo->cpu_id.ident,
                               cpuinfo->cpu_id.machine);
        }
        return p - buffer;
}

