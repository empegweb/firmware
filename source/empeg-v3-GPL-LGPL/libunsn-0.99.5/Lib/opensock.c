/*
 * Lib/opensock.c -- unsn_opensock()
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
# include <Compat/unistd.h>

static inline int intr_socket(int family, int type, int protocol)
{
	int sock, err;
	do {
		sock = socket(family, type, protocol);
	} while(sock == -1 && errno == EINTR);
	do {
		int val = 1;
		err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&val, sizeof(val));
	} while(err == -1 && errno == EINTR);
	return sock;
}

static inline int intr_bc(int (*bc)(int, struct sockaddr const *, socklen_t),
	int sock, struct sockaddr const *addr, socklen_t addrlen)
{
	int ret;
	do {
		ret = bc(sock, addr, addrlen);
	} while(ret == -1 && errno == EINTR);
	return ret;
}

static int open_oneaddr(char const *unsn,
	int (*bindconnect)(int, struct sockaddr const *, socklen_t),
	struct unsn_sockaddrinfo *hints)
{
	int errno_save = errno;
	int err = 0;
	int sock = -1;
	struct unsn_sockaddrinfo const *sai;
	struct unsn_sai_iterator *iter;
	if(!(iter = unsn_mksaiiterator(unsn)))
		return -1;
	while(sai = unsn_sai_iterator_advance(iter)) {
		int nerr;
		if(!sai->sai_addr) {
			nerr = sai->sai_family;
		} else {
			sock = intr_socket(sai->sai_family, sai->sai_type,
				sai->sai_protocol);
			if(sock != -1) {
				if(-1 != intr_bc(bindconnect, sock,
					sai->sai_addr, sai->sai_addrlen)) {
					if(hints) {
						hints->sai_family =
							sai->sai_family;
						hints->sai_type = sai->sai_type;
						hints->sai_protocol =
							sai->sai_protocol;
					}
					err = 0;
					break;
				}
				close(sock);
			}
			nerr = errno;
		}
		err = err ? unsn_errno_max(err, nerr) : nerr;
	}
	unsn_sai_iterator_free(iter);
	if(err) {
		errno = err;
		return -1;
	}
	errno = errno_save;
	return sock;
}

static int open_tryconnect(int sock, struct unsn_sockaddrinfo const *bound,
	struct unsn_sai_iterator *remote_iter)
{
	struct unsn_sockaddrinfo const *sai;
	int err = 0;
	while(sai = unsn_sai_iterator_advance(remote_iter)) {
		int nerr;
		if(!sai->sai_addr) {
			nerr = sai->sai_family;
		} else if(sai->sai_family != bound->sai_family ||
				sai->sai_type != bound->sai_type ||
				sai->sai_protocol != bound->sai_protocol) {
			nerr = UNSN_EADDRINCOMPAT;
		} else {
			if(-1 != intr_bc(connect, sock, sai->sai_addr,
					sai->sai_addrlen)) {
				err = 0;
				break;
			}
			nerr = errno;
		}
		err = err ? unsn_errno_max(err, nerr) : nerr;
	}
	return err;
}

static int open_twoaddr(char const *local_unsn, char const *remote_unsn,
	struct unsn_sockaddrinfo *hints)
{
	int errno_save = errno;
	int err = 0;
	int sock = -1;
	struct unsn_sockaddrinfo const *sai;
	struct unsn_sai_iterator *local_iter, *remote_iter;
	if(!(local_iter = unsn_mksaiiterator(local_unsn)))
		goto out;
	if(!(remote_iter = unsn_mksaiiterator(remote_unsn)))
		goto out_freelocal;
	while(sai = unsn_sai_iterator_advance(local_iter)) {
		int nerr;
		if(!sai->sai_addr) {
			nerr = sai->sai_family;
		} else {
			sock = intr_socket(sai->sai_family, sai->sai_type,
				sai->sai_protocol);
			if(sock == -1) {
				nerr = errno;
			} else {
				if(-1 == intr_bc(bind, sock, sai->sai_addr,
							sai->sai_addrlen)) {
					nerr = errno;
				} else {
					nerr = open_tryconnect(sock, sai,
							remote_iter);
					if(!nerr) {
						if(hints) {
							hints->sai_family =
								sai->sai_family;
							hints->sai_type =
								sai->sai_type;
							hints->sai_protocol =
								sai->
								sai_protocol;
						}
						err = 0;
						break;
					}
				}
				close(sock);
			}
		}
		err = err ? unsn_errno_max(err, nerr) : nerr;
	}
	if(err) {
		errno = err;
		sock = -1;
	} else
		errno = errno_save;
	unsn_sai_iterator_free(remote_iter);
	out_freelocal:
	unsn_sai_iterator_free(local_iter);
	out:
	return sock;
}

int unsn_opensock_gethints(char const *local_unsn,
	char const *remote_unsn,
	struct unsn_sockaddrinfo *hints_ret)
{
	if(!local_unsn) {
		if(!remote_unsn) {
			errno = EINVAL;
			return -1;
		} else
			return open_oneaddr(remote_unsn, connect, hints_ret);
	} else {
		if(!remote_unsn)
			return open_oneaddr(local_unsn, bind, hints_ret);
		else
			return open_twoaddr(local_unsn, remote_unsn, hints_ret);
	}
}

int unsn_opensock(char const *local_unsn, char const *remote_unsn)
{
	return unsn_opensock_gethints(local_unsn, remote_unsn, NULL);
}

#endif /* SUPPORT_SOCKETS */
