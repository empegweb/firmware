/* utf8.cpp
 *
 * Helper routines for UTF8 support
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.34.2.1 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "utf8.h"
#if 0
#include "stringpred.h" // for empeg_alloca
#include "stringops.h" // wcsimplify
#include <algorithm>
#endif

#define TRACE_UTF8 0

namespace util
{

#define IS_CONT(c) (((unsigned char)(c) & 0xC0) == 0x80)
#define IS_LEAD(c) (((unsigned char)(c) & 0xC0) == 0xC0)

/** Unicode "error character" */
#define ERROR_CHAR 0xFFFD

/** Native BOM */
#define BOM_CHAR 0xFEFF

/** Reverse BOM */
#define REVERSE_BOM 0xFFFE

#if 0
const UTF16CHAR utf16_error_string[] = { ERROR_CHAR, 0 };
const UCS4CHAR ucs4_error_string[] = { ERROR_CHAR, 0 };

static inline unsigned int do_swap(unsigned int ch)
{
    return (ch>>8) + ((ch & 0xFF) <<8);
}
#endif

static
int GetCharFromUTF8(const char **pptr)
{
    const unsigned char *const ptr = (const unsigned char *const)*pptr;

    unsigned char c = *ptr;
    if (c < 0x80)
    {
	if (c)
	    (*pptr)++;
	return c;
    }
    else if ((c & 0xC0) == 0x80) // %10xxxxxx
    {
	// Continuation character (error!)
	(*pptr)++;
    }
    else if ((c & 0xE0) == 0xC0) // %110xxxxx
    {
	if (IS_CONT(ptr[1]))
	{
	    unsigned int wc = ((c & 0x1F) << 6)
		+ (ptr[1] & 0x3F);

	    (*pptr) += 2;

	    if (wc >= 0x80)
		return wc;
	}
	else
	    (*pptr)++;
    }
    else if ((c & 0xF0) == 0xE0) // %1110xxxx
    {
	// This is safe because IS_CONT will fail if ptr[1] == '\0'
	if (IS_CONT(ptr[1]) && IS_CONT(ptr[2]))
	{
	    unsigned int wc = ((c & 0xF) << 12)
		+ ((ptr[1] & 0x3F) << 6)
		+ ((ptr[2] & 0x3F));
	    
	    (*pptr) += 3;

	    if (wc >= 0x800)
		return wc;
	}
	else
	    (*pptr)++;
    }
    else if ((c & 0xF8) == 0xF0) // %11110xxx
    {
	if (IS_CONT(ptr[1]) && IS_CONT(ptr[2]) && IS_CONT(ptr[3]))
	{
	    unsigned int wc = ((c & 0x7) << 18)
		+ ((ptr[1] & 0x3F) << 12)
		+ ((ptr[2] & 0x3F) << 6)
		+ ((ptr[3] & 0x3F));
	    
	    (*pptr) += 4;
		
	    if (wc >= 0x10000)
		return wc;
	}
	else
	    (*pptr)++;
    }
    else if ((c & 0xFC) == 0xF8) // %111110xx
    {
	if (IS_CONT(ptr[1]) && IS_CONT(ptr[2]) && IS_CONT(ptr[3])
	    && IS_CONT(ptr[4]))
	{
	    unsigned int wc = ((c & 0x3) << 24)
		+ ((ptr[1] & 0x3F) << 18)
		+ ((ptr[2] & 0x3F) << 12)
		+ ((ptr[3] & 0x3F) << 6)
		+ ((ptr[4] & 0x3F));

	    (*pptr) += 5;
		
	    if (wc >= 0x200000)
		return wc;
	}
	else
	    (*pptr)++;
    }
    else if ((c & 0xFE) == 0xFC) // %1111110x
    {
	if (IS_CONT(ptr[1]) && IS_CONT(ptr[2]) && IS_CONT(ptr[3])
	    && IS_CONT(ptr[4]) && IS_CONT(ptr[5]))
	{
	    unsigned int wc = ((c & 0x1) << 30)
		+ ((ptr[1] & 0x3F) << 24)
		+ ((ptr[2] & 0x3F) << 18)
		+ ((ptr[3] & 0x3F) << 12)
		+ ((ptr[4] & 0x3F) << 6)
		+ ((ptr[5] & 0x3F));

	    (*pptr) += 6;
		
	    if (wc >= 0x4000000)
		return wc;
	}
	else
	    (*pptr)++;
    }
    else // 0x1111111x
    {
	ASSERT(c == 0xFE || c == 0xFF); // All others should be caught above
	(*pptr)++;
    }
    return -1;
}

bool ValidUTF8(const UTF8CHAR *ptr)
{
    while (*ptr)
    {
	int wc = GetCharFromUTF8(&ptr);
	if (wc == -1)
	    return false;
    }

    return true;
}

void foo();

#if 0
/// @todo: This currently gets it wrong - if the string ends
/// in a multibyte character then it will be removed always :(
size_t UTF8ValidSubStringLength(const char *s, size_t max_len)
{
    // The input _must_ be a valid UTF-8 string otherwise we'll
    // go wrong.
    ASSERT(ValidUTF8(s));

    // If the string is shorter then we don't assume that it
    // is actually valid.
    if (max_len >= strlen(s))
	return strlen(s);

    size_t i = max_len;

    while ((i > 0) && IS_CONT(s[i]))
	--i;

#if DEBUG>0
    std::string t(s, s + i);
    ASSERT(ValidUTF8(t));
#endif // DEBUG>0

    return i;
}

bool UTF16HasBOM(const utf16_string& s)
{
    if (s.empty())
	return false;
    if (s[0] == BOM_CHAR || s[1] == REVERSE_BOM)
	return true;
    return false;
}

utf16_string UTF16EnsureBOM(const utf16_string& s)
{
    if (UTF16HasBOM(s))
	return s;
    return ((UTF16CHAR)BOM_CHAR) + s;
}

utf16_string UTF16EnsureNoBOM(const utf16_string& s)
{
    if (s.empty())
	return s;

    switch (s[0])
    {
    case BOM_CHAR:
	return utf16_string(s.begin()+1,s.end());
    case REVERSE_BOM:
    {
	utf16_string result;
	result.reserve(s.length()-1);

	for (unsigned int i=1; i<s.length(); ++i)
	    result += (UTF16CHAR)do_swap(s[i]);
	return result;
    }
    default: // already had no bom
	return s;
    }
}

utf16_string UTF16FromUTF8(const UTF8CHAR *ptr)
{
    utf16_string result;
    result.reserve(strlen(ptr)); // guess

    while (*ptr)
    {
	int wc = GetCharFromUTF8(&ptr);
	if (wc == -1)
	    result += ERROR_CHAR;
	else if (wc < 0x10000)
	    result += (wchar_t)wc;
	else if (wc < 0x110000)
	{
	    // U+0001 0000 -- U+0010 FFFF represented by surrogates
	    wchar_t high_surrogate = 0xD800 + ((wc - 0x10000)/1024);
	    result += high_surrogate;
	    wchar_t low_surrogate =  0xDC00 + ((wc - 0x10000)%1024);
	    result += low_surrogate;
	}
	else
	{
	    // Unrepresentable in UTF16
	    result += ERROR_CHAR;
	}
    }
    return result;
}
#endif

void foo2();

std::string Latin1FromUTF8(const utf8_string &s)
{
    std::string result;
    result.reserve(s.length()); // guess

    const char *ptr = s.c_str();

    while (*ptr)
    {
	int wc = GetCharFromUTF8(&ptr);
	if (wc == -1 || wc > 255)
	    result += '\xBF'; // upside-down question mark for unknown chars
	else
	    result += (char)wc;
    }
    TRACEC(TRACE_UTF8, "LatinFromUTF8(%s)=%s\n", s.c_str(), result.c_str());

    return result;
}

void foo3();

#if 0
std::string Latin1FromUTF16(const utf16_string &s)
{
    std::string r;
    r.reserve(s.size());
    
    for (utf16_string::const_iterator i = s.begin(); i != s.end(); ++i)
    {
	UTF16CHAR ch = *i;
	if (ch & 0xff00)
	    r += '\xBF';
	else
	    r += (ch & 0xFF);
    }

    return r;
}

utf8_string UTF8FromUTF16(const UTF16CHAR *ptr)
{
    std::string result;
    //    result.reserve(wcslen(ptr)*2); // guess

    bool swap = false;

    if (*ptr == BOM_CHAR)
    {
	ptr++;
    }
    else if (*ptr == REVERSE_BOM)
    {
	swap = true;
	ptr++;
    }

    while (*ptr)
    {
	unsigned int wc = (unsigned int)*ptr++;
	if (swap)
	    wc = do_swap(wc);

	if (wc < 0x80)
	{
	    result += (char)wc;
	}
	else if (wc < 0x800)
	{
	    result += (char)( 0xC0 + (wc>>6));
	    result += (char)( 0x80 + (wc & 0x3F));
	} 
	else
	{
	    if (wc >= 0xD800 && wc < 0xDC00)
	    {
		// Surrogate
		if (*ptr >= 0xDC00 && *ptr < 0xE000)
		{
		    unsigned int wc2 = *ptr++;
		    if (swap)
			wc2 = do_swap(wc2);

		    wc = 0x10000 + (wc & 0x3FF) * 1024;
		    wc += (wc2 & 0x3FF);
		}
	    }
	    if (wc < 0x10000)
	    {
		result += (char)(0xE0 + (wc>>12));
		result += (char)(0x80 + ((wc>>6) & 0x3F));
		result += (char)(0x80 + (wc & 0x3F));
	    }
	    else
	    {
		ASSERT(wc < 0x200000);
		result += (char)(0xF0 + (wc>>18));
		result += (char)(0x80 + ((wc>>12) & 0x3F));
		result += (char)(0x80 + ((wc>>6) & 0x3F));
		result += (char)(0x80 + (wc & 0x3F));
	    }
	}
    }
    return result;
}

utf8_string UTF8FromUCS4(const UCS4CHAR *ptr)
{
    std::string result;
    //    result.reserve(wcslen(ptr)*2); // guess

    bool swap = false;

    if (*ptr == BOM_CHAR)
    {
	ptr++;
    }
    else if (*ptr == REVERSE_BOM)
    {
	swap = true;
	ptr++;
    }

    while (*ptr)
    {
	unsigned int wc = (unsigned int)*ptr++;
	if (swap)
	    wc = do_swap(wc);

	if (wc < 0x80)
	{
	    result += (char)wc;
	}
	else if (wc < 0x800)
	{
	    result += (char)( 0xC0 + (wc>>6));
	    result += (char)( 0x80 + (wc & 0x3F));
	} 
	else if (wc < 0x10000)
	{
	    result += (char)(0xE0 + (wc>>12));
	    result += (char)(0x80 + ((wc>>6) & 0x3F));
	    result += (char)(0x80 + (wc & 0x3F));
	}
	else if (wc < 0x200000)
	{
	    result += (char)(0xF0 + (wc>>18));
	    result += (char)(0x80 + ((wc>>12) & 0x3F));
	    result += (char)(0x80 + ((wc>>6) & 0x3F));
	    result += (char)(0x80 + (wc & 0x3F));
	}
	else if (wc < 0x04000000)
	{
	    result += (char)(0xF8 + (wc>>24));
	    result += (char)(0x80 + ((wc>>18) & 0x3F));
	    result += (char)(0x80 + ((wc>>12) & 0x3F));
	    result += (char)(0x80 + ((wc>>6) & 0x3F));
	    result += (char)(0x80 + (wc & 0x3F));
	}
	else if (wc < 0x80000000)
	{
	    result += (char)(0xFC + (wc>>30));
	    result += (char)(0x80 + ((wc>>24) & 0x3F));
	    result += (char)(0x80 + ((wc>>18) & 0x3F));
	    result += (char)(0x80 + ((wc>>12) & 0x3F));
	    result += (char)(0x80 + ((wc>>6) & 0x3F));
	    result += (char)(0x80 + (wc & 0x3F));
	}
	// else it's an invalid UCS4 character
    }
    return result;
}

utf16_string UTF16FromUCS4(const UCS4CHAR *ptr)
{
    bool swap = false;

    if (*ptr == BOM_CHAR)
    {
	ptr++;
    }
    else if (*ptr == REVERSE_BOM)
    {
	swap = true;
	ptr++;
    }

    utf16_string result;

    while (*ptr)
    {
        UCS4CHAR wc = *ptr++;
        if (wc > 0xffff)
        {
            // The code is either too large or it occupies the surrogate space so
            // we'd better construct a surrogate.
            if (wc < 0x110000)
            {
                // High surrogate
                wc -= 0x10000;
                const UTF16CHAR high = 0xd800 | (wc >> 10);
                const UTF16CHAR low = 0xdc00 | (wc & 0x3ff);

                result += high;
                result += low;
            }
            else
            {
                // It's not representable.
                result += ERROR_CHAR;
            }
        }
        else if ((wc & 0xf800) == 0xd800) // Catches both high and low surrogates.
        {
            // Me, a surrogate, in a UCS-4 string, with _my_ reputation?
            result += ERROR_CHAR;
            ASSERT(false);
        }
        else
        {
            // It's already in UTF-16 form then.
            result += (wc & 0xffff);
        }
    }
    return result;
}

ucs4_string UCS4FromUTF16(const UTF16CHAR *ptr)
{
    bool swap = false;

    if (*ptr == BOM_CHAR)
    {
	ptr++;
    }
    else if (*ptr == REVERSE_BOM)
    {
	swap = true;
	ptr++;
    }

    ucs4_string result;

    while (*ptr)
    {
        UTF16CHAR wc = *ptr++;
        if (swap)
            wc = do_swap(wc);

        if ((wc & 0xfc00) == 0xd800)
        {
            // It's a surrogate
            if ((wc & 0xfc00) == 0xdc00)
            {
                // It's a low surrogate :-( that's bad. We don't expect one here.
                ASSERT(false);
            }
            else
            {
                // It's a high surrogate, that's good. Look for the low counterpart.
                const UTF16CHAR high = wc;
                const UTF16CHAR low = *ptr;

                if ((low & 0xfc00) == 0xdc00)
                {
                    // It's a low surrogate, we've got a pair so advance the pointer.
                    ++ptr;

                    // Now concoct the UCS-4 version of that character.
                    UCS4CHAR code = ((static_cast<UCS4CHAR>(high & 0x3ff) << 10) | (static_cast<UCS4CHAR>(low & 0x3ff)));
                    result += (code + 0x10000);
                }
                else
                {
                    // We haven't got a low surrogate so just skip the high one and
                    // process the one that should have been the low one normally.
                    ASSERT(false);
                }
            }
        }
        else
        {
            // If it's not a surrogate then the output string is the same.
            result += wc;
        }
    }
    return result;
}

ucs4_string UCS4FromUTF8(const UTF8CHAR *ptr)
{
    ucs4_string result;

    while (*ptr)
    {
        int code = GetCharFromUTF8(&ptr);
        if (code == -1)
            result += ERROR_CHAR;
        else
            result += code;
    }
    return result;
}

charclass_t UTF8Classify(const std::string& s)
{
    charclass_t result = ASCII;

    const char *ptr = s.c_str();

    while (*ptr)
    {
	int wc = GetCharFromUTF8(&ptr);
	if (wc == -1)
	    return NOT_UTF8;
	else if (wc > 255)
	    return FULL_UNICODE;
	else if (wc > 127)
	    result = ISO_8859_1;
    }

    return result;
}

utf16_string UTF16FromLatin1(const std::string &s)
{
    utf16_string result;
    result.reserve(s.length());

    for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
    {
	// We must make sure it doesn't get sign extended if char is signed.
	unsigned char c = *i;
	result += c;
    }

    return result;
}

utf8_string UTF8FromLatin1(const std::string &s)
{
    std::string result;
    result.reserve(s.length());

    const char *ptr = s.c_str();

    while (*ptr)
    {
	unsigned char c = *(unsigned char*)ptr++;

	if (c < 0x80)
	{
	    result += (char)c;
	}
	else
	{
	    // Must be < 0x100, so there's only one other case
	    result += (char)(0xC0 + (c>>6));
	    result += (char)(0x80 + (c & 0x3F));
	}
    }
    return result;
}

#if WCHAR_MAX <= USHRT_MAX
utf16_string UTF16FromSystem(const std::string &s)
{
    wchar_t *buffer = (wchar_t *)empeg_alloca((s.length() + 1) * sizeof(wchar_t));

    // @bug: What if it overflows?
    int n = mbstowcs(buffer, s.c_str(), s.length() + 1);
    if (n >= 0)
	return utf16_string(buffer);
    else
	return utf16_error_string;
}

std::string SystemFromUTF16(const utf16_string &utf16)
{
#ifdef WIN32
    /* wcstombs stops translating if it finds an unrepresentable character.
     * WideCharToMultiByte emits a default character and carries on.
     */
    size_t chars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, utf16.c_str(), -1, NULL, 0, NULL, NULL);
    char *buffer = reinterpret_cast<char*>(empeg_alloca(chars+1));
    chars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, utf16.c_str(), -1, buffer, chars+1, NULL, NULL);
    buffer[chars] = '\0';
    return std::string(buffer);
#else
    size_t chars = wcstombs(NULL, utf16.c_str(), 0);
    if (chars == (size_t)-1) // Unrepresentable
	return std::string();
    
    char *buffer = reinterpret_cast<char *>(empeg_alloca(chars+1));
    size_t written = wcstombs(buffer, utf16.c_str(), chars);
    ASSERT(written == chars);
    buffer[written] = 0;
    return std::string(buffer);
#endif
}
#else // WCHAR_MAX > USHRT_MAX
ucs4_string UCS4FromSystem(const std::string &s)
{
    wchar_t *buffer = (wchar_t *)empeg_alloca((s.length() + 1) * sizeof(wchar_t));

    // @bug: What if it overflows?
    int n = mbstowcs(buffer, s.c_str(), s.length() + 1);
    if (n >= 0)
	return ucs4_string(buffer);
    else
	return ucs4_error_string;
}

std::string SystemFromUCS4(const ucs4_string &ucs4)
{
    size_t chars = wcstombs(NULL, ucs4.c_str(), 0);
    if (chars == (size_t)-1) // Unrepresentable
	return std::string();
    
    char *buffer = reinterpret_cast<char *>(empeg_alloca(chars+1));
    size_t written = wcstombs(buffer, ucs4.c_str(), chars);
    ASSERT(written == chars);
    buffer[written] = 0;
    return std::string(buffer);
}
#endif // WCHAR_MAX > USHRT_MAX

utf8_string UTF8FromSystem(const std::string &native)
{
    // There isn't an easy way of doing this. Windows lacks a MultibyteToMultibyte function
    // that would be really useful here. So, we just have to convert to UTF-16 and then
    // back to UTF-8

    std::wstring wide = WideFromSystem(native);
    utf8_string utf8 = UTF8FromWide(wide);
    return utf8;
}

std::string SystemFromUTF8(const utf8_string &utf8)
{
    // As with UTF8FromSystem there isn't an easy way of doing this.
    std::wstring wide = WideFromUTF8(utf8);
    std::string native = SystemFromWide(wide);
    return native;
}

int UTF8Collate(const UTF8CHAR *s1, const UTF8CHAR *s2)
{
    int c1, c2;
    const UTF8CHAR *scan1 = s1, *scan2 = s2;
    int lexical = 0; // result without simplification

    while ( c1 = GetCharFromUTF8(&scan1),
	    c2 = GetCharFromUTF8(&scan2), 
	    c1 && c2 )
    {
	if (c1 != c2 && !lexical)
	    lexical = c1 > c2 ? 1 : -1;
	c1 = stringops::wcsimplify(c1);
	c2 = stringops::wcsimplify(c2);
	if (c1 != c2)
	{
	    //TRACE("'%c' != '%c'\n", c1, c2);
	    return c1 > c2 ? 1 : -1;
	}
    }

    if ( !c1 && !c2 )
    {
	// Got to the end of both strings, so they're equal when simplified.
	// Tie-break with lexicographic compare.
	return lexical;
    }

    // We got to the end of one string but not the other
    return c1 ? 1 : -1;
}

std::string TruncateUTF8(const std::string &source, size_t n)
{
    ASSERT(util::ValidUTF8(source));
    size_t len = util::UTF8ValidSubStringLength(source, n);
    return std::string(source.begin(), source.begin() + len);
}
    
utf8_string UTF8Simplify(const UTF8CHAR *s)
{
    int c;
    const UTF8CHAR *scan = s;
    std::wstring result;

    while ((c = GetCharFromUTF8(&scan)) != 0)
    {
	c = stringops::wcsimplify(c);
	if (c)
	    result += c;
    }

    return util::UTF8FromWide(result);
}
#endif

void foo4();

}; // namespace util
