/* DatagramSocket.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.17 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "socket.h"
#include "InterfaceList.h"

#ifdef WIN32
#include <malloc.h>	// for _alloca
typedef DWORD ERROR_TYPE;
typedef int socklen_t;
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include "empeg_error.h"
#include "trace.h"
#include "socket_unix_compat.h"
#endif

#ifndef MSG_NOSIGNAL // Turn off SIGPIPE
#define MSG_NOSIGNAL 0
#endif

#include "NgLog/NgLog.h"

#define TRACE_DatagramSocket 0
#ifdef WIN32
LOG_COMPONENT(DatagramSocket)
#endif

HRESULT DatagramSocket::Create()
{
    return Socket::Create(PF_INET, SOCK_DGRAM, 0);
}

HRESULT DatagramSocket::Create(const IPEndPoint &endpoint)
{
    HRESULT hr = Create();
    if (SUCCEEDED(hr))
	hr = SetReuseAddr(true);
    if (SUCCEEDED(hr))
	hr = Bind(endpoint);
    
    return hr;
}

HRESULT DatagramSocket::SendTo2(const void *msg, size_t count, const IPEndPoint & dest) const
{
    ASSERT(m_soc != INVALID_SOCKET);

#ifdef WIN32
    // On Windows NT 4.0 (at least), the implementation defined behaviour on 
    // broadcasting to the all-ones address is spectacularly damaged - it
    // sends the broadcast out on all interfaces, but only using one particular
    // source IP address. This is bad for a multihomed box. So, we have to
    // iterate over the interfaces and send out a separate packet on each.
    if ( dest.GetIPAddress().ToNetworkLong() == (unsigned long)-1 )
    {
        InterfaceList il = InterfaceList::GetInterfaceList();

        for(InterfaceList::const_iterator i = il.begin(); i != il.end(); ++i)
        {
	    IPAddress address = i->GetAddress();
	    IPAddress netmask = i->GetNetmask();

	    LOG_INFO("Found interface '%s'", address.ToString().c_str());
	
            // Don't trust the broadcast address in the InterfaceList because
            // Win95 can't even get that right.
	    IPEndPoint broadcast( IPAddress::GuessBroadcastAddress(address, netmask),
                                  dest.GetPortAsHost() );

            if ( (i->GetFlags() & (IFF_BROADCAST|IFF_UP|IFF_LOOPBACK)) == (IFF_BROADCAST|IFF_UP)
                 && broadcast.GetIPAddress().ToNetworkLong() != (unsigned long)-1 )
            {
                // Recurse to send the packet to the subnet-directed broadcast address

	        LOG_INFO("Trying to broadcast packet on address '%s'",
                        broadcast.GetIPAddress().ToString().c_str());
	        HRESULT hr = SendTo(msg, count, broadcast);

	        if (FAILED(hr))
                {
	            LOG_WARN("Failed to broadcast packet on interface '%s'", address.ToString().c_str());
                    return hr;
                }
            }
        }

        return S_OK;
    }
#endif

    struct sockaddr_in dest_addr;
    dest.ToSockAddr(&dest_addr);

    int result = sendto(m_soc, (const char *)msg, count,
			MSG_NOSIGNAL, (struct sockaddr *)&dest_addr,
			sizeof(dest_addr));
    if(result == (int)count)
	return S_OK;
    else if(result >= 0)
	return E_FAIL;
    else
	return MakeWin32Status(WSAGetLastError());
}

STATUS DatagramSocket::SendBroadcast( const void *msg, size_t count,
				      short portAsHost ) const
{
    return SendTo2( msg, count, IPEndPoint( IPAddress(255,255,255,255),
					    portAsHost ) );
}

HRESULT DatagramSocket::ReceiveFrom(void *msg, int count, int *bytesRead,
				    IPEndPoint *endpoint) const
{
    ASSERT(m_soc != INVALID_SOCKET);

    struct sockaddr_in source_addr;
    socklen_t source_addr_len = sizeof(source_addr);

    int result = ::recvfrom(m_soc, (char *)msg, count, MSG_NOSIGNAL,
			    (struct sockaddr *)&source_addr, &source_addr_len);

    if(result < 0)
    {
	return MakeWin32Status(WSAGetLastError());
    }

    if(endpoint)
	endpoint->FromSockAddr(&source_addr);

    if ( bytesRead )
	*bytesRead = result;

    return S_OK;
}

HRESULT DatagramSocket::ReceiveFrom(std::string *str, int max_size, IPEndPoint *source) const
{
    ASSERT(m_soc != INVALID_SOCKET);
    ASSERT(str);

    char *buffer = reinterpret_cast<char *>(_alloca(max_size));
    int bytes;

    HRESULT result = ReceiveFrom(buffer, max_size, &bytes, source);
    if(result != S_OK)
	return result;

    str->assign(buffer, buffer + bytes);
    return S_OK;
}

/* eof */
