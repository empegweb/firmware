/*
 * Utils/uclient.c -- uclient utility
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
#include <libunsn.h>
#include <Compat/string.h>
#include <Compat/errno.h>
#include <Compat/unistd.h>
#include <Compat/stdlib.h>
#include <Compat/getopt.h>
#include <Compat/string.h>
#include <Compat/socket.h>
#include <Compat/signal.h>
#include <Compat/poll.h>
#include <Compat/time.h>
#include <stdio.h>

#define BASE_BUFSIZE (1 << 14)

static char const *progname;

static int exitstatus = 0;

static void usage(void)
{
	fprintf(stderr,
		"%s: usage: %s [-b <local-address>] [-t <time-spec>] [-qsrSR]\n"
		"\tusage: %s [-b <local-address>] [-02sr] <command> [<arg> ...]\n",
		progname, progname, progname);
	exit(2);
}

static void serr(char const *str)
{
	exitstatus = 1;
	fprintf(stderr, "%s: %s%s%s\n",
		progname, str ? str : "", str ? ": " : "",
		unsn_strerror(errno));
}

static void serrexit(char const *str)
{
	serr(str);
	exit(1);
}

struct dgram {
	struct dgram *next, *prev;
	size_t len;
	char data[1];   /* struct hack */
};

#define DGRAM_OVERHEAD (sizeof(struct dgram) + 2*sizeof(void *))

struct shovel {
	int infd, outfd;
	unsigned flags;
	size_t pending;
	union {
		struct {
			struct dgram *head, *tail;
		} dgram;
		struct {
			char *buf;
			union {
				struct {
					size_t bufsize;
				} indgram;
				struct {
					size_t min_maxsize, max_maxsize;
				} outdgram;
			} u;
		} stream;
	} u;
};

#define FLAG_PASSIVE   0x01 /* do passive shutdown */
#define FLAG_INDGRAM   0x02 /* input is datagram-based */
#define FLAG_OUTDGRAM  0x04 /* output is datagram-based */
#define FLAG_COMMDGRAM 0x08 /* all communication is datagram-based */

static int fdisdgram(int fd)
{
	int socktype;
	socklen_t len = sizeof(socktype);
	if(-1 == getsockopt(fd, SOL_SOCKET, SO_TYPE, (char *)&socktype, &len)) {
		if(errno == ENOTSOCK)
			return 0;
		serrexit(NULL);
	}
	return socktype != SOCK_STREAM;
}

static int setup_shovel(struct shovel *sh)
{
	if(fdisdgram(sh->infd))
		sh->flags |= FLAG_INDGRAM;
	if(fdisdgram(sh->outfd))
		sh->flags |= FLAG_OUTDGRAM;
	sh->pending = 0;
	if((sh->flags & (FLAG_INDGRAM|FLAG_OUTDGRAM)) ==
			(FLAG_INDGRAM|FLAG_OUTDGRAM)) {
		sh->flags |= FLAG_COMMDGRAM;
		sh->u.dgram.head = sh->u.dgram.tail = NULL;
	} else {
		if(!(sh->u.stream.buf = malloc(BASE_BUFSIZE))) {
			errno = ENOMEM;
			return -1;
		}
		if(sh->flags & FLAG_INDGRAM) {
			sh->u.stream.u.indgram.bufsize = BASE_BUFSIZE;
		} else if(sh->flags & FLAG_OUTDGRAM) {
			sh->u.stream.u.outdgram.min_maxsize = 1;
			sh->u.stream.u.outdgram.max_maxsize = ((size_t)-1)>>1;
		}
	}
	return 0;
}

static void doshutdown(struct shovel *sh, int dir)
{
	int *fdp = dir == SHUT_WR ? &sh->outfd : &sh->infd;
	int fd = *fdp;
	if(fd == -1)
		return;
	if(!(sh->flags & FLAG_PASSIVE)) {
		try:
		if(-1 == shutdown(fd, dir)) {
			if(errno == EINTR)
				goto try;
			if(errno != ENOTSOCK && errno != ENOTCONN)
				serr("shutdown");
		}
		while(-1 == close(fd) && errno == EINTR)
			;
	}
	*fdp = -1;
	if(dir == SHUT_WR) {
		if(sh->flags & FLAG_COMMDGRAM) {
			struct dgram *p = sh->u.dgram.head;
			while(p) {
				struct dgram *np = p->next;
				free(p);
				p = np;
			}
		} else
			free(sh->u.stream.buf);
	}
}

static int check_shovel(struct shovel *sh)
{
	if(sh->infd == -1 && sh->outfd != -1 && !sh->pending) {
		doshutdown(sh, SHUT_WR);
		return 1;
	} else if(sh->infd != -1 && sh->outfd == -1) {
		doshutdown(sh, SHUT_RD);
		return 1;
	} else
		return sh->infd == -1 && sh->outfd == -1;
}

static inline void shovel_read_stream(struct shovel *sh)
{
	size_t pending = sh->pending;
	char *readbuf = sh->u.stream.buf + pending;
	int infd = sh->infd;
	ssize_t nread;
	if(sh->flags & FLAG_INDGRAM) {
		size_t toread = sh->u.stream.u.indgram.bufsize - pending;
		tryagain_bufsize:
		do {
			nread = recv(infd, readbuf, toread, MSG_PEEK);
		} while(nread == -1 && errno == EINTR);
		if(nread == toread) {
			size_t newbufsize = sh->u.stream.u.indgram.bufsize << 1;
			char *newbuf = realloc(sh->u.stream.buf, newbufsize);
			if(!newbuf) {
				nread = -1;
				errno = ENOMEM;
			} else {
				sh->u.stream.u.indgram.bufsize = newbufsize;
				sh->u.stream.buf = newbuf;
				readbuf = newbuf + pending;
				toread = newbufsize - pending;
				goto tryagain_bufsize;
			}
		} else if(nread != -1) {
			int ret;
			do {
				ret = recv(infd, NULL, 0, 0);
			} while(ret == -1 && errno == EINTR);
			if(ret == -1)
				nread = -1;
		}
	} else {
		size_t toread = BASE_BUFSIZE - pending;
		do {
			nread = read(infd, readbuf, toread);
		} while(nread == -1 && errno == EINTR);
		if(nread == 0) {
			doshutdown(sh, SHUT_RD);
			return;
		}
	}
	if(nread == -1) {
		serr("read");
		doshutdown(sh, SHUT_RD);
	} else {
		sh->pending = pending + nread;
	}
}

static inline void shovel_write_stream(struct shovel *sh)
{
	size_t pending = sh->pending;
	char *buf = sh->u.stream.buf;
	int outfd = sh->outfd;
	ssize_t nwritten;
	if(sh->flags & FLAG_OUTDGRAM) {
		size_t min_maxsize = sh->u.stream.u.outdgram.min_maxsize;
		size_t max_maxsize = sh->u.stream.u.outdgram.max_maxsize;
		size_t towrite;
		if(pending < min_maxsize)
			towrite = pending;
		else if(min_maxsize == max_maxsize)
			towrite = min_maxsize;
		else {
			tryagain_msgsize:
			towrite = (min_maxsize + max_maxsize) >> 1;
			if(towrite > pending)
				towrite = pending;
		}
		do {
			nwritten = send(outfd, buf, towrite, 0);
		} while(nwritten == -1 && errno == EINTR);
		if(nwritten==-1 && errno == EMSGSIZE && towrite > min_maxsize) {
			sh->u.stream.u.outdgram.max_maxsize =
				max_maxsize = towrite - 1;
			goto tryagain_msgsize;
		} else if(nwritten > min_maxsize)
			sh->u.stream.u.outdgram.min_maxsize =
				min_maxsize = nwritten;
	} else {
		do {
			nwritten = write(outfd, buf, pending);
		} while(nwritten == -1 && errno == EINTR);
	}
	if(nwritten == -1) {
		if(errno != EPIPE)
			serr("write");
		doshutdown(sh, SHUT_WR);
	} else {
		pending -= nwritten;
		sh->pending = pending;
		memmove(buf, buf + nwritten, pending);
	}
}

static inline void shovel_read_dgram(struct shovel *sh)
{
	int infd = sh->infd;
	ssize_t nread;
	struct dgram *d = NULL;
	size_t alloclen = sizeof(struct dgram);
	size_t toread;
	do {
		struct dgram *newd = realloc(d, alloclen <<= 2);
		if(!newd) {
			errno = ENOMEM;
			goto err;
		}
		d = newd;
		toread = alloclen - offsetof(struct dgram, data);
		do {
			nread = recv(infd, d->data, toread, MSG_PEEK);
		} while(nread == -1 && errno == EINTR);
	} while(nread == toread);
	if(nread != -1) {
		int ret;
		do {
			ret = recv(infd, NULL, 0, 0);
		} while(ret == -1 && errno == EINTR);
		if(ret == -1)
			nread = -1;
	}
	if(nread == -1) {
		err:
		free(d);
		serr("read");
		doshutdown(sh, SHUT_RD);
	} else {
		size_t newsize = offsetof(struct dgram, data) + nread;
		struct dgram *newd;
		if(newsize < sizeof(struct dgram))
			newsize = sizeof(struct dgram);
		newd = realloc(d, newsize);
		if(newd)
			d = newd;
		d->len = nread;
		d->next = NULL;
		d->prev = sh->u.dgram.tail;
		sh->u.dgram.tail = d;
		if(!sh->u.dgram.head)
			sh->u.dgram.head = d;
		sh->pending += DGRAM_OVERHEAD + nread;
	}
}

static inline void shovel_write_dgram(struct shovel *sh)
{
	int outfd = sh->outfd;
	struct dgram *d = sh->u.dgram.head;
	size_t towrite = d->len;
	ssize_t nwritten;
	do {
		nwritten = send(outfd, d->data, towrite, 0);
	} while(nwritten == -1 && errno == EINTR);
	if(nwritten == -1) {
		if(errno != EPIPE)
			serr("write");
		doshutdown(sh, SHUT_WR);
	} else {
		struct dgram *next = d->next;
		sh->pending -= DGRAM_OVERHEAD + towrite;
		sh->u.dgram.head = next;
		if(next)
			next->prev = NULL;
		else
			sh->u.dgram.tail = NULL;
		free(d);
	}
}

static inline void shovel_read(struct shovel *sh)
{
	if(sh->flags & FLAG_COMMDGRAM)
		shovel_read_dgram(sh);
	else
		shovel_read_stream(sh);
}

static inline void shovel_write(struct shovel *sh)
{
	if(sh->flags & FLAG_COMMDGRAM)
		shovel_write_dgram(sh);
	else
		shovel_write_stream(sh);
}

static void shovel_data(struct shovel *shovels,
	int tiedirections, struct timeval const *timeout)
{
	while(1) {
		int polling[4], *pollingp, pollstatus;
		struct pollfd pollfds[4], *pfd;
		struct shovel *sh;
		int nor = check_shovel(&shovels[0]);
		int nos = check_shovel(&shovels[1]);
		if(tiedirections) {
			if(nor && !nos) {
				doshutdown(&shovels[1], SHUT_RD);
				doshutdown(&shovels[1], SHUT_WR);
				nos = 1;
			} else if(!nor && nos) {
				doshutdown(&shovels[0], SHUT_RD);
				doshutdown(&shovels[0], SHUT_WR);
				nor = 1;
			}
		}
		if(nor && nos)
			return;
		pollingp = polling;
		pfd = pollfds;
		for(sh = shovels; sh != &shovels[2]; sh++) {
			if(*pollingp++ = (sh->infd != -1 &&
						sh->pending < BASE_BUFSIZE)) {
				pfd->fd = sh->infd;
				pfd->events = POLLIN;
				pfd++;
			}
			if(*pollingp++ = (sh->outfd != -1 && sh->pending)) {
				pfd->fd = sh->outfd;
				pfd->events = POLLOUT;
				pfd++;
			}
		}
		do {
			pollstatus = pollto(pollfds, pfd-pollfds, timeout);
		} while(pollstatus == -1 && errno == EINTR);
		if(pollstatus == -1)
			serrexit(NULL);
		if(!pollstatus)
			exit(0);
		pollingp = polling;
		pfd = pollfds;
		for(sh = shovels; sh != &shovels[2]; sh++) {
			if(*pollingp++ && pfd++->revents)
				shovel_read(sh);
			if(*pollingp++ && pfd++->revents)
				shovel_write(sh);
		}
	}
}

static int mode_shovel(int sock,
	int tiedirections, struct timeval const *timeout,
	int rshutdown, int sshutdown,
	int rpassive, int spassive)
{
	struct shovel shovels[2] = {
		{ -1, 1, 0 },
		{ 0, -1, 0 },
	};
	shovels[0].infd = sock;
	if(-1 == (shovels[1].outfd = dup(sock)))
		serrexit(NULL);
	if(rpassive)
		shovels[0].flags |= FLAG_PASSIVE;
	if(spassive)
		shovels[1].flags |= FLAG_PASSIVE;
	if(setup_shovel(&shovels[0]) || setup_shovel(&shovels[1]))
		serrexit(NULL);
	signal(SIGPIPE, SIG_IGN);
	if(rshutdown) {
		doshutdown(&shovels[0], SHUT_RD);
		doshutdown(&shovels[0], SHUT_WR);
	}
	if(sshutdown) {
		doshutdown(&shovels[1], SHUT_RD);
		doshutdown(&shovels[1], SHUT_WR);
	}
	shovel_data(shovels, tiedirections, timeout);
	exit(exitstatus);
}

static int mode_exec(int sock, char **argv,
	int argzero, int dupstderr,
	int rshutdown, int sshutdown)
{
	if((rshutdown || sshutdown) &&
		-1 == shutdown(sock, rshutdown && sshutdown ? SHUT_RDWR :
					rshutdown ? SHUT_RD : SHUT_WR))
		serrexit("shutdown");
	if(-1 == dup2(sock, 0))
		serrexit(NULL);
	if(-1 == dup2(sock, 1))
		serrexit(NULL);
	if(dupstderr && -1 == dup2(sock, 2))
		serrexit(NULL);
	if(sock > 1+dupstderr)
		close(sock);
	execvp(*argv, argv+argzero);
	serrexit("exec");
}

int main(int argc, char **argv)
{
	int cant_shovel = 0, cant_exec = 0;
	int argzero = 0, dupstderr = 0;
	int tiedirections = 0, rshutdown = 0, sshutdown = 0;
	int rpassive = 0, spassive = 0;
	char const *localunsn = NULL, *remoteunsn = NULL;
	struct timeval tv, *timeout = NULL;
	int sock;
	int c;
	progname = basename(argv[0]);
	opterr = 0;
	while((c = getopt(argc, argv, "+02b:qt:rsRS")) != EOF) {
		switch(c) {
			case '0': argzero = 1; cant_shovel = 1; break;
			case '2': dupstderr = 1; cant_shovel = 1; break;
			case 'b': {
				if(localunsn)
					usage();
				localunsn = optarg;
				break;
			}
			case 'q': tiedirections = 1; cant_exec = 1; break;
			case 't': {
				if(timeout || getduration(optarg, &tv))
					usage();
				timeout = &tv;
				cant_exec = 1;
				break;
			}
			case 'r': rshutdown = 1; break;
			case 's': sshutdown = 1; break;
			case 'R': rpassive = 1; cant_exec = 1; break;
			case 'S': spassive = 1; cant_exec = 1; break;
			default: usage();
		}
	}
	argv += optind;
	argc -= optind;
	if(!argc)
		usage();
	remoteunsn = *argv++;
	argc--;
	if(argc ? cant_exec : cant_shovel)
		usage();
	sock = unsn_opensock(localunsn, remoteunsn);
	if(sock == -1)
		serrexit(NULL);
	if(argc)
		return mode_exec(sock, argv, argzero, dupstderr,
			rshutdown, sshutdown);
	else
		return mode_shovel(sock, tiedirections, timeout,
			rshutdown, sshutdown, rpassive, spassive);
}
