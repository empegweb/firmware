/*
 * Lib/p_local.c -- "local" protocol functions
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

#include <Compat/stdlib.h>
#include <Compat/errno.h>
#include <Compat/string.h>

#include <ilayers.h>

enum { ADDRTYPE_NONE, ADDRTYPE_PATHNAME, ADDRTYPE_ABSTRACT };

struct ilayer_local {
	struct ilayer i;
	int stype;
	int addrtype;
	size_t len;
};

static void free_ilayer_local(struct ilayer *il)
{
	free(il);
}

#define RETURN_ERRNO(ERR) \
	do { \
		errno = ERR; \
		return NULL; \
	} while(0)

struct ilayer *unsn_private_mkilayer_local(struct sstring const *saddr,
	struct sstring const *stype)
{
	struct ilayer_local *ill;
	int socktype, addrtype;
	size_t addrlen;
	/* check type and validity of address */
	if(!saddr || !saddr->length) {
		addrtype = ADDRTYPE_NONE;
		addrlen = 0;
	} else if(!saddr->string[0]) {
		addrtype = ADDRTYPE_ABSTRACT;
		addrlen = saddr->length;
	} else {
		if(memchr(saddr->string, 0, saddr->length))
			RETURN_ERRNO(UNSN_EUNSNBADVALUE);
		addrtype = ADDRTYPE_PATHNAME;
		addrlen = saddr->length + 1;
	}
	/* check validity of type name */
	if(stype && strspn(stype->string, "_"STR_LOWER) != stype->length)
		RETURN_ERRNO(UNSN_EUNSNBADVALUE);
	/* interpret type name */
#ifdef SOCK_STREAM
	if(!stype || !cmpsstrstr(stype, ""))
		socktype = SOCK_STREAM;
#else /* !SOCK_STREAM */
	if(!stype)
		RETURN_ERRNO(UNSN_EUNSNLAYERNOSUPPORT);
#endif /* !SOCK_STREAM */
#include <p_local.i.ic> /* else if(!cmpsstrstr(stype, "@type@")) socktype = SOCK_@TYPE@; */
	else
		RETURN_ERRNO(UNSN_EUNSNLAYERNOSUPPORT);
	/* package up into structure */
	ill = malloc(sizeof(*ill) + addrlen);
	if(!ill)
		RETURN_ERRNO(ENOMEM);
	ill->i.next = NULL;
	ill->i.protocol = ILAYER_local;
	ill->i.free = free_ilayer_local;
	ill->stype = socktype;
	ill->addrtype = addrtype;
	ill->len = addrlen;
	if(addrlen)
		memcpy(&ill[1], saddr->string, addrlen);
	return &ill->i;
}

#ifdef SUPPORT_SOCK_LOCAL

# include <Compat/sock_local.h>

void unsn_private_ilayer_local_setsai(struct ilayer const *il,
	struct unsn_sockaddrinfo *sai, struct sockaddr_un *saun)
{
	struct ilayer_local const *ill = (struct ilayer_local const *)il;
	size_t sz = ill->len;
	if(sz > sizeof(saun->sun_path)) {
		sai->sai_family = UNSN_EUNSNLAYERNOSUPPORT;
		sai->sai_addr = NULL;
		return;
	}
	sai->sai_family = PF_LOCAL;
	sai->sai_type = ill->stype;
	sai->sai_protocol = 0;
	sai->sai_addr = (struct sockaddr *)saun;
	saun->sun_family = AF_LOCAL;
	memcpy(saun->sun_path, &ill[1], sz);
# ifdef HAVE_STRUCTMEM_SOCKADDR_SA_LEN
	saun->sun_len =
# endif /* HAVE_STRUCTMEM_SOCKADDR_SA_LEN */
		sai->sai_addrlen = offsetof(struct sockaddr_un, sun_path) + sz;
}

#endif /* SUPPORT_SOCK_LOCAL */
