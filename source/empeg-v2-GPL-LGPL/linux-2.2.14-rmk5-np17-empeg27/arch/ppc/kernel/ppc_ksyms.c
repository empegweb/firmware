#include <linux/config.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/elfcore.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/vt_kern.h>
#include <linux/nvram.h>

#include <asm/page.h>
#include <asm/semaphore.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/ide.h>
#include <asm/atomic.h>
#include <asm/bitops.h>
#include <asm/checksum.h>
#include <asm/pgtable.h>
#include <asm/adb.h>
#include <asm/cuda.h>
#include <asm/pmu.h>
#include <asm/prom.h>
#include <asm/system.h>
#include <asm/pci-bridge.h>
#include <asm/irq.h>
#include <asm/feature.h>
#include <asm/spinlock.h>
#include <asm/dma.h>
#include <asm/machdep.h>

/* Tell string.h we don't want memcpy etc. as cpp defines */
#define EXPORT_SYMTAB_STROPS

extern void transfer_to_handler(void);
extern void int_return(void);
extern void syscall_trace(void);
extern void do_IRQ(struct pt_regs *regs, int isfake);
extern void MachineCheckException(struct pt_regs *regs);
extern void AlignmentException(struct pt_regs *regs);
extern void ProgramCheckException(struct pt_regs *regs);
extern void SingleStepException(struct pt_regs *regs);
extern int sys_sigreturn(struct pt_regs *regs);
extern atomic_t ppc_n_lost_interrupts;
extern void do_lost_interrupts(unsigned long);
extern int do_signal(sigset_t *, struct pt_regs *);

asmlinkage long long __ashrdi3(long long, int);
asmlinkage long long __lshrdi3(long long, int);
asmlinkage int abs(int);

EXPORT_SYMBOL(clear_page);
EXPORT_SYMBOL(do_signal);
EXPORT_SYMBOL(syscall_trace);
EXPORT_SYMBOL(transfer_to_handler);
EXPORT_SYMBOL(int_return);
EXPORT_SYMBOL(do_IRQ);
EXPORT_SYMBOL(MachineCheckException);
EXPORT_SYMBOL(AlignmentException);
EXPORT_SYMBOL(ProgramCheckException);
EXPORT_SYMBOL(SingleStepException);
EXPORT_SYMBOL(sys_sigreturn);
EXPORT_SYMBOL(ppc_n_lost_interrupts);
EXPORT_SYMBOL(do_lost_interrupts);
EXPORT_SYMBOL(enable_irq);
EXPORT_SYMBOL(disable_irq);
EXPORT_SYMBOL(disable_irq_nosync);
EXPORT_SYMBOL(ppc_local_irq_count);
EXPORT_SYMBOL(ppc_local_bh_count);

EXPORT_SYMBOL(isa_io_base);
EXPORT_SYMBOL(isa_mem_base);
EXPORT_SYMBOL(pci_dram_offset);
EXPORT_SYMBOL(ISA_DMA_THRESHOLD);
EXPORT_SYMBOL(DMA_MODE_READ);
EXPORT_SYMBOL(DMA_MODE_WRITE);
EXPORT_SYMBOL(_prep_type);
EXPORT_SYMBOL(ucSystemType);

EXPORT_SYMBOL(atomic_add);
EXPORT_SYMBOL(atomic_sub);
EXPORT_SYMBOL(atomic_inc);
EXPORT_SYMBOL(atomic_inc_return);
EXPORT_SYMBOL(atomic_dec);
EXPORT_SYMBOL(atomic_dec_return);
EXPORT_SYMBOL(atomic_dec_and_test);

EXPORT_SYMBOL(set_bit);
EXPORT_SYMBOL(clear_bit);
EXPORT_SYMBOL(change_bit);
EXPORT_SYMBOL(test_and_set_bit);
EXPORT_SYMBOL(test_and_clear_bit);
EXPORT_SYMBOL(test_and_change_bit);
#if 0
EXPORT_SYMBOL(ffz);
EXPORT_SYMBOL(find_first_zero_bit);
EXPORT_SYMBOL(find_next_zero_bit);
#endif

EXPORT_SYMBOL(strcpy);
EXPORT_SYMBOL(strncpy);
EXPORT_SYMBOL(strcat);
EXPORT_SYMBOL(strncat);
EXPORT_SYMBOL(strchr);
EXPORT_SYMBOL(strrchr);
EXPORT_SYMBOL(strpbrk);
EXPORT_SYMBOL(strtok);
EXPORT_SYMBOL(strstr);
EXPORT_SYMBOL(strlen);
EXPORT_SYMBOL(strnlen);
EXPORT_SYMBOL(strspn);
EXPORT_SYMBOL(strcmp);
EXPORT_SYMBOL(strncmp);

/* EXPORT_SYMBOL(csum_partial); already in net/netsyms.c */
EXPORT_SYMBOL(csum_partial_copy_generic);
EXPORT_SYMBOL(ip_fast_csum);
EXPORT_SYMBOL(csum_tcpudp_magic);

EXPORT_SYMBOL(__copy_tofrom_user);
EXPORT_SYMBOL(__clear_user);
EXPORT_SYMBOL(__strncpy_from_user);
EXPORT_SYMBOL(__strnlen_user);

/*
EXPORT_SYMBOL(inb);
EXPORT_SYMBOL(inw);
EXPORT_SYMBOL(inl);
EXPORT_SYMBOL(outb);
EXPORT_SYMBOL(outw);
EXPORT_SYMBOL(outl);
EXPORT_SYMBOL(outsl);*/

EXPORT_SYMBOL(_insb);
EXPORT_SYMBOL(_outsb);
EXPORT_SYMBOL(_insw);
EXPORT_SYMBOL(_outsw);
EXPORT_SYMBOL(_insl);
EXPORT_SYMBOL(_outsl);
EXPORT_SYMBOL(_insw_ns);
EXPORT_SYMBOL(_outsw_ns);
EXPORT_SYMBOL(_insl_ns);
EXPORT_SYMBOL(_outsl_ns);
EXPORT_SYMBOL(ioremap);
EXPORT_SYMBOL(__ioremap);
EXPORT_SYMBOL(iounmap);

EXPORT_SYMBOL(ide_insw);
EXPORT_SYMBOL(ide_outsw);
EXPORT_SYMBOL(ppc_ide_md);

EXPORT_SYMBOL(start_thread);
EXPORT_SYMBOL(kernel_thread);

EXPORT_SYMBOL(__cli);
EXPORT_SYMBOL(__sti);
/*EXPORT_SYMBOL(__restore_flags);*/
EXPORT_SYMBOL(_disable_interrupts);
EXPORT_SYMBOL(_enable_interrupts);
EXPORT_SYMBOL(flush_instruction_cache);
EXPORT_SYMBOL(_get_PVR);
EXPORT_SYMBOL(giveup_fpu);
EXPORT_SYMBOL(enable_kernel_fp);
EXPORT_SYMBOL(flush_icache_range);
EXPORT_SYMBOL(xchg_u32);
#ifdef __SMP__
EXPORT_SYMBOL(__global_cli);
EXPORT_SYMBOL(__global_sti);
EXPORT_SYMBOL(__global_save_flags);
EXPORT_SYMBOL(__global_restore_flags);
EXPORT_SYMBOL(_spin_lock);
EXPORT_SYMBOL(_spin_unlock);
EXPORT_SYMBOL(spin_trylock);
EXPORT_SYMBOL(_read_lock);
EXPORT_SYMBOL(_read_unlock);
EXPORT_SYMBOL(_write_lock);
EXPORT_SYMBOL(_write_unlock);
#endif

EXPORT_SYMBOL(_machine);
EXPORT_SYMBOL(ppc_md);

EXPORT_SYMBOL(adb_request);
EXPORT_SYMBOL(adb_register);
EXPORT_SYMBOL(cuda_request);
EXPORT_SYMBOL(cuda_poll);
EXPORT_SYMBOL(pmu_request);
EXPORT_SYMBOL(pmu_poll);
#ifdef CONFIG_PMAC_PBOOK
EXPORT_SYMBOL(pmu_register_sleep_notifier);
EXPORT_SYMBOL(pmu_unregister_sleep_notifier);
EXPORT_SYMBOL(pmu_enable_irled);
#endif CONFIG_PMAC_PBOOK
EXPORT_SYMBOL(abort);
EXPORT_SYMBOL(find_devices);
EXPORT_SYMBOL(find_type_devices);
EXPORT_SYMBOL(find_compatible_devices);
EXPORT_SYMBOL(find_path_device);
EXPORT_SYMBOL(find_phandle);
EXPORT_SYMBOL(device_is_compatible);
EXPORT_SYMBOL(machine_is_compatible);
EXPORT_SYMBOL(get_property);
EXPORT_SYMBOL(pci_io_base);
EXPORT_SYMBOL(pci_device_loc);
EXPORT_SYMBOL(feature_set);
EXPORT_SYMBOL(feature_clear);
EXPORT_SYMBOL(feature_test);
#ifdef CONFIG_SCSI
EXPORT_SYMBOL(note_scsi_host);
#endif
EXPORT_SYMBOL(kd_mksound);
#ifdef CONFIG_PMAC
EXPORT_SYMBOL(nvram_read_byte);
EXPORT_SYMBOL(nvram_write_byte);
#endif /* CONFIG_PMAC */

EXPORT_SYMBOL(abs);

EXPORT_SYMBOL_NOVERS(__ashrdi3);
EXPORT_SYMBOL_NOVERS(__lshrdi3);
EXPORT_SYMBOL_NOVERS(memcpy);
EXPORT_SYMBOL_NOVERS(memset);
EXPORT_SYMBOL_NOVERS(memmove);
EXPORT_SYMBOL_NOVERS(memscan);
EXPORT_SYMBOL_NOVERS(memcmp);
