/* stringpred.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.25 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 */

#ifndef STRINGPRED_H
#define STRINGPRED_H 1

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef ECOS
#include <malloc.h> // for _alloca
#endif
#include <string>
#include <algorithm>
#include <ctype.h>
#ifndef UTF8_H
#include "utf8.h"
#endif

inline int empeg_stricmp(const char *a, const char *b)
{
#ifdef WIN32
    return stricmp(a,b);
#else
    return strcasecmp(a,b);
#endif
}

#ifdef WIN32
inline int empeg_stricmpw(const wchar_t *a, const wchar_t *b)
{
    return wcsicmp(a,b);
}
#else
// do we need a Linux version?
#endif

inline int empeg_strnicmp(const char *a, const char *b, size_t cch)
{
#ifdef WIN32
    return strnicmp(a,b,cch);
#else
    return strncasecmp(a,b,cch);
#endif
}

#ifdef WIN32
inline int empeg_strnicmpw(const wchar_t *a, const wchar_t *b, size_t cch)
{
    return wcsnicmp(a,b,cch);
}
#else
// do we need a Linux version?
#endif

inline char *empeg_safestrcpy(char *dest, const char *source, size_t n)
{
    strncpy(dest, source, n - 1);
    dest[n-1] = 0;

    return dest;
}

inline size_t empeg_strnlen(const char *start, size_t maxlen)
{
    const char *p = start;
    const char *end = start + maxlen;

    while ((p != end) && (*p))
	++p;

    return (p - start);
}

// alloca can't be an inline
#ifdef WIN32
#define empeg_alloca _alloca
#else
#define empeg_alloca alloca
#endif

/** Convert hex or decimal strings.  We don't do octal deliberately
 ** since it will break on leading zeroes. */
inline long empeg_strtol(const char *nptr, char **endptr)
{
    if (empeg_strnicmp(nptr, "0x", 2) == 0)
        return strtol(nptr, endptr, 16);

    return strtol(nptr, endptr, 10);
}

/** We can't implement a proper strtoll with endptr on Windows
 ** easily so we don't even try. Here's a cool one parameter
 ** version.
 **/
inline LONGLONG empeg_strtoll(const char *nptr)
{
#if defined(WIN32) || defined(ECOS)
    // No strtoll on Windows :(
    LONGLONG ll = 0;
    sscanf(nptr, "%" LONGLONGFMT "d", &ll);
    return ll;
#else
    if (empeg_strnicmp(nptr, "0x", 2) == 0)
	return strtoll(nptr, NULL, 16);
    else
	return strtoll(nptr, NULL, 10);
#endif // !WIN32
}

#if defined(UNICODE)
inline long empeg_strtol(const wchar_t *nptr, wchar_t **endptr)
{
    if (empeg_strnicmpw(nptr, L"0x", 2) == 0)
        return wcstol(nptr, endptr, 16);

    return wcstol(nptr, endptr, 10);
}
#endif

#ifdef ECOS
int empeg_strcasestr(const char *haystack, const char *needle);
#endif

inline unsigned long empeg_strtoul(const char *nptr, char **endptr)
{
    if (empeg_strnicmp(nptr, "0x", 2) == 0)
        return strtoul(nptr, endptr, 16);

    return strtoul(nptr, endptr, 10);
}

/** Returns true if the first strlen(b) characters of a are equal (case
 * insensitive) to b.
 */
inline bool empeg_strprefix(const char *a, const char *b)
{
    while (*b)
    {
	if ( !*a || (tolower((unsigned char)*a) != tolower((unsigned char)*b)) )
	    return false;
	a++;
	b++;
    }
    return true;
}

#ifndef WIN32
inline const char *empeg_strlwr(char *s)
{
    char *r = s;

    while (*s)
    {
	*s = tolower((unsigned char)*s);
	s++;
    }
    
    return r;
}
#else
#define empeg_strlwr _strlwr
#endif

inline int empeg_stricoll(const std::string &a, const std::string &b)
{
#ifndef WIN32
    char *ta = (char *)empeg_alloca(a.length() + 1);
    strcpy(ta, a.c_str());
    
    char *tb = (char *)empeg_alloca(b.length() + 1);
    strcpy(tb, b.c_str());
    
    return strcoll(empeg_strlwr(ta), empeg_strlwr(tb));
#else
    return _stricoll(a.c_str(), b.c_str());
#endif
}

/** Return the length of the common prefix of the two strings (0 if the first
 * characters differ, strlen() for two equal strings).
 */
inline int empeg_strcommon(const char *s1, const char *s2)
{
    int result = 0;
    while (*s1 && *s1 == *s2)
    {
	result++, s1++, s2++;
    }
    return result;
}

namespace stringpred
{
    class IgnoreCaseLt
    {
    public:
	bool operator () (const std::string & a, const std::string & b) const
	{
	    int c = empeg_stricmp(a.c_str(), b.c_str());
	    return c < 0;
	}
    };

    class IgnoreCaseEq
    {
    public:
#if defined(WIN32)
	bool operator () (const std::wstring & a, const std::wstring & b) const
	{
	    if ( a.size() != b.size() )
		return false;
	    int c = empeg_stricmpw(a.c_str(), b.c_str());
	    return c == 0;
	}
#endif

	bool operator () (const std::string & a, const std::string & b) const
	{
	    if ( a.size() != b.size() )
		return false;
	    int c = empeg_stricmp(a.c_str(), b.c_str());
	    return c == 0;
	}
    };

    class IgnoreCaseCompare
    {
    public:
#if defined(WIN32)
	int operator () (const std::wstring & a, const std::wstring & b) const
	{
	    int c = empeg_stricmpw(a.c_str(), b.c_str());
	    return c;
	}
#endif

	int operator () (const std::string & a, const std::string & b) const
	{
	    int c = empeg_stricmp(a.c_str(), b.c_str());
	    return c;
	}

	int operator () (const std::string & a, const std::string & b, int n) const
	{
	    int c = empeg_strnicmp(a.c_str(), b.c_str(), n);
	    return c;
	}
    };

    class IgnoreCaseCollate
    {
    public:
	int operator () (const std::string & a, const std::string & b) const
	{
	    int c = empeg_stricoll(a.c_str(), b.c_str());
	    return c;
	}
    };

    class IgnoreCaseCollateLt
    {
    public:
	bool operator () (const std::string & a, const std::string & b) const
	{
	    int c = empeg_stricoll(a.c_str(), b.c_str());
	    return c < 0;
	}
    };

    /** @todo Break out into empeg_stristr */
    class IgnoreCaseContains
    {
    public:
	bool operator () (const std::string & a, const std::string & b) const
	{
#ifdef WIN32
	    char *ta = (char *)empeg_alloca(a.length() + 1);
	    strcpy(ta, a.c_str());

	    char *tb = (char *)empeg_alloca(b.length() + 1);
	    strcpy(tb, b.c_str());

	    return strstr(strlwr(ta), strlwr(tb)) != NULL;
#elif defined(ECOS)
	    return empeg_strcasestr(a.c_str(), b.c_str()) == 0;
#else
	    return strcasestr( a.c_str(), b.c_str() ) == 0;
#endif
	}
    };

#ifdef WIN32
    class UseLCMapString
    {
    public:	
	int Compare(const std::string &a, const std::string &b) const
	{
	    std::string a_key = GetSortKey(a);
	    std::string b_key = GetSortKey(b);
	    
	    return a_key.compare(b_key);
	}

	int Compare(const std::wstring &a, const std::wstring &b) const
	{
	    std::wstring a_key = GetSortKey(a);
	    std::wstring b_key = GetSortKey(b);
	    
	    return a_key.compare(b_key);
	}
	
    private:
	std::string GetSortKey(const std::string &s) const
	{
	    LCID lcid = GetUserDefaultLCID();
	    const DWORD flags = LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNORESYMBOLS;
	    
	    int required = LCMapStringA(lcid, flags, s.c_str(), s.size(), NULL, 0);
	    
	    char * key = static_cast<char *>(_alloca(required));
	    LCMapStringA(lcid, flags, s.c_str(), s.size(), key, required);
	    
	    return std::string(key, required);
	}

    	std::wstring GetSortKey(const std::wstring &s) const
	{
	    LCID lcid = GetUserDefaultLCID();
	    const DWORD flags = LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNORESYMBOLS;
	    
	    int required = LCMapStringW(lcid, flags, s.c_str(), s.size(), NULL, 0);
	    
	    WCHAR * key = static_cast<WCHAR *>(_alloca(required));
	    LCMapStringW(lcid, flags, s.c_str(), s.size(), key, required);
	    
	    return std::wstring(key, required);
	}
    };

    class UseLCMapStringLt : public UseLCMapString
    {
    public:
	bool operator() (const std::string &a, const std::string &b) const
	{
	    return Compare(a, b) < 0;
	}
	bool operator() (const std::wstring &a, const std::wstring &b) const
	{
	    return Compare(a, b) < 0;
	}
    };
#endif

    class UTF8Less
    {
    public:
	bool operator() (const std::string& a, const std::string& b) const
	{
	    return util::UTF8Collate(a.c_str(), b.c_str()) < 0;
	}
    };

    /** Extract a string of T from a buffer.  The buffer is maxlen*sizeof(T)
     * bytes long, and may or may not contain a 0-terminator.
     *
     * The resulting string will be the correct length (between 0 and maxlen
     * inclusive).
     */
    template <typename T>
    static std::basic_string<T> FromFixedBuffer(const T *buffer, size_t maxlen)
    {
	typedef std::basic_string<T> return_type;

        return return_type(buffer, std::find(buffer, buffer+maxlen, (T)0));
    }
};/* namespace stringpred */

#endif /* INCLUDED_STRINGPRED_H_ */
