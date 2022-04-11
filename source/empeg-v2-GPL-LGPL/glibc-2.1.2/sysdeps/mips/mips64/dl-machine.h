/* Machine-dependent ELF dynamic relocation inline functions.  MIPS version.
   Copyright (C) 1996, 1997, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Kazumoto Kojima <kkojima@info.kanagawa-u.ac.jp>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef dl_machine_h
#define dl_machine_h

#define ELF_MACHINE_NAME "MIPS"

#define ELF_MACHINE_NO_PLT

#include <assert.h>
#include <entry.h>

#ifndef ENTRY_POINT
#error ENTRY_POINT needs to be defined for MIPS.
#endif

#ifndef _RTLD_PROLOGUE
#ifdef __STDC__
#define _RTLD_PROLOGUE(entry) "\n\t.globl " #entry \
			      "\n\t.ent " #entry \
			      "\n\t" #entry ":\n\t"
#else
#define _RTLD_PROLOGUE(entry) "\n\t.globl entry\n\t.ent entry\n\t entry:\n\t"
#endif
#endif

#ifndef _RTLD_EPILOGUE
#ifdef __STDC__
#define _RTLD_EPILOGUE(entry) "\t.end " #entry "\n"
#else
#define _RTLD_EPILOGUE(entry) "\t.end entry\n"
#endif
#endif

/* I have no idea what I am doing. */
#define ELF_MACHINE_RELOC_NOPLT			-1
#define elf_machine_lookup_noplt_p(type)	(1)
#define elf_machine_lookup_noexec_p(type)	(0)

/* Translate a processor specific dynamic tag to the index
   in l_info array.  */
#define DT_MIPS(x) (DT_MIPS_##x - DT_LOPROC + DT_NUM)

#if 0
/* We may need 64k alignment. */
#define ELF_MACHINE_ALIGN_MASK 0xffff
#endif

/*
 * MIPS libraries are usually linked to a non-zero base address.  We
 * subtrace the base address from the address where we map the object
 * to.  This results in more efficient address space usage.
 */
#if 0
#define MAP_BASE_ADDR(l) ((l)->l_info[DT_MIPS(BASE_ADDRESS)] ? \
			  (l)->l_info[DT_MIPS(BASE_ADDRESS)]->d_un.d_ptr : 0)
#else
#define MAP_BASE_ADDR(l) 0x5ffe0000
#endif

/* If there is a DT_MIPS_RLD_MAP entry in the dynamic section, fill it in
   with the run-time address of the r_debug structure  */
#define ELF_MACHINE_DEBUG_SETUP(l,r) \
do { if ((l)->l_info[DT_MIPS (RLD_MAP)]) \
       *(ElfW(Addr) *)((l)->l_info[DT_MIPS (RLD_MAP)]->d_un.d_ptr) = \
       (ElfW(Addr)) (r); \
   } while (0)

/* Return nonzero iff E_MACHINE is compatible with the running host.  */
static inline int __attribute__ ((unused))
elf_machine_matches_host (ElfW(Half) e_machine)
{
  switch (e_machine)
    {
    case EM_MIPS:
    case EM_MIPS_RS4_BE:
      return 1;
    default:
      return 0;
    }
}

static inline ElfW(Addr) *
elf_mips_got_from_gpreg (ElfW(Addr) gpreg)
{
  /* FIXME: the offset of gp from GOT may be system-dependent. */
  return (ElfW(Addr) *) (gpreg - 0x7ff0);
}

/* Return the run-time address of the _GLOBAL_OFFSET_TABLE_.
   Must be inlined in a function which uses global data.  */
static inline ElfW(Addr) *
elf_machine_got (void)
{
  ElfW(Addr) gp;

  __asm__ __volatile__("move %0, $28\n\t" : "=r" (gp));
  return elf_mips_got_from_gpreg (gp);
}


/* Return the run-time load address of the shared object.  */
static inline ElfW(Addr)
elf_machine_load_address (void)
{
  ElfW(Addr) addr;
  asm ("	.set noreorder\n"
       "	dla %0, here\n"
       "	bltzal $0, here\n"
       "	nop\n"
       "here:	dsubu %0, $31, %0\n"
       "	.set reorder\n"
       :	"=r" (addr)
       :	/* No inputs */
       :	"$31");
  return addr;
}

/* The MSB of got[1] of a gnu object is set to identify gnu objects. */
#define ELF_MIPS_GNU_GOT1_MASK 0x80000000

/* Relocate GOT. */
static inline void
elf_machine_got_rel (struct link_map *map, int lazy)
{
  ElfW(Addr) *got;
  ElfW(Sym) *sym;
  int i, n;
  const char *strtab = (const void *) map->l_info[DT_STRTAB]->d_un.d_ptr;

#define RESOLVE_GOTSYM(sym) \
    ({ \
      const ElfW(Sym) *ref = sym; \
      ElfW(Addr) sym_loadaddr; \
      sym_loadaddr = _dl_lookup_symbol (strtab + sym->st_name, &ref, \
					map->l_scope, \
					map->l_name, ELF_MACHINE_RELOC_NOPLT);\
      (ref)? sym_loadaddr + ref->st_value: 0; \
    })

  got = (ElfW(Addr) *) map->l_info[DT_PLTGOT]->d_un.d_ptr;

  /* got[0] is reserved. got[1] is also reserved for the dynamic object
     generated by gnu ld. Skip these reserved entries from relocation.  */
  i = (got[1] & ELF_MIPS_GNU_GOT1_MASK)? 2: 1;
  n = map->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;
  /* Add the run-time display to all local got entries. */
  while (i < n)
    got[i++] += map->l_addr;

  /* Handle global got entries. */
  got += n;
  sym = (ElfW(Sym) *) map->l_info[DT_SYMTAB]->d_un.d_ptr;
  sym += map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;
  i = (map->l_info[DT_MIPS (SYMTABNO)]->d_un.d_val
       - map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val);

  while (i--)
    {
      if (sym->st_shndx == SHN_UNDEF)
	{
	  if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC)
	    {
	      if (sym->st_value && lazy)
		*got = sym->st_value + map->l_addr;
	      else
		*got = RESOLVE_GOTSYM (sym);
	    }
	  else /* if (*got == 0 || *got == QS) */
	    *got = RESOLVE_GOTSYM (sym);
	}
      else if (sym->st_shndx == SHN_COMMON)
	*got = RESOLVE_GOTSYM (sym);
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC
	       && *got != sym->st_value
	       && lazy)
	*got += map->l_addr;
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_SECTION)
	{
	  if (sym->st_other == 0)
	    *got += map->l_addr;
	}
      else
	*got = RESOLVE_GOTSYM (sym);

      got++;
      sym++;
    }

#undef RESOLVE_GOTSYM

  return;
}

/* Set up the loaded object described by L so its stub function
   will jump to the on-demand fixup code in dl-runtime.c.  */

static inline int
elf_machine_runtime_setup (struct link_map *l, int lazy, int profile)
{
  ElfW(Addr) *got;
  extern void _dl_runtime_resolve (ElfW(Word));
  extern int _dl_mips_gnu_objects;

#ifdef RTLD_BOOTSTRAP
    {
      return lazy;
    }
#endif
  if (lazy)
    {
      /* The GOT entries for functions have not yet been filled in.
	 Their initial contents will arrange when called to put an
	 offset into the .dynsym section in t8, the return address
	 in t7 and then jump to _GLOBAL_OFFSET_TABLE[0].  */
      got = (ElfW(Addr) *) l->l_info[DT_PLTGOT]->d_un.d_ptr;

      /* This function will get called to fix up the GOT entry indicated by
	 the register t8, and then jump to the resolved address.  */
      got[0] = (ElfW(Addr)) &_dl_runtime_resolve;

      /* Store l to _GLOBAL_OFFSET_TABLE[1] for gnu object. The MSB
	 of got[1] of a gnu object is set to identify gnu objects.
	 Where we can store l for non gnu objects? XXX  */
      if ((got[1] & ELF_MIPS_GNU_GOT1_MASK) != 0)
	got[1] = (ElfW(Addr)) ((unsigned) l | ELF_MIPS_GNU_GOT1_MASK);
      else
	_dl_mips_gnu_objects = 0;
    }

  /* Relocate global offset table.  */
  elf_machine_got_rel (l, lazy);

  return lazy;
}

/* Get link_map for this object.  */
static inline struct link_map *
elf_machine_runtime_link_map (ElfW(Addr) gpreg, ElfW(Addr) stub_pc)
{
  extern int _dl_mips_gnu_objects;

  /* got[1] is reserved to keep its link map address for the shared
     object generated by gnu linker. If all are such object, we can
     find link map from current GPREG simply. If not so, get link map
     for callers object containing STUB_PC.  */

  if (_dl_mips_gnu_objects)
    {
      ElfW(Addr) *got = elf_mips_got_from_gpreg (gpreg);
      ElfW(Word) g1;

      g1 = ((ElfW(Word) *) got)[1];

      if ((g1 & ELF_MIPS_GNU_GOT1_MASK) != 0)
	return (struct link_map *) (g1 & ~ELF_MIPS_GNU_GOT1_MASK);
    }

    {
      struct link_map *l = _dl_loaded;
      struct link_map *ret = 0;
      ElfW(Addr) candidate = 0;

      while (l)
	{
	  ElfW(Addr) base = 0;
	  const ElfW(Phdr) *p = l->l_phdr;
	  ElfW(Half) this, nent = l->l_phnum;

	  /* Get the base. */
	  for (this = 0; this < nent; this++)
	    if (p[this].p_type == PT_LOAD)
	      {
		base = p[this].p_vaddr + l->l_addr;
		break;
	      }
	  if (! base)
	    {
	      l = l->l_next;
	      continue;
	    }

	  /* Find closest link base addr. */
	  if ((base < stub_pc) && (candidate < base))
	    {
	      candidate = base;
	      ret = l;
	    }
	  l = l->l_next;
	}
      if (candidate && ret && (candidate < stub_pc))
	return ret;
      else if (!candidate)
	return _dl_loaded;
    }

  _dl_signal_error (0, NULL, "cannot find runtime link map");
  return NULL;
}

/* Mips has no PLT but define elf_machine_relplt to be elf_machine_rel. */
#define elf_machine_relplt elf_machine_rel

/* Define mips specific runtime resolver. The function __dl_runtime_resolve
   is called from assembler function _dl_runtime_resolve which converts
   special argument registers t7 ($15) and t8 ($24):
     t7  address to return to the caller of the function
     t8  index for this function symbol in .dynsym
   to usual c arguments.  */

#define ELF_MACHINE_RUNTIME_TRAMPOLINE					      \
/* The flag _dl_mips_gnu_objects is set if all dynamic objects are	      \
   generated by the gnu linker. */					      \
int _dl_mips_gnu_objects = 1;						      \
									      \
/* This is called from assembly stubs below which the compiler can't see.  */ \
static ElfW(Addr)							      \
__dl_runtime_resolve (ElfW(Word), ElfW(Word), ElfW(Addr), ElfW(Addr))	      \
                  __attribute__ ((unused));				      \
									      \
static ElfW(Addr)							      \
__dl_runtime_resolve (ElfW(Word) sym_index,				      \
		      ElfW(Word) return_address,			      \
		      ElfW(Addr) old_gpreg,				      \
		      ElfW(Addr) stub_pc)				      \
{									      \
  struct link_map *l = elf_machine_runtime_link_map (old_gpreg, stub_pc);     \
  const ElfW(Sym) *const symtab						      \
    = (const ElfW(Sym) *) l->l_info[DT_SYMTAB]->d_un.d_ptr;		      \
  const char *strtab = (const void *) l->l_info[DT_STRTAB]->d_un.d_ptr;	      \
  const ElfW(Addr) *got							      \
    = (const ElfW(Addr) *) l->l_info[DT_PLTGOT]->d_un.d_ptr;		      \
  const ElfW(Word) local_gotno						      \
    = (const ElfW(Word)) l->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;	      \
  const ElfW(Word) gotsym						      \
    = (const ElfW(Word)) l->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;	      \
  const ElfW(Sym) *definer;						      \
  ElfW(Addr) loadbase;							      \
  ElfW(Addr) funcaddr;							      \
									      \
  /* Look up the symbol's run-time value.  */				      \
  definer = &symtab[sym_index];						      \
									      \
  loadbase = _dl_lookup_symbol (strtab + definer->st_name, &definer,	      \
				l->l_scope, l->l_name,			      \
				ELF_MACHINE_RELOC_NOPLT);		      \
									      \
  /* Apply the relocation with that value.  */				      \
  funcaddr = loadbase + definer->st_value;				      \
  *(got + local_gotno + sym_index - gotsym) = funcaddr;			      \
									      \
  return funcaddr;							      \
}									      \
									      \
asm ("\n								      \
	.text\n								      \
	.align	3\n							      \
	.globl	_dl_runtime_resolve\n					      \
	.type	_dl_runtime_resolve,@function\n				      \
	.ent	_dl_runtime_resolve\n					      \
_dl_runtime_resolve:\n							      \
	.set noreorder\n						      \
	# Save old GP to $3.\n						      \
	move	$3,$28\n						      \
	# Modify t9 ($25) so as to point .cpload instruction.\n		      \
	daddu	$25,2*8\n						      \
	# Compute GP.\n							      \
	.cpload $25\n							      \
	.set reorder\n							      \
	# Save slot call pc.\n						      \
        move	$2, $31\n						      \
	# Save arguments and sp value in stack.\n			      \
	dsubu	$29, 10*8\n						      \
	.cprestore 8*8\n						      \
	sd	$15, 9*8($29)\n						      \
	sd	$4, 3*8($29)\n						      \
	sd	$5, 4*8($29)\n						      \
	sd	$6, 5*8($29)\n						      \
	sd	$7, 6*8($29)\n						      \
	sd	$16, 7*8($29)\n						      \
	move	$16, $29\n						      \
	move	$4, $24\n						      \
	move	$5, $15\n						      \
	move	$6, $3\n						      \
	move	$7, $2\n						      \
	jal	__dl_runtime_resolve\n					      \
	move	$29, $16\n						      \
	ld	$31, 9*8($29)\n						      \
	ld	$4, 3*8($29)\n						      \
	ld	$5, 4*8($29)\n						      \
	ld	$6, 5*8($29)\n						      \
	ld	$7, 6*8($29)\n						      \
	ld	$16, 7*8($29)\n						      \
	daddu	$29, 10*8\n						      \
	move	$25, $2\n						      \
	jr	$25\n							      \
	.end	_dl_runtime_resolve\n					      \
	.previous\n							      \
");

/* Mask identifying addresses reserved for the user program,
   where the dynamic linker should not map anything.  */
#define ELF_MACHINE_USER_ADDRESS_MASK	0x80000000UL



/* Initial entry point code for the dynamic linker.
   The C function `_dl_start' is the real entry point;
   its return value is the user program's entry point.
   Note how we have to be careful about two things:

   1) That we allocate a minimal stack of 24 bytes for
      every function call, the MIPS ABI states that even
      if all arguments are passed in registers the procedure
      called can use the 16 byte area pointed to by $sp
      when it is called to store away the arguments passed
      to it.

   2) That under Linux the entry is named __start
      and not just plain _start.  */

#define RTLD_START asm ("\
	.text\n\
	.align	3\n"\
_RTLD_PROLOGUE (ENTRY_POINT)\
"	.globl _dl_start_user\n\
	.set noreorder\n\
	bltzal $0, 0f\n\
	nop\n\
0:	.cpload $31\n\
	.set reorder\n\
	# i386 ABI book says that the first entry of GOT holds\n\
	# the address of the dynamic structure. Though MIPS ABI\n\
	# doesn't say nothing about this, I emulate this here.\n\
	dla $4, _DYNAMIC\n\
	sd $4, -0x7ff0($28)\n\
	move $4, $29\n\
	jal _dl_start\n\
	# Get the value of label '_dl_start_user' in t9 ($25).\n\
	dla $25, _dl_start_user\n\
_dl_start_user:\n\
	.set noreorder\n\
	.cpload $25\n\
	.set reorder\n\
	move $16, $28\n\
	# Save the user entry point address in saved register.\n\
	move $17, $2\n\
	# See if we were run as a command with the executable file\n\
	# name as an extra leading argument.\n\
	ld $2, _dl_skip_args\n\
	beq $2, $0, 1f\n\
	# Load the original argument count.\n\
	ld $4, 0($29)\n\
	# Subtract _dl_skip_args from it.\n\
	dsubu $4, $2\n\
	# Adjust the stack pointer to skip _dl_skip_args words.\n\
	dsll $2,2\n\
	daddu $29, $2\n\
	# Save back the modified argument count.\n\
	sd $4, 0($29)\n\
	# Get _dl_default_scope[2] as argument in _dl_init_next call below.\n\
1:	dla $2, _dl_default_scope\n\
	ld $4, 2*8($2)\n\
	# Call _dl_init_next to return the address of an initializer\n\
	# function to run.\n\
	jal _dl_init_next\n\
	move $28, $16\n\
	# Check for zero return,  when out of initializers.\n\
	beq $2, $0, 2f\n\
	# Call the shared object initializer function.\n\
	move $25, $2\n\
	ld $4, 0($29)\n\
	ld $5, 1*8($29)\n\
	ld $6, 2*8($29)\n\
	ld $7, 3*8($29)\n\
	jalr $25\n\
	move $28, $16\n\
	# Loop to call _dl_init_next for the next initializer.\n\
	b 1b\n\
	# Pass our finalizer function to the user in ra.\n\
2:	dla $31, _dl_fini\n\
	# Jump to the user entry point.\n\
	move $25, $17\n\
	ld $4, 0($29)\n\
	ld $5, 1*8($29)\n\
	ld $6, 2*8$29)\n\
	ld $7, 3*8($29)\n\
	jr $25\n"\
_RTLD_EPILOGUE(ENTRY_POINT) \
	"\n.previous"\
);


/* The MIPS never uses Elfxx_Rela relocations.  */
#define ELF_MACHINE_NO_RELA 1

#endif /* !dl_machine_h */

#ifdef RESOLVE

/* Perform the relocation specified by RELOC and SYM (which is fully resolved).
   MAP is the object containing the reloc.  */

static inline void
elf_machine_rel (struct link_map *map, const ElfW(Rel) *reloc,
		 const ElfW(Sym) *sym, const struct r_found_version *version,
		 ElfW(Addr) *const reloc_addr)
{
  ElfW(Addr) loadbase;
  ElfW(Addr) undo __attribute__ ((unused));

  switch (ELFW(R_TYPE) (reloc->r_info))
    {
    case R_MIPS_REL32:
      {
	ElfW(Addr) undo = 0;

	if (ELFW(ST_BIND) (sym->st_info) == STB_LOCAL
	    && (ELFW(ST_TYPE) (sym->st_info) == STT_SECTION
		|| ELFW(ST_TYPE) (sym->st_info) == STT_NOTYPE))
	  {
	    *reloc_addr += map->l_addr;
	    break;
	  }
#ifndef RTLD_BOOTSTRAP
	/* This is defined in rtld.c, but nowhere in the static libc.a;
	   make the reference weak so static programs can still link.  This
	   declaration cannot be done when compiling rtld.c (i.e.  #ifdef
	   RTLD_BOOTSTRAP) because rtld.c contains the common defn for
	   _dl_rtld_map, which is incompatible with a weak decl in the same
	   file.  */
	weak_extern (_dl_rtld_map);
	if (map == &_dl_rtld_map)
	  /* Undo the relocation done here during bootstrapping.  Now we will
	     relocate it anew, possibly using a binding found in the user
	     program or a loaded library rather than the dynamic linker's
	     built-in definitions used while loading those libraries.  */
	  undo = map->l_addr + sym->st_value;
#endif
	  loadbase = RESOLVE (&sym, version, 0);
	  *reloc_addr += (sym ? (loadbase + sym->st_value) : 0) - undo;
	}
      break;
    case R_MIPS_NONE:		/* Alright, Wilbur.  */
      break;
    default:
      assert (! "unexpected dynamic reloc type");
      break;
    }
}

static inline void
elf_machine_lazy_rel (struct link_map *map, const ElfW(Rel) *reloc)
{
  /* Do nothing.  */
}

#endif /* RESOLVE */