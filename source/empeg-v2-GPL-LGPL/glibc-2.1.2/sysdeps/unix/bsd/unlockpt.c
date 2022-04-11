/* Copyright (C) 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.phys.columbia.edu>, 1998.

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

#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Unlock the slave pseudo terminal associated with the master pseudo
   terminal specified by FD.  */
int
unlockpt (int fd)
{
  char buf[sizeof (_PATH_TTY) + 2];

  /* BSD doesn't have a lock, but it does have `revoke'.  */
  if (__ptsname_r (fd, buf, sizeof (buf)))
    return -1;
  return revoke (buf);
}
