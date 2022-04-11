/* playerdb.h
 *
 * Representation of contents of player
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * NOTE: We hope to replace this code with a new player model library, shared
 * with emplode, at some point in the future.
 *
 * (:Empeg Source Release 1.11 01-Apr-2003 18:52 rob:)
 */

#ifndef PLAYERDB_H
#define PLAYERDB_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdarg.h>

#include "database_error.h"
#include "database_tags.h"
#include "node_tags.h"

#include "fids.h"
#include "connection.h"
#include "protocolclient.h"
#include "dynamic_config_file.h"
#include "dyndata_format.h"

// avoid having to use std:: on everything in the universe

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// Player database access class
//////////////////////////////////////////////////////////////////////////////

class FidNode;
class FidPlaylist;
class DatabaseProgressListener;

// inherit observer methods
class PlayerDatabase : public TuneDatabaseObserver, public ProtocolObserver
{
 private:
    // mapping of FIDs to their allocated FidNode's
    class FidNodeMap : public map<FILEID, FidNode *> {
     public:
	FidNode *GetNode(FILEID fid) const throw(); // get FidNode for a FID
	bool AddNode(FILEID fid, FidNode *node);    // add a fid
						    // return true if unique
	bool RemoveNode(FILEID fid) throw();	    // remove a fid
						    // return true if exists
        FILEID FindFree() const throw();	    // find first free FID
	void Clear();
    };

    ProtocolClient &client;			// the protocol interface
    DatabaseProgressListener *progress;		// progress listener
    DynamicConfigFile *dcf;
    bool config_changed;

    bool debug_database, manual_mode;		// debug
    bool dump_mode, debug_connection;		// debug
    int debug_level;				// yet more bloody debug

    DatabaseTags tag_names;			// all database tags
    FidNodeMap all_fids;			// all fids in the database

    // Remove old tag names
    void ClearTags() { tag_names.Clear(); }
    // Remove all FIDs from the database, clear up memory
    void RemoveFids() { all_fids.Clear(); }

    int CheckRootLists();		    // check root + unattached correct

    STATUS LogFreeSpace();

 public:
    PlayerDatabase(ProtocolClient &client,
		   DatabaseProgressListener *progress);
    virtual ~PlayerDatabase();

    // lots of things need access to this
    ProtocolClient &GetClient() const throw() { return client; }
    DatabaseProgressListener *GetProgress() const throw() { return progress; }

    // the magic config.ini fid
    string GetConfig();
    void SetConfigValue( string section, string key, string value );

    // fid management
    FidPlaylist *GetRootPlaylist() const throw();
    FidNode *GetNode(FILEID fid) const throw() { return all_fids.GetNode(fid); }
    FidPlaylist *GetPlaylist(FILEID fid) const throw();
    FILEID FindFreeFid() const throw() { return all_fids.FindFree(); }

    // creation functions
    FidNode *ReadFid(FILEID fid, const NodeTags &tags)
	throw(bad_alloc, DatabaseError::UnknownFidType);

    // tag management
    int GetTagNumber(const char *name) { return tag_names.GetNumber(name); }	// not const!
    const char *GetTagName(int tag) const { return tag_names.GetName(tag); }
    int AddTag(const char *name) { return tag_names.GetNumber(name); }
    DatabaseTags &GetTagNames() { return tag_names; }
    void EnsureBasicTags(); // make sure length, type, title exist
    
    // quick index
    int GetTypeTag() const { return tag_names.GetTypeTag(); }
    int GetTitleTag() const { return tag_names.GetTitleTag(); }

    // should ONLY be called from FidNode::constructor/destructor
    bool AddFid(FILEID fid, FidNode *node) {
	return all_fids.AddNode(fid, node);
    }
    int RemoveFid(FILEID fid) {
	return all_fids.RemoveNode(fid);
    }

    // major actions
    STATUS Download(bool rebuild_on_fail = true) throw(bad_alloc);		// download database
    int Repair(bool do_repair);		// repair everything (optionally)
    STATUS Synchronise();		// synchronise with player
    STATUS RebuildDatabase();		// rebuild just the database

    // debugging
    void SetDebugLevel(int level) { debug_level = level; }

    // callbacks from protocol client in Download()
    virtual STATUS TagIndexAction(int index, const char *name);
    virtual STATUS TagAddAction(const char *name);
    virtual STATUS DatabaseEntryAction(FILEID fid,			// i dont like BYTE *
				       const BYTE *static_entries,	// but never mind
				       const DynamicData *dynamic_entries);

    // from ProtocolObserver - connection activity updates from ProtocolClient
    virtual void ReportProgress(ProtocolClient *client,
				ProtocolClient::Activity a,
				int current, int maximum);
    virtual void ReportWarning(ProtocolClient *client,
			       ProtocolClient::Warning w);
};

//////////////////////////////////////////////////////////////////////////////
// FidNode -- the base class of each node, every FID should have one of these
//////////////////////////////////////////////////////////////////////////////

class FidNode
{
#if DEBUG>0
    // for finding buffer overruns (ugly)
    enum { MAGIC = 0xf1d00f1d };
    unsigned int magic;
#endif

 protected:
    PlayerDatabase &db;				// ref to PlayerDatabase
    FILEID fid;					// the FID of this node
    int dirty;					// mod count, >0 = needs sync
    int ref_count;				// reference count
    NodeTags tags;				// tags
    DynamicData dyndata;                        // plays count etc.
    
 public:
    FidNode(PlayerDatabase &db, FILEID fid)
	: db(db), fid(fid), dirty(0), ref_count(0), tags(&db.GetTagNames()) {
#if DEBUG>0
	magic = MAGIC;
#endif
	db.AddFid(fid, this);					// add this node to db.all_fids
    }
    FidNode(PlayerDatabase &db, FILEID fid, const NodeTags &tags)
	: db(db), fid(fid), dirty(0), ref_count(0), tags(tags) {
#if DEBUG>0
	magic = MAGIC;
#endif
	db.AddFid(fid, this);
    }

    // access to variables
    FILEID GetFid() const { return fid; }			// return the fid of this node
    bool IsDirty() const { return dirty > 0; }			// marked only via member calls
    void MarkDirty() { dirty ++; };				// update on sync
    void IncreaseReference() { ref_count++; }			// increase reference count
    void RemoveReference() {					// referenced one less
	ref_count--;
	if(ref_count < 0) throw DatabaseError::RefCountBelowZero(fid);
    }
    void ResetReferences() { ref_count = 0; }			// danger will robinson
    bool IsReferenced() { return ref_count > 0; }		// are we referenced?
    int GetReferenceCount() { return ref_count; }		// get reference count
    
    // access to tags
    const char *GetTag(int tag) const { return tags.GetValue(tag); }
    void SetTag(int tag, const char *value) {
	dirty ++;						// set dirty!
	return tags.SetValue(tag, value);
    }
    int WhichTags(vector<int> &which_tags) const {		// another wrapper
	return tags.WhichTags(which_tags);
    }
    const NodeTags &GetTags() const { return tags; }
    
    // quick stuff
    const char *GetType() const { return tags.GetType(); }
    const char *GetTitle() const { return tags.GetTitle(); }
    void SetTitle(const char *title) { tags.SetTitle(title); }
    
    // derived classes implement these
    virtual int Repair(bool do_repair) = 0;	// repair this node
    virtual STATUS Synchronise() = 0;		// possibly synchronise this node
    
    virtual ~FidNode() {
	ASSERT(magic == MAGIC);
	db.RemoveFid(fid);					// remove this node from all_fids
#if DEBUG>0
	magic = ~MAGIC;
#endif
    }
};

//////////////////////////////////////////////////////////////////////////////
// FidPlaylist -- Playlists (derived from FidNode)
//////////////////////////////////////////////////////////////////////////////

class FidPlaylist : public FidNode
{
 protected:
    // access through member functions only - so we can mark 'dirty' when changed
    vector<FILEID> fids;
    void RemoveIterator(vector<FILEID>::iterator &i);	// iterator invalidation problems

 public:
    FidPlaylist(PlayerDatabase &db)				// new playlist from scratch
	: FidNode(db, db.FindFreeFid()) { }
    FidPlaylist(PlayerDatabase &db, FILEID fid,			// from provided values
		const NodeTags &tags, const vector<FILEID> &fids = vector<FILEID>())
	: FidNode(db, fid, tags), fids(fids) { }

    // construction
    static FidPlaylist *CreateFromFid(PlayerDatabase &db, const NodeTags &tags, FILEID fid);

    int size() const throw() { return fids.size(); }
    void UpdateLength() {				// make length consistent
	// int oldlength = atol(GetTag(db.GetTagNumber("length")));
	char tmp[16];
	sprintf(tmp, "%d", fids.size()*sizeof(FILEID));
	tags.SetValue(db.GetTagNumber("length"), tmp);
	// printf("UpdateLength %s: %d->%d\n", GetTitle(), oldlength, fids.size()*sizeof(FILEID));
    }

    // populate playlist (but don't mark as dirty)
    void Populate(const vector<FILEID> &from_fids) {
	fids = from_fids;
	UpdateLength();
    }
    
    // implementation of pure virtual functions from FidNode
    virtual int Repair(bool do_repair);		// repair current (recurse)
    virtual STATUS Synchronise();		// synchronise playlist

    // directory commands
    int CreatePlaylist(const char *name, bool inherit = true, FILEID child = 0);
                       // create a playlist
    int Upload(const char *name, const char *path);		// upload a file

    // node list management
    void AddNode(FidNode *node);				// add a node to the list
    void RemoveFid(FILEID fid) {				// remove all nodes with fid
	vector<FILEID>::iterator it;
	for(;;) {
	    it = find(fids.begin(), fids.end(), fid);
	    if(it == fids.end()) break;
	    fids.erase(it);
	}
	UpdateLength();
    }
    void RemoveNode(int pos) {					// position is unique identifier
	vector<FILEID>::iterator i = fids.begin() + pos;
	RemoveIterator(i);
    }
    FILEID operator[](size_t pos) const {			// operator[]
	if ( pos >= fids.size() )
	    return 0;		// validate against bounds
	else
	    return fids[pos];
    }
    FidNode *GetNodeAt(size_t pos) const {			// as above, but grab FidNode
	return db.GetNode((*this)[pos]);
    }
    FidPlaylist *GetPlaylistAt(size_t pos) const {		// as above, but Playlist
	return dynamic_cast<FidPlaylist *>(GetNodeAt(pos));
    }
    int HasName(const char *name) const {
	for(int i=0; i<size(); i++) {
	    if(!strcmp(GetNodeAt(i)->GetTitle(), name)) return i;
	}
	return -1;
    }

    // consistency checking
    FidPlaylist *CheckNoLoops(vector<FILEID> &visits, FILEID *which_ref);	// check loops
    bool ContainsFid(FILEID fid) {				// check containing a fid
	return find(fids.begin(), fids.end(), fid) != fids.end();
    }
};

// another 'backward' declaration

inline FidPlaylist *PlayerDatabase::GetRootPlaylist() const throw()
{
    return dynamic_cast<FidPlaylist *>(GetNode(FID_ROOTPLAYLIST));
}

inline FidPlaylist *PlayerDatabase::GetPlaylist(FILEID fid) const throw()
{
    return dynamic_cast<FidPlaylist *>(GetNode(fid));
}

//////////////////////////////////////////////////////////////////////////////
// Tunes which are on the player -- "Remote Tunes"
//////////////////////////////////////////////////////////////////////////////

class FidRemoteTune : public FidNode
{
 public:
    FidRemoteTune(PlayerDatabase &db, FILEID fid)
	: FidNode(db, fid) { }
    FidRemoteTune(PlayerDatabase &db, FILEID fid, const NodeTags &tags)
	: FidNode(db, fid, tags) { }

    // construction
    static FidRemoteTune *CreateFromFid(PlayerDatabase &db, const NodeTags &tags, FILEID fid);
    
    // implementation of FidNode pure virtual methods
    virtual int Repair(bool) { return 0; }	// much ado 'bout nothing
    virtual STATUS Synchronise();		// sync in tags have changed
};

//////////////////////////////////////////////////////////////////////////////
// Files which are marked for upload -- "Local Files"
//////////////////////////////////////////////////////////////////////////////

class FidLocalFile : public FidNode
{
 protected:
    string path;
    
    typedef map<string,FidLocalFile*> map_t;
    static map_t theMap;

 public:
    FidLocalFile(PlayerDatabase &db, const char *path, const NodeTags &tags)
	: FidNode(db, db.FindFreeFid(), tags), path(path) { MarkDirty(); }
    // seeing as it's always dirty, the tags can be set after construction
    
    ~FidLocalFile();

    static FidLocalFile *Create(PlayerDatabase& db, const char *path,
				const NodeTags& tags);
    
    virtual int Repair(bool) { return 0; }	// TODO - check file exists
    virtual STATUS Synchronise();		// upload
};

//////////////////////////////////////////////////////////////////////////////
// Nodes which are marked for deletion are replaced with one of these
//////////////////////////////////////////////////////////////////////////////

class FidDeleted : public FidNode
{
 public:
    FidDeleted(PlayerDatabase &db, FILEID fid, const NodeTags &tags)
	: FidNode(db, fid, tags) { MarkDirty(); }		// we're always dirty!

    virtual int Repair(bool) { return 0; }	// not much you can repair here
    virtual STATUS Synchronise();		// this causes the deletion
};

#endif

