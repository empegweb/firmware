/* numerics.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Header:)
 */

#ifndef NUMERICS_H
#define NUMERICS_H

namespace Numerics
{
    // 1... Debug
    // 2... Progress
    // 3... Warning
    // 4... Error
    
    // .0.. Connection related
    // .1.. Player related
    // .2.. Database related
    // .3.. Client related
    // .4.. Interface related
    
    enum {
	No_such_numeric				= 0,

	No_root_playlist			= 1200,
	No_unattached_items_playlist		= 1210,

	Creating_playlist_from_player		= 1220,
	    
	Checking_start				= 1250,
	Checking_root				= 1251,
	Checking_tree_start			= 1252,
	Checking_tree_finished			= 1253,
	Checking_references			= 1254,
	Checking_found_deleted			= 1255,
	Checking_playlist_recurse_start		= 1256,
	Checking_loops				= 1257,
	Checking_loops_passed			= 1258,
	Checking_playlist_recurse_finished	= 1259,
	Checking_disk_space                     = 1260,
	Checking_finished			= 1269,

	Found_playlist_member			= 1270,

	Removed_fid				= 1280,
	Removed_fid_and_deleted			= 1281,

	Unrepaired_errors			= 1290,
	Repaired_errors				= 1291,

	Checking_connection			= 2000,
	
	Transfer_read				= 2010,
	Transfer_write				= 2011,
	    
	Synchronising				= 2100,
	Sync_start				= 2101,
	Sync_checking_connection		= 2102,
	Sync_locking_player			= 2103,
	Sync_checking_media			= 2104,
	Sync_enable_write			= 2105,
	Sync_remove_database			= 2106,
	Sync_uploading				= 2107,
	Sync_deleting				= 2108,
	Sync_rebuilding_database		= 2109,
	Sync_write_protecting			= 2110,
	Sync_restarting_player			= 2111,
	Sync_waiting_restart			= 2112,
	Sync_finished				= 2119,

	Downloading				= 2120,
	Download_start				= 2121,
	Download_retrieve_tags			= 2122,
	Download_retrieve_databases		= 2123,
	Download_retrieve_playlists		= 2124,
	Download_building_databases		= 2125,
	Download_finished			= 2129,

	Quitting_player				= 2130,
	
	Hash_printing_on			= 2400,
	Hash_printing_off			= 2401,
	Listing_follows				= 2402,
	Working_directory			= 2403,
	Created_playlist			= 2404,
	Tags_follow				= 2405,
	
	Sync_rebuild_warning			= 3100,
	
	No_database				= 3200,
	    
	Invalid_root_playlist			= 3210,
	    
	Invalid_unattached_items_playlist	= 3220,
	Unattached_unattached_items_playlist	= 3221,
	    
	Unknown_fid_type			= 3230,

	Found_unattached_item			= 3240,
	Repaired_unattached_item		= 3241,
	Removed_playlist_loop			= 3242,
	Found_playlist_loop_stop		= 3243,
	Removed_missing_member			= 3244,
	Found_missing_member			= 3245,

	Cannot_find_unit			= 4000,
	System_connection_error			= 4001,
	Connection_timeout			= 4002,
	Database_retrieve_failed		= 4010,

	Sync_lock_player_failed			= 4100,
	Sync_check_media_failed			= 4101,
	Sync_enable_write_failed		= 4102,
	Sync_remove_database_failed		= 4103,
	Sync_upload_failed			= 4104,
	Sync_read_only_failed			= 4105,
	Sync_restart_failed			= 4106,
	Sync_write_playlist_failed		= 4107,
	Sync_write_tune_tags_failed		= 4108,
	Sync_write_file_failed			= 4109,
	Sync_delete_file_failed			= 4110,

	Rebuild_failed				= 4120,
	
	Error_root_playlist			= 4200,
	Error_unattached_items_playlist		= 4201,
	Error_playlist_database			= 4202,
	Would_loop				= 4203,
	Would_delete_root_playlist		= 4204,
	Would_deref_current_playlist		= 4205,
	Would_delete_unattached_items_playlist	= 4206,
	Playlist_not_empty			= 4207,
	
	Ref_count_below_zero			= 4210,
	    
	Error_creating_playlist			= 4300,
	Error_opening_file			= 4301,
	Fid_not_found				= 4302,
	Filesystem_error			= 4303,
	
	Unknown_command				= 4400,
	Parse_error				= 4401,
	Syntax_error				= 4402,
	Not_a_playlist				= 4403,
	No_matches_found			= 4404,
	Invalid_path_name			= 4405,
	Ambiguous_multiple_matches		= 4406,
	Tag_not_allowed				= 4407,
	Exists_as_file                          = 4408,
	
    	Unexpected_connection_error		= 4099,
	Unexpected_player_error			= 4199,
	Unexpected_database_error		= 4299,
	Unexpected_internal_error		= 4399,
	Unexpected_interface_error		= 4499,
    };
};

#endif
