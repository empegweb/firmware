/*
 * Lib/Compat/unistd.h -- system compatibility hacks -- Unix standard functions
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

#ifndef UNSN_COMPAT_UNISTD_H
#define UNSN_COMPAT_UNISTD_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/* missing prototypes */

#if defined(HAVE_SETREGID) && !defined(HAVE_PROTOTYPE_SETREGID)
int setregid(gid_t, gid_t);
#endif

#if defined(HAVE_SETRESGID) && !defined(HAVE_PROTOTYPE_SETRESGID)
int setresgid(gid_t, gid_t, gid_t);
#endif

#if defined(HAVE_SETREUID) && !defined(HAVE_PROTOTYPE_SETREUID)
int setreuid(uid_t, uid_t);
#endif

#if defined(HAVE_SETRESUID) && !defined(HAVE_PROTOTYPE_SETRESUID)
int setresuid(uid_t, uid_t, uid_t);
#endif

#endif
