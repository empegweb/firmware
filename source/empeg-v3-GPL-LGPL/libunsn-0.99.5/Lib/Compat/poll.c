/*
 * Lib/Compat/poll.c -- poll()
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

#include <Compat/poll.h>

#ifdef NEED_COMPAT_POLL

# include <Compat/errno.h>
# include <Compat/time.h>

# ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
# endif

int poll(struct pollfd *ufds, unsigned nfds, int timeout)
{
	struct pollfd *end_ufds = ufds+nfds, *p;
	fd_set inset, outset, errset;
	int maxfd = -1, ret = 0;
	struct timeval tv;
	FD_ZERO(&inset);
	FD_ZERO(&outset);
	FD_ZERO(&errset);
	for(p = ufds; p != end_ufds; p++) {
		int fd = p->fd;
		if(fd < 0) {
			fd->revents = POLLNVAL;
			ret++;
		} else if(fd >= FD_SETSIZE) {
			errno = ENOSYS;
			return -1;
		} else {
			int events = fd->events;
			if(fd > maxfd)
				maxfd = fd;
			fd->revents = 0;
			if(events & POLLIN)
				FD_SET(fd, &inset);
			if(events & POLLOUT)
				FD_SET(fd, &outset);
			FD_SET(fd, &errset);
		}
	}
	if(ret)
		return ret;
	if(timeval >= 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
	}
	if(-1 == select(maxfd+1, &inset, &outset, &errset,
			timeval<0 ? NULL : &tv))
		return -1;
	for(p = ufds; p != end_ufds; p++) {
		int fd = p->fd;
		int revents = 0;
		if(FD_ISSET(fd, &inset))
			revents |= POLLIN|POLLPRI;
		if(FD_ISSET(fd, &outset))
			revents |= POLLOUT;
		if(FD_ISSET(fd, &errset))
			revents |= POLLERR|POLLHUP;
		revents &= p->events | POLLERR|POLLHUP;
		if(revents) {
			fd->revents = revents;
			ret++;
		}
	}
	return ret;
}

#endif /* NEED_COMPAT_POLL */
