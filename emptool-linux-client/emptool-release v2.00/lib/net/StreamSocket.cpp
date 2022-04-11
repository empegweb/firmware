/* StreamSocket.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.22 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "socket.h"

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

#include "NgLog/NgLog.h"

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

HRESULT StreamSocket::Create()
{
    return Socket::Create(PF_INET, SOCK_STREAM, 0);
}

HRESULT StreamSocket::Create(const IPEndPoint &localEndpoint)
{
    HRESULT hr = Create();
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

HRESULT StreamSocket::Listen(int backlog /* = SOMAXCONN */) const
{
    int result = listen(m_soc, backlog);
    if(result == 0)
	return S_OK;
    else
	return MakeSocketStatus();
}

HRESULT StreamSocket::SetReuseAddr(bool reuseAddr /* = true */)
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
    SOCKET soc = accept(m_soc, NULL, NULL);

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
