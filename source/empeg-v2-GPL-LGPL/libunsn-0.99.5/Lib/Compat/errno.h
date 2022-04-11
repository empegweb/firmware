/*
 * Lib/Compat/errno.h -- system compatibility hacks -- error numbers
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

#ifndef UNSN_COMPAT_ERRNO_H
#define UNSN_COMPAT_ERRNO_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif

/* this declaration might be missing */

#ifndef HAVE_DECLARATION_ERRNO
extern int errno;
#endif

/* alternative error names */

#ifndef EWOULDBLOCK
# define EWOULDBLOCK EAGAIN
#endif

#ifndef EAGAIN
# define EAGAIN EWOULDBLOCK
#endif

#endif
