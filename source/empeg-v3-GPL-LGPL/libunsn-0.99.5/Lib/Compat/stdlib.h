/*
 * Lib/Compat/stdlib.h -- system compatibility hacks -- standard library
 * Copyright (C) 2000  Andrew Main
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UNSN_COMPAT_STDLIB_H
#define UNSN_COMPAT_STDLIB_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

/* prototypes */

#ifndef HAVE_PROTOTYPE_BSEARCH
void *bsearch(void const *, void const *, size_t, size_t,
	int (*)(void const *, void const *));
#endif

#ifndef HAVE_PROTOTYPE_EXIT
void exit(int);
#endif

#ifndef HAVE_PROTOTYPE_FREE
void free(void *);
#endif

#ifndef HAVE_PROTOTYPE_MALLOC
void *malloc(size_t);
#endif

#ifndef HAVE_PROTOTYPE_QSORT
void qsort(void *, size_t, size_t,
	int (*)(void const *, void const *));
#endif

#ifndef HAVE_PROTOTYPE_REALLOC
void *malloc(void *, size_t);
#endif

#ifndef HAVE_PROTOTYPE_STRTOUL
unsigned long strtoul(char const *, char **, int);
#endif

/* this might be missing */

#ifndef offsetof
# define offsetof(TYPE, MEMBER) ( (char *)&((TYPE *)0)->MEMBER - (char *)&((TYPE *)0) )
#endif

#endif
