/* empconsole.h
 *
 * Console interface to player database
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#ifndef EMPCONSOLE_H
#define EMPCONSOLE_H

#include "playerdb.h"
#include "database_progress_listener.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// StringList -- an STL list of strings
//////////////////////////////////////////////////////////////////////////////

typedef list<string> StringList;

class ConsoleInterface;

//////////////////////////////////////////////////////////////////////////////
// ConsoleProgress -- this object takes care of all progress callbacks
//   from PlayerDatabase
//////////////////////////////////////////////////////////////////////////////

// implementation

class ConsoleProgress: public DatabaseProgressListener
{
    int debug_level, operation_code, task_code;
    bool new_line;
    string operation_name;
    string task_name;
    int operation_progress, operation_total, operation_percent;
    int task_progress, task_total, task_percent;

    void force_nl();

 public:
    ConsoleProgress();

    // these declarations hurt the eyes
    
    // Major operations (download, synchronise, etc...)
    virtual void OperationStart(int code, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4)));	// 3 including 'this' hidden argument
    virtual void OperationUpdate(int progress, int total) throw();
    
    // Sub-operations/tasks (send file, receive tags, etc...)
    virtual void TaskStart(int code, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4)));	// 3 including 'this' hidden argument
    virtual void TaskUpdate(int progress, int total) throw();

    // Logging of debug, information and errors during operations
    virtual void Debug(int level, const char *fmt, ...) throw();
    virtual void Log(int code, const char *fmt, ...) throw();
    virtual void Error(int code, const char *fmt, ...) throw();

    void SetDebug(int level) { debug_level = level; }
};

//////////////////////////////////////////////////////////////////////////////
// PlaylistBranch -- tuple of a Playlist and the position of a node in it.
//   This is the only unique way of identifying a node in the entire tree.
//////////////////////////////////////////////////////////////////////////////

class PlaylistBranch
{
 private:
    FidPlaylist *parent;				// playlist containing node
    int pos;						// position in playlist

 public:
    // constructor
    PlaylistBranch(FidPlaylist *parent = NULL, int pos = 0)
	: parent(parent), pos(pos) { }

    FidPlaylist *GetParentPlaylist() const throw() { return parent; }	// get parent
    int GetChildPos() const throw() { return pos; }		// get child position in playlist
    // is this the root playlist? (no parent and no position in it, obvoiusly)
    bool IsRootPlaylist() const throw() { return parent == NULL; }
    FidNode *GetChildNode(PlayerDatabase &db) const;
    FidPlaylist *GetChildPlaylist(PlayerDatabase &db) const;
    // for BranchStackList::RemovedNode() only
    void SetChildPos(int pos) throw() { this->pos = pos; }
};

//////////////////////////////////////////////////////////////////////////////
// BranchStack -- the route through which a node is accessed.
//   An empty object represents the root playlist
//////////////////////////////////////////////////////////////////////////////

class BranchStack : public list<PlaylistBranch>
{
 public:
    FidPlaylist *GetParentPlaylist() const throw();
    int GetChildPos() const throw();
    bool IsRootPlaylist() const throw() { return size() == 0; }
    FidNode *GetChildNode(PlayerDatabase &db) const;
    FidPlaylist *GetChildPlaylist(PlayerDatabase &db) const;
    // update child position if playlist matches
    void RemovedNode(FidPlaylist *playlist, int pos);

    // for BranchStackList::RemovedNode() only
    void SetChildPos(int pos) throw();
};

//////////////////////////////////////////////////////////////////////////////
// BranchStackList -- guess
//////////////////////////////////////////////////////////////////////////////

class BranchStackList : public list<BranchStack>
{
 public:
    // update the list to take account of a shifted position of nodes in a playlist
    void RemovedNode(FidPlaylist *playlist, int pos);
};
    
#ifndef WIN32
//////////////////////////////////////////////////////////////////////////////
// DirectoryEntry -- a directory and its appropriate entry
//////////////////////////////////////////////////////////////////////////////

class DirectoryEntry
{
    bool root_relative;
    bool is_directory;
    int length;
    string name;
    string full_path;
 public:
    struct StatFailed { int err; StatFailed(int err) : err(err) { } };
    
    DirectoryEntry(const char *name, const char *full_path);

    bool IsDirectory() const { return is_directory; }
    int GetLength() const { return length; }
    /*
    bool Exists(const char *title,
		PlayerDatabase &db, const BranchStack &cwdstack) const;
    void CreateDirectories(PlayerDatabase &db, const BranchStack &cwdstack,
			   BranchStackList &branchlist);
    FidLocalFile *Upload(const char *title,
			 PlayerDatabase &db, const BranchStack &cwdstack,
			 BranchStack &resultstack);
    */
    const char *GetName() const { return name.c_str(); }
    const char *GetFullPath() const { return full_path.c_str(); }
};

typedef list<DirectoryEntry> DirectoryEntryList;
#else
#error Bad Operating System
#endif

//////////////////////////////////////////////////////////////////////////////
// ConsoleInterface -- the command line interface to the database.
//   Pretty much everything goes through here at some point.
//////////////////////////////////////////////////////////////////////////////

class ConsoleInterface
{
 private:
    // error codes returned from all the Parse_* functions
    class State {
     public:
	enum {
	    Abort = 0,				// can't continue after operation
	    Error_continue,			// error, but allow more commands
	    OK					// everything's ok
	};
    };

    class ExpandFlags {
     public:
	enum {
	    None			= 0x00,
	    Expand_playlists		= 0x01,
	    Recurse			= 0x02,
	    Default_current		= 0x04,
	    Include_recurses		= 0x08,
	    Long_listing		= 0x10,
	    Escape_paths		= 0x20,
	    Use_existing                = 0x40, // like mkdir -p
	};
    };
    
    class InputEOF { };					// no more input
    class NamedError {
	string name;
     public:
	NamedError(const char *name) : name(name) { }
	const char *GetName() const { return name.c_str(); }
    };
    class FidNotFound : public NamedError {
     public: FidNotFound(const char *name) : NamedError(name) { }
    };
    class NotAPlaylist : public NamedError {
     public: NotAPlaylist(const char *name) : NamedError(name) { }
    };
    class InvalidPathName : public NamedError {
     public: InvalidPathName(const char *name) : NamedError(name) { }
    };				// badly specified path

    // debug
    void PrintBranchStackList(const BranchStackList &branch_list);

    // line parsing
    string GetLine(char *prompt) throw(InputEOF);	// get a line from the console
    int ProcessCommand(StringList &tokens);		// process it
    int Tokenize(const char *line, StringList &tokens);	// tokenize a C string to a list

    int FindPosForIdentifier(const FidPlaylist &playlist, const char *identifier);
							// find playlist position of an identifier
    // given a directory stack and a path argument, form a BranchStack to the target
    BranchStack PathToBranchStack(const BranchStack &initstack, const char *path);
    // given a BranchStack, convert to a literal path string
    string BranchStackToString(const BranchStack &branch_stack);
    // same but return a '/' for an empty string
    string BranchStackToPath(const BranchStack &branc_stack);
    // given an identifier and directory stack, expand any wildacrds and append to branchstack list
    int ExpandIdentifier(BranchStackList &branchlist,	// append to this list
			 BranchStackList::iterator &it,	// iterator position to insert before
			 const BranchStack &initstack,
			 const char *pattern);		// and return number of matches
    string GetBasePath(const char *pattern);		// get the base path of a pattern
    string GetBaseFile(const char *pattern);		// get the glob ending of a pattern
    int ExpandShellGlob(DirectoryEntryList &direntrylist,   // append to this list
			DirectoryEntryList::iterator &it,   // iterator position to insert before
			const char *pattern);		// and return number of matches
    // find the expansion flags in the command line arguments
    int GetExpandFlags(const StringList &args, int flags);
    // expand command line arguments with local database name scoping
    int UploadFiles(StringList::const_iterator begin,
		    StringList::const_iterator end,
		    BranchStack &initstack,
		    BranchStackList &direntry_list, int flags);
    int ExpandArgumentsDatabase(StringList::const_iterator begin,
				StringList::const_iterator end,
				BranchStackList &branchlist, int flags);
    bool AllowedTag(const char *tag);			// can the user alter this tag?
    string EscapePath(const string &path);		// escape spaces in a path name
    int GetLength(FidNode *node);			// get logical length
    void PrintNode(const BranchStack &nodestack, int flags, bool is_recurse);
							// print node details
     
    // args in: list of strings (1 = command, 2 = args, ...)
    // return value, 0 = abort program, 1 = continue
    int Parse_help(const StringList &tokens);		// "help" - help
    int Parse_hash(const StringList &tokens);		// "hash" - hash # printing on/off
    int Parse_ls(const StringList &tokens);		// "ls" - list directories
    int Parse_cd(const StringList &tokens);		// "cd" - change directory
    int Parse_pwd(const StringList &tokens);		// "pwd" - print current directory
    int Parse_mklist(const StringList &tokens);		// "mklist" - make a playlist
    int Parse_link(const StringList &tokens);		// "link" - link a node into cur playlist
    int Parse_rm(const StringList &tokens);		// "rm" - remove a node in cur playlist
    int Parse_repair(const StringList &tokens);		// "repair" - check/repair the database
    int Parse_upload(const StringList &tokens);		// "upload" - upload files
    int Parse_lookup(const StringList &tokens);		// "lookup" - view tags
    int Parse_set(const StringList &tokens);		// "set" - set a tag
    int Parse_unset(const StringList &tokens);		// "unset" - unset a tag
    int Parse_move(const StringList &tokens);		// "move" - move nodes about
    int Parse_rename(const StringList &tokens);		// "rename" - rename a node
    int Parse_config(const StringList &tokens);		// "config" - manipulate configuration
    int Parse_sync(const StringList &tokens);		// "sync" - synchronise to player
    int Parse_quit(const StringList &tokens);		// "quit" - badhello

    int Synchronise();					// synchronise with player

    PlayerDatabase &db;					// reference to database object
    ConsoleProgress *progress;				// pointer to progress object
    
    bool do_reporting;					// whether we want to report progress

    FidPlaylist *root;					// quick pointer to root playlist
    FidPlaylist *cwlist;				// quick pointer to current playlist
    FILEID cwlistfid;					// current playlist's fid
    BranchStack cwdstack;				// working directory stack

    int dirty;						// number of changes, >0 = dirty
    int debug_level;					// debug level

    void ResetDirty() { dirty = 0; }			// database has been sync/download
    void MarkDirty() { dirty ++; }			// change has been made
    bool IsDirty() { return dirty > 0; }		// is databases requiring sync?
    
 public:
    // constructor
    ConsoleInterface(PlayerDatabase &db);
    ~ConsoleInterface();				// one does not enjoy memory leaks

    void SetDebug(int val);
    int Init();						// initialise console
    int ProcessLine();		// Process a line of text, return 0 if abort or user quit
};

#endif
