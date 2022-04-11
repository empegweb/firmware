/*
 * Lib/aitounsn.c -- unsn_aitounsn()
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

# include <Compat/errno.h>
# include <Compat/string.h>
# include <Compat/stdlib.h>
# include <Compat/socket.h>
# include <stdio.h>

# if defined(SUPPORT_SOCK_INET) || defined(SUPPORT_SOCK_INET6)

#  include <Compat/sock_inet.h>
#  include <Compat/ip.h>
#  include <Compat/inetdb.h>

#  define FLAVOR_GNU 1
#  define FLAVOR_SOLARIS 2

static inline char *gethname(int af, void const *address)
{
	int errno_save = errno;
	int herrno;
	int len;
	size_t nlen;
	struct hostent *he = NULL;
	char *name, *ret = NULL;
	if(0)
		;
#  ifdef SUPPORT_SOCK_INET
	else if(af == AF_INET)
		len = 4;
#  endif
#  ifdef SUPPORT_SOCK_INET6
	else if(af == AF_INET6)
		len = 16;
#  endif
	else
		goto doreturn;
	he = getipnodebyaddr(address, len, af, &herrno);
	if(!he || !(name = he->h_name))
		goto doreturn;
	ret = malloc(nlen = strlen(name) + 1);
	if(!ret)
		goto doreturn;
	memcpy(ret, name, nlen);
	doreturn:
	if(he)
		freehostent(he);
	errno = errno_save;
	return ret;
}

#  if (defined(HAVE_GETPROTOBYNUMBER_R) && FUNC_FLAVOR_GETPROTOBYNUMBER_R) || \
	defined(HAVE_GETPROTOBYNUMBER)

static inline char *select_proto_name(struct protoent *proto)
{
	/* find the official protocol name, which starts with a capital */
	char c, *name, **ap;
	if(!(name = proto->p_name))
		return NULL;
	c = *name;
	if(c >= 'A' && c <= 'Z')
		return name;
	if(!(ap = proto->p_aliases))
		return NULL;
	for(; name = *ap; ap++) {
		c = *name;
		if(c >= 'A' && c <= 'Z')
			return name;
	}
	return NULL;
}

static inline char *getprotoname(int number)
{
	int errno_save = errno;
	char *buf = NULL;
	struct protoent *proto;
	char *name;
	size_t len;
#   if defined(HAVE_GETPROTOBYNUMBER_R) && FUNC_FLAVOR_GETPROTOBYNUMBER_R
	struct protoent pe;
	size_t buflen = 128;
	while(1) {
		buf = malloc(buflen);
		if(!buf)
			goto doreturn;
		errno = 0;
#    if FUNC_FLAVOR_GETPROTOBYNUMBER_R == FLAVOR_GNU
		if(!getprotobynumber_r(number, &pe, buf, buflen, &proto) &&
			proto)
			break;
#    elif FUNC_FLAVOR_GETPROTOBYNUMBER_R == FLAVOR_SOLARIS
		if(proto = getprotobynumber_r(number, &pe, buf, buflen))
			break;
#    else /* FUNC_FLAVOR_GETPROTOBYNUMBER_R */
 #error
#    endif /* FUNC_FLAVOR_GETPROTOBYNUMBER_R */
		free(buf);
		buf = NULL;
		if(errno != ERANGE && errno != EMSGSIZE)
			goto doreturn;
		buflen <<= 1;
	}
	name = select_proto_name(proto);
	if(!name) {
		free(buf);
		buf = NULL;
		goto doreturn;
	}
	len = strlen(name) + 1;
	if(len > buflen) {
		free(buf);
		buf = malloc(len);
		if(!buf)
			goto doreturn;
	}
	memmove(buf, name, len);
#   else /* HAVE_GETPROTOBYNUMBER */
	proto = getprotobynumber(number);
	if(!proto)
		goto doreturn;
	name = select_proto_name(proto);
	if(!name)
		goto doreturn;
	buf = malloc(len = strlen(name) + 1);
	if(!buf)
		goto doreturn;
	memcpy(buf, name, len);
#   endif /* HAVE_GETPROTOBYNUMBER */
	doreturn:
	errno = errno_save;
	return buf;
}

#  else /* !HAVE_GETPROTOBYNUMBER_R && !HAVE_GETPROTOBYNUMBER */

static inline char *getprotoname(int number)
{
	return NULL;
}

#  endif /* !HAVE_GETPROTOBYNUMBER_R && !HAVE_GETPROTOBYNUMBER */

#  if (defined(HAVE_GETSERVBYPORT_R) && FUNC_FLAVOR_GETSERVBYPORT_R) || \
	defined(HAVE_GETSERVBYPORT)

static inline char *getservname(int port, char const *proto)
{
	int errno_save = errno;
	char *buf = NULL;
	struct servent *serv;
	size_t len;
#   if defined(HAVE_GETSERVBYPORT_R) && FUNC_FLAVOR_GETSERVBYPORT_R
	struct servent se;
	size_t buflen = 128;
	while(1) {
		buf = malloc(buflen);
		if(!buf)
			goto doreturn;
		errno = 0;
#    if FUNC_FLAVOR_GETSERVBYPORT_R == FLAVOR_GNU
		if(!getservbyport_r(port, proto, &se, buf, buflen, &serv) &&
			serv)
			break;
#    elif FUNC_FLAVOR_GETSERVBYPORT_R == FLAVOR_SOLARIS
		if(serv = getservbyport_r(port, proto, &se, buf, buflen))
			break;
#    else /* FUNC_FLAVOR_GETSERVBYPORT_R */
 #error
#    endif /* FUNC_FLAVOR_GETSERVBYPORT_R */
		free(buf);
		buf = NULL;
		if(errno != ERANGE && errno != EMSGSIZE)
			goto doreturn;
		buflen <<= 1;
	}
	len = strlen(serv->s_name) + 1;
	if(len > buflen) {
		free(buf);
		buf = malloc(len);
		if(!buf)
			goto doreturn;
	}
	memmove(buf, serv->s_name, len);
#   else /* HAVE_GETSERVBYPORT */
	serv = getservbyport(port, proto);
	if(!serv)
		goto doreturn;
	buf = malloc(len = strlen(serv->s_name) + 1);
	if(!buf)
		goto doreturn;
	memcpy(buf, serv->s_name, len);
#   endif /* HAVE_GETSERVBYPORT */
	doreturn:
	errno = errno_save;
	return buf;
}

#  else /* !HAVE_GETSERVBYPORT_R && !HAVE_GETSERVBYPORT */

static inline char *getservname(int port, char const *proto)
{
	return NULL;
}

#  endif /* !HAVE_GETSERVBYPORT_R && !HAVE_GETSERVBYPORT */

#  define ADDRSTRLEN 1
#  if defined(SUPPORT_SOCK_INET) && INET_ADDRSTRLEN > ADDRSTRLEN
#   undef ADDRSTRLEN
#   define ADDRSTRLEN INET_ADDRSTRLEN
#  endif
#  if defined(SUPPORT_SOCK_INET6) && INET6_ADDRSTRLEN > ADDRSTRLEN
#   undef ADDRSTRLEN
#   define ADDRSTRLEN INET6_ADDRSTRLEN
#  endif

static inline char *aitounsn_ip(int sock_family,
	int sock_type, int sock_protocol,
	struct sockaddr const *addr, socklen_t addrlen, unsigned opts)
{
	int af = 0, aflen = 0;
	char const *version = NULL;
	void const *addrptr = NULL;
	int errno_save = errno;
	unsigned portno = 0;
	char const *tlp = NULL;
	char *hostname = NULL, *protoname = NULL, *servname = NULL;
	char *buf, *ptr;
	char addrbuf[ADDRSTRLEN];
	int usehost = 0, useportno = 0, useprotocolno = 0;
	size_t len;
#  ifdef SUPPORT_SOCK_INET
	if(sock_family == PF_INET) {
		af = AF_INET;
		aflen = sizeof(struct sockaddr_in);
		version = "v4";
	}
#  endif
#  ifdef SUPPORT_SOCK_INET6
	if(sock_family == PF_INET6) {
		af = AF_INET6;
		aflen = sizeof(struct sockaddr_in6);
		version = "v6";
	}
#  endif
	if(addr->sa_family != af)
		goto eafnosupport;
	if(addrlen != aflen)
		goto einval;
#  ifdef SUPPORT_SOCK_INET
	if(sock_family == PF_INET) {
		struct sockaddr_in const *sinaddr = (struct sockaddr_in *)addr;
		portno = sinaddr->sin_port;
		addrptr = &sinaddr->sin_addr;
		usehost = sinaddr->sin_addr.s_addr != htonl(INADDR_ANY);
	}
#  endif
#  ifdef SUPPORT_SOCK_INET6
	if(sock_family == PF_INET6) {
		struct sockaddr_in6 const *sin6addr =
			(struct sockaddr_in6 *)addr;
		sin6addr = (struct sockaddr_in6 *)addr;
		portno = sin6addr->sin6_port;
		addrptr = &sin6addr->sin6_addr;
		usehost = !IN6_IS_ADDR_UNSPECIFIED(&sin6addr->sin6_addr);
	}
#  endif
	/* sock_protocol defaults depending on sock_type */
	switch(sock_type) {
		case SOCK_STREAM:
			if(sock_protocol && sock_protocol != IPPROTO_TCP)
				goto eprotonosupport;
			tlp = "tcp";
			useportno = 1;
			break;
		case SOCK_DGRAM:
			if(sock_protocol && sock_protocol != IPPROTO_UDP)
				goto eprotonosupport;
			tlp = "udp";
			useportno = 1;
			break;
		case SOCK_RAW:
#  ifdef SUPPORT_SOCK_INET
			/* If sock_protocol==IPPROTO_RAW, then the user
			   sends the IP headers, and so could send
			   anything, IP or not.  With any other protocol,
			   the kernel adds IP headers, and the user just
			   specifies payload; sock_protocol fills the
			   protocol field in the IP header. */
			if(sock_family == PF_INET &&
				sock_protocol == IPPROTO_RAW)
				goto eprotonosupport;
#  endif /* SUPPORT_SOCK_INET */
			sock_protocol &= 0xff;
			useprotocolno = 1;
			break;
		default:
			goto esocktnosupport;
	}
	/* useportno was set above if the transport layer uses port
	   numbers.  We can still have an unspecified port. */
	if(useportno)
		useportno = portno != htons(0);
	/* Work out what will go into the UNSN, and how long it is. */
	len = 2; /* "ip" */
	if(usehost) {
		if(opts & UNSN_USENAMES)
			hostname = gethname(af, addrptr);
		len++; /* "=" */
		if(hostname)
			len += unsn_encode(NULL, hostname, -1);
		else {
			if(!inet_ntop(af, addrptr, addrbuf, ADDRSTRLEN))
				return NULL;
			len += strlen(addrbuf);
		}
	} else if(!(opts & UNSN_USENAMES))
		len += strlen(version);
	if(useprotocolno) {
		if(opts & UNSN_USENAMES)
			protoname = getprotoname(sock_protocol);
		len += 10; /* ",protocol=" */
		if(protoname)
			len += unsn_encode(NULL, protoname, -1);
		else
			len += 3; /* "nnn" */
	}
	if(tlp)
		len += 1 + /* "/" */
			strlen(tlp);
	if(useportno) {
		if(opts & UNSN_USENAMES)
			servname = getservname(portno, tlp);
		len++; /* "=" */
		if(servname)
			len += unsn_encode(NULL, servname, -1);
		else
			len += 5; /* "nnnnn" */
	}
	/* now build the UNSN */
	ptr = buf = malloc(len+1);
	if(!buf) {
		free(hostname);
		free(protoname);
		free(servname);
		errno = ENOMEM;
		return NULL;
	}
	strcpy(ptr, "ip");
	ptr += 2;
	if(usehost) {
		*ptr++ = '=';
		if(hostname) {
			ptr += unsn_encode(ptr, hostname, -1);
		} else {
			strcpy(ptr, addrbuf);
			ptr = strchr(ptr, 0);
		}
	} else if(!(opts & UNSN_USENAMES)) {
		strcpy(ptr, version);
		ptr = strchr(ptr, 0);
	}
	if(useprotocolno) {
		strcpy(ptr, ",protocol=");
		ptr += 10;
		if(protoname) {
			ptr += unsn_encode(ptr, protoname, -1);
		} else {
			sprintf(ptr, "%d", sock_protocol);
			ptr = strchr(ptr, 0);
		}
	}
	if(tlp) {
		*ptr++ = '/';
		strcpy(ptr, tlp);
		ptr = strchr(ptr, 0);
	}
	if(useportno) {
		*ptr++ = '=';
		if(servname) {
			ptr += unsn_encode(ptr, servname, -1);
		} else {
			sprintf(ptr, "%u", (unsigned)ntohs(portno));
			ptr = strchr(ptr, 0);
		}
	}
	free(hostname);
	free(protoname);
	free(servname);
	errno = errno_save;
	return buf;
	eafnosupport:
	errno = EAFNOSUPPORT;
	return NULL;
	esocktnosupport:
	errno = ESOCKTNOSUPPORT;
	return NULL;
	eprotonosupport:
	errno = EPROTONOSUPPORT;
	return NULL;
	einval:
	errno = EINVAL;
	return NULL;
}

# endif /* SUPPORT_SOCK_INET || SUPPORT_SOCK_INET6 */

# ifdef SUPPORT_SOCK_LOCAL

#  include <Compat/sock_local.h>

static inline char *aitounsn_local(int sock_type, int sock_protocol,
	struct sockaddr const *addr, socklen_t addrlen, unsigned opts)
{
	struct sockaddr_un const *sunaddr;
	char const *typestr;
	char *buf, *ptr;
	char const *path;
	size_t pathlen, len;
	if(addr->sa_family != AF_LOCAL)
		goto eafnosupport;
	if(addrlen < offsetof(struct sockaddr_un, sun_path) ||
		addrlen > sizeof(struct sockaddr_un))
		goto einval;
	sunaddr = (struct sockaddr_un *)addr;
	/* sock_protocol is ignored; sock_type needs to be specified */
	switch(sock_type) {
#include <aitounsn.s.ic> /* case SOCK_@TYPE@: typestr = "@type@"; break; */
		default:
			goto esocktnosupport;
	}
	path = sunaddr->sun_path;
	pathlen = addrlen - offsetof(struct sockaddr_un, sun_path);
	/* if path[0] == 0, we assume Linux' `abstract names' */
	if(pathlen && path[0]) {
		char const *p = memchr(path, 0, pathlen);
		if(p)
			pathlen = p - path;
	}
	len = 5; /* "local" */
	if(pathlen)
		len += 1 + /* "=" */
			unsn_encode(NULL, path, pathlen);
	if(typestr)
		len += 6 + /* ",type=" */
			 strlen(typestr);
	ptr = buf = malloc(len+1);
	if(!buf)
		goto enomem;
	strcpy(ptr, "local");
	ptr += 5;
	if(pathlen) {
		*ptr++ = '=';
		ptr += unsn_encode(ptr, path, pathlen);
	}
	if(typestr) {
		strcpy(ptr, ",type=");
		strcpy(ptr+6, typestr);
	}
	return buf;
	eafnosupport:
	errno = EAFNOSUPPORT;
	return NULL;
	esocktnosupport:
	errno = ESOCKTNOSUPPORT;
	return NULL;
	einval:
	errno = EINVAL;
	return NULL;
	enomem:
	errno = ENOMEM;
	return NULL;
}

# endif /* SUPPORT_SOCK_LOCAL */

char *unsn_aitounsn(int family, int type, int protocol,
	struct sockaddr const *addr, socklen_t addrlen, unsigned opts)
{
	switch(family) {
# if defined(SUPPORT_SOCK_INET) || defined(SUPPORT_SOCK_INET6)
#  ifdef SUPPORT_SOCK_INET
		case PF_INET:
#  endif
#  ifdef SUPPORT_SOCK_INET6
		case PF_INET6:
#  endif
			return aitounsn_ip(family, type, protocol,
				addr, addrlen, opts);
# endif /* SUPPORT_SOCK_INET || SUPPORT_SOCK_INET6 */
# ifdef SUPPORT_SOCK_LOCAL
		case PF_LOCAL:
			return aitounsn_local(type, protocol,
				addr, addrlen, opts);
# endif /* SUPPORT_SOCK_LOCAL */
		default:
			errno = EPFNOSUPPORT;
			return NULL;
	}
}

#endif /* SUPPORT_SOCKETS */
