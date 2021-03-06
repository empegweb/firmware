/* Set a host configuration item kept as the whole contents of a file.
   Copyright (C) 1996,97,99,2000 Free Software Foundation, Inc.
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

#include <fcntl.h>
#include <hurd.h>
#include "hurdhost.h"

ssize_t
_hurd_set_host_config (const char *item, const char *value, size_t valuelen)
{
  error_t err;
  mach_msg_type_number_t nwrote;
  file_t new, dir;
  char *name;

  dir = __file_name_split (item, &name);
  if (dir == MACH_PORT_NULL)
    return -1;

  /* Create a new node.
     Only O_READ|O_WRITE|O_EXEC are meaningful in FLAGS.  */
  err = __dir_mkfile (dir, O_WRITE, 0644, &new);
  if (! err)
    {
      /* Write the contents.  */
      err = __io_write (new, value, valuelen, 0, &nwrote);
      if (! err)
	/* Atomically link the new node onto the name.  */
	err = __dir_link (dir, new, name, 0);
      __mach_port_deallocate (__mach_task_self (), new);
    }
  __mach_port_deallocate (__mach_task_self (), dir);

  return err ? __hurd_fail (err) : nwrote;
}
