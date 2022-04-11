/* Old-versioned functions for binary compatibility with glibc-2.0.
   Copyright (C) 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <hurd.h>

/* This file provides definitions for binary compatibility with
   the GLIBC_2.0 version set for the libc.so.0.2 soname.

   These definitions can be removed when the soname changes.  */

void
_hurd_proc_init_compat_20 (char **argv)
{
  _hurd_proc_init (argv, NULL, 0);
}
symbol_version (_hurd_proc_init_compat_20, _hurd_proc_init, GLIBC_2.0);