/*
 * Lib/Compat/string.h -- system compatibility hacks -- string functions
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

#ifndef UNSN_COMPAT_STRING_H
#define UNSN_COMPAT_STRING_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

/* Prototypes. */

#if defined(HAVE_BASENAME) && !defined(HAVE_PROTOTYPE_BASENAME)
char *basename(char const *);
#endif

#ifndef HAVE_PROTOTYPE_MEMCHR
void *memchr(void const *, int, size_t);
#endif

#if defined(HAVE_MEMCPY) && !defined(HAVE_PROTOTYPE_MEMCPY)
void *memcpy(void *, void *, size_t);
#endif

#if defined(HAVE_MEMMOVE) && !defined(HAVE_PROTOTYPE_MEMMOVE)
void *memmove(void *, void *, size_t);
#endif

#if defined(HAVE_STRCHR) && !defined(HAVE_PROTOTYPE_STRCHR)
char *strchr(char const *, int);
#endif

#ifndef HAVE_PROTOTYPE_STRCMP
int strcmp(char const *, char const *);
#endif

#ifndef HAVE_PROTOTYPE_STRCPY
char *strcpy(char *, char const *);
#endif

#if defined(HAVE_STRRCHR) && !defined(HAVE_PROTOTYPE_STRRCHR)
char *strrchr(char const *, int);
#endif

#ifndef HAVE_PROTOTYPE_STRSPN
size_t strspn(char const *, char const *);
#endif

/* Okay, what's it called this week? */

#ifndef HAVE_MEMMOVE
# define memmove(dest, src, n) bcopy((src), (dest), (n))
#endif

#ifndef HAVE_MEMCPY
# define memcpy memmove
# ifdef HAVE_MEMMOVE
#  define HAVE_MEMCPY 1
# endif
#endif

#ifndef HAVE_STRCHR
# define strchr index
char *index(char const *);
# define HAVE_STRCHR 1
#endif /* !HAVE_STRCHR */

#ifndef HAVE_STRRCHR
# define strrchr rindex
char *rindex(char const *);
# define HAVE_STRRCHR 1
#endif /* !HAVE_STRRCHR */

/* this might be missing */

#ifndef HAVE_BASENAME
# define basename unsn_compat_basename
char *basename(char const *);
# define NEED_COMPAT_BASENAME 1
# define HAVE_BASENAME 1
#endif /* !HAVE_BASENAME */

#endif
