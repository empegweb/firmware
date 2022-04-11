/* file.cpp
 *
 * File and filesystem-related utility routines
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.60 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "file.h"
#include "empeg_stat.h"
#include "empeg_error.h"
#include "stringpred.h"

#ifdef WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <direct.h>
 #include <shlwapi.h>
 #include <shellapi.h>
 #include <io.h>
#else
 #include <unistd.h>
 #include <limits.h>
 #include <fcntl.h>
#ifndef ECOS
 #include <sys/param.h>
#endif
#endif

#include <stdio.h>
#include <errno.h>

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#define TRACE_FILE 0

namespace util {

#ifdef WIN32
tstring GetFileName(const tstring & full_path)
{
    typedef tstring::size_type size_type;
    const size_type npos = tstring::npos;

    size_type pos = full_path.rfind(_T('/'));
    size_type pos1 = full_path.rfind(_T('\\'));
    if (pos == npos)
	pos = pos1;

    if (pos1 != npos && pos1 > pos)
	pos = pos1;

    if (pos == npos)
	return full_path;

    return tstring(full_path.begin() + pos + 1, full_path.end());
}
#else
std::string GetFileName(const std::string & full_path)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    size_type pos = full_path.rfind('/');
    if (pos == npos)
	return full_path;

    return std::string(full_path.begin() + pos + 1, full_path.end());
}
#endif

tstring SanitiseForLeafname(const tstring& candidate)
{
    tstring result;
    result.reserve(candidate.size());
    for (unsigned int i=0; i<candidate.length(); ++i)
    {
	TCHAR c = candidate[i];
	if (!IsInvalidFilenameChar(c))
	    result += c;
    }
    return result;
}
#ifdef WIN32
tstring GetPathName(const tstring &full_path)
{
    typedef tstring::size_type size_type;
    const size_type npos = tstring::npos;

    size_type pos = full_path.rfind(_T('/'));
    size_type pos1 = full_path.rfind(_T('\\'));
    if (pos == npos)
	pos = pos1;

    if (pos1 != npos && pos1 > pos)
	pos = pos1;

    if (pos == npos)
	return _T("");

    return tstring(full_path.begin(), full_path.begin() + pos);
}
#else
std::string GetPathName(const std::string &full_path)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    size_type pos = full_path.rfind('/');
    if (pos == npos)
	return "";

    return std::string(full_path.begin(), full_path.begin() + pos);
}
#endif

#ifdef WIN32
inline static STATUS CanonicalisePath_Win32(const std::string &path, 
					    std::string *resolved_path)
{
    char temp[MAX_PATH];
    if(!PathCanonicalizeA(temp, path.c_str()))
	return E_FAIL;

    *resolved_path = temp;
    return S_OK;
}

inline static STATUS CanonicaliseWidePath_Win32(const std::wstring &path, 
						std::wstring *resolved_path)
{
    WCHAR temp[MAX_PATH];
    if(!PathCanonicalizeW(temp, path.c_str()))
	return E_FAIL;

    *resolved_path = temp;
    return S_OK;
}
#endif

#if HAVE_REALPATH
inline static STATUS CanonicalisePath_Unix(const std::string &path, 
					   std::string *resolved_path)
{
    char temp[MAX_PATH];
    if (realpath(path.c_str(), temp) == NULL)
	return MakeErrnoStatus();

    *resolved_path = temp;
    return S_OK;
}
#endif

STATUS CanonicalisePath(const std::string &path, std::string *resolved_path)
{
#ifdef WIN32
    return CanonicalisePath_Win32(path, resolved_path);
#elif HAVE_REALPATH
    return CanonicalisePath_Unix(path, resolved_path);
#else
    UNUSED(path);
    UNUSED(resolved_path);
    return E_NOTIMPL;
#endif
}

STATUS CanonicalisePath(const std::wstring &path, std::wstring *resolved_path)
{
#ifdef WIN32
    return CanonicaliseWidePath_Win32(path, resolved_path);
#else
    UNUSED(path);
    UNUSED(resolved_path);
    return E_NOTIMPL;
#endif
}

#ifdef WIN32

#define TRACE_CPR2 0

STATUS CanonicalisePathRelativeTo(const TCHAR *start_dir, const TCHAR *pathname, tstring *resolved_path)
{
    if(PathIsUNC(pathname) || PathIsURL(pathname))
    {
	TRACEC(TRACE_CPR2, "CPR2: '%s' is a UNC or a URL, skipping it\n", pathname);
	*resolved_path = pathname;
	return S_OK;
    }

    TCHAR buff[512];

    // If it begins with a \, we need to slap the drive letter from 'playlistpath' on the front...
    if(IsPathSeparator(pathname[0]))
    {
        // It's not a UNC - we already checked that...
	ASSERT(!IsPathSeparator(pathname[1]));
        memcpy(buff, start_dir, 2);
	_tcscpy(&buff[2], pathname);
    }
    else
    {
        _tcscpy(buff, pathname);
    }

    if(PathIsRelative(pathname))
    {
	TRACEC(TRACE_CPR2, "CPR2: '%s' is relative, canonicalising it...\n", pathname);
	TCHAR buff2[512];
	_tcscpy(buff2, start_dir);
	if(!PathAppend(buff2, pathname))
	    return E_NOTFOUND;

	if(!PathCanonicalize(buff, buff2))
	    return E_NOTFOUND;
    }
    
    // It should be an absolute path now...
    if(PathIsRelative(buff))
	return E_NOTFOUND;

    TRACEC(TRACE_CPR2, "CPR2: '%s' is absolute\n", buff);

    *resolved_path = buff;
    return S_OK;
}

#else

/** @carpet Unix version doesn't build if tchar==wchar_t, but that's never the case on Unix
 */
STATUS CanonicalisePathRelativeTo(const char *start_dir, const char *pathname,
				  std::string *result)
{
    if (!IsAbsolutePath(pathname))
    {
	std::string s = start_dir;
	s += "/";
	s += pathname;
	return CanonicalisePath(s, result);
    }

    return CanonicalisePath(start_dir, result);
}

#endif

tstring TruncateFilename(const tstring &filename)
{
    typedef tstring::size_type size_type;
    const size_type npos = tstring::npos;

    tstring s(GetFileName(filename));

    size_type pos = s.rfind(_T('.'));
    if (pos != npos)
	s = tstring(s.begin(), s.begin() + pos);

    return s;
}

tstring GetFileExtension(const tstring &filename)
{
    tstring::size_type pos = filename.rfind(_T('.'));
    if (pos == tstring::npos)
	return tstring();

    tstring suffix(filename.begin() + pos + 1, filename.end());
    if (suffix.find(_T('/')) != tstring::npos)
	return tstring();

#ifdef WIN32
    if (suffix.find(_T('\\')) != tstring::npos)
	return tstring();
#endif

    return suffix;
}

#ifdef WIN32
tstring AppendSlash(tstring s)
{
    ASSERT(!s.empty());

    TCHAR last = s[s.length() - 1];
    if (last != _T('/') && last != _T('\\'))
	s += _T('\\');

    return s;
}
#else
std::string AppendSlash(std::string s)
{
    ASSERT(!s.empty());

    char last = s[s.length() - 1];
    if (last != '/')
	s += '/';

    return s;
}
#endif

tstring AppendPath(const tstring &a, const tstring &b)
{
    if (a.empty())
	return b;

    tstring r(AppendSlash(a));
    r += b;
    return r;
}

#ifdef WIN32
tstring MakeRelativePath(const tstring& srcdir,
			     const tstring& destfile)
{
    TCHAR relative[MAX_PATH];
    if (!PathRelativePathTo(relative,
			    srcdir.c_str(),
			    FILE_ATTRIBUTE_DIRECTORY,
			    destfile.c_str(),
			    FILE_ATTRIBUTE_NORMAL))
    {
	/* Nothing in common */
	return destfile;
    }
    return relative;
}
#else
std::string MakeRelativePath(const std::string& source_dir,
			     const std::string& destfile)
{
    TRACEC(TRACE_FILE, "MakeRelativePath(src=%s, dest=%s)\n",
	   source_dir.c_str(), destfile.c_str());

    std::string srcdir(AppendSlash(source_dir));
    
    char relative[MAX_PATH];
    int common_prefix = empeg_strcommon(srcdir.c_str(), destfile.c_str());

    if (common_prefix == 0)
    {
	/* Shouldn't happen: all pathnames start with '/' */
	return destfile;
    }

    strncpy(relative, srcdir.c_str(), common_prefix+1);
    TRACEC(TRACE_FILE, "relative = '%s'\n", relative);
    relative[common_prefix+1] = '\0';
    TRACEC(TRACE_FILE, "relative = '%s'\n", relative);
    const char *last_slash = strrchr(relative, '/');
    if (!last_slash)
	return destfile; /* Again, shouldn't happen */

    /* How many directories do we have to go up from srcdir to get to the
     * common prefix? For each one, prepend "../" to the answer. Ignore any
     * trailing slash in srcdir.
     */
    *relative = '\0';
    for (unsigned int i = last_slash - relative; i < srcdir.size()-1; ++i)
    {
	TRACEC(TRACE_FILE, "%d: %c\n", i, srcdir[i]);
	if (srcdir[i] == '/')
	    strcat(relative, "../");
    }
    strcat(relative, destfile.c_str() + (last_slash-relative+1));

    return relative;
}
#endif

bool IsSpecialDirectory(const TCHAR *path)
{
    if (path[0] != '.')
	return false;

    if (path[1] == '\0')
	return true;

    if (path[1] != '.')
	return false;

    if (path[2] == '\0')
	return true;

    return false;
}

bool IsAbsolutePath(const TCHAR *path)
{
#ifdef WIN32
    if (PathIsUNC(path))
	return true;

    if (PathIsRelative(path))
	return false;

    return (path[0] != '\\') &&
	   (path[0] != '/');
#else
    return path[0] == '/';
#endif
}

tstring GetCurrentDirectory()
{
    TCHAR pwd[MAX_PATH];
#ifdef WIN32
    ::GetCurrentDirectory(sizeof(pwd),pwd);
#else
    getcwd(pwd,sizeof(pwd));
#endif
    return tstring(pwd);
}

bool FileExists(const tstring &path)
{
    empeg_stat_struct st;
    if (empeg_stat(path.c_str(), &st) < 0)
	return false;

    return true;
}

STATUS EnsurePathExists(const tstring &s)
{
    if (s.empty())
	return S_OK;

    const TCHAR *ptr = s.c_str();
    const TCHAR * const start = ptr;
    ++ptr;

    const TCHAR *slash;
#ifdef WIN32
    const TCHAR *backslash = NULL;
#endif

    do {
	tstring dir;
	int rc;

	slash = empeg_tcschr( ptr, _T('/') );
#ifdef WIN32
	backslash = empeg_tcschr( ptr, _T('\\') );
	if ( backslash && (!slash || slash>backslash) )
	    slash = backslash;
#endif
	if ( slash )
	{
	    dir = tstring( start, slash-start );
	    ptr = slash+1;
	}
	else
	    dir = s;

#ifdef WIN32
	if (dir.length() == 2 && dir[1] == ':')
	    continue;
#endif

//	TRACE( "try and mkdir %s\n", dir.c_str() );
#ifdef WIN32
	rc = _tmkdir( dir.c_str() );
#else
	rc = mkdir( dir.c_str(), 0775 );
#endif
	if ( rc < 0 && errno != EEXIST )
	    return MakeErrnoStatus( errno );

    } while ( slash );

    return S_OK;
}

int GetFileSize(const TCHAR *filename, UINT64 *file_size)
{
    empeg_stat_struct st;
    int result = empeg_stat(filename, &st);
    if (result >= 0)
	*file_size = st.st_size;

    return result;
}

int GetFileSize(const TCHAR *filename, unsigned *file_size)
{
    empeg_stat_struct st;
    int result = empeg_stat(filename, &st);
    if (result >= 0)
	*file_size = st.st_size;

    return result;
}

bool IsDriveSubsted(const TCHAR *p)
{
#ifdef WIN32
    tstring drive(p, sizeof(TCHAR) * 2);
    TCHAR targets[MAX_PATH];
    DWORD dw = QueryDosDevice(drive.c_str(), targets, MAX_PATH);
    if(dw == 0)
	return false;

    // This, I'm worried about...
    if(_tcsncmp(targets, _T("\\?\?\\"), 4) == 0)
	return true;
#else
    UNUSED(p);
#endif
    return false;
}

#ifndef ECOS
STATUS DeleteFileViaRecycleBin(const TCHAR *filename)
{
#ifdef WIN32
    tstring filenamenulnul = filename;
    filenamenulnul += _T('\0');

    SHFILEOPSTRUCT shop;
    shop.hwnd = NULL;
    shop.wFunc = FO_DELETE;
    shop.pFrom = filenamenulnul.c_str();
    shop.pTo = NULL;
    shop.fFlags = FOF_ALLOWUNDO;
    shop.fAnyOperationsAborted = false;
    shop.hNameMappings = NULL;
    shop.lpszProgressTitle = _T("");
    int rc = SHFileOperation(&shop);
    if (rc =! 0)
	return MakeWin32Status( ::GetLastError() );

    if (shop.fAnyOperationsAborted)
    {
	// User recanted at the "Are you sure?" dbox
	return E_USER_CANCELLED;
    }

    return S_OK;
#else
    return remove(filename)<0 ? MakeErrnoStatus() : S_OK;
#endif
}

STATUS DeleteFilesViaRecycleBin(const std::vector<tstring>& names, bool silent)
{
#ifdef WIN32
    if (names.empty())
	return S_OK;

    tstring filenamenulnul;
    for (std::vector<tstring>::const_iterator i = names.begin();
	 i != names.end();
	 ++i)
    {
	filenamenulnul += *i;
	filenamenulnul += _T('\0');
    }  
    filenamenulnul += _T('\0');

    SHFILEOPSTRUCT shop;
    shop.hwnd = NULL;
    shop.wFunc = FO_DELETE;
    shop.pFrom = filenamenulnul.c_str();
    shop.pTo = NULL;
    shop.fFlags = FOF_ALLOWUNDO;
    if (silent)
	shop.fFlags |= FOF_NOCONFIRMATION;
    shop.fAnyOperationsAborted = false;
    shop.hNameMappings = NULL;
    shop.lpszProgressTitle = _T("");
    int rc = SHFileOperation(&shop);
    if (rc != 0)
	return MakeWin32Status( ::GetLastError() );

    if (shop.fAnyOperationsAborted)
    {
	// User recanted at the "Are you sure?" dbox
	return E_USER_CANCELLED;
    }

    return S_OK;
#else
    UNUSED(silent);
    for (std::vector<std::string>::const_iterator i = names.begin();
	 i != names.end();
	 ++i)
    {
	if (remove(i->c_str()))
	    return MakeErrnoStatus();
    }  
    return S_OK;
#endif
}

STATUS DeleteFile(const TCHAR *filename)
{
    int rc;
#if defined(UNICODE)
    rc = _wremove(filename);
#else
    rc = remove(filename);
#endif

    if (rc < 0)
	return MakeErrnoStatus();

    return S_OK;
}
#endif

STATUS RenameFile(const TCHAR *from, const TCHAR *to)
{
    int rc;
#if defined(UNICODE)
    rc = _wrename(from, to);
#else
    rc = rename(from, to);
#endif

    if (rc < 0)
	return MakeErrnoStatus();

    return S_OK;
}

#ifdef WIN32
#ifdef UNICODE
#define _tmktemp _wmktemp
#define _tcreat _wcreat
#else
#define _tmktemp _mktemp
#define _tcreat _creat
#endif

inline STATUS empeg_mkstemp(const tstring &prefix, int *fd_out, tstring *filename)
{
    TCHAR tempdir[_MAX_PATH];

    if (GetTempPath(_MAX_PATH, tempdir) > 0)
    {
	TCHAR tempfile[_MAX_PATH];
	if (GetTempFileName(tempdir, prefix.c_str(), 0U, tempfile) > 0)
	{
	    int fd = _tcreat(tempfile, 0600);
	    if (fd >= 0)
	    {
		if (fd_out == NULL)
		    close(fd);
		else
		    *fd_out = fd;
		*filename = tempfile;
		return S_OK;
	    }
	    else
		return MakeErrnoStatus();
	}
	else
	{
	    return MakeWin32Status(GetLastError());
	}
    }
    else
    {
	return MakeWin32Status(GetLastError());
    }
}
#elif defined(__unix__)
inline STATUS empeg_mkstemp(const tstring &prefix, int *fd_out, tstring *filename)
{
    char tempfile[MAX_PATH] = "/tmp/";
    strncat(tempfile, prefix.c_str(), MAX_PATH);
    strncat(tempfile, "-XXXXXX", MAX_PATH);

    int fd = mkstemp(tempfile);
    if (fd >= 0)
    {
	if (fd_out == NULL)
	    close(fd);
	else
	    *fd_out = fd;
	*filename = tempfile;
	return S_OK;
    }
    else
    {
	return MakeErrnoStatus();
    }
}	
#else
inline STATUS empeg_mkstemp(const tstring &prefix, int *fd_out, tstring *filename)
{
    UNUSED(prefix);
    UNUSED(fd_out);
    UNUSED(filename);
    ASSERT(false);
    return E_NOTIMPL;
}
#endif

STATUS CreateTemporaryFile(const tstring &prefix, tstring *path_result)
{
    STATUS status = empeg_mkstemp(prefix, NULL, path_result);
    return status;
}

};  /* namespace util */

/* eof */
