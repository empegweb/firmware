/*
 * Lib/p_ip.c -- "ip"/"ipv4"/"ipv6" protocol functions
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
#include <Compat/ip.h>
#include <Compat/inetdb.h>

#include <ilayers.h>

#define TRY_IPV4 0x01
#define TRY_IPV6 0x02

struct ilayer_ip {
	struct ilayer i;
	unsigned try;
	char *host;
	int protocol;
};

static void free_ilayer_ip(struct ilayer *il)
{
	free(il);
}

#define FLAVOR_GNU 1
#define FLAVOR_SOLARIS 2

#if defined(HAVE_GETPROTOBYNAME_R) && FUNC_FLAVOR_GETPROTOBYNAME_R

static int lookup_protocol_byname(char const *name)
{
	int errno_save = errno;
	struct protoent *proto;
	int ret = -1;
	struct protoent pe;
	char *buf = NULL;
	size_t buflen = 128;
	while(1) {
		buf = malloc(buflen);
		if(!buf)
			goto doreturn;
		errno = 0;
# if FUNC_FLAVOR_GETPROTOBYNAME_R == FLAVOR_GNU
		if(!getprotobyname_r(name, &pe, buf, buflen, &proto) && proto)
			break;
# elif FUNC_FLAVOR_GETPROTOBYNAME_R == FLAVOR_SOLARIS
		if(proto = getprotobyname_r(name, &pe, buf, buflen))
			break;
# else /* FUNC_FLAVOR_GETPROTOBYNAME_R */
 #error
# endif /* FUNC_FLAVOR_GETPROTOBYNAME_R */
		free(buf);
		if(errno != ERANGE && errno != EMSGSIZE)
			goto doreturn;
		buflen <<= 1;
	}
	ret = proto->p_proto;
	free(buf);
	doreturn:
	errno = errno_save;
	return ret;
}

#elif defined(HAVE_GETPROTOBYNAME)

static int lookup_protocol_byname(char const *name)
{
	int errno_save = errno;
	struct protoent *proto;
	int ret = -1;
	proto = getprotobyname(name);
	if(!proto)
		goto doreturn;
	ret = proto->p_proto;
	doreturn:
	errno = errno_save;
	return ret;
}

#else /* !HAVE_GETPROTOBYNAME_R && !HAVE_GETPROTOBYNAME */

static int lookup_protocol_byname(char const *name)
{
	return -1;
}

#endif /* !HAVE_GETPROTOBYNAME_R && !HAVE_GETPROTOBYNAME */

#define RETURN_ERRNO(ERR) \
	do { \
		errno = ERR; \
		return NULL; \
	} while(0)

static struct ilayer *mkilayer_ip(struct sstring const *saddr,
	struct sstring const *sprot, unsigned try, int iprotocol)
{
	struct ilayer_ip *ili;
	int hasaddr, protocol;
	/* check address */
	if(!saddr || !saddr->length)
		hasaddr = 0;
	else if(strspn(saddr->string, ((try&TRY_IPV6)?0:1) + ":-."STR_ALNUM)
			!= saddr->length)
		RETURN_ERRNO(UNSN_EUNSNBADVALUE);
	else
		hasaddr = 1;
	/* interpret protocol */
	if(!sprot || !sprot->length)
		protocol = -1;
	else if(strspn(sprot->string, STR_DIGIT) == sprot->length) {
		/* numeric protocol number: check that it is in range */
		char const *p = sprot->string;
		protocol = 0;
		while(*p) {
			protocol = protocol*10 + *p++-'0';
			if(protocol > 255)
				RETURN_ERRNO(UNSN_EUNSNBADVALUE);
		}
	} else {
		if(strspn(sprot->string, STR_GRAPH) != sprot->length)
			RETURN_ERRNO(UNSN_EUNSNBADVALUE);
		/* named protocol */
		protocol = lookup_protocol_byname(sprot->string);
		if(protocol == -1)
			RETURN_ERRNO(UNSN_EIPPROTOUNREC);
	}
	/* package up into structure */
	ili = malloc(sizeof(*ili) + (hasaddr ? saddr->length+1 : 0));
	if(!ili)
		RETURN_ERRNO(ENOMEM);
	ili->i.next = NULL;
	ili->i.protocol = iprotocol;
	ili->i.free = free_ilayer_ip;
	ili->try = try;
	ili->protocol = protocol;
	if(hasaddr)
		strcpy(ili->host = (char *)&ili[1], saddr->string);
	else
		ili->host = NULL;
	return &ili->i;
}

struct ilayer *unsn_private_mkilayer_ip(struct sstring const *saddr,
	struct sstring const *sprot)
{
	return mkilayer_ip(saddr, sprot, TRY_IPV6|TRY_IPV4, ILAYER_ip);
}

struct ilayer *unsn_private_mkilayer_ipv4(struct sstring const *saddr,
	struct sstring const *sprot)
{
	return mkilayer_ip(saddr, sprot, TRY_IPV4, ILAYER_ipv4);
} struct ilayer *unsn_private_mkilayer_ipv6(struct sstring const *saddr,
	struct sstring const *sprot)
{
	return mkilayer_ip(saddr, sprot, TRY_IPV6, ILAYER_ipv6);
}

/* Address extraction */

int unsn_private_ilayer_ip_getprotocol(struct ilayer const *il)
{
	struct ilayer_ip const *ili = (struct ilayer_ip const *)il;
	return ili->protocol;
}

enum { STATE_NULL, STATE_IPV6, STATE_IPV4 };

struct ilayer_ip_iter {
	int state;
	void *addrptr;
	int err6;
	struct in6_addr *addrs6, *end_addrs6;
	int err4;
	struct in_addr *addrs4, *end_addrs4;
	int protocol;
};

static struct in_addr inaddr_dummy;
static struct in6_addr in6addr_dummy;

struct ilayer_ip_iter *unsn_private_ilayer_ip_mkiter(
	struct ilayer const *il)
{
	struct ilayer_ip const *ili = (struct ilayer_ip const *)il;
	struct ilayer_ip_iter *i;
	i = malloc(sizeof(*i) + (ili->host ? strlen(ili->host)+1 : 0));
	if(!i) {
		errno = ENOMEM;
		return NULL;
	}
	i->state = STATE_NULL;
	i->addrptr = NULL;
	if(ili->host) {
		i->err6 = i->err4 = 0;
		strcpy((char *)&i[1], ili->host);
	} else {
		i->err6 = (ili->try & TRY_IPV6) ? UNSN_EADDRUNSPEC : 0;
		i->err4 = (ili->try & TRY_IPV4) ? UNSN_EADDRUNSPEC : 0;
	}
	i->addrs6 = i->end_addrs6 = (ili->try&TRY_IPV6) ? NULL : &in6addr_dummy;
	i->addrs4 = i->end_addrs4 = (ili->try&TRY_IPV4) ? NULL : &inaddr_dummy;
	i->protocol = ili->protocol;
	return i;
}

void unsn_private_ilayer_ip_iter_free(struct ilayer_ip_iter *i)
{
	if(!i)
		return;
	if(i->addrs6 != i->end_addrs6)
		free(i->addrs6);
	if(i->addrs4 != i->end_addrs4)
		free(i->addrs4);
	free(i);
}

struct ilayer_ip_addr unsn_private_ilayer_ip_iter_getaddr(
	struct ilayer_ip_iter const *i)
{
	struct ilayer_ip_addr a;
	if(i->state == STATE_NULL) {
		a.type = 0;
	} else if(i->state == STATE_IPV6) {
		int e = i->err6;
		if(!e) {
			a.type = 6;
			a.pe.protocol = i->protocol;
			a.addr.addr6 = *(struct in6_addr const *)i->addrptr;
		} else if(e == UNSN_EADDRUNSPEC) {
			a.type = -6;
			a.pe.protocol = i->protocol;
		} else {
			a.type = -1;
			a.pe.error = e;
		}
	} else {
		int e = i->err4;
		if(!e) {
			a.type = 4;
			a.pe.protocol = i->protocol;
			a.addr.addr4 = *(struct in_addr const *)i->addrptr;
		} else if(e == UNSN_EADDRUNSPEC) {
			a.type = -4;
			a.pe.protocol = i->protocol;
		} else {
			a.type = -1;
			a.pe.error = e;
		}
	}
	return a;
}

void unsn_private_ilayer_ip_iter_nullify(struct ilayer_ip_iter *i)
{
	i->state = STATE_NULL;
	i->addrptr = NULL;
}

static int translate_herrno(int herrno)
{
# ifdef TRY_AGAIN
	if(herrno == TRY_AGAIN)
		return UNSN_EIPHOSTTFAIL;
# endif /* TRY_AGAIN */
# ifdef NO_RECOVERY
	if(herrno == NO_RECOVERY)
		return UNSN_EIPHOSTPFAIL;
# endif /* NO_RECOVERY */
# ifdef HOST_NOT_FOUND
	if(herrno == HOST_NOT_FOUND)
		return UNSN_EIPNOHOST;
# endif /* HOST_NOT_FOUND */
# ifdef NO_ADDRESS
	if(herrno == NO_ADDRESS)
		return UNSN_EIPHOSTNOADDR;
# endif /* NO_ADDRESS */
# ifdef NO_DATA
	if(herrno == NO_DATA)
		return UNSN_EIPHOSTNOADDR;
# endif /* NO_DATA */
	return UNSN_EIPHOSTFAIL;
}

static void get_ipv6_addrs(struct ilayer_ip_iter *i)
{
#ifdef AF_INET6
	size_t sz;
	char **ap;
	struct in6_addr *addrs;
	int errno_save = errno, herrno;
	struct hostent *he = getipnodebyname((char const *)&i[1],
		AF_INET6, 0, &herrno);
	if(!he) {
		i->err6 = translate_herrno(herrno);
		errno = errno_save;
		return;
	}
	for(sz = 0, ap = he->h_addr_list; *ap; ap++)
		sz += sizeof(struct in6_addr);
	addrs = malloc(sz);
	if(!addrs) {
		freehostent(he);
		i->err6 = ENOMEM;
		errno = errno_save;
		return;
	}
	i->addrs6 = addrs;
	i->end_addrs6 = addrs + sz/sizeof(*addrs);
	for(ap = he->h_addr_list; *ap; ap++)
		*addrs++ = *(struct in6_addr const *)*ap;
	freehostent(he);
	errno = errno_save;
#else /* !AF_INET6 */
	i->err6 = UNSN_EIPHOSTNOADDR;
#endif /* !AF_INET6 */
}

static void get_ipv4_addrs(struct ilayer_ip_iter *i)
{
#ifdef AF_INET
	size_t sz;
	char **ap;
	struct in_addr *addrs;
	int errno_save = errno, herrno;
	struct hostent *he = getipnodebyname((char const *)&i[1],
		AF_INET, 0, &herrno);
	if(!he) {
		i->err4 = translate_herrno(herrno);
		errno = errno_save;
		return;
	}
	for(sz = 0, ap = he->h_addr_list; *ap; ap++)
		sz += sizeof(struct in_addr);
	addrs = malloc(sz);
	if(!addrs) {
		freehostent(he);
		i->err4 = ENOMEM;
		errno = errno_save;
		return;
	}
	i->addrs4 = addrs;
	i->end_addrs4 = addrs + sz/sizeof(*addrs);
	for(ap = he->h_addr_list; *ap; ap++)
		*addrs++ = *(struct in_addr const *)*ap;
	freehostent(he);
	errno = errno_save;
#else /* !AF_INET */
	i->err4 = UNSN_EIPHOSTNOADDR;
#endif /* !AF_INET */
}

struct ilayer_ip_addr unsn_private_ilayer_ip_iter_advance(
	struct ilayer_ip_iter *i)
{
	if(i->state == STATE_NULL) {
		if(!i->err6 && !i->addrs6) {
			get_ipv6_addrs(i);
			if(i->err6) {
				if(!i->err4 && !i->addrs4)
					get_ipv4_addrs(i);
				if(i->err4) {
					/* both have errors */
					i->err4 = unsn_errno_max(i->err6,
						i->err4);
					blank_ipv6:
					i->err6 = 0;
					i->addrs6 = i->end_addrs6 =
						&in6addr_dummy;
					goto goipv4;
				} else if((i->err6 == UNSN_EIPNOHOST ||
					   i->err6 == UNSN_EIPHOSTNOADDR) &&
					  i->addrs4 != i->end_addrs4)
					goto blank_ipv6;
			}
		}
		if(!i->err6 && i->addrs6 == i->end_addrs6)
			goto goipv4;
		i->state = STATE_IPV6;
		i->addrptr = i->err6 ? NULL : i->addrs6;
	} else if(i->state == STATE_IPV6) {
		if(!i->addrptr) {
			goipv4:
			if(!i->err4 && !i->addrs4) {
				get_ipv4_addrs(i);
				if((i->err4 == UNSN_EIPNOHOST ||
				    i->err4 == UNSN_EIPHOSTNOADDR) &&
				   i->addrs6 != i->end_addrs6) {
					i->err4 = 0;
					i->addrs4 = i->end_addrs4 =
						&inaddr_dummy;
					goto gonull;
				}
			}
			if(!i->err4 && i->addrs4 == i->end_addrs4)
				goto gonull;
			i->state = STATE_IPV4;
			i->addrptr = i->err4 ? NULL : i->addrs4;
		} else {
			struct in6_addr *newptr = i->addrptr;
			newptr++;
			if(newptr == i->end_addrs6)
				goto goipv4;
			i->addrptr = newptr;
		}
	} else {
		if(!i->addrptr) {
			gonull:
			i->state = STATE_NULL;
			i->addrptr = NULL;
		} else {
			struct in_addr *newptr = i->addrptr;
			newptr++;
			if(newptr == i->end_addrs4)
				goto gonull;
			i->addrptr = newptr;
		}
	}
	return unsn_private_ilayer_ip_iter_getaddr(i);
}
