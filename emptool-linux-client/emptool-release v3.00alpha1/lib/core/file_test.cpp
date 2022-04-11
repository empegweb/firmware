/* file_test.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 * (C) 2002 SONICblue Inc.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "file.h"
#include "empeg_tchar.h"

int main(void)
{
#if 0
    wprintf(L"1 %s\n", L"This is a L string");
    _tprintf(_T("2 %s\n"), _T("This is a _T string"));
    printf("3 %s\n", "This is a normal string");

    wprintf(L"4 %S\n", "This is a L string");
    _tprintf(_T("5 %S\n"), "This is a _T string");
    printf("6 %S\n", L"This is a normal string");

    wprintf(L"7 %ls\n", L"This is a L string");
    wprintf(L"8 %hs\n", "This is a normal string");
    printf("9 %ls\n", L"This is a L string");
    printf("A %s\n", "This is a normal string");
#endif
    
    using namespace util;

#ifdef WIN32
    ASSERT(GetFileName(_T("d:\\foo\\bar\\baz.mp3")) == _T("baz.mp3"));
    ASSERT(GetFileName(_T("d:\\foo\\bar\\baz")) == _T("baz"));
    ASSERT(GetFileName(_T("foo\\bar")) == _T("bar"));
#endif
    ASSERT(GetFileName(_T("baz.mp3")) == _T("baz.mp3"));
    ASSERT(GetFileName(_T("foo/bar")) == _T("bar"));
    ASSERT(GetFileName(_T("foo/bar/baz.mp3")) == _T("baz.mp3"));
    ASSERT(GetFileName(_T("/foo/bar/bz.m")) == _T("bz.m"));
    ASSERT(GetFileName(_T("baz")) == _T("baz"));

    ASSERT(GetPathName(_T("foo")) == _T(".") ||
	   GetPathName(_T("foo")) == _T(""));
#ifdef WIN32
    ASSERT(GetPathName(_T("foo\\bar")) == _T("foo"));
#endif

    // TruncateFilename doesn't return the path -- it's used to extract titles from filenames.
    ASSERT(TruncateFilename(_T("foo.mp3")) == _T("foo"));
    ASSERT(TruncateFilename(_T("foo/bar/baz.mp3")) == _T("baz"));
    ASSERT(TruncateFilename(_T("foo")) == _T("foo"));
    ASSERT(TruncateFilename(_T("foo/bar")) == _T("bar"));

    // We make no warranties about second path being absolute
    ASSERT(AppendPath(_T(""), _T("bar")) == _T("bar"));
#ifdef WIN32
    ASSERT(AppendPath(_T("."), _T("bar")) == _T(".\\bar"));
    ASSERT(AppendPath(_T("foo"), _T("bar")) == _T("foo\\bar"));
    ASSERT(AppendPath(_T("foo\\"), _T("bar")) == _T("foo\\bar"));
    ASSERT(AppendPath(_T("foo"), _T("bar\\baz")) == _T("foo\\bar\\baz"));
#else
    ASSERT(AppendPath(_T("."), _T("bar")) == _T("./bar"));
    ASSERT(AppendPath(_T("foo"), _T("bar")) == _T("foo/bar"));
    ASSERT(AppendPath(_T("foo/"), _T("bar")) == _T("foo/bar"));
    ASSERT(AppendPath(_T("foo\\"), _T("bar")) == _T("foo\\/bar"));
    ASSERT(AppendPath(_T("foo"), _T("bar/baz")) == _T("foo/bar/baz"));
#endif

    // First location is assumed to be a directory.
    // We assume that they're relative to the same place.
#ifdef WIN32
//    ASSERT(MakeRelativePath("/a/b/c", "/a/b/d") == "..\\d");
//    ASSERT(MakeRelativePath("/a/b/c", "/b") == "..\\..\\..\\b");
//    ASSERT(MakeRelativePath("/a", "/a") == ".");
    ASSERT(MakeRelativePath(_T("C:\\foo"), _T("D:\\bar")) == _T("D:\\bar"));
    ASSERT(MakeRelativePath(_T("C:\\foo\\bar"), _T("C:\\bar\\baz")) == _T("..\\..\\bar\\baz"));
    ASSERT(MakeRelativePath(_T("C:\\foo\\bar"), _T("C:\\foo\\bap")) == _T("..\\bap"));
    ASSERT(MakeRelativePath(_T("C:\\foo\\bar"), _T("C:\\foo\\bar\\baz")) == _T(".\\baz"));
#endif

#ifdef WIN32
    std::string resolved;
    
    ASSERT(SUCCEEDED(CanonicalisePath("C:\\foo\\bar", &resolved)));
    ASSERT(resolved == "C:\\foo\\bar");

    ASSERT(SUCCEEDED(CanonicalisePath("C:\\foo\\..\\bar", &resolved)));
    ASSERT(resolved == "C:\\bar");

    /** @todo realpath(1) collapses multiple slashes -- PathCanonicalize doesn't */
    //ASSERT(SUCCEEDED(CanonicalisePath("C:\\foo\\\\bar", &resolved)));
    //ASSERT(resolved == "C:\\foo\\bar");

    //ASSERT(SUCCEEDED(CanonicalisePath("C:\\foo\\\\\\bar", &resolved)));
    //ASSERT(resolved == "C:\\foo\\bar");

    ASSERT(SUCCEEDED(CanonicalisePath("C:\\foo\\.\\bar", &resolved)));
    ASSERT(resolved == "C:\\foo\\bar");
#endif
    
    ASSERT(GetFileExtension(_T("foo.mp3")) == _T("mp3"));
    ASSERT(GetFileExtension(_T("bar/foo.mp3")) == _T("mp3"));
    ASSERT(GetFileExtension(_T("bar\\foo.mp3")) == _T("mp3"));
    ASSERT(GetFileExtension(_T("foo")) == _T(""));
    ASSERT(GetFileExtension(_T("foo.bar.mp3")) == _T("mp3"));
    ASSERT(GetFileExtension(_T("foo.mp3/bar")) == _T(""));
    ASSERT(GetFileExtension(_T("foo.mp3/bar.wma")) == _T("wma"));
#ifdef WIN32
    ASSERT(GetFileExtension(_T("foo.mp3\\bar")) == _T(""));
#else
    ASSERT(GetFileExtension(_T("foo.mp3\\bar")) == _T("mp3\\bar"));
#endif
    ASSERT(GetFileExtension(_T("foo.mp3\\bar.wma")) == _T("wma"));
    ASSERT(GetFileExtension(_T("C:\\foo\\bar.wma")) == _T("wma"));

#ifdef WIN32
    ASSERT(AppendSlash(_T("C:\\a\\b")) == _T("C:\\a\\b\\"));
    ASSERT(AppendSlash(_T("C:\\a\\b")) == _T("C:\\a\\b\\"));
    ASSERT(AppendSlash(_T("a/b")) == _T("a/b\\"));
    // If it already ends in one type of slash, don't change it for the other.
    ASSERT(AppendSlash(_T("a/b/")) == _T("a/b/") ||
	   AppendSlash(_T("a/b/")) == _T("a/b\\"));
#else
    ASSERT(AppendSlash(_T("a/b")) == _T("a/b/"));
    ASSERT(AppendSlash(_T("a/b\\")) == _T("a/b\\/"));
    ASSERT(AppendSlash(_T("a/b/")) == _T("a/b/"));
#endif

    ASSERT(IsSpecialDirectory(_T(".")));
    ASSERT(IsSpecialDirectory(_T("..")));
    ASSERT(!IsSpecialDirectory(_T("...")));
    ASSERT(!IsSpecialDirectory(_T("foo")));

    ASSERT(!IsAbsolutePath(_T("foo\\bar")));
    ASSERT(!IsAbsolutePath(_T("\\foo\\bar")));
#ifdef WIN32
    ASSERT(IsAbsolutePath(_T("C:\\foo\\bar")));
    ASSERT(!IsAbsolutePath(_T("/foo/bar")));
    ASSERT(IsAbsolutePath(_T("\\\\foo\\bar"))); // UNCs are absolute
#else
    ASSERT(IsAbsolutePath(_T("/foo/bar")));
#endif

    std::string candidate("a/b\\c:d?e\'f\"g");
    candidate += '\0';
    candidate += "h|i<j>k";
#ifdef WIN32
    ASSERT(SanitiseForLeafname(candidate) == "abcdefghijk");
#else
    ASSERT(SanitiseForLeafname(candidate) == "ab\\c:d?e\'f\"gh|i<j>k");
#endif
    ASSERT(SanitiseForLeafname("abcdefghijk") == "abcdefghijk");
    
    ASSERT(util::TruncateFilename(_T("/foo/bar/something.else/file.mp3")) == _T("file"));
    ASSERT(util::TruncateFilename(_T("foo.mp3")) == _T("foo"));
    ASSERT(util::TruncateFilename(_T("wurdle")) == _T("wurdle"));
    ASSERT(util::TruncateFilename(_T("foo.bar/cheese")) == _T("cheese"));
    ASSERT(util::TruncateFilename(_T("cheese/pickle")) == _T("pickle"));
    ASSERT(util::GetPathName(_T("cheese")) == _T(""));
    ASSERT(util::GetPathName(_T("cheese/pickle")) == _T("cheese"));

#if defined(WIN32)
    ASSERT(util::GetPathName(_T("cheese/pickle\\sandwich")) == _T("cheese/pickle"));
    ASSERT(util::GetPathName(_T("cheese\\pickle/sandwich")) == _T("cheese\\pickle"));
#endif

#if !defined(WIN32)
    std::string result;
    
    result = util::MakeRelativePath(_T("/foo/bar"), _T("/foo/bar/flange"));
    printf("result = %s\n", result.c_str());
    ASSERT(result == _T("flange"));

    ASSERT(util::MakeRelativePath(_T("/foo/bar/"), _T("/foo/bar/flange")) == _T("flange"));
    ASSERT(util::MakeRelativePath(_T("/foo/bar/"), _T("/foo/baz")) == _T("../baz"));
    ASSERT(util::MakeRelativePath(_T("/foo/bar"), _T("/foo/baz")) == _T("../baz"));
    ASSERT(util::MakeRelativePath(_T("/"), _T("/foo")) == _T("foo"));
    ASSERT(util::MakeRelativePath(_T("/foo"), _T("/")) == _T("../"));
    ASSERT(util::MakeRelativePath(_T("/foo/bar"), _T("/")) == _T("../../"));
    ASSERT(util::MakeRelativePath(_T("/foo/bar/"), _T("/")) == _T("../../"));

    result = util::MakeRelativePath(_T("/tmp/footle"), _T("/tmp/foobar/flange"));
    printf("result = %s\n", result.c_str());
    ASSERT(result == _T("../foobar/flange"))
	;
    result = util::MakeRelativePath(_T("/foo"), _T("/foo/bar/flange"));
    printf("result = %s\n", result.c_str());
    ASSERT(result == _T("bar/flange"));

    ASSERT(util::MakeRelativePath(_T("/foo/bar/wurdle"), _T("/foo/bar/flange")) == _T("../flange"));
    ASSERT(util::MakeRelativePath(_T("/foo/bar/wurdle"), _T("/foo/flange")) == _T("../../flange"));
    ASSERT(util::MakeRelativePath(_T("/foo/bar/wurdle"), _T("/flange")) == _T("../../../flange"));
    ASSERT(util::MakeRelativePath(_T("/flange"), _T("/foo")) == _T("../foo"));
    ASSERT(util::MakeRelativePath(_T("/flange"), _T("/foo/bar")) == _T("../foo/bar"));
#endif

    tstring pwd = util::GetCurrentDirectory();
    empeg_tprintf(_T("Current Directory = '%s'\n"), pwd.c_str());

    printf("All tests passed.\n");

    return 0;
}
