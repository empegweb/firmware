/* IpAddrTable.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#ifndef IPADDRTABLE_H
#define IPADDRTABLE_H 1

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iphlpapi.h>

class IPAddress;

class IpAddrTable
{
    char *m_pBuff;

private:
    explicit IpAddrTable(char *pBuff);

public:
    ~IpAddrTable();
    static IpAddrTable GetIpAddrTable();

    class const_iterator {
	DWORD index;
	const MIB_IPADDRTABLE *table;

	const_iterator(const MIB_IPADDRTABLE *pTable, bool end);
	friend class IpAddrTable;
    public:
	bool operator == (const const_iterator & rhs) const;
	bool operator != (const const_iterator & rhs) const
	{ return !operator==(rhs); }
	const_iterator & operator++();
	const MIB_IPADDRROW *operator->() const;
	const MIB_IPADDRROW &operator*() const;
    };

    const_iterator begin() const
    { return const_iterator((const MIB_IPADDRTABLE *)m_pBuff, false); }
    const_iterator end() const
    { return const_iterator((const MIB_IPADDRTABLE *)m_pBuff, true); }
    const_iterator find(const IPAddress & addr) const;
};

#endif /* IPADDRTABLE_H */
