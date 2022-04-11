/*
 * Lib/guessaddrt.c -- unsn_guessaddrtype()
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

#include <Compat/string.h>

unsigned unsn_guessaddrtype(char const *str, unsigned allowed)
{
	switch(allowed) {
		case UNSN_ADDRTYPE_UNSN:
		case UNSN_ADDRTYPE_HOSTNAME:
		case UNSN_ADDRTYPE_PATHNAME:
			return allowed;
		case (UNSN_ADDRTYPE_HOSTNAME|UNSN_ADDRTYPE_PATHNAME):
			return strchr(str, '/') ? UNSN_ADDRTYPE_PATHNAME :
							UNSN_ADDRTYPE_HOSTNAME;
		case (UNSN_ADDRTYPE_UNSN|UNSN_ADDRTYPE_PATHNAME):
		case (UNSN_ADDRTYPE_UNSN|UNSN_ADDRTYPE_HOSTNAME|
						UNSN_ADDRTYPE_PATHNAME):
		case (UNSN_ADDRTYPE_UNSN|UNSN_ADDRTYPE_HOSTNAME):
			if((allowed & UNSN_ADDRTYPE_PATHNAME) &&
			   (str[0] == '/' ||
			    (str[0] == '.' &&
			     (str[1] == '/' ||
			      (str[1] == '.' && str[2] == '/')))))
				return UNSN_ADDRTYPE_PATHNAME;
			if(!(allowed & UNSN_ADDRTYPE_HOSTNAME) ||
				strchr(str, '/') || strchr(str, '=') ||
				strchr(str, '('))
				return UNSN_ADDRTYPE_UNSN;
			return UNSN_ADDRTYPE_HOSTNAME;
		default:
			return 0;
	}
}
