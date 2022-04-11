/*
 * Lib/Compat/poll.h -- system compatibility hacks -- poll()
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

#ifndef UNSN_COMPAT_POLL_H
#define UNSN_COMPAT_POLL_H 1

#include <Compat/base.h>

/* system headers */

#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#if defined(HAVE_POLL) && defined(POLLIN)

/* prototype */

# ifndef HAVE_PROTOTYPE_POLL
int poll(struct pollfd *, unsigned, int);
# endif

#else /* !HAVE_POLL || !POLLIN */

/* compatibility implementation */

# undef HAVE_POLL

struct pollfd {
	int fd;
	short events;
	short revents;
};

# define POLLIN   0x01
# define POLLPRI  0x02
# define POLLOUT  0x04
# define POLLERR  0x10
# define POLLHUP  0x20
# define POLLNVAL 0x40

# define poll unsn_compat_poll
int poll(struct pollfd * /*ufds*/, unsigned /*nfds*/, int /*timeout*/);

# define NEED_COMPAT_POLL 1
# define HAVE_POLL 1

#endif /* !HAVE_POLL || !POLLIN */

#endif
