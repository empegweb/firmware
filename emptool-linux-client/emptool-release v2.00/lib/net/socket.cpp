/* lib/async/socket.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.43 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *   Mike Crowe <mac@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "socket.h"

#ifdef WIN32
#include <malloc.h>	// for _alloca
#include "empeg_error.h"
typedef DWORD ERROR_TYPE;
typedef int socklen_t;
#else
#include "socket_unix_compat.h"
#endif

#include "NgLog/NgLog.h"

#define TRACE_Socket 0
#if DEBUG>0 || defined(WIN32)
LOG_COMPONENT(Socket);
#endif

Socket::Socket()
    : m_soc(INVALID_SOCKET)
{
}

Socket::Socket(SOCKET soc)
    : m_soc(soc)
{
}

HRESULT Socket::Create(int af, int type, int protocol)
{
    ASSERT(m_soc == INVALID_SOCKET);

    m_soc = socket(af, type, protocol);
    if(m_soc == INVALID_SOCKET)
    {
	STATUS lastError = MakeWin32Status(WSAGetLastError());
	LOG_WARN
	    (
		"socket() failed - 0x%x",
		PrintableStatus(lastError)
	    );
	return lastError;
    }

    return S_OK;
}

bool Socket::IsOpen() const
{
    return m_soc != INVALID_SOCKET;
}

void Socket::Close()
{
    LOG_DEBUG("Socket::Close");
    if(m_soc != INVALID_SOCKET)
	closesocket(m_soc);
    m_soc = INVALID_SOCKET;
}

void Socket::Shutdown(int how)
{
    ::shutdown( m_soc, how );
}

// Why doesn't this return HRESULT?
int Socket::Select(int timeoutMS) const
{
    // TODO: put this back in... ASSERT(m_soc != INVALID_SOCKET);
    
    fd_set readfds;
    FD_ZERO(&readfds);

    FD_SET(m_soc, &readfds);
    if (timeoutMS >= 0)
    {
        timeval tv;
        tv.tv_sec  = timeoutMS / 1000;
        tv.tv_usec = 1000 * (timeoutMS % 1000);
        return select(m_soc + 1, &readfds, NULL, NULL, &tv);
    }
    else
        return select(m_soc + 1, &readfds, NULL, NULL, NULL);
    
}

HRESULT Socket::Bind(const IPEndPoint &endpoint)
{
    LOG_INFO("Binding to %s:%d", endpoint.GetIPAddress().ToString().c_str(), endpoint.GetPortAsHost());

    ASSERT(m_soc != INVALID_SOCKET);
    struct sockaddr_in sin;
    endpoint.ToSockAddr(&sin);

    if(bind(m_soc, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) != 0)
    {
	ERROR_TYPE error = WSAGetLastError();
	closesocket(m_soc);
	return MakeWin32Status(error);
    }

    return S_OK;
}

// Why doesn't this return HRESULT?
int Socket::EnableBroadcast(bool b)
{
    ASSERT(m_soc != INVALID_SOCKET);
    int i = b;
    return ::setsockopt(m_soc, SOL_SOCKET, SO_BROADCAST, (const char *)&i, sizeof(i));
}

// Why doesn't this return HRESULT?
int Socket::SetNonBlocking(bool b)
{
    ASSERT(m_soc != INVALID_SOCKET);
    unsigned long param = b;
    return ::ioctlsocket(m_soc, FIONBIO, &param);
}

#if !defined(WIN32)
STATUS Socket::BindToDevice(const char *ifname)
{
    ASSERT(m_soc != INVALID_SOCKET);
    struct ifreq interface;
    strncpy (interface.ifr_ifrn.ifrn_name, ifname, IFNAMSIZ);
    int rc = ::setsockopt(m_soc, SOL_SOCKET, SO_BINDTODEVICE, (const char *)&interface, sizeof(interface));
    if ( rc < 0 )
    {
        STATUS lastError = MakeWin32Status(WSAGetLastError());
        WARN("BindToDevice %s failed - 0x%x\n", ifname, PrintableStatus(lastError));
        return lastError;
    }
    return S_OK;
}
#endif

STATUS Socket::SetReuseAddr(bool b)
{
    int i = b;

    int rc = ::setsockopt(m_soc, SOL_SOCKET, SO_REUSEADDR, (const char*) &i, sizeof(i) );
    if ( rc < 0 )
    {
	STATUS lastError = MakeWin32Status(WSAGetLastError());
	LOG_WARN("setsockopt() failed - 0x%x", PrintableStatus(lastError));
	return lastError;
    }

    return S_OK;
}

IPEndPoint Socket::GetLocalEndPoint() const
{
    struct sockaddr sa;
    socklen_t len = sizeof(sa);

    int rc = ::getsockname( m_soc, &sa, &len );
    if ( rc < 0 )
    {
	return IPEndPoint();
    }

    IPEndPoint ep;
    ep.FromSockAddr( (struct sockaddr_in*) &sa );
    return ep;
}

#ifdef WIN32
int Socket::AsyncSelect(HWND hwnd, UINT message, LONG networkEvents)
{
    return WSAAsyncSelect(m_soc, hwnd, message, networkEvents);
}

int Socket::EventSelect(HANDLE hEvent, LONG networkEvents)
{
    return WSAEventSelect(m_soc, hEvent, networkEvents);
}

int Socket::WSAIoctl(DWORD dwIoControlCode,
		     LPVOID lpvInBuffer, DWORD cbInBuffer,
		     LPVOID lpvOutBuffer, DWORD cbOutBuffer,
		     LPDWORD lpcbBytesReturned,
		     LPWSAOVERLAPPED lpOverlapped /*= NULL*/,
		     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine /*= NULL*/) const
{
    return ::WSAIoctl(m_soc, dwIoControlCode, lpvInBuffer, cbInBuffer,
			lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
			lpOverlapped, lpCompletionRoutine);
}

#endif

// This should really not be here. Code using this should be using the
// IPInterface stuff once a collection class has been built for it.

#ifdef WIN32
// TODO: an implementation of the following, but using ioctlsocket(SIO_GET_BROADCAST_ADDRESS)
#else
HRESULT Socket::GetBroadcastAddress(IPAddress *brdaddr)
{
    ASSERT(m_soc != INVALID_SOCKET);

    // This is (necessarily) a real pain. We must iterate over the interfaces.
    char buf[1024];
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if (ioctl(m_soc, SIOCGIFCONF, &ifc) < 0)
    {
        WARN("Failed to get interface configuration (errno=%d).\n", errno);
        return MakeErrnoStatus();
    }

    struct ifreq *ifr = ifc.ifc_req;

    // Find the first AF_INET interface.
    int if_count = ifc.ifc_len/sizeof(struct ifreq);

    for(int i = 0; i < if_count; i++)
    {
        // Skip not AF_INET interfaces.
        if (ifr[i].ifr_addr.sa_family != AF_INET)
        {
            continue;
        }

#if 0
        // For some reason the interface doesn't think it is up.
        if ((ifr->ifr_flags & IFF_UP) == 0)
        {
            continue;
        }
#endif
        if (ifr->ifr_flags & IFF_LOOPBACK)
        {
            continue;
        }
        
        if ((ifr->ifr_flags & IFF_BROADCAST) == 0)
        {
            continue;
        }

        if (ioctl(m_soc,  SIOCGIFBRDADDR, ifr + i) < 0)
        {
            return MakeErrnoStatus();
        }

        brdaddr->Assign((struct sockaddr_in*)&ifr->ifr_broadaddr);
        return S_OK;
    }

    return E_FAIL;
}
#endif
