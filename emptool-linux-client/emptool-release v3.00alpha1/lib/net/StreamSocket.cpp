/* StreamSocket.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.31 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "empeg_error.h"

#include "socket.h"
#include "empeg_error.h"

#ifdef WIN32
#include <malloc.h>	// for _alloca
typedef DWORD ERROR_TYPE;
typedef int socklen_t;
#else
#include "socket_unix_compat.h"
#endif

#ifndef MSG_NOSIGNAL // Turn off SIGPIPE
#define MSG_NOSIGNAL 0
#endif


#define TRACE_StreamSocket 0

/** @todo Move these somewhere shared by the socket files
    -- there's a glorious hack over in DatagramSocket.cpp */
#if defined(WIN32)
inline STATUS MakeSocketStatus()
{
    switch(WSAGetLastError())
    {
    case WSAEWOULDBLOCK:
	return S_AGAIN;
    default:
	return MakeWin32Status(WSAGetLastError());
    }
}
#else
inline STATUS MakeSocketStatus()
{
    if ((errno == EWOULDBLOCK) ||       // EWOULDBLOCK means EAGAIN sometimes
        (errno == EAGAIN))
        return S_AGAIN;
    else
        return MakeErrnoStatus(errno);
}
#endif

STATUS StreamSocket::Create()
{
    return Socket::Create(PF_INET, SOCK_STREAM, 0);
}

STATUS StreamSocket::Create(const IPEndPoint &localEndpoint)
{
    STATUS hr = Create();
    if (SUCCEEDED(hr))
	hr = SetReuseAddr(true);
    if (SUCCEEDED(hr))
	hr = Bind(localEndpoint);
    
    return hr;
}

// TODO: Homogenise the error code returning from here.
STATUS StreamSocket::Connect(const IPEndPoint &remoteEndpoint)
{
    struct sockaddr_in sin;
    remoteEndpoint.ToSockAddr(&sin);

    int result = connect(m_soc, (const struct sockaddr *)&sin, sizeof(struct sockaddr_in));
    if (result == 0)
	return S_OK;
    else
	return MakeSocketStatus();
}

STATUS StreamSocket::Listen(int backlog /* = SOMAXCONN */) const
{
    int result = listen(m_soc, backlog);
    if(result == 0)
	return S_OK;
    else
	return MakeSocketStatus();
}

STATUS StreamSocket::SetReuseAddr(bool reuseAddr /* = true */)
{
    int val = reuseAddr;
    int result = setsockopt(m_soc, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));
    if(result == 0)
	return S_OK;
    else
	return MakeSocketStatus();
}

void StreamSocket::SetNoDelay( bool nodelay /* =true */ ) const
{
#ifdef TCP_NODELAY
    int val = nodelay;
    setsockopt( m_soc, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(val) );
#endif
}

StreamSocket StreamSocket::Accept(bool *pGotOne) const
{
    struct sockaddr sin;
    socklen_t len = sizeof(sin);
    SOCKET soc = accept(m_soc, &sin, &len);

    if ( soc < 0 )
    {
	if ( pGotOne )
	    *pGotOne = false;
	return StreamSocket();
    }

    if ( pGotOne )
	*pGotOne = true;

    return StreamSocket(soc);
}

STATUS StreamSocket::Receive(void *buffer, size_t len, size_t *nRead )
{
    int result = recv(m_soc, static_cast<char *>(buffer), len, MSG_NOSIGNAL);
    if (result >= 0)
    {
	*nRead = result;
	return S_OK;
    }

    *nRead = 0;
    return MakeSocketStatus();
}

STATUS StreamSocket::Send(const void *buffer, size_t len, size_t *pSent)
{
    STATUS status = S_OK;

    int result = send(m_soc, static_cast<const char *>(buffer), len, MSG_NOSIGNAL);
    if (result < 0)
    {
        status = MakeSocketStatus();
        result = 0;
    }
    if (pSent)
	*pSent = result;

    return status;
}

STATUS StreamSocket::Read(void *buffer, unsigned bytesToRead,
			  unsigned *pBytesRead)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_soc, &fds);
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;

    int rc = ::select(m_soc+1, &fds, &fds, &fds, &tv);
    if (rc == 0)
	return E_TIMEOUT;

    return Receive(buffer, (size_t)bytesToRead, (size_t*)pBytesRead);
}

STATUS StreamSocket::Write(const void *buffer, unsigned bytesToWrite,
			   unsigned *pBytesWritten)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_soc, &fds);
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;

    int rc = ::select(m_soc+1, &fds, &fds, &fds, &tv);
    if (rc == 0)
	return E_TIMEOUT;

    return Send(buffer, (size_t)bytesToWrite, (size_t*)pBytesWritten); 
}

#ifndef WIN32
STATUS StreamSocket::CreatePair(StreamSocket *s1, StreamSocket *s2)
{
    int fd[2];

    int rc = ::socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
    if (rc<0)
	return MakeSocketStatus();

    *s1 = StreamSocket(fd[0]);
    *s2 = StreamSocket(fd[1]);

    return S_OK;
}
#endif

