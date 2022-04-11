/* node_tags.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#ifndef NODE_TAGS_H
#define NODE_TAGS_H

#include <map>
#include <vector>
#include <string>

#include "types.h"

class FidNode;	// forward declaration
class FidPlaylist;
class DatabaseTags;

//////////////////////////////////////////////////////////////////////////////
// Each Node has two of these - one for static, one for dynamic
//////////////////////////////////////////////////////////////////////////////

// my god it's a private inheritance (don't tell the government!)
class NodeTags : private std::map<int, std::string> {
    DatabaseTags *db_tags;
    
 public:
    NodeTags(DatabaseTags *db_tags)
	: db_tags(db_tags) { }
    
    // interface to std::map
    const char *GetValue(int tag) const;
    void SetValue(int tag, const char *value);

    // entries come from protocol_client in 0-pascal string-1-...-255(end) style array
    void AddEntries(const BYTE *entries);			// add initial entries
    int GetTableLength() const throw();				// get length of table when stored
    void WriteTable(BYTE *entries) const;			// write out the table

    const char *GetType() const throw();
    const char *GetTitle() const throw();
    void SetTitle(const char *title);

    int WhichTags(std::vector<int> &tag_numbers) const;
};

#endif
