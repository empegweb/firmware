/*
 * Lib/Compat/sock_local.h -- system compatibility hacks -- local-domain sockets
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

#ifndef UNSN_COMPAT_SOCK_LOCAL_H
#define UNSN_COMPAT_SOCK_LOCAL_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif

/* these might have different names */

#if !defined(PF_LOCAL) && defined(PF_UNIX)
# define PF_LOCAL PF_UNIX
#endif
#if !defined(PF_LOCAL) && defined(PF_FILE)
# define PF_LOCAL PF_FILE
#endif

#if !defined(AF_LOCAL) && defined(AF_UNIX)
# define AF_LOCAL AF_UNIX
#endif
#if !defined(AF_LOCAL) && defined(AF_FILE)
# define AF_LOCAL AF_FILE
#endif

#endif
