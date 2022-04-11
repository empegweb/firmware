/*
 * Utils/pollto.c -- pollto()
 * Copyright (C) 1996, 2000  Andrew Main
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <utils.h>
#include <Compat/poll.h>
#include <Compat/time.h>

int pollto(struct pollfd *ufds, unsigned nfds,
	struct timeval const *timeout)
{
	int tmo, pollstatus;
	struct timeval timeleft;
	if(!timeout)
		return poll(ufds, nfds, -1);
	timeleft = *timeout;
	do {
		if(timeleft.tv_sec > 60*60*24)
			tmo = 1000*60*60*24;
		else
			tmo = 1000*timeleft.tv_sec +
				(timeleft.tv_usec+999)/1000;
		pollstatus = poll(ufds, nfds, tmo);
	} while(!pollstatus && (timeleft.tv_sec -= tmo/1000));
	return pollstatus;
}

