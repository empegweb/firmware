/* dospath2.h
 *
 * Really quite a lot like dospath.h, except for using string not CString
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 13-Mar-2003 18:15 rob:)
 */

#ifndef DOSPATH2_H
#define DOSPATH2_H 1

#include <string>

#ifndef WIN32

#include <dirent.h>

// Some of this stuff doesn't exist on Linux
#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif
#ifndef _MAX_DIR
#define _MAX_DIR NAME_MAX
#endif
#ifndef _MAX_PATH
#define _MAX_PATH NAME_MAX
#endif
#ifndef _MAX_FNAME
#define _MAX_FNAME NAME_MAX
#endif
#ifndef _MAX_EXT
#define _MAX_EXT NAME_MAX
#endif

#endif // !defined(WIN32)

class DosPath
{
    char m_szDrive[_MAX_DRIVE];
    char m_szDir[_MAX_DIR];
    char m_szFile[_MAX_FNAME];
    char m_szExt[_MAX_EXT];

public:
    enum { PATH_ONLY = 1 };

    DosPath(const std::string &path, int flags = 0);
    DosPath(const DosPath &old);
    ~DosPath();

    DosPath &operator|=(DosPath &fullpath);
    DosPath operator|(DosPath &fullpath);

    DosPath &operator=(const DosPath &old);

    std::string GetFullPath() const;
    void SetFullPath(std::string path, int flags = 0);

    std::string GetExtension() const
    {
	return m_szExt;
    }
    std::string GetFile() const
    {
	return m_szFile;
    }

    static DosPath CurrentPath();
    static DosPath AppPath();
    static DosPath TempPath();
#ifdef _WINDOWS
    static DosPath WindowsPath();
#endif
};

#endif
