/* file.cpp
 *
 * File and filesystem-related utility routines
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.18 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "file.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <direct.h>
#else
 #include <unistd.h>
 #include <limits.h>
 #include <fcntl.h>
 #include <sys/param.h>
#endif

#include <stdio.h>
#include <errno.h>

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

namespace util {

#ifdef WIN32
// TODO: Win32 also allows forward slashes.  This makes life complicated, if there's a mixture.
std::string GetFileName(const std::string & full_path)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    size_type pos = full_path.rfind('\\');
    if (pos == npos)
	return full_path;

    return std::string(full_path.begin() + pos + 1, full_path.end());
}

std::string GetPathName(const std::string &full_path)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    size_type pos = full_path.rfind('/');
    size_type pos1 = full_path.rfind('\\');
    if (pos == npos)
	pos = pos1;

    if (pos1 != npos && pos1 > pos)
	pos = pos1;

    if (pos == npos)
	return "";

    return std::string(full_path.begin(), full_path.begin() + pos);
}
#else
std::string GetFileName(const std::string & full_path)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    size_type pos = full_path.rfind('/');
    if (pos == npos)
	return "";

    return std::string(full_path.begin() + pos + 1, full_path.end());
}

std::string GetPathName(const std::string &full_path)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    size_type pos = full_path.rfind('/');
    if (pos == npos)
	return full_path;

    return std::string(full_path.begin(), full_path.begin() + pos);
}
#endif

std::string TruncateFilename(const std::string &filename)
{
    typedef std::string::size_type size_type;
    const size_type npos = std::string::npos;

    std::string s(GetFileName(filename));

    size_type pos = s.rfind('.');
    if (pos != npos)
	s = std::string(s.begin(), s.begin() + pos);

    return s;
}

std::string GetFileExtension(const std::string &filename)
{
    std::string::size_type pos = filename.rfind('.');
    if (pos == std::string::npos)
	return std::string();

    return std::string(filename.begin() + pos + 1, filename.end());
}

#ifdef WIN32
std::string AppendSlash(std::string s)
{
    ASSERT(!s.empty());

    char last = s[s.length() - 1];
    if (last != '/' && last != '\\')
	s += '\\';

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

std::string AppendPath(const std::string &a, const std::string &b)
{
    if (a.empty())
	return b;

    std::string r(AppendSlash(a));
    r += b;
    return r;
}

bool IsSpecialDirectory(const char *path)
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

std::string GetCurrentDirectory()
{
    char pwd[MAX_PATH];
#ifdef WIN32
    ::GetCurrentDirectory(sizeof(pwd),pwd);
#else
    getcwd(pwd,sizeof(pwd));
#endif
    return std::string(pwd);
}

STATUS EnsurePathExists(const std::string &s)
{
    if (s.empty())
	return S_OK;

    const char *ptr = s.c_str();
    const char * const start = ptr;
    ++ptr;

    const char *slash;
#ifdef WIN32
    const char *backslash = NULL;
#endif

    do {
	std::string dir;
	int rc;

	slash = strchr( ptr, '/' );
#ifdef WIN32
	backslash = strchr( ptr, '\\' );
	if ( backslash && (!slash || slash>backslash) )
	    slash = backslash;
#endif
	if ( slash )
	{
	    dir = std::string( start, slash-start );
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
	rc = _mkdir( dir.c_str() );
#else
	rc = mkdir( dir.c_str(), 0775 );
#endif
	if ( rc < 0 && errno != EEXIST )
	    return MakeErrnoStatus( errno );

    } while ( slash );

    return S_OK;
}

int GetFileSize(const char *filename, unsigned *file_size)
{
#if defined (WIN32)
    struct _stat st;
    int result = _stat(filename, &st);
#else
    struct stat st;
    int result = stat(filename, &st);
#endif
    if (result >= 0)
	*file_size = st.st_size;

    return result;
}

};  /* namespace util */

#ifdef TEST

using namespace std;

int main( int argc, char *argv[] )
{
    ASSERT(util::TruncateFilename("/foo/bar/something.else/file.mp3") == "file");
    ASSERT(util::TruncateFilename("foo.mp3") == "foo");
    ASSERT(util::TruncateFilename("wurdle") == "wurdle");
    ASSERT(util::TruncateFilename("foo.bar/cheese") == "cheese");
    ASSERT(util::TruncateFilename("cheese/pickle") == "pickle");
    ASSERT(util::GetPathName("cheese") == "");
    ASSERT(util::GetPathName("cheese/pickle") == "cheese");
#if defined(WIN32)
    ASSERT(util::GetPathName("cheese/pickle\\sandwich") == "cheese/pickle");
    ASSERT(util::GetPathName("cheese\\pickle/sandwich") == "cheese\\pickle");
#endif

    char **dir = argv+1;

    std::string pwd = util::GetCurrentDirectory();

    cout << "pwd=" << pwd << endl;

    while ( *dir )
    {
	STATUS s = util::EnsurePathExists( *dir );
	printf( "epe returned %x\n", PrintableStatus(s) );
	dir++;
    }

    return 0;
}

#endif

/* eof */
