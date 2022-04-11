/* empconsole.cpp
 *
 * Console interface to player database
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.27 13-Mar-2003 18:15 rob:)
 */

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "emptool.h"
#include <list>
#include <string>
#include <algorithm>
#include <memory>
#include <set>
#include <iostream>
#include <ctype.h>

#include "trace.h"
#include "empconsole.h"
#include "numerics.h"
#include "wildcard.h"
#include "var_string.h"
#include "tags/tag_extractor.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// ConsoleProgress
//////////////////////////////////////////////////////////////////////////////

ConsoleProgress::ConsoleProgress()
    : debug_level(0), operation_code(0), task_code(0),
      new_line(true),
      operation_name(),
      task_name(),
      operation_progress(-1), operation_total(100), operation_percent(-1),
      task_progress(-1), task_total(100), task_percent(-1)
{
}

void ConsoleProgress::force_nl()
{
    if(!new_line) {
	new_line = true;
	printf("\n");
	fflush(stdout);
    }
}

//////////////////////////////////////////////////////////////////////////////
// PlaylistBranch
//////////////////////////////////////////////////////////////////////////////

FidNode *PlaylistBranch::GetChildNode(PlayerDatabase &db) const
{
    if(!parent) return db.GetNode(FID_ROOTPLAYLIST);
    else return db.GetNode((*parent)[pos]);
}

FidPlaylist *PlaylistBranch::GetChildPlaylist(PlayerDatabase &db) const
{
    return dynamic_cast<FidPlaylist *>(GetChildNode(db));
}


//////////////////////////////////////////////////////////////////////////////
// BranchStack
//////////////////////////////////////////////////////////////////////////////

FidPlaylist *BranchStack::GetParentPlaylist() const throw()
{
	if(IsRootPlaylist()) return NULL;
	else return (*rbegin()).GetParentPlaylist();
}

int BranchStack::GetChildPos() const throw()
{
    if(IsRootPlaylist()) return -1;
    else return (*rbegin()).GetChildPos();
}

FidNode *BranchStack::GetChildNode(PlayerDatabase &db) const
{
    if(IsRootPlaylist()) return db.GetRootPlaylist();
    else return (*rbegin()).GetChildNode(db);
}

FidPlaylist *BranchStack::GetChildPlaylist(PlayerDatabase &db) const
{
    if(IsRootPlaylist()) return db.GetRootPlaylist();
    else return (*rbegin()).GetChildPlaylist(db);
}

void BranchStack::RemovedNode(FidPlaylist *playlist, int pos)
{
    if(playlist != GetParentPlaylist()) return;
    int childpos = GetChildPos();
    if(childpos == -1) return;
    if(childpos == pos) {
	fprintf(stderr, "Oh dear we've got more than one ref of %s[%d]\n",
		playlist->GetTitle(), pos);
	return;
    }
    else if(childpos > pos) SetChildPos(childpos - 1);
}

void BranchStack::SetChildPos(int pos) throw()
{
    if(!IsRootPlaylist()) (*rbegin()).SetChildPos(pos);
}

//////////////////////////////////////////////////////////////////////////////
// BranchStackList
//////////////////////////////////////////////////////////////////////////////

void BranchStackList::RemovedNode(FidPlaylist *playlist, int pos)
{
    for(iterator it = begin(); it != end(); it++) {
	it->RemovedNode(playlist, pos);
    }
}

//////////////////////////////////////////////////////////////////////////////
// ConsoleProgress -- report stuff to the console (file/database progress)
//////////////////////////////////////////////////////////////////////////////

// These do stuff

void ConsoleProgress::OperationStart(int code, const char *fmt, ...) throw()
{
    force_nl();

    operation_code = code;
    
    printf("%04d ", code);

    char tmp[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, 127, fmt, ap);
    tmp[127] = 0;
    va_end(ap);
    operation_name = tmp;

    printf("%s", tmp);

    fflush(stdout);

    new_line = false;
}

void ConsoleProgress::TaskStart(int code, const char *fmt, ...) throw()
{
    force_nl();
    
    task_code = code;
    
    printf("%04d ", code);

    char tmp[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, 127, fmt, ap);
    tmp[127] = 0;
    va_end(ap);
    task_name = tmp;

    printf("%67.67s", tmp);

    fflush(stdout);

    new_line = false;
}

void ConsoleProgress::OperationUpdate(int progress, int total) throw()
{
    if((operation_progress == progress) && (operation_total == total)) return;

    operation_progress = progress;
    operation_total = total;

    int old_percent = operation_percent;
    
    // Use double; this can very easily overflow: 1<<31 / 100 = 20Mb
    // even if we count in K that's 20Gb which is well in range
    if ( operation_total == 0 )
	operation_percent = 0;
    else
	operation_percent = (int)( (100.0*operation_progress)/operation_total );

    if(operation_percent == old_percent) return;

    if(operation_percent != 100) {
	printf("\r%04d %-67.67s [%3d%%]", operation_code, operation_name.c_str(), operation_percent);
    }
    else {
	force_nl();
	printf("\r%04d %-67.67s [Done]", operation_code, operation_name.c_str());
    }

    fflush(stdout);

    new_line = false;
}

void ConsoleProgress::TaskUpdate(int progress, int total) throw()
{
    if((task_progress == progress) && (task_total == total)) return;

    task_progress = progress;
    task_total = total;

    int old_percent = task_percent;
    
    // Use double to prevent overflow
    if ( task_total == 0 )
	task_percent = 0;
    else
	task_percent = (int)( (100.0*task_progress)/task_total );
    
    if(task_percent == old_percent) return;

    if(task_percent != 100) {
	printf("\r%04d %-67.67s [%3d%%]", task_code, task_name.c_str(), task_percent);
    }
    else {
	printf("\r%04d %-67.67s [Done]", task_code, task_name.c_str());
    }

    fflush(stdout);

    new_line = false;
}

void ConsoleProgress::Debug(int level, const char *fmt, ...) throw()
{
    force_nl();

    if(level <= debug_level) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
    }

    fflush(stdout);
}

void ConsoleProgress::Log(int code, const char *fmt, ...) throw()
{
    force_nl();
    
    printf("%04d ", code);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    fflush(stdout);
}   

void ConsoleProgress::Error(int code, const char *fmt, ...) throw()
{
    force_nl();
    
    printf("%04d ", code);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    fflush(stdout);
}

//////////////////////////////////////////////////////////////////////////////
// DirectoryEntry -- local filesystem directories and files
//////////////////////////////////////////////////////////////////////////////

DirectoryEntry::DirectoryEntry(const char *name, const char *full_path)
    : name(name), full_path(full_path)
{
    if(!*name) throw StatFailed(ENOTDIR);
    
    struct stat statbuf;
    int err = stat(full_path, &statbuf);
    if(err) throw StatFailed(errno);

    if(S_ISDIR(statbuf.st_mode)) {
	is_directory = true;
	length = 0;
    }
    else {
	is_directory = false;
	length = (int) statbuf.st_size;
    }
}

/**************

void DirectoryEntry::CreateDirectories(PlayerDatabase &db, const BranchStack &cwdstack,
				       BranchStackList &branchlist)
{
    // grab the base branch stack that everything leafs off
    BranchStack basestack = root_relative ? BranchStack() : cwdstack;
    
    // grab the final playlist that we're based off
    FidPlaylist *playlist = cwdstack.GetChildPlaylist(db);

    // if this isn't a directory, we don't create the last string (obviously -- it's the file)
    list<string>::const_iterator last_path = end();
    if(!IsDirectory()) last_path --;

    // loop through from start of path to the end of it
    for(list<string>::const_iterator pit = begin(); pit != last_path; pit ++) {
	// grab the next branch name we need to create
	const char *dirname = pit->c_str();
	int playlistlen = playlist->size();
	// check it doesn't already exist (if so, use that one)
	for(int i=0; i<playlistlen; i++) {
	    FidPlaylist *sublist = playlist->GetPlaylistAt(i);
	    if(sublist) {
		if(!strcmp(sublist->GetTitle(), dirname)) {
		    // there already exists a playlist with this name
		    basestack.push_back(PlaylistBranch(playlist, i));
		    branchlist.push_back(basestack);
		    playlist = sublist;
		    continue;
		}
	    }
	}
	// ok there wasn't already one, so let's make one (we're like that, ya know)
	FidPlaylist *sublist = playlist->GetPlaylistAt(playlist->CreatePlaylist(dirname));
	if(!sublist) {
	    db.GetProgress().Error(Numerics::Unexpected_database_error,
				   "Couldn't create playlist \"%s\" under \"%s\"\n",
				   dirname, playlist->GetTitle());
	    return;
	}
	// this one should be the last entry in the playlist
	basestack.push_back(PlaylistBranch(playlist, playlist->size()-1));
	branchlist.push_back(basestack);
	playlist = sublist;
    }
}

***************/

/***************

FidLocalFile *DirectoryEntry::Upload(const char *title,
				     PlayerDatabase &db, const BranchStack &cwdstack,
				     BranchStack &resultstack)
{
    // oh dear there's a lot of overlap with above. but... damn
    if(IsDirectory()) {
	db.GetProgress().Error(Numerics::Unexpected_database_error,
			       "Upload(...) called for a directory!\n");
	return NULL;
    }
    
    // grab the base branch stack that everything leafs off
    resultstack = root_relative ? BranchStack() : cwdstack;
    
    // grab the final playlist that we're based off
    FidPlaylist *playlist = resultstack.GetChildPlaylist(db);

    // this isn't a directory, so we don't search the last string (obviously -- it's the file)
    list<string>::const_iterator last_path = --end();

    // loop through from start of path to the end of it
    for(list<string>::const_iterator pit = begin(); pit != last_path; pit ++) {
	// grab the next branch name we need to create
	const char *dirname = pit->c_str();
	int playlistlen = playlist->size();
	// check it already exists
	for(int i=0; i<playlistlen; i++) {
	    FidPlaylist *sublist = playlist->GetPlaylistAt(i);
	    if(sublist) {
		if(!strcmp(sublist->GetTitle(), dirname)) {
		    // there exists a playlist with this name
		    resultstack.push_back(PlaylistBranch(playlist, i));
		    playlist = sublist;
		    continue;
		}
	    }
	}
	// ok there wasn't already one, so we've messed up with the previous call
	db.GetProgress().Error(Numerics::Unexpected_database_error,
			       "Playlist path wasn't created for tune \"%s\"\n",
			       GetName().c_str());
	return NULL;
    }

    // 'playlist' now points to the parent playlist
    FidLocalFile *file = dynamic_cast<FidLocalFile *>
	(playlist->GetNodeAt(playlist->Upload(title, GetPath().c_str())));
	 
    return file;
}

**************/

/**************

bool DirectoryEntry::Exists(const char *title,
			    PlayerDatabase &db, const BranchStack &cwdstack) const
{
    // grab the base branch stack that everything leafs off
    BranchStack basestack = root_relative ? BranchStack() : cwdstack;
    
    // grab the final playlist that we're based off
    FidPlaylist *playlist = basestack.GetChildPlaylist(db);

    // this isn't a directory, so we don't search the last string (obviously -- it's the file)
    list<string>::const_iterator last_path = end();
    if(!IsDirectory()) last_path--;

    // loop through from start of path to the end of it
    for(list<string>::const_iterator pit = begin(); pit != last_path; pit ++) {
	// grab the next branch name we need to create
	const char *dirname = pit->c_str();
	int playlistlen = playlist->size();
	// check it already exists
	for(int i=0; i<playlistlen; i++) {
	    FidPlaylist *sublist = playlist->GetPlaylistAt(i);
	    if(sublist) {
		if(!strcmp(sublist->GetTitle(), dirname)) {
		    // there exists a playlist with this name
		    playlist = sublist;
		    continue;
		}
	    }
	}

	// nope, nothing matched
	return false;
    }

    // if it was just a directory check, we're done -- found it
    if(IsDirectory()) return true;

    // otherwise we need to check the length etc
    int playlistlen = playlist->size();
    for(int i=0; i<playlistlen; i++) {
	FidNode *node = playlist->GetNodeAt(i);
	if(!strcmp(node->GetTitle(), title)) {
	    if(atol(node->GetTag(db.GetTagNumber("length"))) == GetLength()) return true;
	}
    }
    return false;
}

***********/
    
//////////////////////////////////////////////////////////////////////////////
// ConsoleInterface -- user interaction through here (Or Else)
//////////////////////////////////////////////////////////////////////////////

// Contructuctor

ConsoleInterface::ConsoleInterface(PlayerDatabase &db)
    : db(db), progress(dynamic_cast<ConsoleProgress *>(db.GetProgress())),
      do_reporting(true), debug_level(0)
{
}

// Destructor (muhaha)

ConsoleInterface::~ConsoleInterface()
{
    // doh, nothing to do, mate
}

// Init -- download database, check for faults

int ConsoleInterface::Init()
{
    progress->OperationStart(Numerics::Download_start, "Downloading");
    // grab the database
    STATUS err = db.Download();
    if (FAILED(err)) {
	// download failed. oh dear
	return 0;
    }
    
    // we need the root playlist
    root = db.GetRootPlaylist();
    if(!root) {
	// root playlist doesn't exist!
	progress->Error(Numerics::Invalid_root_playlist,
		       "Invalid or missing root playlist!\n");
	return 0;
    }

    // check for faults
    int repairs = db.Repair(0);
    if(repairs) {
	// tell user there's unrepaired errors in the database
	progress->Error(Numerics::Unrepaired_errors,
		       "There are %d unrepaired errors in the database\n", repairs);
    }

    // current directory: ROOT
    cwlist = root;
    cwlistfid = FID_ROOTPLAYLIST;
    // current directory: ROOT/
    cwdstack.clear();

    // "clean" (we just downloaded the database afterall)
    dirty = 0;

    return 1;
}

// Synchronise changes with player

int ConsoleInterface::Synchronise()
{
    // synchronise everything (this looks deceptively small!)

    progress->OperationStart(Numerics::Synchronising, "Synchronising");
    progress->OperationUpdate(0, 1);
    if( FAILED(db.Synchronise()) ) {	// disappear for a few minutes
	// synchronise failed (call should have done a progress->Error())
	return 0;
    }
    progress->OperationUpdate(1, 1);

    progress->Log(Numerics::Sync_finished, "Finished synchronise\n");
    
    // reinitialise

    progress->OperationStart(Numerics::Downloading, "Downloading");

    progress->OperationUpdate(0, 1);
    root = db.GetRootPlaylist();
    if(!root) {
	// root playlist doesn't exist!
	progress->Error(Numerics::Invalid_root_playlist,
		       "Invalid or missing root playlist!\n");
	return 0;
    }
    progress->OperationUpdate(1, 1);
    
    progress->Log(Numerics::Download_finished, "Finished download\n");

    // check for faults

    int repairs = db.Repair(0);
    if(repairs) {
	progress->Error(Numerics::Unrepaired_errors,
		       "There are %d unrepaired errors in the database\n", repairs);
    }

    // current directory to root
    cwlist = root;
    cwlistfid = FID_ROOTPLAYLIST;
    // directory stack to root
    cwdstack.clear();

    // no unsynchronised changes
    dirty = 0;

    return 1;
}

// Get a line of input from the user

string ConsoleInterface::GetLine(char *prompt) throw(InputEOF)
{
    // print prompt
    fprintf(stdout, "%s", prompt);
    fflush(stdout);

    char line[1024];
    if(!fgets(line, 1024, stdin)) throw InputEOF();

    int len = strlen(line);
    if(line[len - 1] == '\n') line[len - 1] = 0;

    return line;
}

// Process the input command -- convert to tokens and pass it along

int ConsoleInterface::ProcessLine()
{
    char prompt[1024];
    string cwdprompt = BranchStackToPath(cwdstack);
    snprintf(prompt, 1023, "%s> ", cwdprompt.c_str());
    prompt[1023] = 0;

    try {
	char line[1024];
	strncpy(line, GetLine(prompt).c_str(), 1023);
	line[1023] = 0;
	
	StringList tokens;
	if(!Tokenize(line, tokens)) return 1;
	
	if(!tokens.size()) return 1;
	
	return ProcessCommand(tokens);
    }
    catch(InputEOF) {
	return 0;
    }
}

// Convert a character string to a list of tokens, called from ProcessLine()

int ConsoleInterface::Tokenize(const char *line, StringList &tokens)
{
    static const int Whitespace = 1, Text = 2, Quoted = 3;

    int state = Whitespace;
    bool escape_next = false;
    std::string token;

    for(;;) {
	char c = *line;

	if(escape_next) {
	    switch(c) {
	    case 0:
	    case '\n':
		progress->Error(Numerics::Parse_error,
			       "Unexpected escape '\\' at end of string\n");
		return 0;
		
	    default:
		token += c;
		line ++;
	    }
	    escape_next = false;
	    continue;
	}
	
	switch(state) {
	case Whitespace:
	    switch(c) {
	    case 0:
		return 1;

	    case '\n':
	    case ' ':
	    case '\t':
		line++;
		break;

	    case '\"':
		state = Quoted;
		line ++;
		break;

	    case '\\':
		escape_next = true;
		line ++;
		break;

	    default:
		state = Text;
		token = "";
		break;
	    }
	    break;

	case Text:
	    switch(c) {
	    case 0:
		tokens.push_back(token);
		token = std::string();
		return 1;

	    case ' ':
	    case '\t':
	    case '\n':
		line++;
		state = Whitespace;
		tokens.push_back(token);
		token = std::string();
		break;

	    case '\"':
		progress->Error(Numerics::Parse_error,
			       "Unexpected quote '\"' in string\n");
		return 0;

	    case '\\':
		escape_next = true;
		line ++;
		break;

	    default:
		token += c;
		line ++;
		break;
	    }
	    break;

	case Quoted:
	    switch(c) {
	    case 0:
		progress->Error(Numerics::Parse_error,
			       "Unexpected end of line in quoted string\n");
		return 0;

	    case '\"':
		tokens.push_back(token);
		token = std::string();
		line ++;
		state = Whitespace;
		break;

	    case '\\':
		escape_next = true;
		line ++;
		break;

	    default:
		token += c;
		line ++;
		break;
	    }
	    break;
	}
    }
    
    return 0;	// silly compiler, this isn't reachable! (don't prove me wrong though)
}

// Take the list of tokens and execute the appropriate command (1st token)

// ASSUMPTION -- tokens.size() >= 1 on entry
int ConsoleInterface::ProcessCommand(StringList &tokens)
{
    string command = *tokens.begin();

    // don't even bother telling me i should be using a hash table or tree
    
    if(command == "help") {
	return Parse_help(tokens);
    }
    else if(command == "hash") {
	return Parse_hash(tokens);
    }
    else if(command == "ls") {
	return Parse_ls(tokens);
    }
    else if(command == "cd") {
	return Parse_cd(tokens);
    }
    else if(command == "pwd") {
	return Parse_pwd(tokens);
    }
    else if(command == "mklist") {
	return Parse_mklist(tokens);
    }
    else if(command == "link") {
	return Parse_link(tokens);
    }
    else if(command == "rm") {
	return Parse_rm(tokens);
    }
    else if(command == "repair") {
	return Parse_repair(tokens);
    }
    else if(command == "upload") {
	return Parse_upload(tokens);
    }
    else if(command == "lookup") {
	return Parse_lookup(tokens);
    }
    else if(command == "set") {
	return Parse_set(tokens);
    }
    else if(command == "unset") {
	return Parse_unset(tokens);
    }
    else if(command == "move") {
	return Parse_move(tokens);
    }
    else if(command == "rename") {
	return Parse_rename(tokens);
    }
    else if(command == "sync") {
	return Parse_sync(tokens);
    }
    else if(command == "config") {
	return Parse_config(tokens);
    }
    else if((command == "quit") ||
	    (command == "exit") ||
	    (command == "bye") ||
	    (command == "q") ||
	    (command == "cows_have_spoons")) {
	return Parse_quit(tokens);
    }
    else {
	progress->Error(Numerics::Unknown_command, "Unknown command\n");
	return 1;
    }
}

int ConsoleInterface::FindPosForIdentifier(const FidPlaylist &playlist, const char *identifier)
{
    // grab length of playlist
    int len = playlist.size();

    // check if it's a special identifier: "#begin" "#end" "#{digits}" "%{hex digits}"
    if(identifier[0] == '#') {
	if(!identifier[1]) return -1;
	
	if(!strcmp(identifier+1, "begin")) {
	    if(!len) return -1;				// can't have a #begin of an empty list
	    else return 0;
	}
	else if(!strcmp(identifier+1, "end")) {
	    if(!len) return -1;				// can't have a #end of an empty list too
	    else return len - 1;
	}
	else {
	    // must be digits - but check this assumption first
	    for(const char *p = identifier+1; *p; p++) if(!isdigit(*p)) return -1;
	    int n = atol(identifier+1) - 1;
	    // validate against array bounds
	    return ((n >= 0) && (n < len)) ? n : -1;
	}
    }
    else if(identifier[0] == '%') {
	if(!identifier[1]) return -1;
	
	// check we're actually looking at hex digits
	for(const char *p = identifier+1; *p; p++) if(!isxdigit(*p)) return -1;
	int n = -1;
	sscanf(identifier+1, "%x", &n);			// grab value (icky sscanf)
	return n;
    }

    // otherwise it's a title of a member of the list
    for(int i=0; i<len; i++) {
	FidNode *node = playlist.GetNodeAt(i);
	if(!node) continue;				// paranoia

	const char *title = node->GetTitle();		// grab title of this node
	if(!strcasecmp(title, identifier)) 	  
	    return i;	// positive match - return position
    }

    // ok, no Title Matches... it's bogus
    return -1;
}

// Convert a supplied directory stack and a path to a BranchStack, which uniquely identifies
//   the position of a node relative to initstack.

BranchStack ConsoleInterface::PathToBranchStack(const BranchStack &initstack,
						const char *path_name)
{
    // initialise the branch stack from initstack
    BranchStack branches = initstack;
    
    int len = strlen(path_name);

    // copy the path_name into a local buffer we can plonk NULLs into
    auto_ptr<char> name_temp(new char[len+1]);
    char *name = name_temp.get();			// auto_ptr<> is my latest fetish
    strcpy(name, path_name);

    // check empty or if the path starts with a '/' -- indicating root directory relative
    if(name[0] == 0) {
	return branches;				// return given branches
    }
    else if(name[0] == '/') {
	// skip the character
	name ++;
	len --;
	// clear branch stack, which means root
	branches.clear();
	// if the string is just "/", then return a blank BranchStack
	if(!name[0]) return BranchStack();		// return empty branch stack == root
	// continue onwards...
    }
    // otherwise we've got a branch stack as given to start with

    // current playlist is either the root playlist or current working directory
    FidPlaylist *playlist;
    if(!branches.size()) playlist = root;
    else {
	playlist = branches.rbegin()->GetChildPlaylist(db);
	// check it's actually a playlist
	if(!playlist) throw NotAPlaylist(branches.rbegin()->GetChildNode(db)->GetTitle());
    }
    
    char subfid[1024];

    // loop through the string
    int offset = 0;				// offset into storing in subfid[]

    bool playlistid = false, eos = false;
    do {
	char c = *name++;
	if(c == '/') {
	    if(*name == '/') {
		throw InvalidPathName(path_name);	// can't have '//'
	    }
	    c = 0;				// convert '/' to a NULL if necessary
	    playlistid = true;			// definitely a playlist identifier
	}
	else if(!c) {
	    eos = true;
	    playlistid = false;			// not necessarily a playlist identifier
	}

	subfid[offset++] = c;
	if(c) continue;				// keep going until we hit a terminator
	
	offset = 0;				// start at beginning of subfid[] again

	if(!strcmp(subfid, ".")) {		// is string "."?
	    continue;				//   == current directory, skip it
	}
	else if(!strcmp(subfid, "..")) {	// is string ".."?
	    if(!branches.size()) {
		throw InvalidPathName(path_name);	// too much backtracking?
	    }
	    playlist = branches.rbegin()->GetParentPlaylist();
	    // take last branch off the stack
	    branches.pop_back();

//	    printf("backtrack to: %s\n", playlist->GetTitle());
//	    printf("new stack: %s\n", BranchStackToPath(branches).c_str());
	    
	    continue;
	}

	// ok, we've got an identifier to deal with here
	int pos = FindPosForIdentifier(*playlist, subfid);
	// check it exists
	if(pos == -1) {
	    throw FidNotFound(subfid);		// nope!
	}

	// if it's definitely a playlist identifier, check this fid really is a playlist
	if(playlistid) {
	    // grab the node
	    FidNode *node = playlist->GetNodeAt(pos);
	    FidPlaylist *sublist = dynamic_cast<FidPlaylist *>(node);	// try casting it
	    if(!sublist) {
		// tune used as a playlist
		throw NotAPlaylist(node->GetTitle());
	    }
	    // push this onto the end of the directory stack
	    branches.push_back(PlaylistBranch(playlist, pos));
	    playlist = sublist;
	}
	else
	{
	    // push this branch onto the end of the directory stack
	    branches.push_back(PlaylistBranch(playlist, pos));
	}
    } while(!eos);

    // yay
    return branches;
}

// given a BranchStack, convert to a literal path
string ConsoleInterface::BranchStackToString(const BranchStack &branch_stack)
{
    BranchStack::const_iterator branch_it = branch_stack.begin();

    if(branch_stack.size()) {
	if(branch_it->GetParentPlaylist() != root) {
	    progress->Error(Numerics::Unexpected_interface_error,
			   "BranchStackToPath called with a non-empty stack "
			   "not starting at root!");
	    return "/";
	}
    }
    
    string path;

    for(; branch_it != branch_stack.end(); branch_it ++) {
	FidNode *node = branch_it->GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Unexpected_database_error,
			   "PlaylistBranch has non-existant node: %s[%d]\n",
			   branch_it->GetParentPlaylist()->GetTitle(),
			   branch_it->GetChildPos());
	    return "?";
	}
	path += "/";
	path += node->GetTitle();
    }

    return path;
}

// given a BranchStack, convert to a literal path but have a '/' for empty strings
string ConsoleInterface::BranchStackToPath(const BranchStack &branch_stack)
{
    string path = BranchStackToString(branch_stack);
    if(!path.size()) return "/";
    else return path;
}

// Expand an identifier (pattern) with wildcards into the branch list
// may throw - InvalidPathName, FidNotFound, NotAPlayList

int ConsoleInterface::ExpandIdentifier(BranchStackList &branchlist,
				       BranchStackList::iterator &it,
				       const BranchStack &initstack,
				       const char *pattern)
{
    // strip off the last identifier in the path (wahey!)

    int len = strlen(pattern);
    int i;
    for(i=len-1; i>-1; i--) {
	if(pattern[i] == '/') break;
    }

    i++;	// point to character after separator (or point to beginning)
    
    // wildpattern == last identifier in path
    const char *wildpattern = pattern + i;

    // directory == all identifiers in path except last
    auto_ptr<char> directory_temp(new char[len+1]);
    char *directory = directory_temp.get();
    
    // have a leading relative directory
    if(i > 1) {
	strncpy(directory, pattern, i-1);
	directory[i-1] = 0;
    }
    else if(i == 1) {		// start from root
	directory = "/";
    }
    else {			// no directory
	directory[0] = 0;
    }

//    printf("expand: directory \"%s\" wildcard \"%s\"\n", directory, wildpattern);
    
//    printf("initstack: %s\n", BranchStackToString(initstack).c_str());
    
    // use the base directory to get a base branch stack
    BranchStack base_branch = PathToBranchStack(initstack, directory);

//    printf("base_branch: %s\n", BranchStackToString(base_branch).c_str());
    
    FidPlaylist *base_playlist;
    
    // check the last branch is actually a playlist!
    if(!base_branch.size()) base_playlist = root;
    else {
	PlaylistBranch &branch = *base_branch.rbegin();
	base_playlist = branch.GetChildPlaylist(db);
	if(!base_playlist) {
	    string dir = BranchStackToString(base_branch);
	    progress->Error(Numerics::Not_a_playlist,
			   "\"%s\" is not a playlist\n", dir.c_str());
	    return 0;
	}
    }

    if(!wildpattern[0] || !strcmp(wildpattern, ".")) {
	it = ++branchlist.insert(it, base_branch);
//	printf("expand: added %s\n", BranchStackToString(base_branch).c_str() );
	return 1;
    }
    else if(!strcmp(wildpattern, "..")) {
	if(base_branch.size() == 1) {
	    it = ++branchlist.insert(it, BranchStack());
	    return 1;
	}
	else if(!base_branch.size()) {
	    return 0;
	}
	base_branch.pop_back();
	it = ++branchlist.insert(it, base_branch);
	return 1;
    }
    
    int matches = 0;
    
    // now loop through all fids for this playlist and wildcard match it
    int playlistlen = base_playlist->size();

    for(int i=0; i<playlistlen; i++) {
	FidNode *node = base_playlist->GetNodeAt(i);
	if(!node) {
	    matches --;
	    continue;
	}

	const char *title = node->GetTitle();
	if(strcasewild(title, wildpattern)) {
	    BranchStack this_branch = base_branch;
	    this_branch.push_back(PlaylistBranch(base_playlist, i));
	    branchlist.insert(it, this_branch);
	    printf("expand: added %s\n", BranchStackToString(this_branch).c_str() );
	    matches ++;
	}
    }

    if ( !matches )
    {
	char namebuf[16];
	char namebuf2[16];

	// go through again matching against #n and fid numbers
	//
	// this is separate from the above loop so that if you have a song
	// called "#begin" or "5d0" then emptool goes for the real name rather
	// than the alternative name
	for ( int i=0; i<playlistlen; i++ )
	{
	    FidNode *node = base_playlist->GetNodeAt(i);
	    
	    if ( node )
	    {
		sprintf( namebuf, "#%d", i+1 );
		sprintf( namebuf2, "%x", node->GetFid() );
		if ( strcasewild( namebuf, wildpattern )
		     || strcasewild( namebuf2, wildpattern )
		     || (i==0 && strcasewild("#begin",wildpattern))
		     || (i==playlistlen-1 && strcasewild("#end",wildpattern)) )
		{
		    BranchStack this_branch = base_branch;
		    this_branch.push_back(PlaylistBranch(base_playlist, i));
		    branchlist.insert(it, this_branch);
		    printf("expand: added %s\n",
			   BranchStackToString(this_branch).c_str() );
		    matches ++;
		}
	    }
	}
    }

    return matches;
}

string ConsoleInterface::GetBasePath(const char *pattern)
{
    if(!*pattern) return string();
    else if(!strcmp(pattern, "/")) {
	return string();
    }
    else if(strstr(pattern, "//")) {
	return string();
    }
    
    string pat;
    int len = strlen(pattern);
    if(pattern[len-1] == '/') pat = string(pattern, len - 1);
    else pat = pattern;

    const char *patstr = pat.c_str();
    const char *ending = strrchr(patstr, '/');
    if(!ending) return string();
    else return string(patstr, ending - patstr);
}

string ConsoleInterface::GetBaseFile(const char *pattern)
{
    const char *ending = strrchr(pattern, '/');
    const char *pat;
    if(!ending) pat = pattern;
    else pat = ending + 1;

    int len = strlen(pat);
    if(pat[len-1] == '/') return string(pat, len-1);
    else return pat;
}

int ConsoleInterface::ExpandShellGlob(DirectoryEntryList &direntrylist,
				      DirectoryEntryList::iterator &it,
				      const char *pattern)
{
    if(!*pattern) return 0;
    
    string base_path = GetBasePath(pattern);
    string glob_pattern = GetBaseFile(pattern);

    struct dirent **namelist;
    
    int n;
    if(base_path.size()) {
//	printf("scandir: dir \"%s\" glob \"%s\"\n", base_path.c_str(), glob_pattern.c_str());
	n = scandir(base_path.c_str(), &namelist, NULL, alphasort);
    }
    else {
//	printf("scandir: dir . glob \"%s\"\n", glob_pattern.c_str());
	n = scandir(".", &namelist, NULL, alphasort);
    }

    if(n < 0) {
	printf("scandir failed with errno %d (%s)\n", errno, strerror(errno));
	return 0;
    }

    int total = 0;
    
    const char *glob = glob_pattern.c_str();
//    printf("list: %d\n", n);
    for(int i=0; i<n; i++) {
	const char *name = namelist[i]->d_name;
	if(!strcmp(name, ".") ||
	   !strcmp(name, "..")) {
	    free(namelist[i]);
	    continue;
	}
//	printf("%d: %s\n", i, namelist[i]->d_name);
	if(strwild(name, glob)) {
//	    printf("Matched: %s\n", name);
	    string full_path = base_path.size() ? base_path + string("/") + name : name;
	    try {
		direntrylist.insert(it, DirectoryEntry(name, full_path.c_str()));
		total ++;
	    }
	    catch(DirectoryEntry::StatFailed &s) {
		if(s.err == ENOENT) {
		    progress->Error(Numerics::Filesystem_error,
				   "File not found: \"%s\"\n", full_path.c_str());
		}
		else {
		    progress->Error(Numerics::Filesystem_error,
				   "Error on file \"%s\": %d (%s)\n",
				   full_path.c_str(), s.err, strerror(s.err));
		}
	    }
	}
	free(namelist[i]);
    }

    free(namelist);

    return total;
}

int ConsoleInterface::GetExpandFlags(const StringList &args, int flags)
{
    StringList::const_iterator it = args.begin();
    for(; it != args.end(); it++) {
	const char *pattern = it->c_str();
	if(pattern[0] == '-') {
	    if(strchr(pattern, 'd')) {
		flags &= ~ExpandFlags::Expand_playlists;
	    }
	    if(strchr(pattern, 'r')) {
		flags |= ExpandFlags::Recurse;
	    }
	    if(strchr(pattern, 'l')) {
		flags |= ExpandFlags::Long_listing;
	    }
	    if(strchr(pattern, 'e')) {
		flags |= ExpandFlags::Escape_paths;
	    }
	    if(strchr(pattern, 'p')) {
		flags |= ExpandFlags::Use_existing;
	    }
	}
    }
    
    return flags;
}

int ConsoleInterface::ExpandArgumentsDatabase(StringList::const_iterator begin,
					      StringList::const_iterator end,
					      BranchStackList &branch_list,
					      int flags)
{
    int total = 0;
    bool have_list = false;

//    printf("parse_ls: cwdstack = \"%s\"\n", BranchStackToString(cwdstack).c_str());

    for(StringList::const_iterator args_it = begin; args_it != end; args_it ++) {
	// get next token
	const char *pattern = args_it->c_str();
	if(pattern[0] == '-') continue;

	have_list = true;				// we have at least one list argument
	BranchStackList::iterator it = branch_list.end();
	// expand wildcards and append to list
	int matches = ExpandIdentifier(branch_list, it, cwdstack, pattern);
	// this may throw lots of things

	if ( !matches )
	{
	    progress->Error( Numerics::No_matches_found,
			     "No matches for %s\n", pattern );

	    return 0;
	}

	total += matches;
    }

    // if there were no command line identifiers, list the current directory
    if(!have_list) {
	// if we don't want to default if nothing is specified, then just do nothing
	if(!(flags & ExpandFlags::Default_current)) return 0;

	// don't expand base playlists in first pass
	flags &= ~ExpandFlags::Expand_playlists;

	BranchStackList::iterator it = branch_list.end();
	// throws all sorts of things
	total = ExpandIdentifier(branch_list, it, cwdstack, "*");
	if(!total) return 0; 	// early out if there's nothing expanded
    }
    
    if((flags & ExpandFlags::Expand_playlists) && !(flags & ExpandFlags::Recurse)) {
	BranchStackList from_list = branch_list;		// copy old list
	branch_list.clear();					// erase current list

	// loop through list, adding tunes and expanding playlists
	BranchStackList::iterator branch_it = from_list.begin();
	for(; branch_it != from_list.end(); branch_it ++) {
	    BranchStack &branches = *branch_it;
//	    printf("Expanding: (%d) %s\n", branches.size(), BranchStackToPath(branches).c_str());
	    if(!branches.size()) {
		BranchStackList::iterator it = branch_list.end();
		total += ExpandIdentifier(branch_list, it, branches, "*");
	    }
	    else {
		PlaylistBranch &branch = *branches.rbegin();
		FidNode *node = branch.GetChildNode(db);
		if(!node) {
		    progress->Error(Numerics::Fid_not_found,
				   "doh: %s[%d] (not found)\n",
				   branch.GetParentPlaylist()->GetTitle(),
				   branch.GetChildPos());
		}
		else {
		    FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
		    if(!playlist) {
			branch_list.push_back(branches);
		    }
		    else {
			BranchStackList::iterator it = branch_list.end();
			total += ExpandIdentifier(branch_list, it, branches, "*");
		    }
		}
	    }
	}
    }
    else if(flags & ExpandFlags::Recurse) {
	// loop through list, adding tunes and expanding playlists as we go
	BranchStackList::iterator branch_it = branch_list.begin();
	while(branch_it != branch_list.end()) {
	    BranchStack branches = *branch_it;
//	    printf("Expanding: (%d) %s\n", branches.size(), BranchStackToPath(branches).c_str());
	    if(!branches.size()) {
		branch_it = branch_list.erase(branch_it);
		total --;
		BranchStackList::iterator it = branch_list.end();
		total += ExpandIdentifier(branch_list, it, branches, "*");
		// we'll hit these entries later on
	    }
	    else {
		PlaylistBranch &branch = *branches.rbegin();
		FidNode *node = branch.GetChildNode(db);
		if(!node) {
		    progress->Error(Numerics::Fid_not_found,
				   "doh: %s[%d] (not found)\n",
				   branch.GetParentPlaylist()->GetTitle(),
				   branch.GetChildPos());
		    branch_it = branch_list.erase(branch_it);
		    total --;
		}
		else {
		    FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
		    if(playlist) {
			BranchStackList::iterator it = branch_list.end();
			total += ExpandIdentifier(branch_list, it, branches, "*");
			if(!(flags & ExpandFlags::Include_recurses)) {
			    branch_it = branch_list.erase(branch_it);
			    total --;
			}
			else branch_it ++;
			// we'll hit these entries later on too
		    }
		    else branch_it++;
		}
	    }
	}
    }

    return total;
}

// bit of a copy and paste

int ConsoleInterface::UploadFiles(StringList::const_iterator begin,
				  StringList::const_iterator end,
				  BranchStack &initstack,
				  BranchStackList &branch_list, int flags)
{
    int total = 0;

//    printf("parse_ls: cwdstack = \"%s\"\n", BranchStackToString(cwdstack).c_str());

    for(StringList::const_iterator args_it = begin; args_it != end; args_it ++) {
	// get next token
	const char *pattern = args_it->c_str();
	if(pattern[0] == '-') continue;

	DirectoryEntryList direntry_list;
	DirectoryEntryList::iterator it = direntry_list.end();
	// expand wildcards and append to list
	// this may throw lots of things
	total += ExpandShellGlob(direntry_list, it, pattern);

	FidPlaylist *curlist = initstack.GetChildPlaylist(db);

	it = direntry_list.begin();
	while(it != direntry_list.end()) {
	    DirectoryEntry &direntry = *it;
	    const char *name = direntry.GetName();
	    if(direntry.IsDirectory()) {
		if(flags & ExpandFlags::Recurse) {
		    int pos = curlist->HasName(name);

		    if ( pos >= 0 && !(flags & ExpandFlags::Use_existing) )
			continue;

		    if(pos == -1) {
			pos = curlist->CreatePlaylist(name);
			FidPlaylist *sublist = curlist->GetPlaylistAt(pos);
			if(!sublist) {
			    printf("oopsie, couldn't recurse directory->playlist\n");
			    it = direntry_list.erase(it);
			    continue;
			}
		    }
			BranchStack substack = initstack;
			substack.push_back(PlaylistBranch(curlist, pos));
			StringList string_list;
			string next_dir = direntry.GetFullPath();
			next_dir += "/*";
			string_list.push_back(next_dir);
			branch_list.push_back(substack);
			UploadFiles(string_list.begin(), string_list.end(),
				    substack, branch_list, flags);
			//}
		}
	    }
	    else {
 		for(int i=0; i<curlist->size(); i++) {
		    FidNode *node = curlist->GetNodeAt(i);
		    if(!node) continue;
		    if(!strcmp(node->GetTitle(), name)) {
			int nodelength = atol(node->GetTag(db.GetTagNumber("length")));
			if(nodelength == direntry.GetLength()) {
			    it = direntry_list.erase(it);
			    goto erased_entry;
			}
		    }
		}
		
		int pos = curlist->Upload(name, direntry.GetFullPath());
		FidNode *node = curlist->GetNodeAt(pos);
		
		TagExtractor *tex;

		STATUS status = TagExtractor::Create( direntry.GetFullPath(),
						      "", &tex);
		TagExtractToMap teo;

		if (SUCCEEDED(status))
		{
		    status = tex->ExtractTags( direntry.GetFullPath(),
					       &teo );
		}

		if (FAILED(status)) {
		    it = direntry_list.erase(it);
		    goto erased_entry;
		}
		else {
		    node->SetTitle( teo["title"].c_str() );

		    for (TagExtractToMap::const_iterator ii = teo.begin();
			 ii != teo.end();
			 ++ii)
		    {
			node->SetTag( db.GetTagNumber(ii->first.c_str()),
				      ii->second.c_str() );
		    }
		}
		
		BranchStack substack = initstack;
		substack.push_back(PlaylistBranch(curlist, pos));
		branch_list.push_back(substack);
	    }
	    it++;
	erased_entry:
	    ;
	}
    }

    return total;
    
/*************
  
    // if there were no command line identifiers, list the current directory
    if(!have_list) {
	// if we don't want to default if nothing is specified, then just do nothing
	if(!(flags & ExpandFlags::Default_current)) return 0;

	// don't expand base playlists in first pass
	flags &= ~ExpandFlags::Expand_playlists;

	DirectoryEntryList::iterator it = direntry_list.end();
	// throws all sorts of things
	total = ExpandShellGlob(direntry_list, it, "*");
	if(!total) return 0; 	// early out if there's nothing expanded
    }

    
    if((flags & ExpandFlags::Expand_playlists) && !(flags & ExpandFlags::Recurse)) {
	DirectoryEntryList from_list = direntry_list;		// copy old list
	direntry_list.clear();					// erase current list

	// loop through list, adding tunes and expanding playlists
	DirectoryEntryList::iterator direntry_it = from_list.begin();
	for(; direntry_it != from_list.end(); direntry_it ++) {
	    DirectoryEntry &direntry = *direntry_it;
//	    printf("Expanding: (%d) %s\n", branches.size(), BranchStackToPath(branches).c_str());
	    if(direntry.IsDirectory()) {
		DirectoryEntryList::iterator it = direntry_list.end();
		total += ExpandShellGlob(direntry_list, it, "*");
	    }
	    else {
		PlaylistBranch &branch = *direnrty.rbegin();
		FidNode *node = branch.GetChildNode(db);
		if(!node) {
		    progress->Error(Numerics::Fid_not_found,
				   "doh: %s[%d] (not found)\n",
				   branch.GetParentPlaylist()->GetTitle(),
				   branch.GetChildPos());
		}
		else {
		    FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
		    if(!playlist) {
			branch_list.push_back(branches);
		    }
		    else {
			DirectoryEntryList::iterator it = branch_list.end();
			total += ExpandShellGlob(branch_list, it, "*");
		    }
		}
	    }
	}
    }
    else if(flags & ExpandFlags::Recurse) {
	// loop through list, adding tunes and expanding playlists as we go
	DirectoryEntryList::iterator direntry_it = branch_list.begin();
	while(direntry_it != direntry_list.end()) {
	    DirectoryEntry direntry = *branch_it;
//	    printf("Expanding: (%d) %s\n", branches.size(), BranchStackToPath(branches).c_str());
	    if(direntry.IsDirectory()) {
		// actually let's keep all of the directories
//		direntry_it = direntry_list.erase(direntry_it);
//		total --;
		DirectoryEntryList::iterator it = direntry_list.end();
		total += ExpandShellGlob(branch_list, it, "*");
		// we'll hit these entries later on
	    }
	}
    }

    return total;
****************/
}

bool ConsoleInterface::AllowedTag(const char *tag)
{
    if(!strcmp(tag, "type")) return false;
    else if(!strcmp(tag, "length")) return false;
    else return true;
}

string ConsoleInterface::EscapePath(const string &path)
{
    string out;
    for(string::const_iterator it = path.begin(); it != path.end(); it ++) {
	char c = *it;
	if(c == ' ') out += "\\ ";
	else out += c;
    }
    return out;
}

int ConsoleInterface::GetLength(FidNode *node)
{
    FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
    if(playlist) return playlist->size();
    return (int) atol(node->GetTag(db.GetTagNumber("length"))); // slow slow slow. damn
}

void ConsoleInterface::PrintNode(const BranchStack &nodestack, int flags, bool is_recurse)
{
    string title;
    FidNode *node;
    FidPlaylist *playlist;

    if(nodestack.size()) {
	title = BranchStackToPath(nodestack).c_str();
	if(flags & ExpandFlags::Escape_paths) title = EscapePath(title);
	
	node = nodestack.GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Fid_not_found,
			   "doh: \"%s\"[%d] (not found)\n",
			   nodestack.GetParentPlaylist()->GetTitle(),
			   nodestack.GetChildPos());
	    return;
	}
	playlist = dynamic_cast<FidPlaylist *>(node);
    }
    else {
	title = "/";
	
	playlist = db.GetRootPlaylist();
	node = playlist;
    }
    
    if(!(flags & ExpandFlags::Long_listing)) {
	if(playlist && is_recurse) printf("\n%s:\n\n", title.c_str());
	else printf("%s\n", title.c_str());
    }
    else {
	if(playlist && is_recurse) {
	    printf("\n%8x %-8s %-51s %9d\n\n",
		   node->GetFid(),
		   node->GetType(),
		   title.c_str(),
		   GetLength(node));
	}
	else {
	    printf("%8x %-8s %-51s %9d\n",
		   node->GetFid(),
		   node->GetType(),
		   title.c_str(),
		   GetLength(node));
	}
    }

}

// "help" command

int ConsoleInterface::Parse_help(const StringList &tokens)
{
    if(tokens.size() != 2) {
	printf("Help\n"
	       "\n"
	       "Help is split into sections, accessed with the following commands:\n"
	       "\n"
	       "help commands      - Available commands\n"
	       "(more in future)\n");
	return State::OK;
    }

    StringList::const_iterator token_it = tokens.begin();
    token_it ++;

    if(*token_it == "commands") {
	printf("List of commands\n"
	       "\n"
	       /*	// hehe
	       "Help available on individual commands (e.g \"help ls\")\n"
	       "\n"
	       */
	       "help        - This text\n"
	       "ls          - List entries\n"
	       "cd          - Change directory\n"
	       "mklist      - Make new playlist (directory)\n"
	       "link        - Link a tune or playlist into current playlist\n"
	       "rm          - Remove a link to a tune or playlist\n"
	       "repair      - Perform repairs on database\n"
	       "upload      - Upload a tune\n"
	       "lookup      - Lookup details on an entry\n"
	       "move        - Move an entry\n"
	       "set         - Set a tag for an entry\n"
	       "unset       - Unset a tag for an entry\n"
	       "rename      - Rename a tune or playlist\n"
	       "sync        - Synchronise changes with player (IMPORTANT)\n"
	       "quit        - Quit client\n");
    }
    /*	// muhaha
    else if(*token_it == "help") {
	printf("Help on \"help\"\n"
	       "\n"
	       "Displays help\n"
	       "\n"
	       "syntax:\n"
	       "  help <help topic>\n"
	       "\n"
	       "example:\n"
	       "  help help   (displays this screen)\n"
	       "  help ls     (displays help on ls)\n");
    }
    else if(*token_it == "ls") {
	printf("Help on \"ls\"\n"
	       "\n"
	       "List the contents of a playlist\n"
	       "\n"
	       "syntax:\n"
	       "  ls <playlist name or fid number>\n"
	       "\n"
	       "example:\n"
	       "  ls 1c0          (lists the contents of playlist fid 1c0)\n"
	       "  ls Foobar       (lists the contents of playlist \"Foobar\")\n"
	       "  ls \"Long Name\"  (lists the contents of playlist \"Long Name\")\n");
    }
    else if(*token_it == "cd") {
	printf("Help on \"cd\"\n"
	       "\n"
	       "Change directory (playlist)\n"
	       "\n"
	       "syntax:\n"
	       "  cd <playlist name or fid number>\n"
	       "\n"
	       "example:\n"
	       "  cd 1c0      (change to playlist fid 1c0)\n"
	       "  cd Foobar   (change to playlist Foobar)\n");
    }
    */
    else {
	// Eat my shorts
	printf("Don't have any help on \"%s\"\n", token_it->c_str());
    }

    return State::OK;
}

// "hash" command

int ConsoleInterface::Parse_hash(const StringList &tokens)
{
    if(tokens.size() > 1) {
	progress->Error(Numerics::Syntax_error, "hash takes no arguments\n");
	return State::OK;
    }

    do_reporting = !do_reporting;
//    progress->DoReporting(do_reporting);
    if(do_reporting) {
	progress->Log(Numerics::Hash_printing_on, "Hash mark printing on\n");
    }
    else {
	progress->Log(Numerics::Hash_printing_off, "Hash mark printing off\n");
    }

    return State::OK;
}

// "ls" command (duh)

int ConsoleInterface::Parse_ls(const StringList &tokens)
{
//    printf("parse_ls: cwdstack = \"%s\"\n", BranchStackToString(cwdstack).c_str());

    BranchStackList branch_list;			// start with empty branches list

    int flags = GetExpandFlags(tokens,
			       ExpandFlags::Default_current |
			       ExpandFlags::Expand_playlists);

    int total;

    try {
	// expand wilcards (hopefully we only get one expansion!)
	total = ExpandArgumentsDatabase(++tokens.begin(), tokens.end(), branch_list, flags);
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Tune or playlist not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    progress->Log(Numerics::Listing_follows, "Listing total: %d\n", total);
    
    // now take the branch list and, er, list it
    FidPlaylist *last_playlist = cwlist;

    BranchStackList::iterator branch_it = branch_list.begin();
    for(; branch_it != branch_list.end(); branch_it ++) {
	BranchStack &branches = *branch_it;

//	printf("BranchStack: %s\n", BranchStackToPath(branches).c_str());
	
	FidNode *node;
	if(!branches.size()) node = NULL;
	else {
	    PlaylistBranch &branch = *branches.rbegin();
	    node = branch.GetParentPlaylist();
	    if(!node) {
		progress->Error(Numerics::Fid_not_found,
			       "doh: \"%s\"[%d] (not found)\n",
			       branch.GetParentPlaylist()->GetTitle(),
			       branch.GetChildPos());
	    }
	}
	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	if(playlist && (flags & ExpandFlags::Recurse) && (playlist != last_playlist)) {
	    last_playlist = playlist;
	    BranchStack path_stack = branches;
	    path_stack.pop_back();

	    // print node details (is a recursion)
	    PrintNode(path_stack, flags, true);

	    if(!node) continue;
	    PlaylistBranch &branch = *branches.rbegin();
	    playlist = branch.GetChildPlaylist(db);
	    if(playlist) continue;
	}

	// otherwise just print the details normally
	PrintNode(branches, flags, false);
    }
    
//    printf("Total: %d tunes, %d playlists\n", tunes, playlists);
    printf("\n");

    return State::OK;
}

// "cd" command

int ConsoleInterface::Parse_cd(const StringList &tokens)
{
    // check number of arguments    
    if(tokens.size() != 2) {
	progress->Log(Numerics::Syntax_error, "cd <playlist ID number>\n");
	return State::Error_continue;
    }

    BranchStackList branch_list;			// start with empty branches list

    try {
	// expand wilcards (hopefully we only get one expansion!)
	int n = ExpandArgumentsDatabase(++tokens.begin(), tokens.end(),
					branch_list, ExpandFlags::None);
	
	if(!n) {
	    progress->Log(Numerics::No_matches_found,
			 "No matches for \"%s\"\n", (tokens.rbegin())->c_str());
	    return State::Error_continue;
	}
	else if(n > 1) {
	    // check the duplicates are the same (this is ok, of course)
	    BranchStackList::iterator branch_it = branch_list.begin();
	    const char *first_name = BranchStackToPath(*branch_it).c_str();
	    for(; branch_it != branch_list.end(); branch_it++) {
		if(strcmp(first_name, BranchStackToPath(*branch_it).c_str())) {
		    progress->Log(Numerics::Ambiguous_multiple_matches,
				 "Ambiguous multiple matches found, not changing\n");
		    return State::Error_continue;
		}
	    }
	}
    }
    catch(InvalidPathName &path) {
	progress->Log(Numerics::Invalid_path_name,
		     "Invalid path name: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Log(Numerics::Fid_not_found,
		     "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Log(Numerics::Not_a_playlist,
		     "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    BranchStackList::iterator branch_it = branch_list.begin();
    BranchStack branches = *branch_it;

    std::string path = BranchStackToPath(branches);
    FidPlaylist *playlist = branches.GetChildPlaylist(db);
    
    if(!playlist) {
	progress->Log(Numerics::Not_a_playlist,
		     "\nNot a playlist: \"%s\"\n\n", path.c_str());
	return State::Error_continue;
    }

    progress->Log(Numerics::Working_directory,
		 "Current directory: \"%s\"\n", path.c_str());

    cwdstack = branches;
    if (cwdstack.empty())
	cwlist = db.GetRootPlaylist();
    else
	cwlist = cwdstack.rbegin()->GetChildPlaylist(db);

    cwlistfid = cwlist->GetFid();
    return State::OK;
}

int ConsoleInterface::Parse_pwd(const StringList &tokens)
{
    if(tokens.size() != 1) {
	progress->Error(Numerics::Syntax_error,
		       "pwd takes no arguments\n");
	return State::Error_continue;
    }

    progress->Log(Numerics::Working_directory,
		 "Current directory: \"%s\"\n", BranchStackToPath(cwdstack).c_str());

    return State::OK;
}

int ConsoleInterface::Parse_mklist(const StringList &tokens)
{
    if(tokens.size() < 2) {
	progress->Error(Numerics::Syntax_error,
		       "mklist [-p] <identifier> [identifer ...]\n");
	return State::Error_continue;
    }

    StringList::const_iterator token_it = ++tokens.begin();

    // "mklist -p foo" uses an existing directory foo if any (whereas
    // "mklist foo" would create a second one)
    bool use_existing = false;
    if ( *token_it == "-p" )
    {
	use_existing = true;
	++token_it;
    }

    for(; token_it != tokens.end(); token_it ++) {
	string name = *token_it;
	FidPlaylist *playlist;

	int pos = cwlist->HasName( name.c_str() );

	if ( pos != -1 && use_existing )
	{
	    playlist = dynamic_cast<FidPlaylist *>(cwlist->GetNodeAt(pos));
	    if ( !playlist )
	    {
		progress->Error( Numerics::Exists_as_file,
				 "Can't create playlist \"%s\" - there is a file of that name\n", name.c_str() );
		continue;
	    }
	}
	else
	{
	    pos = cwlist->CreatePlaylist(name.c_str());
	    if(pos == -1) {
		progress->Error(Numerics::Error_creating_playlist,
				"Failed to create playlist\n");
		continue;
	    }
	
	    playlist = dynamic_cast<FidPlaylist *>(cwlist->GetNodeAt(pos));
	}
	
	progress->Log(Numerics::Created_playlist,
		     "Created playlist: \"%s\" fid %x\n",
		     name.c_str(), playlist->GetFid());

	dirty ++;
    }
    
    return State::OK;
}

int ConsoleInterface::Parse_link(const StringList &tokens)
{
    if(tokens.size() < 2) {
	progress->Error(Numerics::Syntax_error,
		       "link [-e] [-l] <identifier> [identifier ...]\n");
	return State::Error_continue;
    }

    BranchStackList branch_list;			// start with empty branches list

    int total;

    int flags = GetExpandFlags(tokens, ExpandFlags::None);
    
    // expand wildcards and append to list
    try {
	total = ExpandArgumentsDatabase(++tokens.begin(), tokens.end(),
					branch_list, ExpandFlags::None);
	if(!total) {
	    progress->Error(Numerics::No_matches_found,
			   "No matches found\n");
	    return State::Error_continue;
	}
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    progress->Log(Numerics::Listing_follows, "Linking total: %d\n", total);

    // now take the branch list and, er, linked them to the current directory
    BranchStackList::iterator branch_it = branch_list.begin();
    for(; branch_it != branch_list.end(); branch_it ++) {
	BranchStack &branches = *branch_it;

	if(!branches.size()) {
	    progress->Error(Numerics::Would_loop,
			   "Can't link root playlist - would cause a loop!\n");
	}
	else {
	    PlaylistBranch &branch = *branches.rbegin();
	    FidNode *node = branch.GetChildNode(db);
	    if(!node) {
		progress->Error(Numerics::Fid_not_found,
			       "doh: \"%s\"[%d] (not found)\n",
			       branch.GetParentPlaylist()->GetTitle(),
			       branch.GetChildPos());
	    }
	    const char *name = node->GetTitle();
	    FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	    if(playlist) {
		// check for loops
		std::vector<FILEID> visits;
		visits.push_back(cwlistfid);
		FILEID which_ref;
		FidPlaylist *looplist = playlist->CheckNoLoops(visits, &which_ref);
		if(looplist) {
		    const char *first_name = looplist->GetTitle();
		    FidNode *second_node = db.GetNode(which_ref);
		    const char *second_name;
		    if(!second_node) second_name = "unknown";
		    else second_name = second_node->GetTitle();
		    progress->Error(Numerics::Would_loop,
				   "Not linking \"%s\" - loop "
				   "detected from playlist %s referencing %s\n",
				   name, first_name, second_name);
		    continue;
		}
	    }
	    // add to current playlist
	    PrintNode(branches, flags, false);

	    cwlist->AddNode(node);
	}
    }

    dirty ++;
    
    return State::OK;
}

int ConsoleInterface::Parse_rm(const StringList &tokens)
{
    if(tokens.size() < 2) {
	progress->Error(Numerics::Syntax_error,
		      "rm [-e] [-l] <entry> [entry ...]\n");
	return State::Error_continue;
    }

    BranchStackList branch_list;			// start with empty branches list

    int flags = GetExpandFlags(tokens, ExpandFlags::None);
    
    int total;
    
    // expand wildcards and append to list
    try {
	// no recursion - just wildcards
	total = ExpandArgumentsDatabase(++tokens.begin(), tokens.end(),
					branch_list, ExpandFlags::None);
	if(!total) {
	    progress->Error(Numerics::No_matches_found, "No matches found\n");
	    return State::Error_continue;
	}
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name, "Invalid path name: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    // prevent deletion of root playlist or playlists containing this one

    BranchStackList::iterator branch_it = branch_list.begin();
    while(branch_it != branch_list.end()) {
	BranchStack &branches = *branch_it;

	if(!branches.size()) {
	    progress->Error(Numerics::Would_delete_root_playlist,
			   "Not removing the root playlist!\n");
	    return State::Error_continue;
	}

	PlaylistBranch &branch = *branches.rbegin();
	FidNode *node = branch.GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Fid_not_found,
			   "doh: \"%s\"[%d] (not found)\n",
			   branch.GetParentPlaylist()->GetTitle(),
			   branch.GetChildPos());
	    branch_it = branch_list.erase(branch_it);
	    continue;
	}

	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	if(playlist) {
	    std::vector<FILEID> visits;
	    visits.push_back(cwlistfid);
	    FILEID which_ref;
	    FidPlaylist *looplist = playlist->CheckNoLoops(visits, &which_ref);
	    if(looplist) {
		const char *name = node->GetTitle();
		progress->Error(Numerics::Would_deref_current_playlist,
			       "Removing \"%s\" would remove current playlist\n",
			       name);
		return State::Error_continue;
	    }
	}
	
	branch_it ++;
    }
    
    // ok, it's safe, so let's remove some things...

    progress->Log(Numerics::Listing_follows, "Deleting %d\n", branch_list.size());
    
    branch_it = branch_list.begin();
    while(branch_it != branch_list.end()) {
//	printf("--- before\n");
	PrintBranchStackList(branch_list);
	
	// grab next branch
	BranchStack branches = *branch_it;
//	printf("next: %s\n", BranchStackToPath(branches).c_str());
	// grab child node
	FidNode *node = branches.GetChildNode(db);
	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	// is it a playlist?
	if(playlist) {
	    // is it about to get deleted completely?
	    if(playlist->GetReferenceCount() == 1) {
		// check we're not deleting something we're not allowed to
		if(playlist->GetFid() == FID_LOSTANDFOUND) {
		    progress->Error(Numerics::Would_delete_unattached_items_playlist,
				   "Not deleting unattached items playlist\n");
		    branch_it = branch_list.erase(branch_it);
		    continue;
		}
		// is it non-empty?
		if(playlist->size()) {
		    // if so, we need to recurse its children, and delete this later
//		    printf("playlist ref==1\n");
//		    printf("%s\n", BranchStackToPath(branches).c_str());
		    // this gets printed next time we hit it
		    branch_it = branch_list.erase(branch_it);
		    // add the contents at this position in the list
		    int n = ExpandIdentifier(branch_list, branch_it, branches, "*");
		    // plonk the current playlist here
		    branch_it = branch_list.insert(branch_it, branches);
		    // start from the beginning of the expanded playlist
		    for(int i=0; i<n; i++) branch_it --;
		    continue;
		}
	    }
	}

	// print the line of information about the node
	PrintNode(branches, flags, false);

	branches.GetParentPlaylist()->RemoveNode(branches.GetChildPos());
	branch_it = branch_list.erase(branch_it);
	branch_list.RemovedNode(branches.GetParentPlaylist(), branches.GetChildPos());
	cwdstack.RemovedNode(branches.GetParentPlaylist(), branches.GetChildPos());

//	printf("--- after\n");
	PrintBranchStackList(branch_list);
    }

    dirty ++;

    return State::OK;
}

void ConsoleInterface::PrintBranchStackList(const BranchStackList &branch_list)
{
    // no debug. IT'S TOO BLOODY BIG.
    return;
    
    printf("BranchStackList:\n");
    for(BranchStackList::const_iterator it = branch_list.begin(); it != branch_list.end(); it++) {
	const BranchStack &branches = *it;
	printf("  %s\n", BranchStackToPath(branches).c_str());
    }
    printf("end\n");
}

int ConsoleInterface::Parse_repair(const StringList &tokens)
{
    if(tokens.size() != 1) {
	printf("Repair takes no arguments\n");
	return State::Error_continue;
    }
    printf("Repairing database...\n");
    int repairs = db.Repair(1);
    if(!repairs) printf("No faults found\n");
    else {
	printf("Repaired %d faults\n", repairs);
	printf("Use \"sync\" to sychronise changes with player\n");
    }

    dirty += repairs;

    return State::OK;
}

int ConsoleInterface::Parse_upload(const StringList &tokens)
{
    if(tokens.size() < 2) {
	printf("upload [-e] [-l] [-r] <local path> [local path ...]\n");
	return State::Error_continue;
    }

    int flags = GetExpandFlags(tokens, ExpandFlags::None);
    
    int total;

    BranchStackList branch_list;
    
    try {
	// expand wilcards (hopefully we only get one expansion!)
	total = UploadFiles(++tokens.begin(), tokens.end(), cwdstack, branch_list, flags);
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }
    catch(DirectoryEntry::StatFailed &s) {
	progress->Error(Numerics::Filesystem_error,
		       "Error opening file: errno %d\n", s.err);
	return State::Error_continue;
    }

    progress->Log(Numerics::Listing_follows, "Uploading total: %d\n", branch_list.size());
    
    for(BranchStackList::iterator it = branch_list.begin(); it != branch_list.end(); it++) {
	BranchStack &branches = *it;

	// print the details
	PrintNode(branches, flags, false);
    }
    
    return State::OK;
}

int ConsoleInterface::Parse_lookup(const StringList &tokens)
{
    if(tokens.size() < 2) {
	printf("lookup [-r] <identifier> [identifier...]\n");
	return State::Error_continue;
    }

    BranchStackList branch_list;			// start with empty branches list

    int flags = GetExpandFlags(tokens, ExpandFlags::Escape_paths);

    int total;

    try {
	// expand wilcards (hopefully we only get one expansion!)
	total = ExpandArgumentsDatabase(++tokens.begin(), tokens.end(), branch_list, flags);
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    // now take the branch list and, er, linked them to the current directory
    BranchStackList::iterator branch_it = branch_list.begin();
    for(; branch_it != branch_list.end(); branch_it ++) {
	BranchStack &branches = *branch_it;

	FidNode *node = branches.GetChildNode(db);
	if(!node) {
	    printf("doh: (not found)\n");
	    continue;
	}
	string title = node->GetTitle();
	if(flags & ExpandFlags::Escape_paths) title = EscapePath(title);

	printf("\n");
	progress->Log(Numerics::Tags_follow, "%x %s:\n", node->GetFid(), title.c_str());

	vector<int> which_tags;
	int n = node->WhichTags(which_tags);
	
	for(int i=0; i<n; i++) {
	    int tag = which_tags[i];
	    const char *tag_name = db.GetTagName(tag);
	    const char *tag_value = node->GetTag(tag);
	    printf("%s = %s\n", tag_name, tag_value);
	}
    }
    
    return State::OK;
}

int ConsoleInterface::Parse_set(const StringList &tokens)
{
    if(tokens.size() < 3) {
	progress->Error(Numerics::Syntax_error,
		      "set <tag=value> <identifier> [identifier...]\n");
	return State::Error_continue;
    }

    string tagstring, valuestring;
    const char *tagvalue = (++tokens.begin())->c_str();
    const char *equals = strchr(tagvalue, '=');
    if(!equals) {
	progress->Error(Numerics::Syntax_error,
		       "No tag=value specified\n");
	return State::Error_continue;
    }

    int idx = equals - tagvalue;
    tagstring = tagvalue;
    tagstring.resize(idx);
    valuestring = equals + 1;

    StringList idtokens = tokens;
    idtokens.pop_front();
    idtokens.pop_front();
    
    const char *tag = tagstring.c_str();
    const char *value = valuestring.c_str();
    int tagnumber = db.GetTagNumber(tag);
    
    if(!AllowedTag(tag)) {
	progress->Error(Numerics::Tag_not_allowed,
		       "Not allowed to set tag \"%s\"\n", tag);
	return State::Error_continue;
    }
    
    BranchStackList branch_list;			// start with empty branches list

    int flags = GetExpandFlags(tokens, ExpandFlags::None);

    int total;

    try {
	// expand wilcards (hopefully we only get one expansion!)
	total = ExpandArgumentsDatabase(idtokens.begin(), idtokens.end(), branch_list, flags);
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    progress->Log(Numerics::Listing_follows, "Set %s=%s for:\n\n", tag, value);
    
    // now take the branch list and, er, linked them to the current directory
    BranchStackList::iterator branch_it = branch_list.begin();
    for(; branch_it != branch_list.end(); branch_it ++) {
	BranchStack &branches = *branch_it;
	FidNode *node = branches.GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Fid_not_found,
			   "doh: \"%s\"[%d] (not found)\n",
			   branches.GetParentPlaylist()->GetTitle(),
			   branches.GetChildPos());
	    continue;
	}

	PrintNode(branches, flags, false);

	node->SetTag(tagnumber, value);
    }

    dirty ++;
    
    return State::OK;
}

int ConsoleInterface::Parse_unset(const StringList &tokens)
{
    if(tokens.size() < 3) {
	progress->Error(Numerics::Syntax_error,
		      "unset <tag> <identifier> [identifier...]\n");
	return State::Error_continue;
    }

    const char *tag = (++tokens.begin())->c_str();

    if(!AllowedTag(tag)) {
	progress->Error(Numerics::Tag_not_allowed,
		       "Not allowed to unset tag \"%s\"\n", tag);
	return State::Error_continue;
    }
    
    int tagnumber = db.GetTagNumber(tag);

    StringList idtokens = tokens;
    idtokens.pop_front(); // the command
    idtokens.pop_front(); // the tag
        
    BranchStackList branch_list;			// start with empty branches list

    int flags = GetExpandFlags(tokens, ExpandFlags::None);

    int total;

    try {
	// expand wilcards (hopefully we only get one expansion!)
	total = ExpandArgumentsDatabase(idtokens.begin(), idtokens.end(), branch_list, flags);
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    progress->Log(Numerics::Listing_follows, "Unset %s for:\n\n", tag);
    
    // now take the branch list and, er, linked them to the current directory
    BranchStackList::iterator branch_it = branch_list.begin();
    for(; branch_it != branch_list.end(); branch_it ++) {
	BranchStack &branches = *branch_it;
	FidNode *node = branches.GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Fid_not_found,
			   "doh: \"%s\"[%d] (not found)\n",
			   branches.GetParentPlaylist()->GetTitle(),
			   branches.GetChildPos());
	    continue;
	}

	PrintNode(branches, flags, false);
	
	node->SetTag(tagnumber, NULL);
    }

    dirty ++;
    
    return State::OK;
}

int ConsoleInterface::Parse_move(const StringList &tokens)
{
    if(tokens.size() < 3) {
	printf("move [-e] [-l] <identifier> [identifier...] <to playlist>\n");
	return State::Error_continue;
    }
    
    BranchStackList branch_list;			// start with empty branches list

    int flags = GetExpandFlags(tokens, ExpandFlags::None);
    
    int total;
    
    // expand wildcards and append to list
    try {
	// disallow any expansions
	total = ExpandArgumentsDatabase(++tokens.begin(), tokens.end(),
					branch_list, ExpandFlags::None);
	if(!total) {
	    progress->Error(Numerics::No_matches_found,
			   "No matches found\n");
	    return State::Error_continue;
	}
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    // last argument (do this first, it's not an error if the *last* argument
    // is '/' or the CSD)
    BranchStack branch_to = *(branch_list.rbegin());
    branch_list.pop_back();
    FidPlaylist *playlist_to = branch_to.GetChildPlaylist(db);
    if(!playlist_to) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", branch_to.GetChildNode(db)->GetTitle());
	return State::Error_continue;
    }
    total--; // we don't move the last arg
    
    // prevent deletion of root playlist or playlists containing this one
    BranchStackList::iterator branch_it = branch_list.begin();
    while(branch_it != branch_list.end()) {
	BranchStack &branches = *branch_it;

	if(!branches.size()) {
	    progress->Error(Numerics::Would_delete_root_playlist,
			   "Not moving the root playlist!\n");
	    return State::Error_continue;
	}

	if ( branches.GetChildNode(db)->GetFid() == playlist_to->GetFid() )
	{
	    progress->Error(Numerics::Would_loop,
			    "Not moving a playlist into itself!\n");
	    return State::Error_continue;
	}

	if(branches.GetChildNode(db)->GetFid() == FID_LOSTANDFOUND) {
	    progress->Error(Numerics::Would_delete_unattached_items_playlist,
			   "Not moving the 'Unattached Items' playlist\n");
	    return State::Error_continue;
	}
	
	PlaylistBranch &branch = *branches.rbegin();
	FidNode *node = branch.GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Fid_not_found,
			   "doh: \"%s\"[%d] (not found)\n",
			   branch.GetParentPlaylist()->GetTitle(),
			   branch.GetChildPos());
	    branch_it = branch_list.erase(branch_it);
	    continue;
	}

	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	if(playlist) {
	    std::vector<FILEID> visits;
	    visits.push_back(cwlistfid);
	    FILEID which_ref;
	    FidPlaylist *looplist = playlist->CheckNoLoops(visits, &which_ref);
	    if(looplist) {
		const char *name = node->GetTitle();
		progress->Error(Numerics::Would_deref_current_playlist,
			       "Removing \"%s\" would remove current playlist\n",
			       name);
		return State::Error_continue;
	    }
	}
	
	branch_it ++;
    }
    
    progress->Log(Numerics::Listing_follows, "Moved total: %d\n\n", total);

    // now take the branch list and, er, move them to the target directory
    branch_it = branch_list.begin();
    while(branch_it != branch_list.end()) {
	BranchStack branches = *branch_it;

	PlaylistBranch branch = *branches.rbegin();
	FidNode *node = branch.GetChildNode(db);
	if(!node) {
	    progress->Error(Numerics::Fid_not_found,
			   "doh: \"%s\"[%d] (not found)\n",
			   branch.GetParentPlaylist()->GetTitle(),
			   branch.GetChildPos());
	    branch_it = branch_list.erase(branch_it);
	    continue;
	}

	const char *name = node->GetTitle();
	FidPlaylist *playlist = dynamic_cast<FidPlaylist *>(node);
	if(playlist) {
	    // check for loops
	    std::vector<FILEID> visits;
	    visits.push_back(cwlistfid);
	    FILEID which_ref;
	    FidPlaylist *looplist = playlist->CheckNoLoops(visits, &which_ref);
	    if(looplist) {
		const char *first_name = looplist->GetTitle();
		FidNode *second_node = db.GetNode(which_ref);
		const char *second_name;
		if(!second_node) second_name = "unknown";
		else second_name = second_node->GetTitle();
		progress->Error(Numerics::Would_loop,
			       "Not moving \"%s\" - loop "
			       "detected from playlist %s referencing %s\n",
			       name, first_name, second_name);
		branch_it = branch_list.erase(branch_it);
		continue;
	    }
	}

	// ok, it's safe to move it

	// remove branch from the list
	branch_it = branch_list.erase(branch_it);
	
	// 1) print what we're doing (while we call the FBI)
	PrintNode(branches, flags, false);
	
	// 2) add the node to the 'to' playlist
	// this ends up at the end of the list, so we're safe
	playlist_to->AddNode(node);
	
	// 3) remove the node from the old playlist
	playlist = branch.GetParentPlaylist();
	if(!playlist) {
	    progress->Error(Numerics::Unexpected_database_error,
			   "Parent playlist to \"%s\" doesn't exist!\n", name);
	    return State::Error_continue;
	}
	
	playlist->RemoveNode(branch.GetChildPos());
	branch_list.RemovedNode(branches.GetParentPlaylist(), branches.GetChildPos());
	cwdstack.RemovedNode(branches.GetParentPlaylist(), branches.GetChildPos());
    }

    dirty ++;
        
    return State::OK;
}

int ConsoleInterface::Parse_rename(const StringList &tokens)
{
    if(tokens.size() != 3) {
	printf("rename <identifier> <new name>\n");
	return State::Error_continue;
    }

    const char *from_pattern = (++tokens.begin())->c_str();
    const char *to_pattern = (++++tokens.begin())->c_str();	// starting to look like smileys

    if ( strchr( to_pattern, '/' ) )
    {
	progress->Error( Numerics::Invalid_path_name,
			 "No / characters allowed in the new name (did you want 'move' not 'rename'?)\n" );
	return State::Error_continue;
    }

    FidNode *node;
    
    try {
	BranchStackList branch_list;
	BranchStackList::iterator it = branch_list.end();
	int n = ExpandIdentifier(branch_list, it, cwdstack, from_pattern);
	if(!n) {
	    progress->Error(Numerics::No_matches_found,
			   "No matches for \"%s\"\n", from_pattern);
	    return State::Error_continue;
	}
	else if(n > 1) {
	    progress->Error(Numerics::Ambiguous_multiple_matches,
			   "Multiple matches for \"%s\"\n", from_pattern);
	    return State::Error_continue;
	}

	node = branch_list.begin()->GetChildNode(db);
    }
    catch(InvalidPathName &path) {
	progress->Error(Numerics::Invalid_path_name,
		       "Invalid path: \"%s\"\n", path.GetName());
	return State::Error_continue;
    }
    catch(FidNotFound &f) {
	progress->Error(Numerics::Fid_not_found,
		       "Internal error, not found: \"%s\"\n", f.GetName());
	return State::Error_continue;
    }
    catch(NotAPlaylist &n) {
	progress->Error(Numerics::Not_a_playlist,
		       "Not a playlist: \"%s\"\n", n.GetName());
	return State::Error_continue;
    }

    node->SetTitle(to_pattern);

    dirty ++;
    
    return State::OK;   
}

int ConsoleInterface::Parse_config( const StringList& tokens )
{
    if ( tokens.size() == 1 )
    {
	// config by itself dumps the config file
	std::cout << db.GetConfig();
	return State::OK;
    }

    if ( tokens.size() != 4 )
    {
	printf( "Usage: config\n" );
	printf( "       config <section> <key> <value>\n" );
	return State::Error_continue;
    }

    list<string>::const_iterator li = tokens.begin();

    const string section = *++li;
    const string key = *++li;
    const string value = *++li;
    db.SetConfigValue( section, key, value );

    return State::OK;
}

int ConsoleInterface::Parse_sync(const StringList &tokens)
{
    if(tokens.size() != 1) {
	printf("Sync takes no arguments\n");
	return State::Error_continue;
    }
    (void) Synchronise();

    return State::OK;
}

int ConsoleInterface::Parse_quit(const StringList &tokens)
{
    if(tokens.size() != 1) {
	printf("Quit takes no arguments :)\n");
	return State::Error_continue;
    }

    if(dirty) {
	printf("You have unsynchronised changes\n");
	printf("Typing \"quit\" again before another change will really quit\n");
	dirty = 0;
	return State::OK;
    }

    return State::Abort;
}

void ConsoleInterface::SetDebug(int val)
{
    debug_level = val;
    db.SetDebugLevel(val);				// debug database connection
    progress->SetDebug(val >= 2);			// progress reporter debug on/off
}
