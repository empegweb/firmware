/* Copyright (C) 1991,92,93,94,95,97,2000 Free Software Foundation, Inc.
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

#include <errno.h>
#include <unistd.h>
#include <hurd.h>
#include <hurd/fd.h>

/* Seek to OFFSET on FD, starting from WHENCE.  */
off_t
__libc_lseek (int fd, off_t offset, int whence)
{
  error_t err;
  if (err = HURD_DPORT_USE (fd, __io_seek (port, offset, whence, &offset)))
    return __hurd_dfail (fd, err);
  return offset;
}

weak_alias (__libc_lseek, __lseek)
weak_alias (__libc_lseek, lseek)