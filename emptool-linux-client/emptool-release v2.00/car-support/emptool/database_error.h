/* database_error.h
 *
 * DatabaseError - namespace of exceptions generated by any database class
 *
 * (C) 1999-2000 empeg ltd.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 */

#ifndef DATABASE_ERROR_H
#define DATABASE_ERROR_H

#include "node_tags.h"
#include "packets.h"

using namespace std;

#define DATABASE_ASSERT(pred) \
	{ if(!(pred)) throw DatabaseError::InternalError(__FILE__, __LINE__); }

namespace DatabaseError
{
    // basic class dealing with FID errors
    class FidError {
	FILEID fid;
	int result;
     public:
	FidError(FILEID fid, int result = 0) : fid(fid), result(result) { }
	FILEID GetFid() const throw() { return fid; }
	int GetResult() const throw() { return result; }
    };
    // basic class dealing with FID Tags errors
    class FidTagsError : public FidError {
	NodeTags tags;
     public:
	FidTagsError(FILEID fid, const NodeTags &tags, int result = 0)
	    : FidError(fid, result), tags(tags) { }
	const NodeTags &GetTags() const throw() { return tags; }
    };

    // Fid was not found (not returned by GetNode() - done as an error if NULL unexpected)
    class FidNotFound : public FidError {
     public: FidNotFound(FILEID fid, int result = 0) : FidError(fid, result) { }
    };
    // Fid was already in use
    class FidInUse : public FidError {
     public: FidInUse(FILEID fid, int result = 0) : FidError(fid, result) { }
    };
    // Fid does not exist
    class InvalidFid : public FidError {
     public: InvalidFid(FILEID fid, int result = 0) : FidError(fid, result) { }
    };
    // Couldn't retrieve tags for fid|0
    class ReadTagsFailed : public FidError {
     public: ReadTagsFailed(FILEID fid, int result = 0) : FidError(fid, result) { }
    };
    // Fid has unknown type
    class UnknownFidType : public FidTagsError {
     public:
	UnknownFidType(FILEID fid, const NodeTags &tags, int result = 0) :
	    FidTagsError(fid, tags, result) { }
    };
    // Reference count dropped below zero
    class RefCountBelowZero : public FidError {
     public: RefCountBelowZero(FILEID fid) : FidError(fid) { }
    };

    // unexpected
    class NoFreeFids { };

    // Can't do much if we don't have a root or unattached items
    class InvalidRoot { };
    class InvalidUnattached { };

    // Assertion failure (stuff that's too silly to think about writing code for)
    class InternalError {
	const char *file;
	int line;
     public:
	InternalError(const char *file, int line)
	    : file(file), line(line) { }

	const char *GetFile() const { return file; }
	int GetLine() const { return line; }
    };
};

#endif
