/* dospath2.cpp
 *
 * Really quite a lot like dospath.cpp, except for using string not CString
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "dospath2.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#else
#include <string.h>
#include <stdio.h> // sprintf
#include <unistd.h>
#endif
#include <stdlib.h>
#include <malloc.h>

DosPath::DosPath(const std::string &path, int nFlags)
{
    SetFullPath(path, nFlags);
}

DosPath::DosPath(const DosPath &old)
{
    strcpy(m_szDrive, old.m_szDrive);
    strcpy(m_szDir, old.m_szDir);
    strcpy(m_szFile, old.m_szFile);
    strcpy(m_szExt, old.m_szExt);
}

DosPath &DosPath::operator=(const DosPath &old)
{
    strcpy(m_szDrive, old.m_szDrive);
    strcpy(m_szDir, old.m_szDir);
    strcpy(m_szFile, old.m_szFile);
    strcpy(m_szExt, old.m_szExt);
    return *this;
}
	
DosPath::~DosPath()
{
}

#ifndef WIN32
static void splitpath_byhand( const char *path,
			      char *drive, char *dir, char *file, char *ext )
{
    *drive = '\0'; // no drives on unix
    char *fptr = strrchr( path, '/' );

    if ( fptr )
    {
	sprintf( dir, "%.*s", fptr-path, path );
	strcpy( file, fptr+1 );
    }
    else
    {
	*dir = '\0';
	strcpy( file, path );
    }

    char *eptr = strrchr( file, '.' );
    if ( eptr )
    {
	strcpy( ext, eptr+1 );
	*eptr = '\0';
    }
    else
    {
	*ext = '\0';
    }
}
#endif

void DosPath::SetFullPath(std::string path, int flags)
{
    if (flags & PATH_ONLY)
    {
#ifdef WIN32
	if (path.length() == 0 || *(path.end() - 1) != '\\')
	    path += '\\';
#else
	if (path.length() == 0 || *(path.end() - 1) != '/')
	    path += '/';
#endif
    }

#ifdef WIN32
    _splitpath(path.c_str(), m_szDrive, m_szDir, m_szFile, m_szExt);
#else
    splitpath_byhand( path.c_str(), m_szDrive, m_szDir, m_szFile, m_szExt );
#endif
}
	
DosPath &DosPath::operator|=(DosPath &fullpath)
{
    if (m_szDrive[0] == '\0')
    {
	strcpy(m_szDrive, fullpath.m_szDrive);
    }
    if (m_szDir[0] == '\0')
    {
	strcpy(m_szDir, fullpath.m_szDir);
    }
    if (m_szFile[0] == '\0')
    {
	strcpy(m_szFile, fullpath.m_szFile);
    }
    if (m_szExt[0] == '\0')
    {
	strcpy(m_szExt, fullpath.m_szExt);
    }
    return *this;
}

DosPath DosPath::operator|(DosPath &fullpath)
{
    DosPath temp(GetFullPath());
    temp |= fullpath;
    return temp;
}

std::string DosPath::GetFullPath() const
{
    char buffer[ _MAX_PATH + 1 ];

#ifdef WIN32
    _makepath(buffer, m_szDrive, m_szDir, m_szFile, m_szExt);
#else
    if ( m_szDir[0] )
	sprintf( buffer, "%s/", m_szDir );
    else
	*buffer = '\0';

    strcat( buffer, m_szFile );
    
    if ( m_szExt[0] )
    {
	strcat( buffer, "." );
	strcat( buffer, m_szExt );
    }
#endif

    return std::string(buffer);
}

DosPath DosPath::CurrentPath()
{
    char buffer[ _MAX_PATH + 1 ];
#ifdef WIN32
    _getcwd(buffer, _MAX_PATH);
#else
    getcwd(buffer, _MAX_PATH);
#endif
    return DosPath(buffer, PATH_ONLY);
}

#ifdef WIN32
DosPath DosPath::AppPath()
{
#ifdef MFC_VER
    char *buffer = reinterpret_cast<char *>(_alloca(_MAX_PATH + 1));
    GetModuleFileName(AfxGetApp()->m_hInstance, buffer, _MAX_PATH);
    return DosPath(buffer);
#else
    return DosPath(_pgmptr);
#endif
}
#endif

DosPath DosPath::TempPath()
{
#ifdef WIN32
    char *buffer = reinterpret_cast<char *>(_alloca(_MAX_PATH + 1));
    GetTempPath(_MAX_PATH, buffer);
#else
    if (getenv("TEMP"))
	return DosPath(getenv("TEMP"), PATH_ONLY);
    if (getenv("TMP"))
	return DosPath(getenv("TMP"), PATH_ONLY);

    return DosPath("/tmp", PATH_ONLY);

#ifdef _WINDOWS
    buffer[0] = GetTempDrive(0);
    buffer[1] = '\0';
    return CDosPath(buffer, PATH_ONLY);
#endif
#endif
    return CurrentPath();
}

#ifdef _WINDOWS
DosPath DosPath::WindowsPath()
{
    char *buffer = reinterpret_cast<char *>(_alloca(_MAX_PATH + 1));
    GetWindowsDirectory(buffer, _MAX_PATH);
    return DosPath(buffer, PATH_ONLY);
}
#endif

#ifdef MAKE_EXE
#include <stdio.h>

int main(int ac, char *av[])
{
    if (ac != 3)
    {
	    printf("Usage: dospath incomplete complete\n");
	    return 0;
    }
    DosPath a( av[1], 0 );
    DosPath b( av[2], 0 );
    a |= b;
    printf("%s |= %s = %s\n", av[1], av[2], a.GetFullPath().c_str());
    printf("\n\n\n\n");
    a = DosPath::CurrentPath();
    printf("Current = %s\n", (const char *)a.GetFullPath().c_str());
#ifdef WIN32
    a = DosPath::AppPath();
    printf("App = %s\n", (const char *)a.GetFullPath().c_str());
#endif
    a = DosPath::TempPath();
    printf("Temp = %s\n", (const char *)a.GetFullPath().c_str());

    return 0;
}
#endif
