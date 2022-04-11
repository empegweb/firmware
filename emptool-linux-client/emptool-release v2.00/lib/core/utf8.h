/* utf8.h
 *
 * Helper routines for UTF8 support
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.38.2.1 01-Apr-2003 18:52 rob:)
 */

#ifndef UTF8_H
#define UTF8_H

#include <string>
#include <limits.h>
#include "types.h"
typedef char UTF8CHAR;
typedef std::basic_string<UTF8CHAR> utf8_string;

namespace util
{

enum charclass_t
{
    ASCII,
    ISO_8859_1,
    FULL_UNICODE,
    NOT_UTF8, // For errors
};

/** What character set is needed to represent this UTF8 string? */
charclass_t UTF8Classify(const utf8_string &utf8string);

/** Like strcoll but for UTF8.
 *
 * @todo Currently only deals with Latin-1 and CJKV. Doesn't implement
 *       idiosyncracies e.g. eszett, Dutch "ij", French reverse accent ordering
 */
int UTF8Collate(const UTF8CHAR *s1, const UTF8CHAR *s2);
inline int UTF8Collate(const utf8_string& s1, const utf8_string& s2) { return UTF8Collate(s1.c_str(), s2.c_str()); }

#if 0
/** Convert UTF16 string (assumed big-endian if no BOM) to UTF8 */
utf8_string UTF8FromUTF16(const UTF16CHAR *w);
inline utf8_string UTF8FromUTF16(const utf16_string& s) { return UTF8FromUTF16(s.c_str()); }

/** Convert UCS4 string (possibly containing UTF16 BOM) to UTF8 */
utf8_string UTF8FromUCS4(const UCS4CHAR *w);
inline utf8_string UTF8FromUCS4(const ucs4_string& s) { return UTF8FromUCS4(s.c_str()); }

/** Convert a UCS4 string (possibly containing UTF16 BOM) to UTF16 */
utf16_string UTF16FromUCS4(const UCS4CHAR *w);
inline utf16_string UTF16FromUCS4(const ucs4_string &s) { return UTF16FromUCS4(s.c_str()); }

/** Convert a UTF8 string to a UCS4 string */
ucs4_string UCS4FromUTF8(const UTF8CHAR *u);
inline ucs4_string UCS4FromUTF8(const utf8_string &s) { return UCS4FromUTF8(s.c_str()); }

/** Convert UTF8 string to UTF16. Returned string is host byte order and does
 * not contain a BOM.
 */
utf16_string UTF16FromUTF8(const char *s);
inline utf16_string UTF16FromUTF8(const utf8_string &s) { return UTF16FromUTF8(s.c_str()); }

inline utf8_string UTF8FromWide(const wchar_t *w)
    {
#if WCHAR_MAX > USHRT_MAX
	return UTF8FromUCS4(w);
#else
	return UTF8FromUTF16(w);
#endif
    }

inline utf8_string UTF8FromWide(const std::wstring &w)
{
    return UTF8FromWide(w.c_str());
}

inline std::wstring WideFromUTF8(const char *s)
{
#if WCHAR_MAX > USHRT_MAX
	return UCS4FromUTF8(s);
#else
	return UTF16FromUTF8(s);
#endif
}

inline std::wstring WideFromUTF8(const std::string &s)
{
    return WideFromUTF8(s.c_str());
}

/** Ensure that s begins with a valid BOM */
utf16_string UTF16EnsureBOM(const utf16_string& s);

/** Ensure that s does NOT begin with a BOM. This involves byte-swapping every
 * character if s begins with a reversed BOM.
 */
utf16_string UTF16EnsureNoBOM(const utf16_string& s);

/** Does this string have a BOM of either sort? (Handy for assertions.) */
bool UTF16HasBOM(const utf16_string& s);

/** Convert Latin-1 (or ASCII) string to UTF8 - only to be used if you really
 * know what you're doing. Are you sure it's a Latin-1 string? Are you sure
 * it's not in the current code page? How about UTF8FromSystem?
 */
utf8_string UTF8FromLatin1(const std::string &s);
#endif

/** Convert UTF8 to Latin-1 (or ASCII if the input can be represented as ASCII).
 * Unrepresentable characters come out as U+00BF upside-down question mark. Only
 * use this if you know what you're doing. Are you sure you don't want a string
 * in the current code page instead?
 */
std::string Latin1FromUTF8(const utf8_string &s);

#if 0
/** Convert UTF16 to Latin-1 (or ASCII). Unrepresentable characters come out
 * as U+00BF upside-down question mark. See notes attached to Lation1FromUTF8
 * above before use.
 */
std::string Latin1FromUTF16(const utf16_string &s);

/** Convert a Latin-1 (or ASCII) string to a UTF16 string. See notes attached
 * to UTF8FromLatin1 above before use.
 */
utf16_string UTF16FromLatin1(const std::string &s);
#endif

/** Is this a valid UTF8 string? */
bool ValidUTF8(const UTF8CHAR *p);
inline bool ValidUTF8(const utf8_string &s)
{
    return ValidUTF8(s.c_str());
}

#if 0
/** Returns UCS-4 character at ptr, advancing ptr as appropriate.
 * Returns -1 (an invalid UCS-4 character) for UTF8 format errors.
 * Returns 0 and does not increase ptr if *ptr == '\0'
 *
 * See O'Reilly, "CJKV Information Processing", pp192-193
 */
int GetCharFromUTF8(const char **pptr);

#if WCHAR_MAX <= USHRT_MAX
/** Convert a system (narrow) string to UTF-16 nomatter what
 * build type we are
 */
utf16_string UTF16FromSystem(const std::string &s);

/** Convert a UTF-16 string to system (narrow) nomatter what
 * build type we are
 */
std::string SystemFromUTF16(const utf16_string &s);

inline std::wstring WideFromSystem(const std::string &s) { return UTF16FromSystem(s); }
inline std::string SystemFromWide(const std::wstring &w) { return SystemFromUTF16(w); }
inline std::wstring WideFromLatin1(const std::string &s) { return UTF16FromLatin1(s); }
inline std::string Latin1FromWide(const std::wstring &w) { return Latin1FromUTF16(w); }

#else // WCHAR_MAX > USHRT_MAX

/** Convert a system (narrow) string to UTF-16 nomatter what
 * build type we are
 */
ucs4_string UCS4FromSystem(const std::string &s);

/** Convert a UTF-16 string to system (narrow) nomatter what
 * build type we are
 */
std::string SystemFromUCS4(const ucs4_string &s);

inline std::wstring WideFromSystem(const std::string &s) { return UCS4FromSystem(s); }
inline std::string SystemFromWide(const std::wstring &w) { return SystemFromUCS4(w); }

#endif // WCHAR_MAX > USHRT_MAX

/** Convert a system (narrow) string to UTF-8 nomatter what
 * build type we are
 */
utf8_string UTF8FromSystem(const std::string &s);

/** Convert a UTF-8 string to system (narrow) nomatter what
 * build type we are
 */
std::string SystemFromUTF8(const utf8_string &s);

#if defined(UNICODE)
// Noone should use these, they're just too tempting. I'm sure there are occasional
// valid uses for them but until we find one they're staying commented out.
// inline tstring TFromLatin1(const std::string &s) { return UTF16FromLatin1(s); }
// inline std::string Latin1FromT(const tstring &s) { return Latin1FromUTF16(s); }

inline tstring TFromUTF8(const utf8_string &s) { return UTF16FromUTF8(s); }
inline utf8_string UTF8FromT(const tstring &s) { return UTF8FromUTF16(s); }
inline utf8_string UTF8FromT(const TCHAR *s) { return UTF8FromUTF16(s); }

inline tstring TFromSystem(const std::string &s) { return UTF16FromSystem(s); }
inline std::string SystemFromT(const tstring &s) { return SystemFromUTF16(s); }

#if WCHAR_MAX <= USHRT_MAX
inline utf16_string UTF16FromT(const tstring &s) { return s; }
inline tstring TFromUTF16(const utf16_string &s) { return s; }
#endif // WCHAR_MAX <= USHRT_MAX

inline tstring TFromWide(const std::wstring &s) { return s; }
inline std::wstring WideFromT(const tstring &s) { return s; }

#else
//inline tstring TFromLatin1(const std::string &s) { }
//inline std::string Latin1FromT(const tstring &s) { }  

#if 0
inline tstring TFromSystem(const std::string &s) { return s; }
inline std::string SystemFromT(const tstring &s) { return s; }

inline tstring TFromUTF8(const utf8_string &s) { return SystemFromUTF8(s); }
inline utf8_string UTF8FromT(const tstring &s) { return UTF8FromSystem(s); }
inline utf8_string UTF8FromT(const TCHAR *s) { return UTF8FromSystem(s); }

#if WCHAR_MAX <= USHRT_MAX
inline utf16_string UTF16FromT(const tstring &s) { return UTF16FromSystem(s); }
inline tstring TFromUTF16(const utf16_string &s) { return SystemFromUTF16(s); }
inline tstring TFromWide(const std::wstring &s) { return SystemFromUTF16(s); }
inline std::wstring WideFromT(const tstring &s) { return UTF16FromT(s); }
#else // WCHAR_MAX <= USHRT_MAX
inline tstring TFromWide(const std::wstring &s) { return SystemFromUCS4(s); }
inline std::wstring WideFromT(const tstring &s) { return UCS4FromSystem(s); }
#endif // WCHAR_MAX > USHRT_MAX
#endif // 0

#endif

/** Convert a pathname into UTF8. UTF8FromT is not good enough, as it uses the
 * local codepage on 8-bit builds, and not all octet sequences are necessarily
 * representable in the local codepage. This means that a path may go into the
 * database and be different when it comes out again. We require a reversible
 * map, and these functions provide one. (All octet sequences are representable
 * in Latin-1.)
 */
inline utf8_string UTF8FromPathname(const tstring& s)
{
#ifdef UNICODE
    return UTF8FromUTF16(s);
#else
    return UTF8FromLatin1(s);
#endif
}

#if 0
/** Convert a UTF8 pathname back to native (see comments for UTF8FromPathname).
 * NB You should *display* it in the native codepage (because that's what
 * Explorer would have done with the original) -- in other words, treat it as
 * a normal tstring. It *hasn't* been transcoded to Latin1 by the time it comes
 * out of PathnameFromUTF8 -- it's *exactly* how it came off disk.
 */
inline tstring PathnameFromUTF8(const utf8_string& s)
{
#ifdef UNICODE
    return UTF16FromUTF8(s);
#else
    return Latin1FromUTF8(s);
#endif
}
#endif

/** Work out the largest number of bytes smaller than or equal to
 * max_len that constitute a valid UTF-8 string. This is mainly 
 * useful for truncation of UTF-8 strings.
 */
size_t UTF8ValidSubStringLength(const char *s, size_t max_len);

inline size_t UTF8ValidSubStringLength(const std::string &s, size_t max_len)
{
    return UTF8ValidSubStringLength(s.c_str(), max_len);
}

std::string TruncateUTF8(const std::string &source, size_t n);

int GetCharFromUTF8(const char **pptr);
utf8_string UTF8Simplify(const UTF8CHAR *s);

#endif

};  // namespace util

inline bool ishiragana(unsigned unicode)
{
    return ((unicode >= 0x3040) && (unicode <= 0x309F));
}

inline bool iskatakana(unsigned unicode)
{
    return ((unicode >= 0x30A0) && (unicode <= 0x30FF));
}

#endif
