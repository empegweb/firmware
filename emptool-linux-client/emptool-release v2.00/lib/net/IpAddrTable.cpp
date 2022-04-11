/* IpAddrTable.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "IpAddrTable.h"
#include "ipaddress.h"

IpAddrTable::IpAddrTable(char *pBuff)
    : m_pBuff(pBuff)
{
}

IpAddrTable::~IpAddrTable()
{
    delete [] m_pBuff;
}

IpAddrTable IpAddrTable::GetIpAddrTable()
{
    ULONG cbBuff = 0;
    char *pBuff = NULL;
    DWORD result = ::GetIpAddrTable((MIB_IPADDRTABLE *)pBuff, &cbBuff, TRUE);
    if(result == ERROR_INSUFFICIENT_BUFFER)
    {
	pBuff = NEW char[cbBuff];
	result = ::GetIpAddrTable((MIB_IPADDRTABLE *)pBuff, &cbBuff, TRUE);
    }

    if(result != NO_ERROR)
    {
	delete [] pBuff;
	return IpAddrTable(NULL);
    }

    return IpAddrTable(pBuff);
}

IpAddrTable::const_iterator::const_iterator(const MIB_IPADDRTABLE *table, bool end)
    : table(table), index(end ? table->dwNumEntries : 0)
{
}

const MIB_IPADDRROW * IpAddrTable::const_iterator::operator -> () const
{
    return &table->table[index];
}

const MIB_IPADDRROW & IpAddrTable::const_iterator::operator * () const
{
    return table->table[index];
}

IpAddrTable::const_iterator & IpAddrTable::const_iterator::operator ++ ()
{
    if(index < table->dwNumEntries)
	++index;

    return *this;
}

bool IpAddrTable::const_iterator::operator == (const IpAddrTable::const_iterator & rhs) const
{
    return index == rhs.index;
}

IpAddrTable::const_iterator IpAddrTable::find(const IPAddress & addr) const
{
    for(const_iterator i = begin(); i != end(); ++i)
    {
	if(addr.ToNetworkLong() == i->dwAddr)
	    return i;
    }

    return end();
}
