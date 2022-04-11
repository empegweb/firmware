/*
 * Lib/strerror.c -- unsn_strerror()
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

#include <libunsn_pr.h>

#include <Compat/string.h>
#include <Compat/errno.h>

#ifndef HAVE_STRERROR

extern char *sys_errlist[];
extern int sys_nerr;

static inline char *strerror(int errnum)
{
	if(errnum >= 0 && errnum < sys_nerr)
		return sys_errlist[errnum];
	else
		return (char *)"Unknown error";
}

#else /* HAVE_STRERROR */

# ifndef HAVE_PROTOTYPE_STRERROR
char *strerror(int);
# endif

#endif /* HAVE_STRERROR */

#include <strerror.m.ic>

char const *unsn_strerror(int errnum)
{
	if(errnum >= ERRMIN && errnum <= ERRMAX)
		return errmsg[errnum - ERRMIN];
	else
		return strerror(errnum);
}
