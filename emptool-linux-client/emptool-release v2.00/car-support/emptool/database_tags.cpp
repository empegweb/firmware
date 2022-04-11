/* database_tags.cpp
 *
 * (C) 1999-2000 empeg ltd.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#include "core/config.h"
#include "database_tags.h"
#include "trace.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// class DatabaseTags
//////////////////////////////////////////////////////////////////////////////

const char *DatabaseTags::TagsByNumber::GetName(int tag) const
{
    const_iterator i = find(tag);
    if(i != end()) return i->second.c_str();
    else
    { 
	return NULL;
    }
}

void DatabaseTags::TagsByNumber::SetName(int tag, const char *name)
{
    iterator i = find(tag);
    if(i != end()) erase(i);
    if(name) {
	if(*name) insert(end(), make_pair(tag, string(name)));
    }
}

int DatabaseTags::TagsByNumber::FindFree() const throw()
{
    int i;
    for(i=0; ; i++) if(find(i) == end()) break;
    return i;
}

int DatabaseTags::TagsByName::GetNumber(const char *name) const throw()
{
    const_iterator i = find(string(name));
    if (i != end())
	return i->second;

    return -1;
}

void DatabaseTags::TagsByName::SetNumber(const char *name, int tag)
{
    iterator i = find(string(name));
    if(i != end()) erase(i);
    if(tag != -1) {
	insert(end(), make_pair(string(name), tag));
    }
    if(!strcmp(name, "type")) type_tag = tag;
    else if(!strcmp(name, "title")) title_tag = tag;
}

void DatabaseTags::SetName(int tag, const char *name)
{
    tags_by_number.SetName(tag, name);
    tags_by_name.SetNumber(name, tag);
}

int DatabaseTags::GetNumber(const char *name)
{		// can't guarentee const
    int n = tags_by_name.GetNumber(name);		// adds if doesn't exist
    if(n != -1) return n;
    n = tags_by_number.FindFree();
    SetName(n, name);					// e.g
    return n;
}

int DatabaseTags::AddTag(const char *name)
{
    int n = tags_by_number.FindFree();
    SetName(n, name);
    return n;
}
