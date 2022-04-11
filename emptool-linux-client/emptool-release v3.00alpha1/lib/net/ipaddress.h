/* ipaddress.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.41 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 */

#ifndef INCLUDED_IPADDRESS_H
#define INCLUDED_IPADDRESS_H 1

#include <string>
#include "empeg_status.h"
#include "empeg_endian.h"

// Natty little helper classes that help people be really explicit about the
// ordering of stuff they pass into IPAddress.
struct FromHostOrder
{
    unsigned long addr;

    explicit FromHostOrder(unsigned long l)
	: addr(l) {}
};

struct FromNetworkOrder
{
    unsigned long addr;

    explicit FromNetworkOrder(unsigned long l)
	: addr(l) {}
};

struct sockaddr_in;

class IPAddress
{
    // This is stored in network endianness.
    unsigned long m_addr;

public:
    static const IPAddress ANY;
    static const IPAddress ZERO;

    static bool GetFirstIP(IPAddress *);

    IPAddress();
    explicit IPAddress(const FromNetworkOrder &j);
    explicit IPAddress(const FromHostOrder &j);
    explicit IPAddress(const char *s);
    explicit IPAddress(const std::string &s);
    explicit IPAddress(const struct sockaddr_in *s);
    IPAddress(BYTE w, BYTE x, BYTE y, BYTE z);

    void Assign(const char *s);
    void Assign(const std::string & s);
    void Assign(const FromNetworkOrder &j);
    void Assign(const FromHostOrder &j);
    void Assign(BYTE w, BYTE x, BYTE y, BYTE z);
    void Assign(const struct sockaddr_in *s);

    std::string ToString() const;

    int Compare(const IPAddress &other) const;
    bool operator==(const IPAddress &other) const;
    bool operator!=(const IPAddress &other) const;
    bool operator<(const IPAddress &other) const;
    bool operator<=(const IPAddress &other) const;
    bool operator>(const IPAddress &other) const;
    bool operator>=(const IPAddress &other) const;

    void ToNetworkByteOrderAddress(char *pb) const;
    void ToNetworkByteOrderAddress(BYTE *pb) const;

    unsigned long ToHostLong() const;
    unsigned long ToNetworkLong() const;
    void ToSockAddr(struct sockaddr_in *sin) const;

    void FromSockAddr(const struct sockaddr_in *sin);
    STATUS FromHostName(const std::string &host);
    bool IsUPNP() const;

#ifndef WIN32
    /* Deprecated, use lib/net/interface.h */
    STATUS GetAddressForInterface(const char *iface);
    STATUS GetMACAddress(unsigned char *mac);
#endif
    friend class IPAddressRange;

    void ApplyNetMask(IPAddress mask);
    static bool OnSameSubnet(const IPAddress ip1, const IPAddress ip2, const IPAddress mask);
    static IPAddress GuessBroadcastAddress(const IPAddress ip, const IPAddress mask);

    bool IsValidNetmask() const;
    bool IsBroadcastAddress(const IPAddress netmask) const;
    bool IsNetworkAddress(const IPAddress netmask) const;
    IPAddress GuessNetmask() const;
    bool InBadRange() const;
};
#include <vector>
typedef std::vector<IPAddress> IPAddressVec;

inline IPAddress::IPAddress()
{
    m_addr = 0xffffffff;
}

inline IPAddress::IPAddress(const FromNetworkOrder &j)
{
    m_addr = j.addr;
}

inline IPAddress::IPAddress(const FromHostOrder &j)
{
    m_addr = cpu_to_be32(j.addr);
}

inline IPAddress::IPAddress(const char *s)
{
    Assign(s);
}

inline IPAddress::IPAddress(const std::string &s)
{
    Assign(s.c_str());
}

inline IPAddress::IPAddress(BYTE w, BYTE x, BYTE y, BYTE z)
{
    Assign(w,x,y,z);
}

inline void IPAddress::Assign(const std::string & s)
{
    Assign(s.c_str());
}

inline void IPAddress::Assign(const FromNetworkOrder &j)
{
    m_addr = j.addr;
}

inline void IPAddress::Assign(const FromHostOrder &j)
{
    m_addr = cpu_to_be32(j.addr);
}

inline void IPAddress::Assign(BYTE w, BYTE x, BYTE y, BYTE z)
{
    m_addr = cpu_to_be32(w | (x<<8) | (y<<16) | (z<<24));
}

inline void IPAddress::Assign(const struct sockaddr_in *s)
{
    FromSockAddr(s);
}

inline unsigned long IPAddress::ToHostLong() const
{
    return be32_to_cpu(m_addr);
}

inline int IPAddress::Compare(const IPAddress &other) const
{
    unsigned long us_host = ToHostLong();
    unsigned long them_host = other.ToHostLong();
    return us_host - them_host;
}

inline bool IPAddress::operator==(const IPAddress &other) const
{
    return Compare(other) == 0;
}

inline bool IPAddress::operator!=(const IPAddress &other) const
{
    return Compare(other) != 0;
}

inline bool IPAddress::operator<(const IPAddress &other) const
{
    return Compare(other) < 0;
}

inline bool IPAddress::operator<=(const IPAddress &other) const
{
    return Compare(other) <= 0;
}

inline bool IPAddress::operator>(const IPAddress &other) const
{
    return Compare(other) > 0;
}

inline bool IPAddress::operator>=(const IPAddress &other) const
{
    return Compare(other) >= 0;
}

inline void IPAddress::ToNetworkByteOrderAddress(BYTE *pb) const
{
    pb[0] = m_addr;
    pb[1] = m_addr >> 8;
    pb[2] = m_addr >> 16;
    pb[3] = m_addr >> 24;
}

inline void IPAddress::ToNetworkByteOrderAddress(char *pb) const
{ 
    ToNetworkByteOrderAddress((BYTE *)pb); 
}

inline unsigned long IPAddress::ToNetworkLong() const
{
    return m_addr;
}

inline bool IPAddress::IsUPNP() const
{
    return (ToHostLong() & 0xffff0000) == 0xa9fe0000;
}

inline void IPAddress::ApplyNetMask(IPAddress mask)
{
    m_addr &= mask.m_addr;
}

/*static*/
inline bool IPAddress::OnSameSubnet(const IPAddress ip1, const IPAddress ip2, const IPAddress mask)
{
    unsigned long l1 = ip1.ToHostLong();
    unsigned long l2 = ip2.ToHostLong();
    unsigned long m = mask.ToHostLong();

    return (l1 & m) == (l2 & m);
}

inline bool IPAddress::IsBroadcastAddress(const IPAddress netmask) const
{
    // The broadcast address means that all the subnet bits are one, therefore
    // the bitwise inverse will be zero.
    return ~(ToHostLong() | netmask.ToHostLong()) == 0;
}

inline bool IPAddress::IsNetworkAddress(const IPAddress netmask) const
{
    // The network address means that all the bits that are zero in the netmask
    // are also zero in the address.
    return (ToHostLong() & ~netmask.ToHostLong()) == 0;
}

inline IPAddress IPAddress::GuessBroadcastAddress(const IPAddress ip, const IPAddress mask)
{
    return IPAddress(FromHostOrder(ip.ToHostLong() | ~mask.ToHostLong()));
}

class IPAddressRange
{
    IPAddress m_first;
    IPAddress m_last;
    IPAddress m_mask;

public:
    IPAddressRange(const IPAddress &first, const IPAddress &last, const IPAddress &mask)
	: m_first(first), m_last(last), m_mask(mask) 
    {
	//ASSERT(first <= last);
    }
    int GetCount() const
    {
	return m_last.ToHostLong() - m_first.ToHostLong() + 1;
    }
    IPAddress GetAtIndex(int j) const
    {
//	ASSERT(j < GetCount());
	unsigned long a = m_first.ToHostLong();
	a += j;
	return IPAddress(FromHostOrder(a));
    }
    const IPAddress &GetMask() const
    {
	return m_mask;
    }
    const IPAddress &GetFirst() const
    {
	return m_first;
    }
    const IPAddress &GetLast() const
    {
	return m_last;
    }
    void SetRange(const IPAddress &first, const IPAddress &last, const IPAddress &mask)
    {
	m_first = first;
	m_last = last;
	m_mask = mask;
    }
    bool Contains(const IPAddress &ip) const;
    bool Validate(const IPAddress &host) const;

    std::string ToString() const
	{
	    return m_first.ToString() + "-" + m_last.ToString();
	}

    static IPAddressRange DefaultDhcpRange(const IPAddress &host, const IPAddress &mask);
};

class IPEndPoint
{
    IPAddress address;
    short port;              // Host byte order

public:
    IPEndPoint() : address(), port(0) {}
    IPEndPoint(const IPAddress &ip, short p)
	: address(ip), port(p) {}

    void ToSockAddr(struct sockaddr_in *s_in) const;
    void FromSockAddr(struct sockaddr_in *s_in);
    IPAddress GetIPAddress() const
    {
	return address;
    }
    short GetPortAsNetwork() const
    {
	return cpu_to_be16(port);
    }
    short GetPortAsHost() const
    {
	return port;
    }
};

#endif
