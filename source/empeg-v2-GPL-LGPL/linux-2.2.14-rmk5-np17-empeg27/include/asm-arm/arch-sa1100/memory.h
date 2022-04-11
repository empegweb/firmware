/*
 * linux/include/asm-arm/arch-sa1100/memory.h
 *
 * Copyright (c) 1999 Nicolas Pitre <nico@cam.org>
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H


/*
 * Task size: 3GB
 */
#define TASK_SIZE       (0xc0000000UL)

/*
 * Page offset: 3GB
 */
#define PAGE_OFFSET     (0xc0000000UL)


#include <linux/config.h>


/* Number of bytes per bank */
#if defined(CONFIG_SA1100_BRUTUS)

/* 4Mb per bank in this case */
#define RAM_IN_BANK_0  4*1024*1024
#define RAM_IN_BANK_1  4*1024*1024
#define RAM_IN_BANK_2  4*1024*1024
#define RAM_IN_BANK_3  4*1024*1024

#elif defined(CONFIG_SA1100_EMPEG)

/* 8Mb in 2 banks in this case */
/* But we support up to 16Mb in four banks */
#define RAM_IN_BANK_0  4*1024*1024
#define RAM_IN_BANK_1  4*1024*1024
#define RAM_IN_BANK_2  4*1024*1024
#define RAM_IN_BANK_3  4*1024*1024

#elif defined(CONFIG_SA1100_LART)

/* 16 MB per bank, 2 banks, but A23 is not connected. */
#define RAM_IN_BANK_0  16*1024*1024
#define RAM_IN_BANK_1  16*1024*1024
#define RAM_IN_BANK_2  0
#define RAM_IN_BANK_3  0

#define IS_A23_DISCONNECTED(b) (b <= 1 ? 1 : 0)
#define LINE_A23        0x00800000
#define LINE_A24        0x01000000

#elif defined(CONFIG_SA1100_VICTOR)

/* 4Mb in one bank */
#define RAM_IN_BANK_0  4*1024*1024
#define RAM_IN_BANK_1  0
#define RAM_IN_BANK_2  0
#define RAM_IN_BANK_3  0

#elif defined(CONFIG_SA1100_THINCLIENT)

/* One banks with 16Mb */
#define RAM_IN_BANK_0  16*1024*1024
#define RAM_IN_BANK_1  0
#define RAM_IN_BANK_2  0
#define RAM_IN_BANK_3  0

#elif defined(CONFIG_SA1100_TIFON)

/* Two banks with 16Mb in each */
#define RAM_IN_BANK_0  16*1024*1024
#define RAM_IN_BANK_1  16*1024*1024
#define RAM_IN_BANK_2  0
#define RAM_IN_BANK_3  0

#else
#error missing memory configuration
#endif


#define MEM_SIZE  (RAM_IN_BANK_0+RAM_IN_BANK_1+RAM_IN_BANK_2+RAM_IN_BANK_3)


/* translation macros */
#define __virt_to_phys__is_a_macro
#define __phys_to_virt__is_a_macro

#if (RAM_IN_BANK_1 + RAM_IN_BANK_2 + RAM_IN_BANK_3 == 0)

/* only one bank */
#define __virt_to_phys(x) (x)
#define __phys_to_virt(x) (x)

#elif (RAM_IN_BANK_0 == RAM_IN_BANK_1) && \
      (RAM_IN_BANK_2 + RAM_IN_BANK_3 == 0) && \
      !defined(CONFIG_SA1100_LART)

/* Two identical banks */
#define __virt_to_phys(x) \
	  ( ((x) < PAGE_OFFSET+RAM_IN_BANK_0) ? \
	    ((x) - PAGE_OFFSET + _DRAMBnk0) : \
	    ((x) - PAGE_OFFSET - RAM_IN_BANK_0 + _DRAMBnk1) )
#define __phys_to_virt(x) \
	  ( ((x)&0x07ffffff) + \
	    (((x)&0x08000000) ? PAGE_OFFSET+RAM_IN_BANK_0 : PAGE_OFFSET) )

#else

/* It's more efficient for all other cases to use the function call */
#undef __virt_to_phys__is_a_macro
#undef __phys_to_virt__is_a_macro
extern unsigned long __virt_to_phys(unsigned long vpage);
extern unsigned long __phys_to_virt(unsigned long ppage);

#endif


/*
 * Virtual view <-> DMA view memory address translations
 * virt_to_bus: Used to translate the virtual address to an
 *              address suitable to be passed to set_dma_addr
 * bus_to_virt: Used to convert an address for DMA operations
 *              to an address that the kernel can use.
 *
 * On the SA1100, bus addresses are equivalent to physical addresses.
 */
#define __virt_to_bus__is_a_macro
#define __virt_to_bus(x)        __virt_to_phys(x)
#define __bus_to_virt__is_a_macro
#define __bus_to_virt(x)        __phys_to_virt(x)


#endif
