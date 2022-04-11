/* InterfaceList.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.12 13-Mar-2003 18:15 rob:)
 */

#ifndef INCLUDED_INTERFACELIST_H
#define INCLUDED_INTERFACELIST_H 1

#ifndef INCLUDED_IPADDRESS_H
#include "ipaddress.h"
#endif

class InterfaceInfo
{
    unsigned int m_flags; // IFF_*
    IPAddress m_addr;
    IPAddress m_broadcast;
    IPAddress m_netmask;
    std::string m_name;
    int m_index;

public:
    InterfaceInfo(unsigned int flags, const IPAddress &addr, const IPAddress &broadcast, 
                  const IPAddress &netmask, const std::string & name, const int index)
	: m_flags(flags), m_addr(addr), m_broadcast(broadcast), m_netmask(netmask), m_name(name), m_index(index)
    {
    }

    unsigned int GetFlags() const { return m_flags; }
    IPAddress GetAddress() const { return m_addr; }
    IPAddress GetBroadcastAddress() const { return m_broadcast; }
    IPAddress GetNetmask() const { return m_netmask; }
    std::string GetName() const { return m_name; }
    int GetIndex() const { return m_index; }

    bool operator== (const InterfaceInfo &rhs) const
    {
	return GetAddress() == rhs.GetAddress();
    }

    bool operator< (const InterfaceInfo &rhs) const
    {
	return GetAddress() < rhs.GetAddress();
    }
};

class InterfaceList : public std::vector<InterfaceInfo>
{
public:
    static InterfaceList GetInterfaceList();
};

#endif /* INCLUDED_INTERFACELIST_H_ */
