/* playerdb.cpp
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
 * (:Empeg Source Release 1.22 13-Mar-2003 18:15 rob:)
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <glob.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "trace.h"
#include "tags/tag_extractor.h"
#include "wildcard.h"

#include "playerdb.h"
#include "emptool.h"
#include "numerics.h"
#include "var_string.h"

#include "database_progress_listener.h"
#include "emptool_error.h"

#define CONNECT_TIMEOUT		10
#define RESTART_TIMEOUT		30

//////////////////////////////////////////////////////////////////////////////
// class PlayerDatabase -- container for all things on the player
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// subclass PlayerDatabase::FidNodeMap
//////////////////////////////////////////////////////////////////////////////

FidNode *PlayerDatabase::FidNodeMap::GetNode(FILEID fid) const throw()
{					// get FidNode for a FID
    const_iterator i = find(fid);

    return (i == end()) ? NULL : i->second;
}

bool PlayerDatabase::FidNodeMap::AddNode(FILEID fid, FidNode *node)
{					// add a fid
    if(find(fid) != end()) return false;		// return true if unique

    insert(end(), make_pair(fid, node));
    return true;
}

bool PlayerDatabase::FidNodeMap::RemoveNode(FILEID fid) throw()
{					// remove a fid
    iterator i = find(fid);				// return true if exists
    if(i == end()) return false;
    erase(i);
    return true;
}

FILEID PlayerDatabase::FidNodeMap::FindFree() const throw()
{					// find first free FID
    int fid;
    for(fid=FID_FIRSTNORMAL; ; fid+=0x10) if(find(fid) == end()) break;
    return fid;
}

void PlayerDatabase::FidNodeMap::Clear()
{
    while(begin() != end()) {
	delete begin()->second;	// FidNode destructor calls RemoveFid()
    }
    clear();			// therefore should already be clear, but never mind
}

// Constructor

PlayerDatabase::PlayerDatabase(ProtocolClient &client,
			       DatabaseProgressListener *progress)
    : client(client),
      progress(progress),
      dcf(0),
      config_changed(false),
      debug_database(false),
      manual_mode(false),
      dump_mode(false),
      debug_connection(false),
      debug_level(0)
{
    GetClient().SetObserver(this);

    // Insert the standard 
}

// Destructor, clean up remaining fids if any

PlayerDatabase::~PlayerDatabase()
{
    GetClient().SetObserver(NULL);
    // Delete all fids - otherwise we memory leak
    RemoveFids();
}

// Check root and unattached items playlists are valid, present and connected

int PlayerDatabase::CheckRootLists()
{
    int retval = 0;

    // check root playlist
    FidNode *root_node = GetNode(FID_ROOTPLAYLIST);	// dont do GetRootPlaylist() - dont cast
    if(!root_node) {					// does it exist?
	progress->Log(Numerics::No_root_playlist,
		     "No root playlist, creating new one\n");

	retval ++;
	
	NodeTags tags(&GetTagNames());

	EnsureBasicTags();
	tags.SetValue(GetTypeTag(), "playlist");
	tags.SetValue(GetTitleTag(), "empeg-car");
	
	FidPlaylist *playlist = NEW FidPlaylist(*this, FID_ROOTPLAYLIST, tags);
	DATABASE_ASSERT(playlist);

	playlist->MarkDirty();				// update on sync
	playlist->IncreaseReference();			// always referenced

	root_node = playlist;
    }
    else if(!dynamic_cast<FidPlaylist *>(root_node)) {	// is it a playlist?
	progress->Log(Numerics::Invalid_root_playlist,
		     "Invalid root playlist, creating new one\n");

	delete root_node;				// remove old one from database

	retval ++;
	
	NodeTags tags(&GetTagNames());

	EnsureBasicTags();
	tags.SetValue(GetTypeTag(), "playlist");
	tags.SetValue(GetTitleTag(), "empeg-car");
	
	FidPlaylist *playlist = NEW FidPlaylist(*this, FID_ROOTPLAYLIST, tags);
	DATABASE_ASSERT(playlist);
	
	playlist->UpdateLength();
	playlist->MarkDirty();				// update on sync
	playlist->IncreaseReference();			// always referenced

	root_node = playlist;
    }

    // convert root node to a root playlist
    FidPlaylist *root_playlist = dynamic_cast<FidPlaylist *>(root_node);

    // check lost+found
    FidNode *lost_node = GetNode(FID_LOSTANDFOUND);
    if(!lost_node) {					// does it exist?
	progress->Log(Numerics::No_unattached_items_playlist,
		     "No 'unattached items' playlist, creating new one\n");

	retval ++;

	FidPlaylist *root_playlist = dynamic_cast<FidPlaylist *>(root_node);
	DATABASE_ASSERT(root_playlist);

	// create a new unattached items
	root_playlist->CreatePlaylist("Unattached Items", false, FID_LOSTANDFOUND);
	
	GetNode( FID_LOSTANDFOUND )->MarkDirty();
    }
    else if(!dynamic_cast<FidPlaylist *>(lost_node)) {
	progress->Log(Numerics::Invalid_unattached_items_playlist,
		     "Invalid 'unattached items' playlist, creating new one\n");

	delete lost_node;				// get rid of the old one

	retval ++;
	
	FidPlaylist *root_playlist = dynamic_cast<FidPlaylist *>(root_node);
	DATABASE_ASSERT(root_playlist);

	// create a new unattached items
	root_playlist->CreatePlaylist("Unattached Items", false, FID_LOSTANDFOUND);

    }

    // check unattached items is attached to root playlist
    if(!root_playlist->ContainsFid(FID_LOSTANDFOUND)) {
	progress->Log(Numerics::Unattached_unattached_items_playlist,
		     "Unattached 'unattached items' playlist, attaching\n");

	retval ++;
	
	root_playlist->AddNode(lost_node);		// add it
    }

    // done!
    
    return retval;
}

// Read a FID from the player and allocate an appropriate node for it (tune/playlist)

FidNode *PlayerDatabase::ReadFid(FILEID fid, const NodeTags &tags)
    throw(bad_alloc, DatabaseError::UnknownFidType)
{
    // Create a Node depending on its type
    const char *type = tags.GetValue(GetTypeTag());
    if(!strcmp(type, "tune")) {
	return FidRemoteTune::CreateFromFid(*this, tags, fid);	// create a tune
    }
    else if(!strcmp(type, "playlist")) {
	return FidPlaylist::CreateFromFid(*this, tags, fid);	// create a playlist
    }
    else {
	throw DatabaseError::UnknownFidType(fid, tags);
    }
}

static string FormatSize( INT64 size )
{
    int i = 0;

    for ( i=0; i<4 && size > 100*1024; ++i )
    {
	size /= 1024;
    }

    return VarString::Printf( "%u%c", ((unsigned int)size),
			      " KMG"[i] );
}

STATUS PlayerDatabase::LogFreeSpace()
{
    // Get disk space available info

    ProtocolClient::DiskSpaceInfo di[3]; // third one for total
    STATUS result = client.GetDiskInfo(di, 2);

    if (FAILED(result))
    {
	progress->Error(Numerics::Unexpected_player_error,
			"GetDiskInfo returned %s",
                        FormatErrorMessage(result).c_str());

	return result;
    }
    else
    {
	di[2].size = 0;
	di[2].space = 0;

	for(int i=0; i<3; i++) {
	    if ( di[i].size )
	    {
		INT64 block_size = di[i].block_size;
		string sz   = FormatSize( di[i].size * block_size );
		string free = FormatSize( di[i].space * block_size );
		string used = FormatSize( (di[i].size - di[i].space)
					  * block_size );

		string desc = ( i<2 ) ? VarString::Printf("Drive %d",i)
		    : "= Total";

		progress->Log(Numerics::Checking_disk_space,
			      "%s size: %s, free: %s, used: %s (%d%%)\n",
			      desc.c_str(),
			      sz.c_str(),
			      free.c_str(),
			      used.c_str(),
			      (int)((double)(di[i].size - di[i].space) * 100.0
				    / di[i].size) );
		di[2].size += di[i].size;
		di[2].space += di[i].space;
		di[2].block_size = di[i].block_size;
	    }
	}
    }
    return S_OK;
}

// Called to download everything off the player

STATUS PlayerDatabase::Download(bool rebuild_on_fail) throw(bad_alloc)
{
    bool do_rebuild = false;

    LogFreeSpace();

    progress->TaskStart(Numerics::Download_start, "Starting download");
    {
	progress->TaskUpdate(0, 1);
	
	// first things first, remove old tag names
	ClearTags();
	
	// and all the old fids
	RemoveFids();
	
	progress->TaskUpdate(1, 1);
    }

    // retrieve the tags

    progress->TaskStart(Numerics::Download_retrieve_tags, "Retrieving tags index");
    {
	progress->TaskUpdate(0, 1);
	
	STATUS result = client.RetrieveTagIndex(this);
	if (FAILED(result)) {
	    // file not found is an error we can recover from, but nothing else
	    if(!client.FileNotFoundStatus(result)) {
		progress->Error(Numerics::Database_retrieve_failed,
			       "Failed to receive tag index, result %s", 
                                FormatErrorMessage(result).c_str());
		return result;
	    }
	    
            do_rebuild = true;
	    goto need_rebuild;
	}
	
	progress->TaskUpdate(1, 1);
    }

    // retrieve the database FIDs

    progress->TaskStart(Numerics::Download_retrieve_databases, "Retrieving databases");
    {
	progress->TaskUpdate(0, 1);
	
	STATUS result = client.RetrieveDatabases(this);
	if (FAILED(result)) {
	    // we can recover from a missing database, but nothing else
	    if(!client.FileNotFoundStatus(result)) {
		progress->Error(Numerics::Database_retrieve_failed,
			       "Failed to receive database, result %s", 
                               FormatErrorMessage(result).c_str());
		return result;
	    }
	    
	    do_rebuild = true;
	    goto need_rebuild;
	}

	progress->TaskUpdate(1, 1);
    }

    progress->TaskStart(Numerics::Download_retrieve_playlists, "Retrieving playlists");
    {
	progress->TaskUpdate(0, 1);

	FILEID *buffer;
	int len;
	
	STATUS result = client.ReadFidToMemory(FID_PLAYLISTDATABASE,
					       (BYTE **) &buffer, &len);
	if (FAILED(result)) {
	    // we can recover from a missing playlist database, but nothing else
	    if(!client.FileNotFoundStatus(result)) {
		progress->Error(Numerics::Database_retrieve_failed,
			       "Failed to receive database, result %s", 
                               FormatErrorMessage(result).c_str());
		return result;
	    }
	    
	    do_rebuild = true;
	    goto need_rebuild;
	}

	FILEID maxfid = 0;
	FidNodeMap::iterator it = all_fids.begin();
	for(; it != all_fids.end(); it++) {
	    if(it->first > maxfid) maxfid = it->first;
	}
	
	FILEID *buffer_start = buffer;
	FILEID *buffer_end = buffer + len / sizeof(FILEID);
	int tag_length = GetTagNumber("length");
	
	for(FILEID i=FID_ROOTPLAYLIST; i <= maxfid; i++) {
	    FidPlaylist *playlist = GetPlaylist(i);
	    if(!playlist) continue;

	    int nfids = atol(playlist->GetTag(tag_length)) / 4;
	    if(buffer_start + nfids > buffer_end) {
		progress->Error(Numerics::Error_playlist_database,
			       "Inconsistency in playlist database\n");
		client.FreeFidMemory((BYTE **) &buffer);
		do_rebuild = true;
		goto need_rebuild;
	    }
	    vector<FILEID> fids(nfids);
	    for(int j=0; j<nfids; j++) fids[j] = *buffer_start++;
	    
	    playlist->Populate(fids);
	}

	client.FreeFidMemory((BYTE **) &buffer);	// leaky leaky (dont)

	if(buffer_start != buffer_end) {
	    progress->Error(Numerics::Error_playlist_database,
			   "Inconsistency in playlist database\n");
	    do_rebuild = true;
	    goto need_rebuild;
	}
    }
    
 need_rebuild:
    // tags or database is missing?
    if(do_rebuild) {
	if(rebuild_on_fail) {
	    progress->TaskStart(Numerics::Download_building_databases,
			       "Rebuilding databases");
	    // Rebuild it!
	    progress->TaskUpdate(0, 1);
	    RebuildDatabase();
	    progress->TaskUpdate(1, 1);

	    
	    // start the whole process over
	    
	    return Download(false);			// but dont rebuild on error
	}
	else {
	    // give in
	    return E_PLAYERDB_DOWNLOAD;
	}
    }

    // Check Stuff

    CheckRootLists();
    
    return S_OK;
}

// Repair faults in the database

int PlayerDatabase::Repair(bool do_repair)
{
    // reset all references to zero to start with
    int repairs = 0;
    for(FidNodeMap::iterator j = all_fids.begin(); j != all_fids.end(); j++) {
	j->second->ResetReferences();			// we're friends with FidNode :)
    }

    progress->Debug(2, "Checking root playlist/unattached items\n");
    
    // check the root playlist and unattached items (throw passed back to caller if it occurs)
    CheckRootLists();

    // recurse the tree of playlists checking for loops
    progress->Debug(2, "Starting tree->Repair() recursion\n");
    
    repairs += GetRootPlaylist()->Repair(do_repair);

    progress->Debug(2, "Finished tree->Repair() recursion\n");
    
    // need lost+found playlist
    FidPlaylist *lost = dynamic_cast<FidPlaylist *>(GetNode(FID_LOSTANDFOUND));
    DATABASE_ASSERT(lost);

    progress->Debug(2, "Checking reference counts\n");

    // root playlist is always referenced
    GetRootPlaylist()->IncreaseReference();
    
    // loop through all fids, looking for unreferenced fids
    for(FidNodeMap::iterator i = all_fids.begin(); i != all_fids.end(); i++) {
	if(dynamic_cast<FidDeleted *>(i->second)) {
	    progress->Debug(Numerics::Checking_found_deleted,
			   "Found deleted fid %x (%s)", i->first, i->second->GetTitle());
	    continue;
	}

	progress->Debug(3, "%x:%d ", i->first, i->second->GetReferenceCount());

	if(!i->second->GetReferenceCount()) {
	    repairs ++;
	    if(do_repair) {
		progress->Log(Numerics::Repaired_unattached_item,
			     "Attached orphaned item %s (fid %x) to 'Unattached Items'\n",
			     i->second->GetTitle(), i->second->GetFid());
		lost->AddNode(i->second); // automatically sets dirty, updates ref_count
	    }
	    else {
		progress->Log(Numerics::Found_unattached_item,
			     "Found orphaned item %s (fid %x)\n",
			     i->second->GetTitle(), i->second->GetFid());
	    }
	}
    }

    progress->Debug(2, "Finished checking reference counts\n");

    // done - return number of repairs that would be/have been done
    return repairs;
}

// Synchronise with the player

STATUS PlayerDatabase::Synchronise()
{
    int total_length_k = 0;
    // not much to do at startup :)
    
    progress->TaskStart(Numerics::Sync_start, "Starting synchronise");
    progress->TaskUpdate(1, 1);
        
    // Check the connection status

    progress->TaskStart(Numerics::Sync_checking_connection, "Checking connection status");
    {
	progress->TaskUpdate(0, CONNECT_TIMEOUT);
	
	for(int j=0; j<CONNECT_TIMEOUT; j++) {
	    if(client.IsUnitConnected()) break;
	    progress->TaskUpdate(j, CONNECT_TIMEOUT);
	    sleep(1);
	}
	
	progress->TaskUpdate(CONNECT_TIMEOUT, CONNECT_TIMEOUT);
    }

    // Lock down the player so the user can't press buttons

    progress->TaskStart(Numerics::Sync_locking_player, "Locking player interface");
    {
	progress->TaskUpdate(0, 1);
	
	STATUS result = client.LockUI(true);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_lock_player_failed,
                           "Locking player UI failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}
	
	progress->TaskUpdate(1, 1);
    }
	
    // Check the partitions for errors

    progress->TaskStart(Numerics::Sync_checking_media, "Checking player media");
    {
	progress->TaskUpdate(0, 1);
	
	unsigned int fsckflags;
	STATUS result = client.CheckMedia(&fsckflags);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_check_media_failed,
			   "Player media check failed (%s) flags 0x%x", 
                            FormatErrorMessage(result).c_str(), fsckflags);
	    goto read_only;
	}
	
	progress->TaskUpdate(1, 1);
    }

    // Make the player music partitions read-write

    progress->TaskStart(Numerics::Sync_enable_write, "Enabling write access");
    {
	progress->TaskUpdate(0, 1);
	
	STATUS result = client.EnableWrite(true);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_enable_write_failed,
                          "Enabling write access failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}
	
	progress->TaskUpdate(1, 1);
    }

    // Remove the old databases

    progress->TaskStart(Numerics::Sync_remove_database, "Removing old database");
    {
	progress->TaskUpdate(0, 1);
	
	STATUS result = client.DeleteDatabases();
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_remove_database_failed,
                          "Remove old databases failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}

	progress->TaskUpdate(1, 1);
    }

    // Write the config file if necessary

    if ( dcf && config_changed )
    {
	progress->TaskStart( Numerics::Sync_uploading, 
			     "Updating configuration" );
	progress->TaskUpdate( 0, 1 );

	string s = "";
	dcf->ToString( &s );

	STATUS result = client.WriteFidFromMemory( FID_CONFIGFILE,
						   (const BYTE*)s.c_str(),
						   s.length() );

	if ( FAILED(result) ) {
	    progress->Error(Numerics::Sync_remove_database_failed,
                          "Remove old databases failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}

	progress->TaskUpdate( 1, 1 );

	config_changed = false;
    }
    
    // Upload new stuff
    {
	// First things first, let's get the length of the entire batch
	
	int length_tag = GetTagNumber("length");
	FidNodeMap::iterator it;
	
	for(it = all_fids.begin(); it != all_fids.end(); it++) {
	    if(!it->second->IsDirty()) continue;		// only if dirty

	    // this calculation must match the one below
	    int this_len = atoi( it->second->GetTag(length_tag) );
	    this_len /= 1024;
	    if ( !this_len )
		this_len = 1;

	    total_length_k += this_len;
	}
	
	total_length_k += 8;		// let's say 8KB to go after upload

	int total_done_k = 0;
	
	// Now let's do the actual uploads
	
	for(it = all_fids.begin(); it != all_fids.end(); it ++) {
	    FidNode *node = it->second;
	    if(!node->IsDirty())
		continue;			// only if dirty
	    
	    const char *title = node->GetTitle();
	    int length = atol(node->GetTag(length_tag));
	    
	    if(dynamic_cast<FidDeleted *>(node)) {
		progress->TaskStart(Numerics::Sync_deleting, "Deleting %s (fid %x)",
				   title, node->GetFid());
	    }
	    else {
		progress->TaskStart(Numerics::Sync_uploading, "Uploading %s (fid %x) length %d",
				   title, node->GetFid(), length);
	    }
	    progress->OperationUpdate(total_done_k, total_length_k);
	    progress->TaskUpdate(0, length);
	    
	    STATUS result = node->Synchronise();
	    if (FAILED(result)) {		// error in sync action
		progress->Error(Numerics::Sync_upload_failed,
			       "Error synchronising %s fid %x (%s)",
			       title, node->GetFid(),
                               FormatErrorMessage(result).c_str());
		
		goto read_only;			// abort abort abort!
	    }
	    
	    progress->TaskUpdate(length, length);
	    
	    // this calculation must match the one above
	    int this_len = atoi(node->GetTag(length_tag));
	    this_len /= 1024;
	    if ( !this_len )
		this_len = 1;
	    
	    total_done_k += this_len;
	}
	
	progress->OperationUpdate(total_done_k, total_length_k);
    }

    // Rebuild the player databases now

    progress->TaskStart(Numerics::Sync_rebuilding_database, "Rebuilding databases");
    {
	progress->TaskUpdate(0, 1);
	
	STATUS result = client.RebuildPlayerDatabase(0);
	
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_rebuild_warning,
                           "Rebuild databases returned an error: %s", FormatErrorMessage(result).c_str());
	}

	progress->TaskUpdate(1, 1);
    }
    
    // Get disk space available info

    LogFreeSpace();

    // Put everything back to normal operation (also jump here to abort)
    
 read_only:
    // Write protect

    progress->TaskStart(Numerics::Sync_write_protecting, "Disabling write access");
    {
	progress->TaskUpdate(0, 1);
    
	STATUS result = client.EnableWrite(false);
	if(FAILED(result)) {
            progress->Error(Numerics::Sync_read_only_failed, "%s", FormatErrorMessage(result).c_str());
	    return result;
	}

	progress->TaskUpdate(1, 1);
    }

    // Restart player (must do this or the in memory database will be wrong)

    progress->TaskStart(Numerics::Sync_restarting_player, "Restarting player");
    {
	progress->TaskUpdate(0, 1);
    
	STATUS result = client.RestartPlayer();
	if (FAILED(result)) {
            progress->Error(Numerics::Sync_restart_failed, "%s", FormatErrorMessage(result).c_str());
	    return result;
	}

	progress->TaskUpdate(1, 1);
    }

    // Wait for it to come back to life

    progress->TaskStart(Numerics::Sync_waiting_restart, "Waiting for player to restart");
    {
	progress->TaskUpdate(0, RESTART_TIMEOUT);
	// give threads a chance, this works without sleep now but there's no harm in waiting
	sleep(2);
	int i;
	for(i=0; i<RESTART_TIMEOUT; i++) {
	    if(client.IsUnitConnected()) break;

	    progress->TaskUpdate(i, RESTART_TIMEOUT);
	    
	    sleep(1);
	}

	progress->TaskUpdate(RESTART_TIMEOUT, RESTART_TIMEOUT);

	if(i == RESTART_TIMEOUT) {
	    progress->Error(Numerics::Sync_restart_failed,
			   "Timeout waiting for player to restart\n");
	    return MakeErrnoStatus(ETIMEDOUT);
	}
    }

    // Grab a new copy of the database

    STATUS result = Download();
    if (FAILED(result)) {
	progress->Error(Numerics::Database_retrieve_failed,
                       "Failed to retrieve database after download (%s)\n", FormatErrorMessage(result).c_str());
	return result;
    }

    progress->OperationUpdate(total_length_k, total_length_k);
    
    // Bing!
    
    return S_OK;
}

// Rebuilding the database (when not present on startup)

STATUS PlayerDatabase::RebuildDatabase()
{
    // Lock down the player so the user can't press buttons

    progress->TaskStart(Numerics::Sync_locking_player, "Locking player UI");
    {
	progress->TaskUpdate(0, 1);
	STATUS result = client.LockUI(true);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_lock_player_failed,
                          "Locking player UI failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}
	progress->TaskUpdate(1, 1);
    }
    
    // Check the partitions for errors

    progress->TaskStart(Numerics::Sync_checking_media, "Checking player media");
    {
	progress->TaskUpdate(0, 1);
	unsigned int fsckflags;
	STATUS result = client.CheckMedia(&fsckflags);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_check_media_failed,
			   "Checking player media failed (%s) flags 0x%x",
                            FormatErrorMessage(result).c_str(), fsckflags);
	    goto read_only;
	}
	progress->TaskUpdate(1, 1);
    }
    
    // Make the player music partitions read-write

    progress->TaskStart(Numerics::Sync_enable_write, "Enabling write access");
    {
	progress->TaskUpdate(0, 1);
	STATUS result = client.EnableWrite(true);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_enable_write_failed,
                          "Enabling write access failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}
	progress->TaskUpdate(1, 1);
    }

    // Remove the old databases

    progress->TaskStart(Numerics::Sync_remove_database, "Removing old databases");
    {
	progress->TaskUpdate(0, 1);
	STATUS result = client.DeleteDatabases();
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_remove_database_failed,
                          "Removing old databases failed (%s)", FormatErrorMessage(result).c_str());
	    goto read_only;
	}
	progress->TaskUpdate(1, 1);
    }
    
    // Rebuild the player databases now

    progress->TaskStart(Numerics::Sync_rebuilding_database, "Rebuilding databases");
    {
	progress->TaskUpdate(0, 1);
	STATUS result = client.RebuildPlayerDatabase(0);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_rebuild_warning,
                          "Warning: rebuild databases returned %s", FormatErrorMessage(result).c_str());
	}
	progress->TaskUpdate(1, 1);
    }
    
    // Get disk space available info

    LogFreeSpace();

    // Put everything back to normal operation (also jump here to abort)
    
 read_only:
    // Write protect

    progress->TaskStart(Numerics::Sync_write_protecting, "Disabling write access");
    {
	progress->TaskUpdate(0, 1);
	STATUS result = client.EnableWrite(false);
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_read_only_failed,
                           "Disabling write access failed with error %s", FormatErrorMessage(result).c_str());
	    return result;
	}
	progress->TaskUpdate(1, 1);
    }

    // Restart player (must do this or the in memory database will be wrong)

    progress->TaskStart(Numerics::Sync_restarting_player, "Restarting player");
    {
	progress->TaskUpdate(0, 1);
	STATUS result = client.RestartPlayer();
	if (FAILED(result)) {
	    progress->Error(Numerics::Sync_restart_failed,
                           "Restart player failed with error %s", FormatErrorMessage(result).c_str());
	    return result;
	}
	progress->TaskUpdate(1, 1);
    }

    // Wait for it to come back to life

    progress->TaskStart(Numerics::Sync_waiting_restart, "Waiting for player to restart");
    {
	progress->TaskUpdate(0, RESTART_TIMEOUT);
	// give threads a chance, this works now but there's no harm in waiting
	sleep(2);
	int i;
	for(i=0; i<RESTART_TIMEOUT; i++) {
	    if(client.IsUnitConnected()) break;

	    progress->TaskUpdate(i, RESTART_TIMEOUT);
	    
	    sleep(1);
	}

	progress->TaskUpdate(RESTART_TIMEOUT, RESTART_TIMEOUT);
	
	if(i == RESTART_TIMEOUT) {
	    progress->Error(Numerics::Sync_restart_failed,
			   "Timeout waiting for player to restart");
	    return MakeErrnoStatus(ETIMEDOUT);
	}
    }

    EnsureBasicTags();

    // Bing! (mk. II)
    
    return S_OK;
}

// Callback from ProtocolClient during download - each tag name

STATUS PlayerDatabase::TagIndexAction(int index, const char *name)
{
    // set the name in the main tag database
    tag_names.SetName(index, name);

    return S_OK;
}

STATUS PlayerDatabase::TagAddAction(const char *name)
{
    if (tag_names.GetNumber(name) == -1)
	tag_names.AddTag(name);
    return S_OK;
}

void PlayerDatabase::EnsureBasicTags()
{
    if ( tag_names.GetNumber("type") == -1 )
	tag_names.AddTag( "type" );
    if ( tag_names.GetNumber("length") == -1 )
	tag_names.AddTag( "length" );
    if ( tag_names.GetNumber("title") == -1 )
	tag_names.AddTag( "title" );
}

// Callback from ProtocolClient during download - each fid

STATUS PlayerDatabase::DatabaseEntryAction(FILEID fid,
					   const BYTE *static_entries,
					   const DynamicData *dynamic_entries)
{
    // extract this fid's tags from the static_entries structure
    NodeTags tags(&tag_names);
    tags.AddEntries(static_entries);

    if ( dynamic_entries )
    {
	tags.SetValue( tag_names.GetNumber("PlayCount"),
	    VarString::Printf( "%u", dynamic_entries->play_count ).c_str() );
	tags.SetValue( tag_names.GetNumber("SkippedCount"),
	    VarString::Printf( "%u", dynamic_entries->skipped_count ).c_str() );
	tags.SetValue( tag_names.GetNumber("LastPlayed"),
	    VarString::Printf( "%lu", dynamic_entries->play_last ).c_str() );
	if ( dynamic_entries->mark )
	    tags.SetValue( tag_names.GetNumber("Marked"), "Yes" );
    }

    if (!strcmp(tags.GetType(), "illegal"))
	return S_OK;	// "illegal" placeholder

    // Call the Allocator Of Fids
    try {
	FidNode *node = ReadFid(fid, tags);
	if(!node)
	    return E_PLAYERDB_ACTION;				// "bummer"
    }
    catch(DatabaseError::UnknownFidType) {
	// Unknown fid type exception
	progress->Error(Numerics::Unknown_fid_type,
		       "Unknown fid type %s for fid %x titled \"%s\"\n",
		       tags.GetType(),
		       fid,
		       tags.GetTitle());
    }
    catch(bad_alloc) {
	// Out of memory (this is bad)
	write(2, "Out of memory retrieving a fid\n",
	      sizeof("Out of memory retrieving a fid\n"));
    }
	
    return S_OK;
}

void PlayerDatabase::ReportProgress(ProtocolClient*,
				    ProtocolClient::Activity a,
				    int current, int maximum)
{
    // do something about the activity
    switch(a) {
    case ProtocolClient::Read:
    case ProtocolClient::Write:
    case ProtocolClient::Check:
    case ProtocolClient::Rebuild:
    case ProtocolClient::Prepare:
	// update the current task's progress bar
	progress->TaskUpdate(current, maximum);
	break;

    case ProtocolClient::Stat:
    case ProtocolClient::Delete:
    case ProtocolClient::DiskInfo:
    case ProtocolClient::Remount:
    case ProtocolClient::Waiting:
	break;

    default:
	fprintf(stderr, "Unknown progress activity: %d\n", (int) a);
	break;
    }

    // if debug is turned on,,,
    if(debug_connection) {
	const char *s;
	switch(a) {
	case ProtocolClient::Read:
	    s = "r"; break;
	case ProtocolClient::Write:
	    s = "w"; break;
	case ProtocolClient::Stat:
	    s = "s"; break;
	case ProtocolClient::Prepare:
	    s = "p"; break;
	case ProtocolClient::Delete:
	    s = "d"; break;
	case ProtocolClient::DiskInfo:
	    s = "i"; break;
	case ProtocolClient::Check:
	    s = "c"; break;
	case ProtocolClient::Remount:
	    s = "m"; break;
	case ProtocolClient::Rebuild:
	    s = "b"; break;
	case ProtocolClient::Waiting:
	    s = "."; break;
	default:
	    s = "?"; break;
	}
	// print some funny characters
	fprintf(stderr, "%s", s);
    }
}

void PlayerDatabase::ReportWarning(ProtocolClient *,
				  ProtocolClient::Warning w)
{
    if(debug_level < 2) return;

    // something (not necessarily) bad happened
    const char *s;
    switch(w) {
    case ProtocolClient::LOCAL_TIMEOUT:
	s = "!LOCAL_TIMEOUT!";		// no reponse for a packet sent
	break;
    case ProtocolClient::LOCAL_ACKFAIL:
	s = "!LOCAL_ACKFAIL!";		// old style NAK for a bad packet
	break;
    case ProtocolClient::LOCAL_WRONGPACKET:
	s = "!LOCAL_WRONGPACKET!";		// wrong packet ID ack'd
	break;
    case ProtocolClient::LOCAL_CRCFAIL:	// got a bad CRC for an incoming packet
	s = "!LOCAL_CRCFAIL!";
	break;
    case ProtocolClient::LOCAL_DROPOUT:	// got a break in communications during packet
	s = "!LOCAL_DROPOUT!";
	break;
    case ProtocolClient::REMOTE_TIMEOUT:
	s = "!REMOTE_TIMEOUT!";		// no reponse for a packet sent
	break;
    case ProtocolClient::REMOTE_ACKFAIL:
	s = "!REMOTE_ACKFAIL!";		// old style NAK for a bad packet
	break;
    case ProtocolClient::REMOTE_WRONGPACKET:
	s = "!REMOTE_WRONGPACKET!";		// wrong packet ID ack'd
	break;
    case ProtocolClient::REMOTE_CRCFAIL:	// got a bad CRC for an incoming packet
	s = "!REMOTE_CRCFAIL!";
	break;
    case ProtocolClient::REMOTE_DROPOUT:	// got a break in communications during packet
	s = "!REMOTE_DROPOUT!";
	break;
    default:
	s = "!Unknown!";		// god knows
	break;
    }

    printf("%s", s);
    fflush(stdout);
}

string PlayerDatabase::GetConfig()
{
    if ( !dcf )
    {
	BYTE *pData = (BYTE*)"";
	int size = 0;

	STATUS rc = client.ReadFidToMemory( FID_CONFIGFILE, &pData, &size );
	if ( FAILED(rc) && rc != MakeErrnoStatus(ENOENT) )
	{
	    progress->Error(Numerics::Database_retrieve_failed,
			    "Could not retrieve configuration file from player (%s)",
                            FormatErrorMessage(rc).c_str());
	    return "";
	}

	dcf = NEW DynamicConfigFile();

	string result( (const char*)pData, (unsigned int)size );

	dcf->FromString( result );
    }

    string s = "";
    dcf->ToString(&s);

    return s;
}

void PlayerDatabase::SetConfigValue( string section, string key, string value )
{
    if ( !dcf )
	GetConfig();

    dcf->SetStringValue( section, key, value );
    config_changed = true;
}

//////////////////////////////////////////////////////////////////////////////
// class FidPlaylist -- Playlists
//////////////////////////////////////////////////////////////////////////////

// static method which creates the FidPlaylist grabbed from the database

FidPlaylist *FidPlaylist::CreateFromFid(PlayerDatabase &db, const NodeTags &tags, FILEID fid)
{
//    DatabaseProgressListener &progress = db.GetProgress();
    
//    int *buf, buflen, result;

    // create new playlist
    return NEW FidPlaylist(db, fid, tags);
}

// Perform any repairs necessary on the playlist - loop checking etc

int FidPlaylist::Repair(bool do_repair)
{
    DatabaseProgressListener *progress = db.GetProgress();

    progress->Debug(2, "Recursing playlist repair/check: %s (fid %x)\n",
		   GetTitle(), fid);

    // recurse through every FID in this playlist
    int repairs = 0;
    vector<FILEID>::iterator it = fids.begin();
    while(it != fids.end()) {
	progress->Debug(2, "%x ", *it);

	// check this FID exists
	FidNode *node = db.GetNode(*it);
	if(!node) {	// nope!
	    repairs ++;
	    if(do_repair) {
		// remove reference to offending FID
		progress->Error(Numerics::Removed_missing_member,
			       "Removed missing member (fid %x) in playlist %s\n",
			       *it, GetTitle());
		RemoveIterator(it);		// updates 'it' too
	    }
	    else {
		progress->Error(Numerics::Found_missing_member,
			       "Found missing member (fid %x) in playlist %s\n",
			       *it, GetTitle());
		// ignore it and carry on with the rest of the playlist
		it++;
	    }
	    continue;
	}

	// add a reference to this FID
	node->IncreaseReference();

	// Is this a playlist? If so, we need to check loops and recurse it
	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	if(playlist) {
	    // persist in repairing broken links (may be more than one!)
	    for(;;) {
		progress->Debug(3, "Checking %x for loops\n", fid);

		vector<FILEID> visits;
		visits.push_back(fid);		// don't recurse to self
		
		FILEID which_ref;			// find loopy playlists
		FidPlaylist *looplist = playlist->CheckNoLoops(visits, &which_ref);
		if(!looplist) break;		// none are faulty, break loop

		if(do_repair) {
		    progress->Error(Numerics::Removed_missing_member,
				   "Removed missing member (fid %x) in playlist %s\n",
				   *it, GetTitle());
		    looplist->RemoveFid(which_ref);	// remove offending FID
		}
		else {
		    progress->Error(Numerics::Found_missing_member,
				   "Found missing member (fid %x) in playlist %s\n",
				   *it, GetTitle());
		    break;				// dangerous to proceed with fault
		}
	    }

	    progress->Debug(3, "\nFid %x passed\n", fid);

	    // only recurse through a playlist if nobody else has already
	    if(playlist->GetReferenceCount() == 1) {
		repairs += playlist->Repair(do_repair);
	    }
	}

	it++;	// next!
    }

    progress->Debug(2, "Finished recursion of playlist %s (fid %x)\n", GetTitle(), fid);
    
    return repairs;
}

// Synchronise this playlist -- write out the tags and the new list of FID's

STATUS FidPlaylist::Synchronise()
{
    // need the length of the playlist
    int buflen = fids.size();
    FILEID *buf = NEW FILEID[buflen];

    // transfer the FIDs list to the integer buffer
    // yes, I know you can do this in one line of STL with raw iterators, but I can't be bothered
    int i = 0;
    for(vector<FILEID>::iterator it=fids.begin(); it!=fids.end(); it++, i++) {
	buf[i] = *it;
    }

    // write FID offset 0 (playlist contents)
    STATUS result = db.GetClient().WriteFidFromMemory(fid,
						      (const BYTE *) buf,
						      buflen * sizeof(FILEID));
    // dont need the buffer any more
    delete[] buf;
    
    if (FAILED(result)) {
	// yikes
	db.GetProgress()->Error(Numerics::Sync_write_playlist_failed,
				"Failed to write playlist contents \"%s\" (fid %x) to player (%s)\n",
                                GetTitle(), fid, FormatErrorMessage(result).c_str());
	return result;		// give up on error
    }

    // write out the tags, fid|1

    // set "length" to its proper value
    char tmp[16];
    sprintf(tmp, "%d", buflen * sizeof(int));
    tags.SetValue(db.GetTagNumber("length"), tmp);

    // create tag entries table
    int entries_len = tags.GetTableLength();
    BYTE *entries = NEW BYTE[entries_len];
    tags.WriteTable(entries);
    // write it to the player
    result = db.GetClient().WriteFidFromMemory(fid|1, entries, entries_len);
    // don't need the buffer any more
    delete[] entries;
    
    if (FAILED(result)) {
	db.GetProgress()->Error(Numerics::Sync_write_playlist_failed,
				"Failed to write playlist tags for \"%s\" (fid %x) to player (%s)\n",
                                GetTitle(), fid|1, FormatErrorMessage(result).c_str());
	return result;		// give up on error
    }
    
    dirty = 0;					// clean, no need to sync now (phew!)
    return S_OK;
}

// Create a new playlist by name under this one. If child == 0, find a new FID

int FidPlaylist::CreatePlaylist(const char *name, bool inherit, FILEID child)
{
    bool new_playlist = !child;
    
    NodeTags tags(&db.GetTagNames());

    if(inherit) tags = this->tags;

    // setup tags "type" and "title" appropriately
    tags.SetValue(db.GetTagNumber("type"), "playlist");
    tags.SetValue(db.GetTagNumber("title"), name);
    
    if(!child) {					// no fid specified, find one!
	child = db.FindFreeFid();
    }
    else {
	if(db.GetNode(child)) {				// check fid doesn't already exist
	    throw DatabaseError::FidInUse(child);
	}
    }

    // create blank playlist (fids argument defaults to empty vector)
    FidPlaylist *playlist = NEW FidPlaylist(db, child, tags);
    playlist->UpdateLength();
    if(new_playlist) playlist->MarkDirty();
    
    // add it to the current playlist
    AddNode(playlist);		// automatically does dirty++, playlist->ref_count++

    // should have been added on the end of the list
    
    return fids.size() - 1;
}

// Upload a file into the current playlist
// 'name' is what you want it called, 'path' is where it is relative to working directory

int FidPlaylist::Upload(const char *name, const char *path)
{
    // see if it's accessible
    struct stat statbuf;
    int err = stat(path, &statbuf);
    if(err) {
	// print appropriately confusing error message (muhahaha)
	db.GetProgress()->Error(Numerics::Error_opening_file,
				"Error opening file \"%s\" - %s (errno %d)",
				path, strerror(errno), errno);
	return -1;
    }

    // grab length into string
    int len = statbuf.st_size;
    char tmp[16];
    sprintf(tmp, "%d", len);

    // set 'type' 'title' 'length' tags
    NodeTags tags(&db.GetTagNames());
    tags.SetValue(db.GetTagNumber("type"), "tune");	// type = tune
    tags.SetValue(db.GetTagNumber("title"), name);	// title = name
    tags.SetValue(db.GetTagNumber("length"), tmp);	// length = statbuf.st_size

    // create the tune
    FidLocalFile *localfile = FidLocalFile::Create(db, path, tags);

    // this automatically increases the reference and marks this playlist dirty
    AddNode(localfile);
    
    return fids.size() - 1;
}

// Check a playlist for loops -- recursive function
// Note: using a throw FoundLoop(fid, ref); would bypass unrolling and be MUCH faster
// assumption: visits contains parent fid
FidPlaylist *FidPlaylist::CheckNoLoops(vector<FILEID> &visits, FILEID *which_ref)
{
    if(find(visits.begin(), visits.end(), fid) != visits.end()) {
	*which_ref = fid;
	return this;
    }

    int vlen = visits.size();    
    int len = fids.size();
    for(int i = 0; i < len; i++) {
	FILEID entry_fid = fids[i];
	FidNode *node = db.GetNode(entry_fid);
	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	if(!playlist) continue;

	for(int j=0; j<vlen; j++) {
	    if(visits[j] == entry_fid) {
		*which_ref = entry_fid;
		return this;
	    }
	}

	visits.push_back(fid);
	playlist = playlist->CheckNoLoops(visits, which_ref);
	visits.pop_back();
	if(playlist) return playlist;		// recurse should have set *which_ref
    }

    return NULL;
}

// Add a FID to the current playlist

void FidPlaylist::AddNode(FidNode *node)
{
    fids.push_back(node->GetFid());		// put it on the end of the playlist list
    MarkDirty();				// mark current playlist as changed
    node->IncreaseReference();			// increase the reference count for the node
    UpdateLength();				// update "length" tag of this playlist
}

// Remove the FILEID iterator -- protects against iterator invalidation by updating the arg

void FidPlaylist::RemoveIterator(vector<FILEID>::iterator &i)
{
    // 1) erase the element at the iterator
    FILEID child = *i;
    i = fids.erase(i);

    // 2) this playlist is now dirty
    dirty = 1;

    // 3) find the node it was pointing to
    FidNode *node = db.GetNode(child);
    if(!node) return;				// doh!

    // 4) decrease the reference count
    try {
	node->RemoveReference();
    }
    catch(DatabaseError::RefCountBelowZero) {
	db.GetProgress()->Error(Numerics::Ref_count_below_zero,
			       "Reference count for %s (fid %x) went below zero! Now at %d\n",
			       node->GetTitle(), child, node->GetReferenceCount());
    }

    // 5) check if it's no longer being referenced
    if(node->IsReferenced()) {
	db.GetProgress()->Debug(2, "Removed %s (fid %x) reference count now %d\n",
			     node->GetTitle(), child, node->GetReferenceCount());
    }
    else {
	db.GetProgress()->Debug(2,
			       "Removed %s (fid %x) and deleted\n", node->GetTitle(), child);
	NodeTags oldtags = node->GetTags();
	// if so, remove it from the database
	delete node;
	// and put a 'deleted' fid in its place -- this synchronises by deleting
	NEW FidDeleted(db, child, oldtags);
    }

    UpdateLength();
}

//////////////////////////////////////////////////////////////////////////////
// FidRemoteTune -- Tunes which are on the player, sync updates tags
//////////////////////////////////////////////////////////////////////////////

FidRemoteTune *FidRemoteTune::CreateFromFid(PlayerDatabase &db, const NodeTags &tags, FILEID fid)
{
    // just construct one, already have all the info we need
    return NEW FidRemoteTune(db, fid, tags);
}

// Synchronising a remote tune means we've touched the tags for it -- need to write them

STATUS FidRemoteTune::Synchronise()
{
    // only have to write FID+1 file
    int entries_len = tags.GetTableLength();
    BYTE *entries = NEW BYTE[entries_len];		// allocate space for the table

    tags.WriteTable(entries);				// write out the table
    // blat it over the old FID on the player
    STATUS result = db.GetClient().WriteFidFromMemory(fid|1,
						      (const BYTE *) entries, 
						      entries_len);
    // don't need no steeeeenking memory any more
    delete[] entries;					// uh huh
    
    if (FAILED(result)) {
	db.GetProgress()->Error(Numerics::Sync_write_tune_tags_failed,
		      "Writing tune tags failed for tune \"%s\" (fid %x) (%s)\n",
                      GetTitle(), fid, FormatErrorMessage(result).c_str());
	return result;
    }
    else return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Files which are pending upload to the player
//////////////////////////////////////////////////////////////////////////////

FidLocalFile::map_t FidLocalFile::theMap;

/* How deep do we follow symlinks? */
#define SYMLINK_DEPTH 10

FidLocalFile *FidLocalFile::Create(PlayerDatabase& db, const char *path,
				   const NodeTags& tags)
{
    string s;
    char targetbuf[PATH_MAX];

    // Resolve symlinks before we attempt to share
    if ( realpath(path,targetbuf) )
	s = targetbuf;
    else
	s = path;

    map_t::iterator it = theMap.find(s);
    if ( it != theMap.end() )
    {
	//printf( "Found %s in theMap\n", s.c_str() );
	return it->second;
    }

    //printf( "Didn't find %s in theMap\n", s.c_str() );

    FidLocalFile *result = NEW FidLocalFile( db, s.c_str(), tags );

    theMap[s] = result;

    //printf( "Added %s to theMap\n", s.c_str() );

    return result;
}

FidLocalFile::~FidLocalFile()
{
    theMap.erase( theMap.find(path) );
}

// Local file synchronisation means that we write a long file to the player and its tags

STATUS FidLocalFile::Synchronise()
{
    // write FID|0 file first (the data) -- this may take a while
    // there are callbacks from ProtocolClient to ReportProgress periodically (each packet)

    timeval tvstart;
    gettimeofday(&tvstart, NULL);

    STATUS result = db.GetClient().WriteFidFromFile(fid, path.c_str());
    if (FAILED(result)) {
	db.GetProgress()->Error(Numerics::Sync_write_file_failed,
			       "Failed to write tune data for \"%s\" (file \"%s\") (fid %x), %s\n",
                               GetTitle(), path.c_str(), fid, FormatErrorMessage(result).c_str());
	return result;
    }

    timeval tvstop;
    gettimeofday(&tvstop, NULL);
    float secs = (tvstop.tv_usec - tvstart.tv_usec) * 0.000001;
    secs += tvstop.tv_sec - tvstart.tv_sec;
    int bytes = atol(GetTag(db.GetTagNumber("length")));
    printf("\nTransferred %d bytes in %.2f seconds, %d bytes/sec\n",
	   bytes, secs, (int)(bytes / secs) );


    // write FID|1 file next (The Tags (tm))
    int entries_len = tags.GetTableLength();
    BYTE *entries = NEW BYTE[entries_len];		// allocate space for the table

    tags.WriteTable(entries);				// write out the table
    // blat
    result = db.GetClient().WriteFidFromMemory(fid|1, (const BYTE *) entries, entries_len);
    // splat
    delete entries;

    if(FAILED(result)) {
	// bummer
	db.GetProgress()->Error(Numerics::Sync_write_tune_tags_failed,
			       "Failed to write tune tags for \"%s\" (file \"%s\") (fid %x), %s\n",
                               GetTitle(), path.c_str(), fid, FormatErrorMessage(result).c_str());
	return result;
    }
    else {
	// OH YES
	return S_OK;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Deleted FIDs get called with this to remove them from the player
//////////////////////////////////////////////////////////////////////////////

// Deleted fids -- synchronising one of these causes the fids to be deleted from the player

STATUS FidDeleted::Synchronise()
{
    // delete fid+0, fid+1, fid+2, fid+3 (only 0 and 1 used to date)
    STATUS result = db.GetClient().DeleteFid(fid, 3);
    if (FAILED(result)) {
	// woopsie
	db.GetProgress()->Error(Numerics::Sync_delete_file_failed,
			       "Failed to delete %s (fid %x) from the player, %s",
                               GetTitle(), fid, FormatErrorMessage(result).c_str());
	return result;
    }
    else {
	// Good bye, Mr Fid.
	return S_OK;
    }
}
