/*
 * Utils/time.c -- getduration()
 * Copyright (C) 2000  Andrew Main
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
#include <Compat/time.h>

int getduration(char const *spec, struct timeval *result)
{
	struct timeval r = { 0, 0 };
	long mult = 60*60*24*366;
	while(*spec) {
		switch(*spec) {
			case 't': case 'T':
				if(mult < 60*60*24)
					return -1;
				mult = 60*60*24 - 1;
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9': {
				int gf;
				long n = 0, m, f = 0;
				do {
					n = n*10 + *spec-'0';
				} while(*++spec >= '0' && *spec <= '9');
				if(gf = (*spec == '.' || *spec == ',')) {
					long fm = 100000;
					spec++;
					while(*spec >= '0' && *spec <= '9') {
						f += (*spec++-'0') * fm;
						fm /= 10;
					}
				}
				switch(*spec++) {
					case 's': case 'S': m=1; break;
					case 'm': case 'M': m=60; break;
					case 'h': case 'H': m=60*60; break;
					case 'd': case 'D': m=60*60*24; break;
					case 'w': case 'W': m=60*60*24*7; break;
					default: return -1;
				}
				if(m >= mult)
					return -1;
				r.tv_sec += n*m;
				if(gf) {
					long sec = r.tv_sec;
					long usec = r.tv_usec;
					usec += (f % 1000) * m;
					f = (f / 1000) * m;
					sec += f / 1000;
					usec += (f % 1000) * 1000;
					r.tv_sec = sec + usec / 1000000;
					r.tv_usec = usec % 1000000;
					mult = 0;
				} else
					mult = m;
				break;
			}
			default:
				return -1;
		}
	}
	*result = r;
	return 0;
}
