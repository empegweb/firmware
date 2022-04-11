/*
 * Lib/libunsn_f1.h -- private feature tests header for libunsn
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

#ifndef UNSN_LIBUNSN_F1_H
#define UNSN_LIBUNSN_F1_H 1

#include <Compat/socket.h>

/* Do we have the Sockets API? */

#if defined(HAVE_SOCKET) && defined(HAVE_STRUCT_SOCKADDR)

# define SUPPORT_SOCKETS 1

/* Which socket address families are supported? */

# include <Compat/sock_inet.h>
# if defined(PF_INET) && defined(AF_INET) && defined(HAVE_STRUCT_SOCKADDR_IN)
#  define SUPPORT_SOCK_INET 1
# endif
# if defined(PF_INET6) && defined(AF_INET6) && defined(HAVE_STRUCT_SOCKADDR_IN6)
#  define SUPPORT_SOCK_INET6 1
# endif

# include <Compat/sock_local.h>
# if defined(PF_LOCAL) && defined(AF_LOCAL) && defined(HAVE_STRUCT_SOCKADDR_UN)
#  define SUPPORT_SOCK_LOCAL 1
# endif

#endif /* HAVE_SOCKET && HAVE_STRUCT_SOCKADDR */

#endif
