/*
 * Lib/Compat/socket.h -- system compatibility hacks -- sockets
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

#ifndef UNSN_COMPAT_SOCKET_H
#define UNSN_COMPAT_SOCKET_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif

/* these sometimes have an incorrect prototype */

#ifdef BIND_HAS_NONCONST_PROTOTYPE
# undef bind
# define bind ((int (*)(int, struct sockaddr const *, socklen_t)) bind)
#endif /* BIND_HAS_NONCONST_PROTOTYPE */

#ifdef CONNECT_HAS_NONCONST_PROTOTYPE
# undef connect
# define connect ((int (*)(int, struct sockaddr const *, socklen_t)) connect)
#endif /* CONNECT_HAS_NONCONST_PROTOTYPE */

/* missing prototypes */

#ifdef HAVE_STRUCT_SOCKADDR

# ifndef HAVE_PROTOTYPE_BIND
int bind(int, struct sockaddr const *, socklen_t);
# endif

# ifndef HAVE_PROTOTYPE_CONNECT
int connect(int, struct sockaddr const *, socklen_t);
# endif

# ifndef HAVE_PROTOTYPE_GETPEERNAME
int getpeername(int, struct sockaddr *, socklen_t *);
# endif

# ifndef HAVE_PROTOTYPE_GETSOCKNAME
int getsockname(int, struct sockaddr *, socklen_t *);
# endif

#endif /* HAVE_STRUCT_SOCKADDR */

/* constants that might be missing */

#ifndef SHUT_RD
# define SHUT_RD 0
#endif

#ifndef SHUT_WR
# define SHUT_WR 1
#endif

#ifndef SHUT_RDWR
# define SHUT_RDWR 2
#endif

#ifndef SOMAXCONN
# define SOMAXCONN 128
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
