/* stringpred.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11.2.1 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 */

#ifndef INCLUDED_STRINGPRED_H_
#define INCLUDED_STRINGPRED_H_ 1

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <malloc.h> // for _alloca
#include <string>
#include <algorithm>
#include <ctype.h>

inline int empeg_stricmp(const char *a, const char *b)
{
#ifdef WIN32
    return stricmp(a,b);
#else
    return strcasecmp(a,b);
#endif
}

inline int empeg_strnicmp(const char *a, const char *b, size_t n)
{
#ifdef WIN32
    return strnicmp(a,b,n);
#else
    return strncasecmp(a,b,n);
#endif
}

inline char *empeg_safestrcpy(char *dest, const char *source, size_t n)
{
    strncpy(dest, source, n - 1);
    dest[n-1] = 0;

    return dest;
}

// alloca can't be an inline
#ifdef WIN32
#define empeg_alloca _alloca
#else
#define empeg_alloca alloca
#endif

/** Convert hex or decimal strings.  We don't do octal. */
inline long empeg_strtol(const char *nptr, char **endptr)
{
    if (empeg_strnicmp(nptr, "0x", 2) == 0)
        return strtol(nptr, endptr, 16);

    return strtol(nptr, endptr, 10);
}

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
	if ( !*a || (tolower(*a) != tolower(*b)) )
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
	*s = tolower(*s);
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
#else
	    return strcasestr( a.c_str(), b.c_str() ) == 0;
#endif
	}
    };

#ifdef WIN32
    class UseLCMapString
    {
    public:	
	int Compare(const std::string &a, const std::string &b)
	{
	    std::string a_key = GetSortKey(a);
	    std::string b_key = GetSortKey(b);
	    
	    return a_key.compare(b_key);
	}
	
    private:
	std::string GetSortKey(const std::string &s)
	{
	    LCID lcid = GetUserDefaultLCID();
	    const DWORD flags = LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNORESYMBOLS;
	    
	    int required = LCMapString(lcid, flags, s.c_str(), s.size(), NULL, 0);
	    
	    char * key = static_cast<char *>(_alloca(required));
	    LCMapString(lcid, flags, s.c_str(), s.size(), key, required);
	    
	    return std::string(key, required);
	}
    };

    class UseLCMapStringLt : public UseLCMapString
    {
    public:
	bool operator() (const std::string &a, const std::string &b)
	{
	    return Compare(a, b) < 0;
	}
    };
#endif

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
