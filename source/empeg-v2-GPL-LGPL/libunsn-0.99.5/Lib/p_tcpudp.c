/*
 * Lib/p_tcpudp.c -- "tcp"/"udp" protocol functions
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

#include <libunsn_pr.h>

#include <Compat/errno.h>
#include <Compat/stdlib.h>
#include <Compat/string.h>
#include <Compat/inetdb.h>

#include <ilayers.h>

struct ilayer_tcpudp {
	struct ilayer i;
	long port;
};

static void free_ilayer_tcpudp(struct ilayer *il)
{
	free(il);
}

#define FLAVOR_GNU 1
#define FLAVOR_SOLARIS 2

#if defined(HAVE_GETSERVBYNAME_R) && FUNC_FLAVOR_GETSERVBYNAME_R

static long lookup_port_byname(char const *name, int iprotocol)
{
	char const *tlp = iprotocol == ILAYER_tcp ? "tcp" : "udp";
	int errno_save = errno;
	struct servent *serv;
	long ret = -1;
	struct servent se;
	char *buf = NULL;
	size_t buflen = 128;
	while(1) {
		buf = malloc(buflen);
		if(!buf)
			goto doreturn;
		errno = 0;
# if FUNC_FLAVOR_GETSERVBYNAME_R == FLAVOR_GNU
		if(!getservbyname_r(name, tlp, &se, buf, buflen, &serv) && serv)
			break;
# elif FUNC_FLAVOR_GETSERVBYNAME_R == FLAVOR_SOLARIS
		if(serv = getservbyname_r(name, tlp, &se, buf, buflen))
			break;
# else /* FUNC_FLAVOR_GETSERVBYNAME_R */
 #error
# endif /* FUNC_FLAVOR_GETSERVBYNAME_R */
		free(buf);
		if(errno != ERANGE && errno != EMSGSIZE)
			goto doreturn;
		buflen <<= 1;
	}
	ret = ntohs(serv->s_port);
	free(buf);
	doreturn:
	errno = errno_save;
	return ret;
}

#elif defined(HAVE_GETSERVBYNAME)

static long lookup_port_byname(char const *name, int iprotocol)
{
	char const *tlp = iprotocol == ILAYER_tcp ? "tcp" : "udp";
	int errno_save = errno;
	struct servent *serv;
	long ret = -1;
	serv = getservbyname(name, tlp);
	if(!serv)
		goto doreturn;
	ret = ntohs(serv->s_port);
	doreturn:
	errno = errno_save;
	return ret;
}

#else /* !HAVE_GETSERVBYNAME_R && !HAVE_GETSERVBYNAME */

static long lookup_port_byname(char const *name, int iprotocol)
{
	return -1;
}

#endif /* !HAVE_GETSERVBYNAME_R && !HAVE_GETSERVBYNAME */

#define RETURN_ERRNO(ERR) \
	do { \
		errno = ERR; \
		return NULL; \
	} while(0)

static struct ilayer *mkilayer_tcpudp(struct sstring const *sport,
	int iprotocol)
{
	struct ilayer_tcpudp *ilt;
	long port;
	/* interpret port */
	if(!sport || !sport->length)
		port = -1;
	else if(strspn(sport->string, STR_DIGIT) == sport->length) {
		/* numeric port number: check that it is in range */
		char const *p = sport->string;
		port = 0;
		while(*p) {
			port = port*10 + *p++-'0';
			if(port > 65535 || (iprotocol == ILAYER_tcp && !port))
				RETURN_ERRNO(UNSN_EUNSNBADVALUE);
		}
	} else {
		if(strspn(sport->string, STR_GRAPH) != sport->length)
			RETURN_ERRNO(UNSN_EUNSNBADVALUE);
		/* named port */
		port = lookup_port_byname(sport->string, iprotocol);
		if(port == -1)
			RETURN_ERRNO(iprotocol == ILAYER_tcp ?
				UNSN_ETCPPORTUNREC : UNSN_EUDPPORTUNREC);
	}
	/* package up into structure */
	ilt = malloc(sizeof(*ilt));
	if(!ilt)
		RETURN_ERRNO(ENOMEM);
	ilt->i.next = NULL;
	ilt->i.protocol = iprotocol;
	ilt->i.free = free_ilayer_tcpudp;
	ilt->port = port;
	return &ilt->i;
}

struct ilayer *unsn_private_mkilayer_tcp(struct sstring const *sport)
{
	return mkilayer_tcpudp(sport, ILAYER_tcp);
}

struct ilayer *unsn_private_mkilayer_udp(struct sstring const *sport)
{
	return mkilayer_tcpudp(sport, ILAYER_udp);
}

long unsn_private_ilayer_tcpudp_getport(struct ilayer const *il)
{
	struct ilayer_tcpudp const *ilt = (struct ilayer_tcpudp const *)il;
	return ilt->port;
}
