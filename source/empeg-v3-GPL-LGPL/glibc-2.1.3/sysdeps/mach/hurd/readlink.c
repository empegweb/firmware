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

#include <unistd.h>
#include <hurd.h>
#include <hurd/paths.h>
#include <fcntl.h>
#include <string.h>

/* Read the contents of the symbolic link FILE_NAME into no more than
   LEN bytes of BUF.  The contents are not null-terminated.
   Returns the number of characters read, or -1 for errors.  */
ssize_t
__readlink (file_name, buf, len)
     const char *file_name;
     char *buf;
     size_t len;
{
  error_t err;
  file_t file;
  struct stat st;

  file = __file_name_lookup (file_name, O_READ | O_NOLINK, 0);
  if (file == MACH_PORT_NULL)
    return -1;

  err = __io_stat (file, &st);
  if (! err)
    if (S_ISLNK (st.st_mode))
      {
	char *rbuf = buf;
	size_t got;

	err = __io_read (file, &rbuf, &got, 0, len);
	if (got < len)
	  len = got;
	if (!err && rbuf != buf)
	  {
	    memcpy (buf, rbuf, len);
	    __vm_deallocate (__mach_task_self (), (vm_address_t)rbuf, got);
	  }
      }
    else
      err = EINVAL;

  __mach_port_deallocate (__mach_task_self (), file);

  if (err)
    return __hurd_fail (err);
  else
    return len;
}
weak_alias (__readlink, readlink)
