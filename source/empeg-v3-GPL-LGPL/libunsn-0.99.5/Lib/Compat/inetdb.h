/*
 * Lib/Compat/inetdb.h -- system compatibility hacks -- Internet databases
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

#ifndef UNSN_COMPAT_INETDB_H
#define UNSN_COMPAT_INETDB_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

/* RFC 2553 name/address conversion functions */

#ifndef HAVE_STRUCT_HOSTENT
struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};
# define HAVE_STRUCT_HOSTENT 1
#endif /* !HAVE_STRUCT_HOSTENT */

#ifndef HOST_NOT_FOUND
# define HOST_NOT_FOUND 1
# define NO_ADDRESS 2
# define NO_RECOVERY 3
# define TRY_AGAIN 4
#endif /* !HOST_NOT_FOUND */

#if !defined(HAVE_GETIPNODEBYNAME) || !defined(HAVE_GETIPNODEBYADDR)

# define NEED_COMPAT_IPNODE 1

# undef getipnodebyaddr
# undef HAVE_GETIPNODEBYADDR
# define getipnodebyaddr unsn_compat_getipnodebyaddr
struct hostent *getipnodebyaddr(void const * /*src*/, size_t /*len*/,
	int /*af*/, int * /*error_num*/);
# define NEED_COMPAT_GETIPNODEBYADDR 1
# define HAVE_GETIPNODEBYADDR 1

# undef getipnodebyname
# undef HAVE_GETIPNODEBYNAME
# define getipnodebyname unsn_compat_getipnodebyname
struct hostent *getipnodebyname(char const * /*name*/, int /*af*/,
	int /*flags*/, int * /*error_num*/);
# define NEED_COMPAT_GETIPNODEBYNAME 1
# define HAVE_GETIPNODEBYNAME 1

# undef freehostent
# define freehostent unsn_compat_freehostent
void freehostent(struct hostent * /*ptr*/);

#endif /* !HAVE_GETIPNODEBYNAME || !HAVE_GETIPNODEBYADDR */

#endif
