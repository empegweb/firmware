/* IfTable.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "IfTable.h"

IfTable::IfTable(char *pBuff)
    : m_pBuff(pBuff)
{
}

IfTable::~IfTable()
{
    delete [] m_pBuff;
}

IfTable IfTable::GetIfTable()
{
    ULONG cbBuff = 0;
    char *pBuff = NULL;
    DWORD result = ::GetIfTable((MIB_IFTABLE *)pBuff, &cbBuff, TRUE);
    if(result == ERROR_INSUFFICIENT_BUFFER)
    {
	pBuff = NEW char[cbBuff];
	result = ::GetIfTable((MIB_IFTABLE *)pBuff, &cbBuff, TRUE);
    }

    if(result != NO_ERROR)
    {
	delete [] pBuff;
	return IfTable(NULL);
    }

    return IfTable(pBuff);
}

IfTable::const_iterator::const_iterator(const MIB_IFTABLE *table, bool end)
    : table(table), index(end ? table->dwNumEntries : 0)
{
}

const MIB_IFROW * IfTable::const_iterator::operator -> () const
{
    return &table->table[index];
}

const MIB_IFROW & IfTable::const_iterator::operator * () const
{
    return table->table[index];
}

IfTable::const_iterator & IfTable::const_iterator::operator ++ ()
{
    if(index < table->dwNumEntries)
	++index;

    return *this;
}

bool IfTable::const_iterator::operator == (const IfTable::const_iterator & rhs) const
{
    return index == rhs.index;
}
