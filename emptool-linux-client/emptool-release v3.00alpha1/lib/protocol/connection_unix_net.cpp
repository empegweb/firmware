/* connection_unix_net.cpp
 *
 * Unix-specific code in support of connecting to players over net
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"

#ifndef ECOS

#include "connection.h"
#include <unistd.h>
#include "time_calc.h"
#include "protocol_errors.h"

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include "net/ipaddress.h"
#include "net/interface.h"
#include <net/if.h>
#include <stdio.h>
#include <sys/time.h>

#ifndef MSG_DONTWAIT
// some glibcs don't have this defined
enum {
    MSG_DONTWAIT		= 0x40
};
#define MSG_DONTWAIT	MSG_DONTWAIT
#endif

#define TCP_SEND_TIMEOUT	5000	// 5 seconds

// OutSocketConnection

OutSocketConnection::OutSocketConnection()
    : fd(-1)
{
}

OutSocketConnection::~OutSocketConnection()
{
    Close();
}

void OutSocketConnection::Close()
{
    if (fd >= 0)
    {
	close(fd);
	fd = -1;
    }
}

/** Send a block of data
 *
 * Unlike unix calls, this write will only return when either the entire buffer
 * has been sent or an error occurrs. A partial write will cause a retry.
 */
STATUS OutSocketConnection::Send(const unsigned char *buffer, int length)
{
    // Write entire data packet: retry as necessary. We only return
    // when the entire buffer has been sent or there has been an error
    int done = 0, r;

    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;

    timeval giveup_tv;
    TimeCalc::At(&giveup_tv, TCP_SEND_TIMEOUT);

    while(done < length)
    {
	int remain = TimeCalc::Remaining(&giveup_tv);
	if(remain < 0)
	    remain = 0;

	pfd.revents = 0;
	int n = poll(&pfd, 1, remain);
	if(n < 0)	// error on socket
	{
	    if(errno == EINTR) continue;
	    return MakeErrnoStatus();
	}

	if(!n)
	{
	    // nothing available over requested time
	    return MakeErrnoStatus(ETIMEDOUT);
	}

	if(pfd.revents & (POLLERR|POLLHUP))
	    return MakeErrnoStatus(EIO);

	// data available
	if ((r = write(fd, buffer + done, length - done)) <= 0)
	    return MakeErrnoStatus();

	done += r;
    }

    // Done OK
    return S_OK;
}

/** Get data from remote end with timeout
 *
 * if timeout>0 , it indicates a timeout in milliseconds to wait for data
 * if timeout==0, it indicates that we should read only if we can read
 *                without delay (non-blocking read, basically)
 * if timeout<0 , it indicates that we should block until we can read
 *
 * Note that in accordance with unixisms, the read will return as many bytes
 * as are available, up to the buffer size. If we reach the read() call,
 * we will get at least one byte.
 *
 * Returns -errno for an error, otherwise we return the number of bytes read.
 */
STATUS OutSocketConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    bytes_read = 0;

    int n;
    if (timeout_ms >= 0)
    {
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	n = poll(&pfd, 1, timeout_ms);
	if (n < 0)
	{
	    TRACE_WARN("poll on socket failed: %s (%d)\n", strerror(errno), errno);
	    return MakeErrnoStatus();
	}
	if (!n)
	{
	    TRACE_WARN("poll on socket timed out (%dms)\n", timeout_ms);
	    return CONN_E_TIMEDOUT;
	}
    }

    // We do a non-blocking read, as Linux poll() will occasionally erroneously
    // return 1 immediately after connect() -- on SMP machines only :-S
    n = recv(fd, buffer, buffer_size, MSG_DONTWAIT);

    //TRACE_WARN("otc::fb read returned %d\n", n );

    if (n < 0)
    {
	TRACE_WARN("read on socket failed: %s (%d)\n", strerror(errno), errno);
	if (errno == EWOULDBLOCK)
	    return CONN_E_TIMEDOUT;

	return MakeErrnoStatus();
    }

    bytes_read = n;

    return S_OK;
}

int OutSocketConnection::GetPollDescriptor()
{
    return fd;
}

// OutTcpConnection

STATUS OutTcpConnection::CreateAndBind()
{
    // Lookup the hostname
    struct hostent host_entry;
    struct hostent *host_entry_ptr;
    int host_errno;
    char host_buffer[8192];

    if (gethostbyname_r(host.c_str(), &host_entry,
			host_buffer, sizeof host_buffer,
			&host_entry_ptr, &host_errno) < 0)
    {
        return MakeErrnoStatus(ENOTNAM);
    }

    if (!host_entry_ptr)
        return MakeErrnoStatus(ENOTNAM);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return TCP_E_OPEN;

    int one = 1;
    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
		     (const char *)&one, sizeof(one)) < 0 )
    {
	close( fd );
	fd = -1;
        return TCP_E_SETSOCKOPT;
    }

    // Disable Nagle algorithm for better responsiveness (we only do single
    // write()s anyway for USB reasons)
    one = 1;
    if ( setsockopt( fd, IPPROTO_TCP, TCP_NODELAY,
		     &one, sizeof(one) ) < 0 )
    {
	close( fd );
	fd = -1;
        return TCP_E_SETSOCKOPT;
    }

    struct sockaddr_in sa;

    unsigned int **h_ent_list = (unsigned int **) host_entry_ptr->h_addr_list;
    int nents = 0;
    while(*h_ent_list)
    {
	nents++;
	h_ent_list++;
    }

    if(!nents)
        return TCP_E_ADDR;

    int pick_ent = random() % nents;

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = *reinterpret_cast<unsigned int *>
	(host_entry_ptr->h_addr_list[pick_ent]);
    sa.sin_port = htons(port);

// TRACE_WARN( "%s has IP address 0x%08x\n", host.c_str(), sa.sin_addr.s_addr );

    // Bloody hell we don't want a SIGPIPE to happen.
    // Yes that's sensible - a socket closed therefore our process dies.
    // Yes indeed.
    struct sigaction act;

    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGPIPE, &act, NULL) < 0)
    {
	TRACE_ERROR("SIGACTION FAILED: %d\n", errno);
    }

    if (connect(fd, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
    {
	close(fd);
	fd = -1;
        return TCP_E_CONNECT;
    }

    //printf( "TCP socket %d connected\n", fd );

    return S_OK;
}


// InSocketConnection

InSocketConnection::InSocketConnection()
    : fd_accept(-1), fd_pipe(-1)
{
}

InSocketConnection::~InSocketConnection()
{
    Close();
}

void InSocketConnection::Close()
{
    if (fd_pipe >= 0)
    {
	close(fd_pipe);
	fd_pipe = -1;
    }
    if (fd_accept >= 0)
    {
	close(fd_accept);
	fd_accept = -1;
    }
}

int InSocketConnection::OpenPipeIfNecessary(int timeout_ms)
{
    if (fd_pipe < 0)
    {
	// Check whether there is a connection pending.
	struct pollfd pfd;
	pfd.fd = fd_accept;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int n = poll(&pfd, 1, timeout_ms);
	if (n < 0)
	{
	    TRACE_WARN("Poll on accept socket failed.\n");
	    return n;
	}
	else if (n == 0)
	{
	    // No activity
	    return 0;
	}

	// We got some activity, better accept it.
	struct sigaction act;

	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGPIPE, &act, NULL) < 0)
	{
	    TRACE_WARN("SIGACTION FAILED: %d\n", errno);
	}
	n = Accept();
	if (n < 0)
	    return n;

	return 2;
    }
    else
	return 1;
}

int InSocketConnection::ClosePipe()
{
    if (fd_pipe >= 0)
    {
	close(fd_pipe);
	fd_pipe = -1;
    }
    return 0;
}

// Get data from remote end with timeout
//
// if timeout>0 , it indicates a timeout in milliseconds to wait for data
// if timeout==0, it indicates that we should read only if we can read
//                without delay (non-blocking read, basically)
// if timeout<0 , it indicates that we should block until we can read
//
// Note that in accordance with unixisms, the read will return as many bytes
// as are available, up to the buffer size. If we reach the read() call,
// we will get at least one byte.
//
STATUS InSocketConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    bytes_read = 0;

    int result = OpenPipeIfNecessary(timeout_ms);
    if (result <= 0)
	return SOCK_E_OPENPIPE;

    int n;
    if (timeout_ms >= 0)
    {
	struct pollfd pfd;
	pfd.fd = fd_pipe;
	pfd.events = POLLIN;
	pfd.revents = 0;

	n = poll(&pfd, 1, timeout_ms);
	if (n < 0)
	{
	    TRACE_WARN("poll on socket failed: %s (%d)\n", strerror(errno), errno);
	    if (errno == ECONNRESET)
		ClosePipe();

	    return CONN_E_RESET;
	}
	if (!n)
	{
//	    TRACE_WARN("poll on socket timed out (%dms)\n", timeout_ms);
	    return CONN_E_TIMEDOUT;
	}
    }

    // Get some stuff: will block until at least one byte comes in
    n = read(fd_pipe, buffer, buffer_size);
    if (n < 0)
    {
	TRACE_WARN("read on socket failed: %s (%d)\n", strerror(errno), errno);
	if (errno == ECONNRESET)
	    ClosePipe();

	return CONN_E_RESET;
    }
    else if (n == 0)
    {
	ClosePipe();
	return CONN_E_RESET;
    }

    bytes_read = n;

    return S_OK;
}

/** Send a block of data
 *
 * Unlike unix calls, this write will only return when either the entire buffer
 * has been sent or an error occurrs. A partial write will cause a retry.
 */
STATUS InSocketConnection::Send(const unsigned char *buffer, int length)
{
    if (fd_pipe < 0)
	return MakeErrnoStatus(ECONNRESET);

    // Write entire data packet: retry as necessary. We only return
    // when the entire buffer has been sent or there has been an error
    int done = 0, r;

    while(done < length)
    {
	if ((r = write(fd_pipe,buffer+done,length-done)) <= 0)
	{
	    if (errno == ECONNRESET)
		ClosePipe();
	    return MakeErrnoStatus(ECONNRESET);
	}
	done += r;
    }

    // Done OK
    return S_OK;
}

int InSocketConnection::GetPollDescriptor()
{
    if (fd_pipe >= 0)
	return fd_pipe;
    else
	return fd_accept;
}

STATUS InSocketConnection::Open()
{
    // Close device if it's open already
    Close();

    STATUS result;
    if (FAILED(result = CreateAndBind()))
	return result;

    if (listen(fd_accept, 1) < 0)
    {
	close(fd_accept);
	fd_accept = -1;
        return SOCK_E_LISTEN;
    }

    EmptyReceiveBuffer();
    return S_OK;
}

// InTcpConnection

InTcpConnection::InTcpConnection(int p)
    : port(p)
{
}

int InTcpConnection::PacketSize()
{
    // Return maximum USB packet payload
    return TCP_MAXPAYLOAD;
}

STATUS InTcpConnection::CreateAndBind()
{
    fd_accept = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_accept < 0)
    {
	TRACE_WARN("Failed to create socket.\n");
        return TCP_E_OPEN;
    }

    struct linger linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;

    setsockopt(fd_accept, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));

    int one = 1;
    if ( setsockopt( fd_accept, SOL_SOCKET, SO_REUSEADDR,
		     (const char *)&one, sizeof(one)) < 0 )
    {
	close( fd_accept );
        return TCP_E_SETSOCKOPT;
    }

    // Must disable Nagle algorithm so the other end gets our restart
    // reply before we've actually restarted
    one = 1;
    if ( setsockopt( fd_accept, IPPROTO_TCP, TCP_NODELAY,
		     &one, sizeof(one) ) < 0 )
    {
	close( fd_accept );
        return TCP_E_SETSOCKOPT;
    }

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);

    if (bind(fd_accept, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
    {
	close(fd_accept);
        return TCP_E_BIND;
    }

    return S_OK;
}

int InTcpConnection::Accept()
{
    // We must assume that someone has connected.
    struct sockaddr_in sa;
    socklen_t sa_size = sizeof(sa);

    fd_pipe = accept(fd_accept, reinterpret_cast<struct sockaddr *>(&sa), &sa_size);
    if (fd_pipe < 0)
    {
	TRACE_WARN("Accept failed: %d\n", errno);
	return -1;
    }

    IPAddress peer_addr( &sa );

    std::string ifname = IPInterface::GetInterfaceNameForAddress(peer_addr);
    unsigned int flags;
    IPInterface(ifname.c_str()).GetFlags(&flags);
    bool ppp = (flags & IFF_POINTOPOINT) > 0;

    TRACE("TCP connection %d from listening socket %d (ip %s if %s ppp=%s)\n",
	  fd_pipe,
	  fd_accept, peer_addr.ToString().c_str(),
	  ifname.c_str(), ppp ? "yes" : "no");

    if (ppp)
    {
	ClosePipe();
	errno = EPERM;
	return -1;
    }

    return 0;
}

#endif // !defined(ECOS)
