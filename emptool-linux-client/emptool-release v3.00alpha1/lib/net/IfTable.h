/* IfTable.h
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

#ifndef IFTABLE_H
#define IFTABLE_H 1

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iphlpapi.h>

/* Example:
 * IfTable t = IfTable::GetIfTable();
 * for (IfTable::const_iterator i = t.begin(); i != t.end(); ++i)
 * {
 *  printf("Adapter %d : %s\n", i->dwIndex, i->bDescr);
 * }
 */
class IfTable
{
    char *m_pBuff;

private:
    explicit IfTable(char *pBuff);
    const MIB_IFTABLE *AsTable() const { return (const MIB_IFTABLE *)m_pBuff; }

public:
    ~IfTable();
    static IfTable GetIfTable();

    class const_iterator {
	DWORD index;
	const MIB_IFTABLE *table;

	const_iterator(const MIB_IFTABLE *pTable, bool end);
	friend class IfTable;
    public:
	bool operator == (const const_iterator & rhs) const;
	bool operator != (const const_iterator & rhs) const
	{ return !operator==(rhs); }
	const_iterator & operator++();
	const MIB_IFROW *operator->() const;
	const MIB_IFROW &operator*() const;
    };

    const_iterator begin() const
    {
	return const_iterator(AsTable(), false);
    }

    const_iterator end() const
    {
	return const_iterator(AsTable(), true);
    }

    bool empty() const
    {
	if(AsTable() && AsTable()->dwNumEntries > 0)
	    return false;

	return true;
    }
};

#endif /* IFTABLE_H */
