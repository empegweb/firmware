/* Copyright (C) 1991, 1994, 1997 Free Software Foundation, Inc.
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <hurd.h>

static ssize_t
pwrite (void *cookie,
	const char *buf,
	size_t n)
{
  error_t error = __io_write ((io_t) cookie, buf, n, -1,
			      (mach_msg_type_number_t *) &n);
  if (error)
    return __hurd_fail (error);
  return n;
}


/* Write formatted output to PORT, a Mach port supporting the i/o protocol,
   according to the format string FORMAT, using the argument list in ARG.  */
int
vpprintf (io_t port,
	  const char *format,
	  va_list arg)
{
  int done;
  FILE f;

  /* Create an unbuffered stream talking to PORT on the stack.  */
  memset ((void *) &f, 0, sizeof (f));
  f.__magic = _IOMAGIC;
  f.__mode.__write = 1;
  f.__cookie = (void *) port;
  f.__room_funcs = __default_room_functions;
  f.__io_funcs.__write = pwrite;
  f.__seen = 1;
  f.__userbuf = 1;

  /* vfprintf will use a buffer on the stack for the life of the call.  */
  done = vfprintf (&f, format, arg);

  return done;
}
