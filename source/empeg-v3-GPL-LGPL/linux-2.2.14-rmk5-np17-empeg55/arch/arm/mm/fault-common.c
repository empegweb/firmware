/*
 *  linux/arch/arm/mm/fault-common.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Modifications for ARM processor (c) 1995-1999 Russell King
 */
#include <linux/config.h>
#include <linux/pagemap.h>
#include <linux/swap.h>

extern void die(char *msg, struct pt_regs *regs, unsigned int err);

#ifdef CONFIG_DEBUG_USER
extern void c_backtrace (unsigned long fp, int pmode);
#endif

void __bad_pmd(pmd_t *pmd)
{
	printk("Bad pmd in pte_alloc: %08lx\n", pmd_val(*pmd));
#ifdef CONFIG_DEBUG_ERRORS
	__backtrace();
#endif
	set_pmd(pmd, mk_user_pmd(BAD_PAGETABLE));
}

void __bad_pmd_kernel(pmd_t *pmd)
{
	printk("Bad pmd in pte_alloc_kernel: %08lx\n", pmd_val(*pmd));
#ifdef CONFIG_DEBUG_ERRORS
	__backtrace();
#endif
	set_pmd(pmd, mk_kernel_pmd(BAD_PAGETABLE));
}

/*
 * This is useful to dump out the page tables associated with
 * 'addr' in mm 'mm'.
 */
void show_pte(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd;

	pgd = pgd_offset(mm, addr);
	printk(KERN_ALERT "*pgd = %08lx", pgd_val(*pgd));

	do {
		pmd_t *pmd;
		pte_t *pte;

		if (pgd_none(*pgd))
			break;

		if (pgd_bad(*pgd)) {
			printk("(bad)\n");
			break;
		}

		pmd = pmd_offset(pgd, addr);
		printk(", *pmd = %08lx", pmd_val(*pmd));

		if (pmd_none(*pmd))
			break;

		if (pmd_bad(*pmd)) {
			printk("(bad)\n");
			break;
		}

		pte = pte_offset(pmd, addr);
		printk(", *pte = %08lx", pte_val(*pte));
#ifdef CONFIG_CPU_32
		printk(", *ppte = %08lx", pte_val(pte[-PTRS_PER_PTE]));
#endif
	} while(0);

	printk("\n");
}

/*
 * Oops. The kernel tried to access some bad page. We'll have to
 * terminate things with extreme prejudice.
 */
static void
kernel_page_fault(unsigned long addr, int write_access, struct pt_regs *regs,
		  struct task_struct *tsk, struct mm_struct *mm)
{
	char *reason;

	if (addr < PAGE_SIZE)
		reason = "NULL pointer dereference";
	else
		reason = "paging request";

	printk(KERN_ALERT "Unable to handle kernel %s at virtual address %08lx\n",
		reason, addr);
	printk(KERN_ALERT "memmap = %08lX, pgd = %p\n", tsk->tss.memmap, mm->pgd);
	show_pte(mm, addr);
	die("Oops", regs, write_access);

	do_exit(SIGKILL);
}

static void do_page_fault(unsigned long addr, int mode, struct pt_regs *regs)
{
	struct task_struct *tsk;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long fixup;
	int fault;

	tsk = current;
	mm  = tsk->mm;

	/*
	 * If we're in an interrupt or have no user
	 * context, we must not take the fault..
	 */
	if (in_interrupt() || mm == &init_mm)
		goto no_context;

	down(&mm->mmap_sem);
	vma = find_vma(mm, addr);
	if (!vma)
		goto bad_area;
	if (vma->vm_start <= addr)
		goto good_area;
	if (!(vma->vm_flags & VM_GROWSDOWN) || expand_stack(vma, addr))
		goto bad_area;

	/*
	 * Ok, we have a good vm_area for this memory access, so
	 * we can handle it..
	 */
good_area:
	if (READ_FAULT(mode)) { /* read? */
		if (!(vma->vm_flags & (VM_READ|VM_EXEC)))
			goto bad_area;
	} else {
		if (!(vma->vm_flags & VM_WRITE))
			goto bad_area;
	}

	/*
	 * If for any reason at all we couldn't handle the fault,
	 * make sure we exit gracefully rather than endlessly redo
	 * the fault.
	 */
survive:
	fault = handle_mm_fault(tsk, vma, addr & PAGE_MASK, DO_COW(mode));
	if (!fault)
		goto do_sigbus;
	if (fault < 0)
		goto out_of_memory;

	up(&mm->mmap_sem);
	return;

	/*
	 * Something tried to access memory that isn't in our memory map..
	 * Fix it, but check if it's kernel or user first..
	 */
bad_area:
	up(&mm->mmap_sem);

	/* User mode accesses just cause a SIGSEGV */
	if (user_mode(regs)) {
		tsk->tss.error_code = mode;
		tsk->tss.trap_no = 14;
#ifdef CONFIG_DEBUG_USER
		printk("%s(%d): memory violation at pc=0x%08lx, lr=0x%08lx (bad address=0x%08lx, code %d)\n",
		       tsk->comm, tsk->pid,
		       regs->ARM_pc, regs->ARM_lr, addr, mode);
		show_regs(regs);
		c_backtrace(regs->ARM_fp, regs->ARM_cpsr);
#endif
		force_sig(SIGSEGV, tsk);
		return;
	}

no_context:
	/* Are we prepared to handle this kernel fault?  */
	if ((fixup = search_exception_table(instruction_pointer(regs))) != 0) {
#ifdef DEBUG
		printk(KERN_DEBUG "%s(%d): Exception at [<%lx>] addr=%lx (fixup: %lx)\n",
		       tsk->comm, tsk->pid, regs->ARM_pc, addr, fixup);
#endif
		regs->ARM_pc = fixup;
		return;
	}

	kernel_page_fault(addr, mode, regs, tsk, mm);
	return;

/*
 * We ran out of memory, or some other thing happened to us that made
 * us unable to handle the page fault gracefully.
 */
out_of_memory:
	if (tsk->pid == 1) {
		tsk->policy |= SCHED_YIELD;
		schedule();
		goto survive;
	}
	up(&mm->mmap_sem);
	if (user_mode(regs)) {
		printk("VM: killing process %s\n", tsk->comm);
		printk("buffermem       : %ld\n"
		       "page_cache_size : %ld\n"
		       "nr_free_pages   : %d\n"
		       "num_physpages   : %ld\n",
		       buffermem,
		       page_cache_size,
		       nr_free_pages,
		       num_physpages);
		do_exit(SIGKILL);
	}
	goto no_context;

do_sigbus:
	/*
	 * We ran out of memory, or some other thing happened to us that made
	 * us unable to handle the page fault gracefully.
	 */
	up(&mm->mmap_sem);

	/*
	 * Send a sigbus, regardless of whether we were in kernel
	 * or user mode.
	 */
	tsk->tss.error_code = mode;
	tsk->tss.trap_no = 14;
#ifdef CONFIG_DEBUG_USER
		printk("%s(%d): memory violation at pc=0x%08lx, lr=0x%08lx (bad address=0x%08lx, code %d)\n",
		       tsk->comm, tsk->pid,
		       regs->ARM_pc, regs->ARM_lr, addr, mode);
		show_regs(regs);
		c_backtrace(regs->ARM_fp, regs->ARM_cpsr);
#endif
	force_sig(SIGBUS, tsk);

	/* Kernel mode? Handle exceptions or die */
	if (!user_mode(regs))
		goto no_context;
}


