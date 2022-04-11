/* lib/async/ipaddress.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.36 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 */

#include "config.h"
#include "trace.h"

#ifdef WIN32
    #include <winsock2.h>
    typedef in_addr win_in_addr;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include "types.h"
    #include "empeg_error.h"

struct win_in_addr {
    union {
	struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
	struct { unsigned short s_w1,s_w2; } S_un_w;
	unsigned long S_addr;
    } S_un;

    operator const in_addr() const
    {
	in_addr ia;
	ia.s_addr = S_un.S_addr;
	return ia;
    }
    win_in_addr operator=(const in_addr &ia)
    {
	S_un.S_addr = ia.s_addr;
	return *this;
    }
};

#endif
#if !defined(WIN32)
#include <stdio.h>
#include <unistd.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif
#include "ipaddress.h"
#include "var_string.h"
#include "net/net_errors.h"

const IPAddress IPAddress::ANY(FromNetworkOrder((unsigned long)INADDR_ANY));
const IPAddress IPAddress::ZERO(FromNetworkOrder((unsigned long)0));

#if !defined(WIN32)
typedef struct hostent HOSTENT;
#endif

IPAddress::IPAddress(const struct sockaddr_in *s)
{
    FromSockAddr(s);
}

void IPAddress::FromSockAddr(const struct sockaddr_in *s_in)
{
    m_addr = s_in->sin_addr.s_addr;
}

void IPAddress::ToSockAddr(struct sockaddr_in *s_in) const
{
    s_in->sin_family = AF_INET;
    s_in->sin_addr.s_addr = m_addr;
}

void IPAddress::Assign(const char *s)
{
    m_addr = inet_addr(s);
}

std::string IPAddress::ToString() const
{
    // yikes, this isn't thread safe
    // return std::string(inet_ntoa(m_addr));
    unsigned long n = ToHostLong();
    return VarString::Printf("%lu.%lu.%lu.%lu",
			     (n >> 24) & 255,
			     (n >> 16) & 255,
			     (n >> 8) & 255,
			     n & 255);
}

STATUS IPAddress::FromHostName(const std::string &hostname)
{
    // Check for dotted IP first.
    unsigned long ul = inet_addr(hostname.c_str());
    if (ul != INADDR_NONE)
    {
	Assign(FromNetworkOrder(ul));
    }
    else
    {
	// It failed, try a name lookup...
        HOSTENT *h;
#if defined(WIN32)
        h = gethostbyname(hostname.c_str());
	if (!h)
	    return MAKE_STATUS(SEVERITY_ERROR, FACILITY_WIN32, WSAGetLastError());
#elif defined(ECOS)
        h = gethostbyname(hostname.c_str());
	if (!h)
	    return RESOLVER_E_NOTFOUND;
#else
        struct hostent host;
        int host_errno;
        char host_buffer[2048];
        if (gethostbyname_r(hostname.c_str(), &host,
                            host_buffer, sizeof(host_buffer),
                            &h, &host_errno) != 0)
        {
	    switch (host_errno)
	    {
	    case ERANGE:
		// This is a problem with the buffer size - better programming required!
                TRACE_WARN("IPAddress::FromHostName application error - make the buffer bigger!\n");
                return MakeErrnoStatus(ENOMEM);
	    case HOST_NOT_FOUND:
		TRACE_WARN("IPAddress::FromHostName host '%s' not found\n", hostname.c_str());
		return RESOLVER_E_NOTFOUND;
	    case TRY_AGAIN:
		return RESOLVER_E_SERVERUNAVAILABLE;
	    case NO_RECOVERY:
		return RESOLVER_E_UNRECOVERABLE;
	    case NO_ADDRESS:
		return RESOLVER_E_NOADDRESS;
	    default:
		return RESOLVER_E_UNKNOWN;
	    }
        }

	if (!h)
	{
	    TRACE_ERROR("gethostbyname_r failed in a wierd way - h_errno=%d\n", host_errno);
            return MakeErrnoStatus(ENOENT);
	}
#endif
        memcpy(&m_addr, h->h_addr_list[0], sizeof(unsigned long));
    }

    return S_OK;
}

#ifdef WIN32
bool IPAddress::GetFirstIP(IPAddress *ip)
{
    char hostname[512];

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
	struct hostent *host = gethostbyname(hostname);
	struct sockaddr_in s_in;
	memcpy(&s_in.sin_addr, host->h_addr, sizeof(host->h_addr));

	ip->FromSockAddr(&s_in);
	return true;
    }

    return false;
}
#else
bool IPAddress::GetFirstIP(IPAddress *ip)
{
#ifndef ECOS
    char hostname[512];
    
    if (gethostname(hostname, sizeof(hostname)) != 0) return false;
    
    struct hostent host;
    struct hostent *host_entry_ptr;
    int host_errno;
    char host_buffer[8192];
    if (gethostbyname_r(hostname, &host,
			host_buffer, sizeof(host_buffer),
			&host_entry_ptr, &host_errno) != 0) return false;
    
    sockaddr_in s_in;
    
    memcpy(&s_in.sin_addr, host.h_addr, sizeof(host.h_addr));
    
    ip->FromSockAddr( &s_in );

    return true;
#else
    UNUSED(ip);
    return false;
#endif
}

STATUS IPAddress::GetAddressForInterface(const char *iface)
{
    struct ifreq ifr;
    int sock;

    memset(&ifr, 0, sizeof(ifr));
    
    // Create a socket - we don't need to bind to anything
    if ((sock=socket(PF_INET,SOCK_DGRAM,0))<0)
	return MakeErrnoStatus();

    // Set up the arp ioctl request (device name & protocol address)
    strcpy(ifr.ifr_name, iface);
    //ToSockAddr((struct sockaddr_in*)&r.arp_pa);

    // Get the MAC address from the ARP cache
    if (ioctl(sock,SIOCGIFADDR, &ifr)<0)
    {
	close(sock);
	return MakeErrnoStatus();
    }

    // Close the socket
    close(sock);

    // We've got it!
    FromSockAddr(reinterpret_cast<sockaddr_in *>(&ifr.ifr_addr));

    return S_OK;
}

STATUS IPAddress::GetMACAddress(unsigned char *mac)
{
#ifndef ECOS
    struct arpreq r;
    int sock;

    memset(&r, 0, sizeof(arpreq));
    
    // Create a socket - we don't need to bind to anything
    if ((sock=socket(PF_INET,SOCK_DGRAM,0))<0)
	return MakeErrnoStatus();

    // Set up the arp ioctl request (device name & protocol address)
    strcpy(r.arp_dev,"eth0");
    ToSockAddr((struct sockaddr_in*)&r.arp_pa);

    // Get the MAC address from the ARP cache
    if (ioctl(sock,SIOCGARP,&r)<0)
	return MakeErrnoStatus();

    // Close the socket
    close(sock);

    // Return it
    memcpy(mac,r.arp_ha.sa_data,6);

    return S_OK;
#else
    UNUSED(mac);
    return E_NOTIMPL;
#endif
}

#endif


/** The odd phrasing of this function is to avoid a gcc 2.95.[23] bug that
 * causes a compiler crash here.
 */
bool IPAddressRange::Contains(const IPAddress &ip) const
{
    if (ip < m_first)
	return false;
    if (ip > m_last)
	return false;
    return true;
}

bool IPAddressRange::Validate(const IPAddress &host) const
{
    // Is the mask valid?
    if(!m_mask.IsValidNetmask())
	return false;
    
    // Is it in the right order?
    if(GetFirst() >= GetLast())
	return false;

    //    Are the first and last addresses in the same network?
    if(!IPAddress::OnSameSubnet(GetFirst(), GetLast(), GetMask()))
	return false;

    //    Is the range we're handing out on our network?
    if(!IPAddress::OnSameSubnet(GetFirst(), host, GetMask()))
	return false;

    if(!IPAddress::OnSameSubnet(GetLast(), host, GetMask()))
	return false;

    //    Ensure our IP address is _not_ in the range.
    if(Contains(host))
	return false;

    return true;
}

IPAddress IPAddress::GuessNetmask() const
{
    // We make an educated guess as to what the netmask will be. If we don't know
    // then we use 0.
    unsigned long ip = ToHostLong();
    unsigned long c = (ip >> 24);

    if (c > 0 && c <= 126)
    {
	// We're class A
	return IPAddress(255, 0, 0, 0);
    }
    else if (c >= 128 && c < 192)
    {
	// We're class B
	return IPAddress(255, 255, 0, 0);
    }
    else if (c >= 192 && c < 223)
    {
	// We're class C
	return IPAddress(255, 255, 255, 0);
    }
    else
	return IPAddress();
}

bool IPAddress::InBadRange() const
{
    unsigned long ip = ToHostLong();
    unsigned long c = (ip >> 24);
    if (c == 0)
	return true;
    else if (c == 127)
	return true;
    else if (c > 223)
	return true;
    else
	return false;
}

bool IPAddress::IsValidNetmask() const
{
    // A valid netmask fits 1+0+ and fills up all the bits.
    unsigned long m = ToHostLong();

    // So, working from the bottom we should have at least two zeros followed
    // by a number of ones.


    // Scan for zeros
    int i;
    for(i = 32 ; i > 0; --i)
    {
	if (m & 1)
	    break;
	m >>= 1;
    }
    if (i > 30)
    {
	// Not enough zeros at the end.
	return false;
    }
    if (i == 0)
    {
	// All zeros
	return false;
    }
    // Now scan for ones.
    for(;i > 0; --i)
    {
	if ((m & 1) == 0)
	    break;
	m >>= 1;
    }
    if (i > 0)
    {
	// Not enough ones at the beginning.
	return false;
    }
    return true;
}

/* static */
IPAddressRange IPAddressRange::DefaultDhcpRange(const IPAddress &host,
						const IPAddress &mask)
{
    // Come up with some defaults, just in case...
    unsigned long ipAddress  = host.ToHostLong();
    unsigned long netMask    = mask.ToHostLong();
    unsigned long netAddress = ipAddress & netMask;

    // So that we can pick a range that will be usable that doesn't
    // include the network card address we split the subnet in half,
    // work out which half our IP address is in and then use the other
    // half for the autoconfiguration. This could be disastrous in
    // some circumstances but it is what the marketing people
    // require. ;-(
    unsigned long netBits = ~netMask;
    unsigned long lowerHalf = netBits >> 1;
    unsigned long adapterAddressOnNetwork = ipAddress & netBits;

    unsigned long minAddress, maxAddress;

    if ((adapterAddressOnNetwork & lowerHalf) == adapterAddressOnNetwork)
    {
	// It's in the lower half, we'll use the upper half then.
	minAddress = netAddress | (lowerHalf + 2);
	maxAddress = netAddress | (~netMask - 1);
    }
    else
    {
	// It's in the upper half, we'll use the lower half then.
	minAddress = netAddress | 1;
	maxAddress = netAddress | (lowerHalf - 1);
    }

    IPAddress first = IPAddress(FromHostOrder(minAddress));
    IPAddress last  = IPAddress(FromHostOrder(maxAddress));

    return IPAddressRange(first, last, mask);
}

void IPEndPoint::ToSockAddr(struct sockaddr_in *s_in) const
{
    address.ToSockAddr(s_in);
    s_in->sin_port = htons(port);
}

void IPEndPoint::FromSockAddr(struct sockaddr_in *s_in)
{
    address.FromSockAddr(s_in);
    port = ntohs(s_in->sin_port);
}
