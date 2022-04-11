/* file.h
 *
 * File and filesystem-related utility routines
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.37 13-Mar-2003 18:15 rob:)
 */

#ifndef UTIL_FILE_H
#define UTIL_FILE_H

#include <string>
#include <vector>

#include "types.h"
#include "empeg_tchar.h"
#ifndef EMPEG_STATUS_H
#include "empeg_status.h"
#endif

namespace util {

/** Get the filename, including the extension. */
tstring GetFileName(const tstring &full_path);

/** Get the directory part of the path, like dirname(1) */
tstring GetPathName(const tstring &full_path);

/** Get the filename, not including the extension. */
tstring TruncateFilename(const tstring &filename);

/** Put two path components together, inserting a slash or backslash if required. */
tstring AppendPath(const tstring &a, const tstring &b);

/** Return the optimal relative path for getting to destfile from srcdir. Both
 * must be absolute and canonicalised.
 */
tstring MakeRelativePath(const tstring& srcdir, const tstring& destfile);

/** Canonicalise the path:
 * On Unix, calls realpath().
 * On Win32, calls PathCanonicalize.
 */
STATUS CanonicalisePath(const std::string &path, std::string *resolved_path);
STATUS CanonicalisePath(const std::wstring &path, std::wstring *resolved_path);

tstring GetFileExtension(const tstring &filename);
tstring AppendSlash(tstring s);
bool IsSpecialDirectory(const TCHAR *path);
bool IsAbsolutePath(const TCHAR *path);

/** Resolve a relative path, then canonicalise the result */
STATUS CanonicalisePathRelativeTo(const TCHAR *start_dir, const TCHAR *pathname,
				  tstring *resolved_path);

inline bool IsPathSeparator(TCHAR ch)
{
#ifdef WIN32
    return (ch == _T('/') || ch == _T('\\'));
#else
    return (ch == '/');
#endif
}

/** Is this character invalid in file (not path) names? */
inline bool IsInvalidFilenameChar(TCHAR ch)
{
#ifdef WIN32
    return ch == _T('/') || ch == _T('\\') || ch == _T(':') || ch == _T('?') || ch == _T('\'')
	|| ch == _T('\"') || ch == _T('\0') || ch == _T('|') || ch == _T('<') || ch == _T('>');
#else
    return ch == '/' || ch == '\0';
#endif
}

/** If candidate contains characters that aren't valid for the filesystem,
 ** they're dropped on the floor.
 **/
tstring SanitiseForLeafname(const tstring& candidate);

tstring GetCurrentDirectory();
STATUS EnsurePathExists(const tstring &path);
bool FileExists(const tstring &path);
int GetFileSize(const TCHAR *filename, unsigned *file_size);
int GetFileSize(const TCHAR *filename, UINT64 *file_size);

bool IsDriveSubsted(const TCHAR *p);

STATUS RenameFile(const TCHAR *from, const TCHAR *to);

/** Delete a file, without going via the Recycle Bin */
STATUS DeleteFile(const TCHAR *filename);

/** Delete a file, putting in the Recycle Bin if there is one.
 * This operation may display a dialog box under Windows. Also,
 * you MUST supply a full pathname under Windows, or the deletion
 * succeeds but does not go via the recycle bin.
 */
STATUS DeleteFileViaRecycleBin(const TCHAR *filename);

/** Delete several files via the recycle bin. The same comments as
 * DeleteFileViaRecycleBin apply, but calling this once is better as it
 * will only show you the "Are you sure?" dialog once. If you set the
 * 'silent' flag, no dialog will be shown (but stuff will still go to
 * the Recycle Bin if present).
 */
STATUS DeleteFilesViaRecycleBin(const std::vector<tstring>& names, bool silent=false);

/** Generate a temporary filename, create the file and then return its name.
 * The file is created inside the function to allow it to be implemented in a
 * symlink attack safe way on Unix.
 */
STATUS CreateTemporaryFile(const tstring &prefix, tstring *path_result);
};

#endif
