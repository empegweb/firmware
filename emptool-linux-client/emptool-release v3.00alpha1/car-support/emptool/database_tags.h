/* database_tags.h
 *
 * Collection of all database tags, access through interface
 *
 * (C) 1999-2000 empeg ltd.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#ifndef DATABASE_TAGS_H
#define DATABASE_TAGS_H

#include <map>
#include <string>
#include <stdexcept>

#ifndef TRACE_H
#include "trace.h"
#endif

class DatabaseTags
{
    // map from tag number to its name, interface speaks for itself
    // beware the private inheritance! probably better as a std::vector
    class TagsByNumber : private std::map<int, std::string> {
     public:
	const char *GetName(int tag) const;
	void SetName(int tag, const char *name);
	int FindFree() const throw();
	void Clear() { clear(); };
    };

    // map from tag name to its number, interface speaks for itself
    // yes, a hash map would be faster
    class TagsByName : private std::map<std::string, int> {
	int type_tag;			// quick index to common "type"
	int title_tag;			// quick index to common "title"
     public:
	explicit TagsByName(const TagsByName &);
	
	TagsByName() throw(std::bad_alloc)
	    : type_tag(-1), title_tag(-1) { }
	
	int GetNumber(const char *name) const throw();
	void SetNumber(const char *name, int tag);
	int GetTypeTag() const throw() { ASSERT(type_tag!=-1); return type_tag; }
	int GetTitleTag() const throw() { ASSERT(title_tag!=-1); return title_tag; }
	void Clear() { clear(); }
    };

    TagsByNumber tags_by_number;
    TagsByName tags_by_name;
 public:
    DatabaseTags() { }
    explicit DatabaseTags(const DatabaseTags &);
    
    // interface to above classes (this gets inlined out to no code)
    const char *GetName(int tag) const throw() { return tags_by_number.GetName(tag); }
    void SetName(int tag, const char *name);
    int GetNumber(const char *name);				// can't guarentee const
    void SetNumber(const char *name, int tag) { SetName(tag, name); }
    int AddTag(const char *name);

    // quick index
    int GetTypeTag() const throw() { return tags_by_name.GetTypeTag(); }
    int GetTitleTag() const throw() { return tags_by_name.GetTitleTag(); }

    // clear
    void Clear() {
	tags_by_number.Clear();
	tags_by_name.Clear();
    }
};

#endif
