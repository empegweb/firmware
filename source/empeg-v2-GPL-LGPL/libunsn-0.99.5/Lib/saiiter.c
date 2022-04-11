/*
 * Lib/saiiter.c -- unsn_sai_iterator functions
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

#ifdef SUPPORT_SOCKETS

# include <Compat/socket.h>
# include <Compat/errno.h>
# include <Compat/stdlib.h>

# include <ilayers.h>

struct unsn_sai_iterator {
	int (*advance)(struct unsn_sai_iterator *);
	struct unsn_alt_iterator *ai;
	void *extra;
	void (*free_extra)(void *);
	struct unsn_sockaddrinfo sai;
};

static void free_donothing(void *extra)
{
}

static int advance_altiter(struct unsn_sai_iterator *i);

/* Socket address generation */

static void seterr(struct unsn_sai_iterator *i, int errnum)
{
	i->sai.sai_family = errnum;
	i->sai.sai_type = 0;
	i->sai.sai_protocol = 0;
	i->sai.sai_addr = NULL;
	i->sai.sai_addrlen = 0;
}

# ifdef SUPPORT_SOCK_LOCAL

#  include <Compat/sock_local.h>

static void setsai_local(struct unsn_sai_iterator *i, struct ilayer const *il)
{
	int errno_save = errno;
	struct sockaddr_un *saun = malloc(sizeof(*saun));
	if(!saun) {
		errno = errno_save;
		seterr(i, ENOMEM);
		return;
	}
	i->extra = saun;
	i->free_extra = free;
	unsn_private_ilayer_local_setsai(il, &i->sai, saun);
}

# endif /* SUPPORT_SOCK_LOCAL */

# if defined(SUPPORT_SOCK_INET) || defined(SUPPORT_SOCK_INET6)

#  include <Compat/sock_inet.h>
#  include <Compat/ip.h>

struct ip_iter {
	struct ilayer_ip_iter *i;
	int saitype;
	int saiprotocol;
	in_port_t portno;
	union {
		struct sockaddr a;
#  ifdef SUPPORT_SOCK_INET
		struct sockaddr_in in;
#  endif /* SUPPORT_SOCK_INET */
#  ifdef SUPPORT_SOCK_INET6
		struct sockaddr_in6 in6;
#  endif /* SUPPORT_SOCK_INET6 */
	} addr;
};

static void free_ipiter(void *extra)
{
	struct ip_iter *ipiter = extra;
	unsn_private_ilayer_ip_iter_free(ipiter->i);
	free(ipiter);
}

static int advance_ipiter(struct unsn_sai_iterator *i)
{
	struct ip_iter *ipiter = i->extra;
	struct ilayer_ip_addr a;
	/* get next alternative */
	a = unsn_private_ilayer_ip_iter_advance(ipiter->i);
	switch(a.type) {
		case 0: {
			/* no more addresses */
			i->advance = advance_altiter;
			return advance_altiter(i);
		}
		case -1: {
			/* error */
			seterr(i, a.pe.error);
			return 1;
		}
#  ifdef SUPPORT_SOCK_INET
		case -4:
			a.addr.addr4.s_addr = htonl(INADDR_ANY);
			/* fall through */
		case 4: {
			ipiter->addr.in.sin_family = AF_INET;
			ipiter->addr.in.sin_port = ipiter->portno;
			ipiter->addr.in.sin_addr = a.addr.addr4;
			i->sai.sai_family = PF_INET;
			i->sai.sai_addrlen = sizeof(ipiter->addr.in);
			break;
		}
#  endif /* SUPPORT_SOCK_INET */
#  ifdef SUPPORT_SOCK_INET6
		case -6:
			a.addr.addr6 = in6addr_any;
			/* fall through */
		case 6: {
			ipiter->addr.in6.sin6_family = AF_INET6;
			ipiter->addr.in6.sin6_port = ipiter->portno;
			ipiter->addr.in6.sin6_addr = a.addr.addr6;
			ipiter->addr.in6.sin6_flowinfo = 0;
#   ifdef HAVE_STRUCTMEM_SOCKADDR_IN6_SIN6_SCOPE_ID
			ipiter->addr.in6.sin6_scope_id = 0;
#   endif
			i->sai.sai_family = PF_INET6;
			i->sai.sai_addrlen = sizeof(ipiter->addr.in6);
			break;
		}
#  endif /* SUPPORT_SOCK_INET6 */
		default: {
			/* unsupported address type */
			seterr(i, EPFNOSUPPORT);
			return 1;
		}
	}
#  ifdef HAVE_STRUCTMEM_SOCKADDR_SA_LEN
	ipiter->addr.a.sa_len = i->sai.sai_addrlen;
#  endif
	i->sai.sai_type = ipiter->saitype;
	i->sai.sai_protocol = ipiter->saiprotocol;
	i->sai.sai_addr = &ipiter->addr.a;
	return 1;
}

static void setsai_ip(struct unsn_sai_iterator *i, struct ilayer const *il)
{
	int errno_save;
	struct ip_iter *ipiter;
	int protocol = unsn_private_ilayer_ip_getprotocol(il);
	int saitype;
	in_port_t portno;
	struct ilayer_ip_iter *addriter;
	/* determine transport layer protocol number */
	if(!il->next) {
		if(protocol == -1) {
			seterr(i, UNSN_EADDRUNSPEC);
			return;
		}
#  ifdef SOCK_RAW
		saitype = SOCK_RAW;
#  else /* !SOCK_RAW */
		seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
		return;
#  endif /* !SOCK_RAW */
	} else if(il->next->protocol == ILAYER_tcp) {
#  ifdef SOCK_STREAM
		if(protocol != -1 && protocol != IPPROTO_TCP) {
			seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
			return;
		}
		protocol = IPPROTO_TCP;
		saitype = SOCK_STREAM;
#  else /* !SOCK_STREAM */
		seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
		return;
#  endif /* !SOCK_STREAM */
	} else {
#  ifdef SOCK_DGRAM
		if(protocol != -1 && protocol != IPPROTO_UDP) {
			seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
			return;
		}
		protocol = IPPROTO_UDP;
		saitype = SOCK_DGRAM;
#  else /* !SOCK_DGRAM */
		seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
		return;
#  endif /* !SOCK_DGRAM */
	}
	if(protocol == IPPROTO_RAW)
		protocol |= 0x100;
	/* get port number for transport layer */
	if(!il->next) {
		portno = 0;
	} else {
		long p = unsn_private_ilayer_tcpudp_getport(il->next);
		if(!p) {
			seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
			return;
		}
		portno = (p == -1) ? 0 : p;
		portno = htons(portno);
	}
	/* create address iterator */
	errno_save = errno;
	addriter = unsn_private_ilayer_ip_mkiter(il);
	if(!addriter) {
		seterr(i, errno);
		errno = errno_save;
		return;
	}
	/* package up results */
	ipiter = malloc(sizeof(*ipiter));
	if(!ipiter) {
		unsn_private_ilayer_ip_iter_free(addriter);
		errno = errno_save;
		seterr(i, ENOMEM);
		return;
	}
	ipiter->i = addriter;
	ipiter->saitype = saitype;
	ipiter->saiprotocol = protocol;
	ipiter->portno = portno;
	/* hand over to iteration function */
	i->extra = ipiter;
	i->free_extra = free_ipiter;
	i->advance = advance_ipiter;
	advance_ipiter(i);
}

# endif /* SUPPORT_SOCK_INET || SUPPORT_SOCK_INET6 */

static int advance_altiter(struct unsn_sai_iterator *i)
{
	int errno_save;
	struct unsn_a_layerset *ls;
	struct ilayer *il;
	size_t nlayers;
	i->free_extra(i->extra);
	i->free_extra = free_donothing;
	/* get next alternative */
	if(!unsn_alt_iterator_advance(i->ai))
		return 0;
	errno_save = errno;
	ls = unsn_private_getalt(i->ai);
	if(!ls) {
		seterr(i, errno);
		errno = errno_save;
		return 1;
	}
	nlayers = ls->nlayers;
	/* interpret and check for semantic errors */
	il = unsn_private_interplayerset(ls);
	unsn_private_freealt(ls);
	if(!il) {
		seterr(i, errno);
		errno = errno_save;
		return 1;
	}
	/* identify and handle the layer combination */
# ifdef SUPPORT_SOCK_LOCAL
	if(nlayers == 1 && il->protocol == ILAYER_local) {
		setsai_local(i, il);
		goto free_ret;
	}
# endif /* SUPPORT_SOCK_LOCAL */
# if defined(SUPPORT_SOCK_INET) || defined(SUPPORT_SOCK_INET6)
	if((nlayers == 1 || nlayers == 2) &&
		(il->protocol == ILAYER_ip ||
		 il->protocol == ILAYER_ipv4 || il->protocol == ILAYER_ipv6) &&
		(!il->next ||
		 il->next->protocol == ILAYER_tcp ||
		 il->next->protocol == ILAYER_udp)) {
		setsai_ip(i, il);
		goto free_ret;
	}
# endif /* SUPPORT_SOCK_INET || SUPPORT_SOCK_INET6 */
	/* unsupported layer combination */
	seterr(i, UNSN_EUNSNCOMBINOSUPPORT);
	free_ret:
	unsn_private_freeilayers(il);
	return 1;
}

/* Iterator operations */

void unsn_sai_iterator_free(struct unsn_sai_iterator *i)
{
	if(!i)
		return;
	unsn_alt_iterator_free(i->ai);
	i->free_extra(i->extra);
	free(i);
}

struct unsn_sockaddrinfo const *unsn_sai_iterator_sai(
	struct unsn_sai_iterator const *i)
{
	return unsn_alt_iterator_nonnull(i->ai) ? &i->sai : NULL;
}

void unsn_sai_iterator_nullify(struct unsn_sai_iterator *i)
{
	i->free_extra(i->extra);
	i->free_extra = free_donothing;
	unsn_alt_iterator_nullify(i->ai);
	i->advance = advance_altiter;
}

struct unsn_sockaddrinfo const *unsn_sai_iterator_advance(
	struct unsn_sai_iterator *i)
{
	return i->advance(i) ? &i->sai : NULL;
}

struct unsn_sai_iterator *unsn_private_mksaiiterator(
	struct unsn_alt_iterator *ai)
{
	struct unsn_sai_iterator *ret;
	ret = malloc(sizeof(*ret));
	if(!ret) {
		unsn_alt_iterator_free(ai);
		errno = ENOMEM;
		return NULL;
	}
	ret->advance = advance_altiter;
	ret->ai = ai;
	ret->extra = NULL;
	ret->free_extra = free_donothing;
	return ret;
}

#endif /* SUPPORT_SOCKETS */
