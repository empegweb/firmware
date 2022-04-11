/* node_tags.cpp
 *
 * Class NodeTags -- list of tags for a node
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "core/config.h"
#include "node_tags.h"
#include "database_tags.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// class NodeTags -- list of tags for a node
//////////////////////////////////////////////////////////////////////////////

const char *NodeTags::GetValue(int tag) const
{
    const_iterator i = find(tag);
    return (i == end()) ? "" : i->second.c_str();
}

void NodeTags::SetValue(int tag, const char *value)
{
    //printf("NodeTags::SetValue(%d, %s)\n", tag, value);
    iterator i = find(tag);
    if(i != end()) erase(i);
    if(value) {
	if(*value) insert(make_pair(tag, string(value)));
    }
}

// Add entries to the tags list from specified 0-pascal string-...-255 structure

void NodeTags::AddEntries(const BYTE *entries)
{
    char tmp[257];
    while(*entries != 255) {
	int type = *entries++;				// type first
	int len = *entries++;				// then length of name
	for(int i=0; i<len; i++) {			// followed by the name
	    tmp[i] = *entries++;
	}
	if(!len) continue;				// don't bother with blank tags
	
	tmp[len] = 0;
	insert(end(), make_pair(type, string(tmp)));
    }
}

int NodeTags::GetTableLength() const throw()
{
    int len = 0;
    for(const_iterator it = begin(); it != end(); it++) {
	if(!it->second.size()) continue;		// don't output blanks
	
	int tag = it->first;
	const char *tagname = db_tags->GetName(tag);
	if ( !strcmp(tagname, "PlayCount")
	     || !strcmp(tagname, "SkippedCount")
	     || !strcmp(tagname, "LastPlayed")
	     || !strcmp(tagname, "Marked") )
	    continue;

	len += strlen(tagname);		                // "tag"
	len++;						// "="
	len += it->second.size();			// "value"
	len++;						// "\n"
    }
    return len;
}

void NodeTags::WriteTable(BYTE *text) const
{
    char *s = (char *) text;
    for(const_iterator it = begin(); it != end(); it++) {
	if(!it->second.size()) continue;		// don't output blank tags

	int tag = it->first;
	const char *tagname = db_tags->GetName(tag);
	if ( !strcmp(tagname, "PlayCount")
	     || !strcmp(tagname, "SkippedCount")
	     || !strcmp(tagname, "LastPlayed")
	     || !strcmp(tagname, "Marked") )
	    continue;

	int len = strlen(tagname);

	int i;
	for(i=0; i<len; i++) *s++ = tagname[i];	// "tag"

	*s++ = '=';					// "="

	string value = it->second;
	
	for(i=0; (unsigned) i<value.size(); i++) *s++ = value[i];	// "value"

	*s++ = '\n';					// "\n"
    }
}

const char *NodeTags::GetType() const throw()
{
    return GetValue(db_tags->GetTypeTag());
}

const char *NodeTags::GetTitle() const throw()
{
    return GetValue(db_tags->GetTitleTag());
}

void NodeTags::SetTitle(const char *title)
{
    SetValue(db_tags->GetTitleTag(), title);
}

int NodeTags::WhichTags(vector<int> &tag_numbers) const
{
    for(const_iterator it = begin(); it != end(); it++) {
	tag_numbers.push_back(it->first);
    }
    return size();
}

