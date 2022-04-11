/*
 * Lib/Compat/ipnode.c -- IPv6 host name lookups
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

#include <Compat/inetdb.h>

#ifdef NEED_COMPAT_IPNODE

# include <Compat/ip.h>
# include <Compat/errno.h>
# include <Compat/string.h>
# include <Compat/stdlib.h>

# define FLAVOR_GNU 1
# define FLAVOR_SOLARIS 2

# if defined(HAVE_GETHOSTBYADDR_R) && FUNC_FLAVOR_GETHOSTBYADDR_R

struct hostent *getipnodebyaddr(void const *src, size_t len,
	int af, int *error_num)
{
	int errno_save = errno;
	struct hostent host, *he;
	size_t buflen;
	char *buf;
	int herrno = 0;
#  if defined(AF_INET) && defined(AF_INET6)
	/* this is in case the local gethostbyaddr() doesn't understand IPv6
	   addresses -- we still want to be able to handle embedded IPv4
	   addresses */
	int embedv4 = len == 16 && af == AF_INET6 &&
			(IN6_IS_ADDR_V4MAPPED((struct in6_addr const *)src) ||
			IN6_IS_ADDR_V4COMPAT((struct in6_addr const *)src));
	if(embedv4) {
		src = 12+(char const *)src;
		len = 4;
		af = AF_INET;
	}
#  endif /* AF_INET && AF_INET6 */
	for(buflen = 128; ; buflen <<= 1) {
		buf = malloc(sizeof(struct hostent) + buflen
#  if defined(AF_INET) && defined(AF_INET6)
			+ (embedv4 ? 16 : 0)
#  endif /* AF_INET && AF_INET6 */
			);
		if(!buf) {
			errno = ENOMEM;
			if(error_num)
				*error_num = -1;
			return NULL;
		}
		errno = 0;
#  if FUNC_FLAVOR_GETHOSTBYADDR_R == FLAVOR_GNU
		if(!gethostbyaddr_r(src, len, af, &host,
			buf+sizeof(struct hostent), buflen, &he, &herrno) && he)
			break;
#  elif FUNC_FLAVOR_GETHOSTBYADDR_R == FLAVOR_SOLARIS
		if(he = gethostbyaddr_r(src, len, af, &host,
			buf + sizeof(struct hostent), buflen, &herrno))
			break;
#  else /* FUNC_FLAVOR_GETHOSTBYADDR_R */
 #error
#  endif /* FUNC_FLAVOR_GETHOSTBYADDR_R */
		free(buf);
		if(errno != ERANGE && errno != EMSGSIZE) {
			if(error_num)
				*error_num = herrno;
			if(errno == 0)
				errno = errno_save;
			return NULL;
		}
	}
#  if defined(AF_INET) && defined(AF_INET6)
	if(embedv4) {
		if(!he->h_addr_list ||
				!he->h_addr_list[0] || he->h_addr_list[1] ||
				memcmp(he->h_addr_list[0], src, 4)) {
			free(buf);
			if(error_num)
				*error_num = NO_RECOVERY;
			errno = errno_save;
			return NULL;
		}
		he->h_addrtype = AF_INET6;
		he->h_length = 16;
		memcpy(he->h_addr_list[0] =
				buf + sizeof(struct hostent) + buflen,
			((char const *)src)-12, 16);
	}
#  endif /* AF_INET && AF_INET6 */
	memcpy(buf, he, sizeof(struct hostent));
	errno = errno_save;
	return (struct hostent *)buf;
}

# elif defined(HAVE_GETHOSTBYADDR)

static size_t copy_hostent(struct hostent const *src,
	struct hostent *dst, char *buf, size_t buflen);
#  define NEED_COPY_HOSTENT 1

struct hostent *getipnodebyaddr(void const *src, size_t len,
	int af, int *error_num)
{
	struct hostent *he, *ret;
	size_t buflen;
	char *buf;
#  if defined(AF_INET) && defined(AF_INET6)
	/* this is in case the local gethostbyaddr() doesn't understand IPv6
	   addresses -- we still want to be able to handle embedded IPv4
	   addresses */
	if(len == 16 && af == AF_INET6 &&
			(IN6_IS_ADDR_V4MAPPED((struct in6_addr const *)src) ||
			 IN6_IS_ADDR_V4COMPAT((struct in6_addr const *)src))) {
		he = gethostbyaddr(12+(char const *)src, 4, AF_INET);
		if(he && he->h_addr_list &&
			 he->h_addr_list[0] && !he->h_addr_list[1] &&
			 !memcmp(he->h_addr_list[0], 12+(char const *)src, 4)) {
			he->h_addrtype = AF_INET6;
			he->h_length = 16;
			he->h_addr_list[0] = (char *)src;
		} else {
			he = NULL;
			h_errno = NO_RECOVERY;
		}
	} else
#  endif /* AF_INET && AF_INET6 */
		he = gethostbyaddr(src, len, af);
	if(!he) {
		if(error_num)
			*error_num = h_errno;
		return NULL;
	}
	buflen = copy_hostent(he, NULL, NULL, 0);
	buf = malloc(sizeof(struct hostent) + buflen);
	if(!buf) {
		errno = ENOMEM;
		if(error_num)
			*error_num = -1;
		return NULL;
	}
	ret = (struct hostent *)buf;
	copy_hostent(he, ret, buf + sizeof(struct hostent), buflen);
	return ret;
}

# else /* !HAVE_GETHOSTBYADDR_R && !HAVE_GETHOSTBYADDR */

struct hostent *getipnodebyaddr(void const *src, size_t len,
	int af, int *error_num)
{
#  ifdef AF_INET
	if(af == AF_INET && len != 4) {
		*error_num = NO_RECOVERY;
		return NULL;
	}
#  endif /* AF_INET */
#  ifdef AF_INET6
	if(af == AF_INET6 && len != 16) {
		*error_num = NO_RECOVERY;
		return NULL;
	}
#  endif /* AF_INET6 */
	*error_num = HOST_NOT_FOUND;
	return NULL;
}

# endif /* !HAVE_GETHOSTBYADDR && !HAVE_GETHOSTBYADDR_R */

# if !(defined(HAVE_GETHOSTBYNAME2_R) && FUNC_FLAVOR_GETHOSTBYNAME2_R) && \
	!defined(HAVE_GETHOSTBYNAME2)

#  if defined(HAVE_GETHOSTBYNAME_R) && FUNC_FLAVOR_GETHOSTBYNAME_R
#   define HAVE_GETHOSTBYNAME2_R 1

#   if FUNC_FLAVOR_GETHOSTBYNAME_R == FLAVOR_GNU
#    define FUNC_FLAVOR_GETHOSTBYNAME2_R FLAVOR_GNU

static int gethostbyname2_r(char const *name, int af,
	struct hostent *result_buf, char *buf, size_t buflen,
	struct hostent **result, int *h_errnop)
{
	if(af != AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}
	return gethostbyname_r(name, result_buf, buf, buflen, result, h_errnop);
}


#   elif FUNC_FLAVOR_GETHOSTBYNAME_R == FLAVOR_SOLARIS
#    define FUNC_FLAVOR_GETHOSTBYNAME2_R FLAVOR_SOLARIS

static struct hostent *gethostbyname2_r(char const *name, int af,
	struct hostent *result_buf, char *buf, size_t buflen, int *h_errnop)
{
	if(af != AF_INET) {
		errno = EAFNOSUPPORT;
		return NULL;
	}
	return gethostbyname_r(name, result_buf, buf, buflen, h_errnop);
}

#   else /* FUNC_FLAVOR_GETHOSTBYNAME_R */
 #error
#   endif /* FUNC_FLAVOR_GETHOSTBYNAME_R */

#  elif defined(HAVE_GETHOSTBYNAME)
#   define HAVE_GETHOSTBYNAME2 1

static struct hostent *gethostbyname2(char const *name, int af)
{
	if(af != AF_INET) {
		errno = EAFNOSUPPORT;
		return NULL;
	}
	return gethostbyname(name);
}

#  endif /* HAVE_GETHOSTBYNAME */

# endif /* !HAVE_GETHOSTBYNAME2_R && !HAVE_GETHOSTBYNAME2 */

static struct hostent *getnamedipnode(char const *name, int af, int *error_num);

# if defined(HAVE_GETHOSTBYNAME2_R) && FUNC_FLAVOR_GETHOSTBYNAME2_R

static struct hostent *getnamedipnode(char const *name, int af, int *error_num)
{
	int errno_save = errno;
	struct hostent host, *he;
	size_t buflen;
	char *buf;
	int herrno = 0;
	for(buflen = 128; ; buflen <<= 1) {
		buf = malloc(sizeof(struct hostent) + buflen);
		if(!buf) {
			errno = ENOMEM;
			if(error_num)
				*error_num = -1;
			return NULL;
		}
		errno = 0;
#  if FUNC_FLAVOR_GETHOSTBYNAME2_R == FLAVOR_GNU
		if(!gethostbyname2_r(name, af, &host,
			buf+sizeof(struct hostent), buflen, &he, &herrno) && he)
			break;
#  elif FUNC_FLAVOR_GETHOSTBYNAME2_R == FLAVOR_SOLARIS
		if(he = gethostbyname2_r(name, af, &host,
			buf + sizeof(struct hostent), buflen, &herrno))
			break;
#  else /* FUNC_FLAVOR_GETHOSTBYNAME2_R */
 #error
#  endif /* FUNC_FLAVOR_GETHOSTBYNAME2_R */
		free(buf);
		if(errno != ERANGE && errno != EMSGSIZE) {
			if(error_num)
				*error_num = herrno;
			if(errno == 0)
				errno = errno_save;
			return NULL;
		}
	}
	memcpy(buf, he, sizeof(struct hostent));
	errno = errno_save;
	return (struct hostent *)buf;
}

# elif defined(HAVE_GETHOSTBYNAME2)

static size_t copy_hostent(struct hostent const *src,
	struct hostent *dst, char *buf, size_t buflen);
#  define NEED_COPY_HOSTENT 1

static struct hostent *getnamedipnode(char const *name, int af, int *error_num)
{
	struct hostent *he, *ret;
	size_t buflen;
	char *buf;
	he = gethostbyname2(name, af);
	if(!he) {
		if(error_num)
			*error_num = h_errno;
		return NULL;
	}
	buflen = copy_hostent(he, NULL, NULL, 0);
	buf = malloc(sizeof(struct hostent) + buflen);
	if(!buf) {
		errno = ENOMEM;
		if(error_num)
			*error_num = -1;
		return NULL;
	}
	ret = (struct hostent *)buf;
	copy_hostent(he, ret, buf + sizeof(struct hostent), buflen);
	return ret;
}

# else /* !HAVE_GETHOSTBYNAME2_R && !HAVE_GETHOSTBYNAME2 */

static struct hostent *getnamedipnode(char const *name, int af, int *error_num)
{
	*error_num = HOST_NOT_FOUND;
	return NULL;
}

# endif /* !HAVE_GETHOSTBYNAME2 && !HAVE_GETHOSTBYNAME2_R */

/* NOTE: this is not a full implementation of getipnodebyname() -- it
   ignores the flags argument */

struct hostent *getipnodebyname(char const *name, int af,
	int flags, int *error_num)
{
	union addr {
#  ifdef AF_INET
		struct in_addr in;
#  endif /* AF_INET */
#  ifdef AF_INET6
		struct in6_addr in6;
#  endif /* AF_INET6 */
	} addr;
	size_t asz;
	/* check that the address family is valid */
#  ifdef AF_INET
	if(af == AF_INET)
		asz = sizeof(struct in_addr);
	else
#  endif /* AF_INET */
#  ifdef AF_INET6
	if(af == AF_INET6)
		asz = sizeof(struct in6_addr);
	else
#  endif /* AF_INET6 */
	{
		err:
		*error_num = NO_RECOVERY;
		return NULL;
	}
	/* check for an address argument */
	if(inet_pton(af, name, &addr)) {
		struct {
			struct hostent he;
			char *ptrs[2];
		} *buf;
		int errno_save = errno;
		buf = malloc(sizeof(*buf) + asz + strlen(name) + 1);
		if(!buf) {
			errno = errno_save;
			goto err;
		}
		buf->he.h_name = ((char *)&buf[1]) + asz;
		buf->he.h_aliases = &buf->ptrs[1];
		buf->he.h_addrtype = af;
		buf->he.h_length = asz;
		buf->he.h_addr_list = buf->ptrs;
		buf->ptrs[0] = (char *)&buf[1];
		buf->ptrs[1] = NULL;
		memcpy(&buf[1], &addr, asz);
		strcpy(buf->he.h_name, name);
		return &buf->he;
	}
	return getnamedipnode(name, af, error_num);
}

# ifdef NEED_COPY_HOSTENT

static size_t copy_hostent(struct hostent const *src,
	struct hostent *dst, char *buf, size_t buflen)
{
	size_t needbuf = 0, nptrs = 0;
	char *name, **ap, **dp;
	int alength = src->h_length;
	/* how much buffer space do we need? */
	if(name = src->h_name)
		needbuf += strlen(name) + 1;
	if(ap = src->h_aliases) {
		for(; name = *ap; ap++) {
			needbuf += strlen(name) + 1;
			nptrs++;
		}
		nptrs++;
	}
	if(ap = src->h_addr_list) {
		for(; *ap; ap++) {
			needbuf += alength;
			nptrs++;
		}
		nptrs++;
	}
	needbuf += nptrs * sizeof(char *);
	if(!dst || buflen < needbuf)
		return needbuf;
	/* copy */
	dst->h_addrtype = src->h_addrtype;
	dst->h_length = alength;
	dp = (char **)buf;
	buf += nptrs * sizeof(char *);
	if(name = src->h_name) {
		dst->h_name = buf;
		strcpy(buf, name);
		buf = strchr(buf, 0) + 1;
	} else
		dst->h_name = NULL;
	if(ap = src->h_aliases) {
		dst->h_aliases = dp;
		for(; name = *ap; ap++) {
			*dp++ = buf;
			strcpy(buf, name);
			buf = strchr(buf, 0) + 1;
		}
		*dp++ = NULL;
	} else
		dst->h_aliases = NULL;
	if(ap = src->h_addr_list) {
		dst->h_addr_list = dp;
		for(; name = *ap; ap++) {
			*dp++ = buf;
			memcpy(buf, name, alength);
			buf += alength;
		}
		*dp++ = NULL;
	} else
		dst->h_addr_list = NULL;
	return needbuf;
}

# endif /* NEED_COPY_HOSTENT */

void freehostent(struct hostent *ptr)
{
	free(ptr);
}

#endif /* NEED_COMPAT_IPNODE */
