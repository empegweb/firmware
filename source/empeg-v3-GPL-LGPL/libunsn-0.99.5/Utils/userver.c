/*
 * Utils/userver.c -- userver utility
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
#include <Compat/errno.h>
#include <Compat/socket.h>
#include <Compat/string.h>
#include <Compat/signal.h>
#include <Compat/fcntl.h>
#include <Compat/stdlib.h>
#include <stdio.h>
#include <Compat/pwdgrp.h>
#include <Compat/time.h>
#include <Compat/string.h>
#include <Compat/syslog.h>
#include <Compat/getopt.h>
#include <Compat/unistd.h>
#include <Compat/poll.h>
#include <Compat/wait.h>
#include <stdarg.h>

static char const *progname;

static int log_to_syslog = 0, log_to_stderr = 0, errors_to_stderr;
static FILE *logstderr;

#ifndef LOG_PID
# define LOG_PID 0
#endif

#ifndef LOG_NDELAY
# define LOG_NDELAY 0
#endif

#ifndef LOG_DAEMON
# define LOG_DAEMON 0
#endif

#ifndef LOG_ERR
# define LOG_ERR 3
#endif

#ifndef LOG_WARNING
# define LOG_WARNING 4
#endif

#ifndef LOG_NOTICE
# define LOG_NOTICE 5
#endif

#ifndef LOG_INFO
# define LOG_INFO 6
#endif

static void doopenlog(void)
{
#if (defined(HAVE_VSYSLOG) || defined(HAVE_SYSLOG)) && defined(HAVE_OPENLOG)
	if(log_to_syslog) {
		openlog(progname, LOG_PID|LOG_NDELAY, LOG_DAEMON);
	}
#endif /* (HAVE_VSYSLOG || HAVE_SYSLOG) && HAVE_OPENLOG */
}

static void dolog(int priority, char const *fmt, ...)
{
#if defined(HAVE_VSYSLOG) || defined(HAVE_SYSLOG)
	if(log_to_syslog) {
		va_list ap;
# ifdef HAVE_VSYSLOG
		va_start(ap, fmt);
		vsyslog(LOG_DAEMON|priority, fmt, ap);
# else /* !HAVE_VSYSLOG */
		int a[5];
		va_start(ap, fmt);
		a[0] = va_arg(ap, int);
		a[1] = va_arg(ap, int);
		a[2] = va_arg(ap, int);
		a[3] = va_arg(ap, int);
		a[4] = va_arg(ap, int);
		syslog(LOG_DAEMON|priority, fmt, a[0], a[1], a[2], a[3], a[4]);
# endif /* !HAVE_VSYSLOG */
		va_end(ap);
	}
#endif /* HAVE_VSYSLOG || HAVE_SYSLOG */
	if(log_to_stderr || (errors_to_stderr && priority == LOG_ERR)) {
		time_t t;
		char tbuf[sizeof("yyyy-mm-ddThh:mm:ss")];
		va_list ap;
		t = time(NULL);
		strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S",
				localtime(&t));
		fprintf(logstderr, "%s %s[%lu]: ",
			tbuf, progname, (unsigned long)getpid());
		va_start(ap, fmt);
		vfprintf(logstderr, fmt, ap);
		va_end(ap);
		fputc('\n', logstderr);
		fflush(logstderr);
	}
}

static void serrlog(int priority, char const *str)
{
	dolog(priority, "%s%s%s", str ? str : "", str ? ": " : "",
		unsn_strerror(errno));
}

static void serrexit(char const *str)
{
	serrlog(LOG_ERR, str);
	exit(1);
}

static void usage(void)
{
        fprintf(stderr,
		"%s: usage: %s [-dlL1w02] [-p <file>] [-m <maximum>]\n"
		"\t\t\t[-R <directory>] [-U <user-spec>] [-C <directory>]\n"
		"\t\t\t[-t <time-spec>] [-a <token>]\n"
		"\t\t\t<local-address> <command> [<arg> ...]\n",
		progname, progname);
	exit(2);
}

static int fd_valid(int fd)
{
	int tfd = dup(fd);
	if(tfd != -1) {
		close(tfd);
		return 1;
	}
	if(errno != EBADF)
		serrexit("initialization");
	return 0;
}

static void squash_fd(int fd)
{
	int pipes[2];
	if(-1 == pipe(pipes))
		serrexit("initialization");
	close(pipes[1]);
	if(pipes[0] != fd) {
		if(-1 == dup2(pipes[0], fd))
			serrexit("initialization");
		close(pipes[0]);
	}
	if(-1 == fcntl(fd, F_SETFD, FD_CLOEXEC))
		serrexit("initialization");
}

static void daemonize(void)
{
	pid_t pid = fork();
	if(pid == -1)
		serrexit("daemonization");
	if(pid)
		_exit(0);
#ifdef HAVE_SETSID
	setsid();
#endif
}

#if defined(HAVE_SIGPROCMASK) && defined(HAVE_SIGSUSPEND)

typedef sigset_t signal_mask;

static inline void block_sigchld(signal_mask *save)
{
	sigset_t chldmask;
	sigemptyset(&chldmask);
	sigaddset(&chldmask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &chldmask, save);
}

static inline void restore_mask(signal_mask const *save)
{
	sigprocmask(SIG_SETMASK, save, NULL);
}

static inline void suspend_mask(signal_mask const *save)
{
	sigsuspend(save);
}

#elif defined(HAVE_SIGBLOCK) && defined(HAVE_SIGPAUSE)

typedef int signal_mask;

static inline void block_sigchld(signal_mask *save)
{
	*save = sigblock(sigmask(SIGCHLD));
}

static inline void restore_mask(signal_mask const *save)
{
	sigsetmask(*save);
}

static inline void suspend_mask(signal_mask const *save)
{
	sigpause(*save);
}

#else /* no signal blocking */

typedef int signal_mask;

static inline void block_sigchld(signal_mask *save)
{
}

static inline void restore_mask(signal_mask const *save)
{
}

static inline void suspend_mask(signal_mask const *save)
{
}

#endif /* no signal blocking */

static struct child {
	pid_t pida, pidb;
} *children = NULL;
size_t nchildren = 0, maxchildren = (size_t)-1;

static inline void dowaitchildren(signal_mask *oldmaskp)
{
	if(nchildren == maxchildren) {
		dolog(LOG_NOTICE, "reached maximum of %lu concurrent servers, "
				"throttling", (unsigned long)maxchildren);
		while(nchildren == maxchildren)
			suspend_mask(oldmaskp);
	}
}

static inline void dodelchild(pid_t pid)
{
	struct child *c;
	for(c = children+nchildren; c-- != children; ) {
		if(c->pida == pid)
			c->pida = 0;
		else if(c->pidb == pid)
			c->pidb = 0;
		else
			continue;
		if(!c->pida && !c->pidb)
			*c = children[--nchildren];
		return;
	}
}

static RETSIGTYPE sigchld_reap(int signum)
{
	pid_t pid;
# ifdef wait_any_flags
	while((pid = wait_any_flags(NULL, WNOHANG)) > 0)
# else /* !wait_any_flags */
	if((pid = wait_any(NULL)) > 0)
# endif /* !wait_any_flags */
		if(children)
			dodelchild(pid);
}

static RETSIGTYPE sig_term(int signum)
{
	dolog(LOG_NOTICE, "terminating on signal %d", signum);
	exit(0);
}

typedef RETSIGTYPE (*sig_handler)(int);

#if defined(HAVE_STRUCT_SIGACTION) && defined(HAVE_SIGACTION)

typedef struct sigaction sighandler_save;

static void setup_signal(int signum, sig_handler hnd, sighandler_save *save)
{
	struct sigaction sa;
	sa.sa_handler = hnd;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP|SA_RESTART;
	sigaction(signum, &sa, save);
}

static void restore_signal(int signum, sighandler_save const *save)
{
	sigaction(signum, save, NULL);
}

#else /* !HAVE_STRUCT_SIGACTION || !HAVE_SIGACTION */

typedef sig_handler sighandler_save;

static void setup_signal(int signum, sig_handler hnd, sighandler_save *save)
{
	*save = signal(signum, hnd);
}

static void restore_signal(int signum, sighandler_save const *save)
{
	signal(signum, *save);
}

#endif /* !HAVE_STRUCT_SIGACTION || !HAVE_SIGACTION */

static sighandler_save old_sigchld, old_sigint, old_sigterm;

static void setup_sighand(int dosigchld, sig_handler sig_chld)
{
	if(dosigchld)
		setup_signal(SIGCHLD, sig_chld, &old_sigchld);
	setup_signal(SIGINT, sig_term, &old_sigint);
	setup_signal(SIGTERM, sig_term, &old_sigterm);
}

static void restore_sighand(int dosigchld)
{
	if(dosigchld)
		restore_signal(SIGCHLD, &old_sigchld);
	restore_signal(SIGINT, &old_sigint);
	restore_signal(SIGTERM, &old_sigterm);
}

static void write_pidfile(char const *pidfile)
{
	FILE *pf;
	if(!(pf = fopen(pidfile, "w"))) {
		serrlog(LOG_WARNING, "writing PID file");
		return;
	}
	fprintf(pf, "%lu\n", (unsigned long)getpid());
	if(fclose(pf))
		serrlog(LOG_WARNING, "writing PID file");
}

static inline int setallgids(gid_t gid)
{
#ifdef HAVE_SETRESGID
	return setresgid(gid, gid, gid);
#elif defined(HAVE_SETREGID)
	return setregid(gid, gid);
#else /* !HAVE_SETRESGID && !HAVE_SETREGID */
	return setgid(gid);
#endif /* !HAVE_SETRESGID && !HAVE_SETREGID */
}

static inline int setalluids(uid_t uid)
{
#ifdef HAVE_SETRESUID
	return setresuid(uid, uid, uid);
#elif defined(HAVE_SETREUID)
	return setreuid(uid, uid);
#else /* !HAVE_SETRESUID && !HAVE_SETREUID */
	return setuid(uid);
#endif /* !HAVE_SETRESUID && !HAVE_SETREUID */
}

static void do_setid(char const *userspec)
{
	char *username, *groupname;
	char const *p = userspec;
	struct passwd *pw;
	struct group *gr;
	if(*p && *p != ':') {
		size_t ulen = strcspn(p, ":");
		username = malloc(ulen + 1);
		if(!username) {
			errno = ENOMEM;
			serrexit("setid");
		}
		memcpy(username, p, ulen);
		username[ulen] = 0;
		p += ulen;
	} else
		username = NULL;
	if(*p == ':')
		p++;
	if(*p && *p != ':') {
		size_t glen = strcspn(p, ":");
		groupname = malloc(glen + 1);
		if(!groupname) {
			errno = ENOMEM;
			serrexit("setid");
		}
		memcpy(groupname, p, glen);
		groupname[glen] = 0;
		p += glen;
	} else
		groupname = NULL;
	if(*p) {
		dolog(LOG_ERR, "setid: malformed ID specification");
		exit(1);
	}
	if(!username && !groupname)
		return;
	if(!username)
		pw = NULL;
	else if(!(pw = getpwnam(username))) {
		dolog(LOG_ERR, "setid: unknown user");
		exit(1);
	}
	if(!groupname)
		gr = NULL;
	else if(!(gr = getgrnam(groupname))) {
		dolog(LOG_ERR, "setid: unknown group");
		exit(1);
	}
	{
		gid_t primary_gid = gr ? gr->gr_gid : pw->pw_gid;
		if(-1 == setallgids(primary_gid))
			serrexit("setid");
		if(-1 == (pw ? initgroups(pw->pw_name, primary_gid) :
				setgroups(1, &primary_gid)))
			serrexit("setid");
	}
	if(pw && -1 == setalluids(pw->pw_uid))
		serrexit("setid");
	free(username);
	free(groupname);
}

static pid_t myfork(void)
{
	pid_t pid = fork();
	if(pid == -1)
		serrlog(LOG_ERR, "fork");
	return pid;
}

static int connection_oriented;
static int mainsock;
static struct unsn_sockaddrinfo mainsock_addr;

static inline void setup_mainsock(char const *specified_unsn)
{
	int err;
	char *locunsn;
	mainsock = unsn_opensock_gethints(specified_unsn, NULL, &mainsock_addr);
	if(mainsock == -1)
		serrexit("opening socket");
	if(-1 == unsn_getsockaddr(mainsock, &mainsock_addr))
		serrexit("unsn_getsockaddr");
	do {
		err = listen(mainsock, SOMAXCONN);
	} while(err == -1 && errno == EINTR);
	if(err == -1) {
		if(errno != EOPNOTSUPP)
			serrexit("listen");
		connection_oriented = 0;
	} else
		connection_oriented = 1;
	locunsn = unsn_saitounsn(&mainsock_addr, 0);
	if(!locunsn)
		serrexit("unsn_saitounsn");
	dolog(LOG_NOTICE, "listening for %s on %s",
		connection_oriented ? "connections" : "packets", locunsn);
	free(locunsn);
}

static int accept_conn(char **remunsnp, pid_t *extrapidp,
	int *infdp, int *outfdp)
{
	int connsock;
	char *remunsn;
	do {
		connsock = accept(mainsock, NULL, NULL);
	} while(connsock == -1 && errno != EINTR);
	if(connsock == -1) {
		serrlog(LOG_WARNING, "accept");
		return -1;
	}
	remunsn = unsn_getpeerunsn_withhints(connsock, &mainsock_addr, 0);
	if(!remunsn) {
		serrlog(LOG_ERR, "unsn_getpeerunsn_withhints");
		dolog(LOG_INFO, "dropping connection");
		close(connsock);
		return -1;
	}
	dolog(LOG_INFO, "got connection from %s", remunsn);
	*remunsnp = remunsn;
	*outfdp = *infdp = connsock;
	return 0;
}

static ssize_t intr_recvfrom(int sock, char *buf, size_t buflen,
	int flags, struct sockaddr *addr, socklen_t *addrlen)
{
	ssize_t ret;
	do {
		ret = recvfrom(sock, buf, buflen, flags, addr, addrlen);
	} while(ret == -1 && errno == EINTR);
	return ret;
}

static int recv_packet(int sock, char **ret_data, size_t *ret_datalen)
{
	size_t buflen = 256>>2;
	char *buf = NULL;
	ssize_t len;
	do {
		char *obuf = buf;
		buf = realloc(buf, buflen <<= 2);
		if(!buf) {
			free(obuf);
			errno = ENOMEM;
			return -1;
		}
		len = intr_recvfrom(sock, buf, buflen, MSG_PEEK, NULL, NULL);
	} while(len == buflen);
	if(-1 == intr_recvfrom(sock, NULL, 0, 0, NULL, NULL) || len == -1) {
		free(buf);
		return -1;
	}
	*ret_data = buf;
	*ret_datalen = len;
	return 0;
}

static int packet_conn(char **remunsnp, pid_t *extrapidp,
	int *infdp, int *outfdp)
{
	struct sockaddr sa;
	struct unsn_sockaddrinfo sai;
	char *remunsn;
	char *pkt;
	size_t pktlen;
	int inpipe[2], outpipe[2];
	pid_t forkresult;
	sai = mainsock_addr;
	sai.sai_addrlen = 0;
	if(-1 == intr_recvfrom(mainsock, NULL, 0,
				MSG_PEEK, &sa, &sai.sai_addrlen)) {
		serrlog(LOG_WARNING, "recv(peek)");
		return -1;
	}
	if(sai.sai_addrlen <= sizeof(sa)) {
		sai.sai_addr = &sa;
	} else {
		sai.sai_addr = malloc(sai.sai_addrlen);
		if(!sai.sai_addr) {
			errno = ENOMEM;
			serrlog(LOG_WARNING, "recv(peek)");
			dolog(LOG_INFO, "dropping packet");
			intr_recvfrom(mainsock, NULL, 0, 0, NULL, NULL);
			return -1;
		}
		if(-1 == intr_recvfrom(mainsock, NULL, 0, MSG_PEEK,
					sai.sai_addr, &sai.sai_addrlen)) {
			serrlog(LOG_WARNING, "recv(peek)");
			free(sai.sai_addr);
			return -1;
		}
	}
	remunsn = unsn_saitounsn(&sai, 0);
	if(!remunsn) {
		if(sai.sai_addr != &sa)
			free(sai.sai_addr);
		serrlog(LOG_ERR, "unsn_saitounsn");
		dolog(LOG_INFO, "dropping packet");
		intr_recvfrom(mainsock, NULL, 0, 0, NULL, NULL);
		return -1;
	}
	dolog(LOG_INFO, "got packet from %s", remunsn);
	if(-1 == recv_packet(mainsock, &pkt, &pktlen)) {
		free(remunsn);
		if(sai.sai_addr != &sa)
			free(sai.sai_addr);
		serrlog(LOG_WARNING, "recv");
		return -1;
	}
	if(-1 == pipe(inpipe)) {
		pipe_err:
		free(remunsn);
		if(sai.sai_addr != &sa)
			free(sai.sai_addr);
		serrlog(LOG_ERR, "creating local pipe");
		dolog(LOG_INFO, "dropping packet");
		free(pkt);
		return -1;
	}
	if(-1 == pipe(outpipe)) {
		close(inpipe[0]);
		close(inpipe[1]);
		goto pipe_err;
	}
	forkresult = myfork();
	if(!forkresult) {
		int incoming_writing = 1, outgoing_reading = 1;
		int outgoing_send = 0;
		char *opkt = NULL;
		size_t pktoff = 0, opktlen = 0, opktallocated = 256>>2;
		close(inpipe[0]);
		close(outpipe[1]);
		dolog(LOG_INFO, "relaying packets%s%s", remunsn ? " for " : "",
			remunsn ? remunsn : "");
		do {
			struct pollfd pfds[2];
			if(incoming_writing) {
				pfds[0].fd = inpipe[1];
				pfds[0].events = POLLOUT;
			}
			if(outgoing_reading) {
				pfds[1].fd = outpipe[0];
				pfds[1].events = POLLIN;
			} else if(outgoing_send) {
				pfds[1].fd = mainsock;
				pfds[1].events = POLLOUT;
			}
			pollagain:
			if(-1 == poll(pfds+!incoming_writing,
					incoming_writing+(outgoing_reading||
						outgoing_send), -1)) {
				if(errno == EINTR)
					goto pollagain;
				serrexit("datagram relay");
			}
			if(incoming_writing && pfds[0].revents) {
				ssize_t nwritten;
				do {
					nwritten = write(inpipe[1], pkt+pktoff,
							pktlen-pktoff);
				} while(nwritten == -1 && errno == EINTR);
				if(nwritten == -1) {
					if(errno != EPIPE)
						serrexit("datagram relay");
					goto inwrite_done;
				} else if((pktoff+=nwritten) == pktlen) {
					inwrite_done:
					close(inpipe[1]);
					free(pkt);
					incoming_writing = 0;
				}
			}
			if(outgoing_reading && pfds[1].revents) {
				ssize_t nread;
				if(opktallocated < (opktlen<<1) || !opkt) {
					opktallocated <<= 2;
					opkt = realloc(opkt, opktallocated);
					if(!opkt)
						serrexit("datagram relay");
				}
				do {
					nread = read(outpipe[0], opkt+opktlen,
							opktallocated-opktlen);
				} while(nread == -1 && errno == EINTR);
				if(nread == -1) {
					serrexit("datagram relay");
				} else if(!nread) {
					close(outpipe[0]);
					outgoing_reading = 0;
					outgoing_send = 1;
				} else
					opktlen += nread;
			} else if(outgoing_send && pfds[1].revents) {
				int ret;
				do {
					ret = sendto(mainsock, opkt, opktlen, 0,
						sai.sai_addr, sai.sai_addrlen);
				} while(ret == -1 && errno == EINTR);
				if(ret == -1)
					serrexit("datagram relay");
				close(mainsock);
				free(opkt);
				outgoing_send = 0;
			}
		} while(incoming_writing || outgoing_reading || outgoing_send);
		exit(0);
	}
	close(inpipe[1]);
	close(outpipe[0]);
	free(pkt);
	if(sai.sai_addr != &sa)
		free(sai.sai_addr);
	if(forkresult == -1) {
		serrlog(LOG_ERR, "creating local pipe");
		dolog(LOG_INFO, "dropping packet");
		free(remunsn);
		close(inpipe[0]);
		close(outpipe[1]);
		return -1;
	}
	*remunsnp = remunsn;
	*extrapidp = forkresult;
	*infdp = inpipe[0];
	*outfdp = outpipe[1];
	return 0;
}

static int do_exec(int conn_in, int conn_out, char const *remunsn,
	char **argv, int argzero, int dupstderr)
{
	if(-1 == dup2(conn_in, 0))
		serrexit("pre-exec");
	if(-1 == dup2(conn_out, 1))
		serrexit("pre-exec");
	if(dupstderr == -1)
		close(2);
	else if(dupstderr && -1 == dup2(1, 2))
		serrexit("pre-exec");
	close(conn_in);
	if(conn_out != conn_in)
		close(conn_out);
	dolog(LOG_INFO, "executing %s%s%s", *argv,
		remunsn ? " for " : "", remunsn ? remunsn : "");
	execvp(*argv, argv+argzero);
	serrexit("exec");
}

static inline int intr_chroot(char const *pathname)
{
	int ret;
	do {
		ret = chroot(pathname);
	} while(ret == -1 && errno == EINTR);
	return ret;
}

static inline int intr_chdir(char const *pathname)
{
	int ret;
	do {
		ret = chdir(pathname);
	} while(ret == -1 && errno == EINTR);
	return ret;
}

int main(int argc, char **argv)
{
	int singleshot = 0, will_daemonize = 1, waitmode = 0;
	int argzero = 0, dupstderr = 0;
	char const *localunsn = NULL;
	char const *chroot_to = NULL, *setid_to = NULL, *chdir_to = NULL;
	struct timeval tv, *timeout = NULL;
	char const *pidfile = NULL, *remunsn_token = NULL;
	int c;
	progname = basename(argv[0]);
	opterr = 0;
        while((c = getopt(argc, argv, "+p:m:R:U:C:dlLt:1w02a:")) != EOF) {
                switch(c) {
			case 'p': {
				if(pidfile)
					usage();
				pidfile = optarg;
				break;
			}
			case 'm': {
				unsigned long m;
				char *p;
				if(maxchildren != (size_t)-1)
					usage();
				errno = 0;
				m = strtoul(optarg, &p, 10);
				if(!*optarg || *p || errno || m >= (size_t)-1)
					usage();
				maxchildren = m;
				break;
			}
			case 'R': {
				if(chroot_to)
					usage();
				chroot_to = optarg;
				break;
			}
			case 'U': {
				if(setid_to)
					usage();
				setid_to = optarg;
				break;
			}
			case 'C': {
				if(chdir_to)
					usage();
				chdir_to = optarg;
				break;
			}
			case 'd': will_daemonize = 0; break;
			case 'l': log_to_syslog = 1; break;
			case 'L': log_to_stderr = 1; break;
			case 't': {
				if(timeout || getduration(optarg, &tv))
					usage();
				timeout = &tv;
				break;
			}
			case '1': singleshot = 1; break;
                        case 'w': waitmode = 1; break;
                        case '0': argzero = 1; break;
                        case '2': dupstderr = 1; break;
			case 'a': {
				if(remunsn_token)
					usage();
				remunsn_token = optarg;
				break;
			}
                        default: usage();
                }
        }
	if(waitmode && remunsn_token)
		usage();
	if(maxchildren != (size_t)-1 && (singleshot || waitmode))
		usage();
	errors_to_stderr = !log_to_syslog && !log_to_stderr;
	argv += optind;
	argc -= optind;
	if(argc < 2)
		usage();
	localunsn = *argv++;
	argc--;
	/* don't need these file descriptors any more */
	squash_fd(0);
	squash_fd(1);
	if(log_to_stderr || errors_to_stderr) {
		int stderrcopy = dup(2);
		logstderr = stderr;
		if(stderrcopy == -1) {
			if(errno != EBADF)
				serrexit("initialization");
			log_to_stderr = errors_to_stderr = 0;
		} else {
			FILE *log = fdopen(stderrcopy, "w");
			if(!log || -1 == fcntl(stderrcopy, F_SETFD, FD_CLOEXEC))
				serrexit("initialization");
			logstderr = log;
		}
	}
	if(dupstderr || !fd_valid(2))
		squash_fd(2);
	/* daemonize */
	doopenlog();
	if(will_daemonize)
		daemonize();
	/* set up signal handling */
	if(maxchildren != (size_t)-1) {
		children = malloc(maxchildren * sizeof(*children));
		if(!children) {
			errno = ENOMEM;
			serrexit(NULL);
		}
	}
	setup_sighand(!singleshot, waitmode ? SIG_DFL : sigchld_reap);
	/* set up socket */
	dolog(LOG_INFO, "with address %s", localunsn);
	setup_mainsock(localunsn);
	if(pidfile)
		write_pidfile(pidfile);
	/* change location */
	if(chroot_to && -1 == intr_chroot(chroot_to))
		serrexit("chroot");
	if(setid_to)
		do_setid(setid_to);
	if(chdir_to && -1 == intr_chdir(chdir_to))
		serrexit("chdir");
	/* main loop */
	while(1) {
		int pollstatus;
		signal_mask oldmask;
		struct child ch;
		int conn_in, conn_out;
		char *remunsn;
		do {
			struct pollfd pfd;
			pfd.fd = mainsock;
			pfd.events = POLLIN;
			pollstatus = pollto(&pfd, 1, timeout);
		} while(pollstatus == -1 && errno == EINTR);
		if(pollstatus == -1) {
			serrlog(LOG_ERR, "poll");
			continue;
		}
		if(!pollstatus) {
			dolog(LOG_NOTICE, "terminating due to timeout");
			exit(0);
		}
		if(waitmode) {
			pid_t child = 0;
			dolog(LOG_INFO, "got input");
			if(singleshot || !(child = myfork())) {
				restore_sighand(!singleshot);
				do_exec(mainsock, mainsock, NULL,
					argv, argzero, dupstderr);
			}
			if(child == -1)
				continue;
			while(
# ifdef wait_specific
				wait_specific(child, NULL)
# else /* !wait_specific */
				wait_any(NULL)
# endif /* !wait_specific */
				!= child)
				;
			dolog(LOG_INFO, "%s terminated", *argv);
			continue;
		}
		if(children) {
			block_sigchld(&oldmask);
			dowaitchildren(&oldmask);
		}
		ch.pidb = 0;
		if(-1 == (connection_oriented ? accept_conn : packet_conn)
				(&remunsn, &ch.pidb, &conn_in, &conn_out)) {
			if(children)
				restore_mask(&oldmask);
			continue;
		}
		if(singleshot || !(ch.pida = myfork())) {
			restore_sighand(!singleshot);
			if(children)
				restore_mask(&oldmask);
			close(mainsock);
			if(remunsn_token) {
				char **p;
				for(p = argv; *p; p++)
					if(!strcmp(*p, remunsn_token))
						*p = remunsn;
			}
			do_exec(conn_in, conn_out, remunsn,
				argv, argzero, dupstderr);
		}
		if(children) {
			if(ch.pida != -1)
				children[nchildren++] = ch;
			else if(ch.pidb) {
				ch.pida = 0;
				children[nchildren++] = ch;
			}
			restore_mask(&oldmask);
		}
		close(conn_in);
		if(conn_out != conn_in)
			close(conn_out);
		free(remunsn);
	}
}
