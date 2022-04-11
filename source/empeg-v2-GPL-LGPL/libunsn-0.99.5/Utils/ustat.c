/*
 * Utils/ustat.c -- ustat utility
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

#include <libunsn.h>
#include <Compat/errno.h>
#include <Compat/string.h>
#include <stdio.h>
#include <Compat/stdlib.h>
#include <Compat/getopt.h>
#include <Compat/socket.h>

static char const *progname;

static int exitstatus = 0;
static int accept_enotsock = 0, accept_enotconn = 0;
static int unsntype_numeric = 0, unsntype_symbolic = 0;

static void usage(void)
{
	fprintf(stderr, "%s: usage: %s [-0ycs] {-T|-L|-R}\n",
		progname, progname);
	exit(2);
}

static void carp(void)
{
	if(errno == ENOTSOCK && accept_enotsock) {
		fputs("-s", stdout);
	} else if(errno == ENOTCONN && accept_enotconn) {
		fputs("-c", stdout);
	} else {
		fputs("--", stdout);
		unsn_perror(progname);
		exitstatus = 1;
	}
}

static void dotype(void)
{
	int sock_type;
	socklen_t len = sizeof(sock_type);
	if(-1 == getsockopt(0, SOL_SOCKET, SO_TYPE, (char *)&sock_type, &len)) {
		carp();
		putchar('\n');
		return;
	}
	switch(sock_type) {
#include <ustat.s.ic> /* case SOCK_@TYPE@: puts("@TYPE@"); break; */
		default: puts("unknown"); break;
	}
}

static void doaddr(char *(*getunsn)(int, unsigned))
{
	char *unsn;
	if(unsntype_numeric) {
		unsn = getunsn(0, 0);
		if(unsn) {
			fputs(unsn, stdout);
			free(unsn);
		} else
			carp();
		if(unsntype_symbolic)
			putchar(' ');
	}
	if(unsntype_symbolic) {
		unsn = getunsn(0, UNSN_USENAMES);
		if(unsn) {
			fputs(unsn, stdout);
			free(unsn);
		} else
			carp();
	}
	putchar('\n');
}

int main(int argc, char **argv)
{
	int dosocktype = 0, dolocaladdr = 0, doremoteaddr = 0;
	int c;
	progname = basename(argv[0]);
	opterr = 0;
	while((c = getopt(argc, argv, "+TLR0ycs")) != EOF) {
		switch(c) {
			case 'T': dosocktype = 1; break;
			case 'L': dolocaladdr = 1; break;
			case 'R': doremoteaddr = 1; break;
			case '0': unsntype_numeric = 1; break;
			case 'y': unsntype_symbolic = 1; break;
			case 'c': accept_enotconn = 1; break;
			case 's': accept_enotsock = 1; break;
			default: usage();
		}
	}
	if(argc != optind)
		usage();
	if(!dosocktype && !dolocaladdr && !doremoteaddr)
		usage();
	if(!unsntype_numeric && !unsntype_symbolic)
		unsntype_numeric = 1;
	if(dosocktype)
		dotype();
	if(dolocaladdr)
		doaddr(unsn_getsockunsn);
	if(doremoteaddr)
		doaddr(unsn_getpeerunsn);
	exit(exitstatus);
}
