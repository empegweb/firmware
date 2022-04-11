/* Linux specific extensions to pathconf.
   Copyright (C) 1991, 1995, 1996, 1998 Free Software Foundation, Inc.
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
#include <limits.h>
#include <sys/statfs.h>

#include "linux_fsinfo.h"

static long int posix_pathconf (const char *path, int name);


/* Get file-specific information about descriptor FD.  */
long int
__pathconf (path, name)
     const char *path;
     int name;
{
  if (name == _PC_LINK_MAX)
    {
      struct statfs fsbuf;

      /* Determine the filesystem type.  */
      if (__statfs (path, &fsbuf) < 0)
	/* not possible, return the default value.  */
	return LINK_MAX;

      switch (fsbuf.f_type)
	{
	case EXT2_SUPER_MAGIC:
	  return EXT2_LINK_MAX;

	case MINIX_SUPER_MAGIC:
	case MINIX_SUPER_MAGIC2:
	  return MINIX_LINK_MAX;

	case MINIX2_SUPER_MAGIC:
	case MINIX2_SUPER_MAGIC2:
	  return MINIX2_LINK_MAX;

	case XENIX_SUPER_MAGIC:
	  return XENIX_LINK_MAX;

	case SYSV4_SUPER_MAGIC:
	case SYSV2_SUPER_MAGIC:
	  return SYSV_LINK_MAX;

	case COH_SUPER_MAGIC:
	  return COH_LINK_MAX;

	case UFS_MAGIC:
	case UFS_CIGAM:
	  return UFS_LINK_MAX;

	default:
	  return LINK_MAX;
	}
    }

  return posix_pathconf (path, name);
}

#define __pathconf static posix_pathconf
#include <sysdeps/posix/pathconf.c>