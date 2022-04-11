/* stringops.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.19 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *   Roger Lipscombe <roger@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "stringops.h"
#include <ctype.h>
#include "utf8.h"

#ifdef WIN32
// For the wide character stuff below.
#include <windows.h>
#include <malloc.h>
#endif

namespace stringops {
    
    std::string EscapeTagString(const std::string &value)
    {
	std::string r;
	for (std::string::const_iterator i = value.begin(); i != value.end(); ++i)
	{
	    if (*i == '\n')
		r += "\\n";
	    else if (*i == '\r')
		r += "\\r";
	    else if (*i == '\\')
		r += "\\\\";
	    else
		r += *i;
	}
	
	return r;
    }
    
    std::string UpperCase(const std::string &value)
    {
        std::string r;
        for (std::string::const_iterator i = value.begin(); i != value.end(); ++i)
            r += toupper((unsigned char)*i);
        return r;
    }

    std::string LowerCase(const std::string &value)
    {
        std::string r;
        for (std::string::const_iterator i = value.begin(); i != value.end(); ++i)
            r += tolower((unsigned char)*i);
        return r;
    }

#if defined(WIN32)
    std::wstring LowerCase(const std::wstring &value)
    {
        std::wstring r;
        for (std::wstring::const_iterator i = value.begin(); i != value.end(); ++i)
            r += towlower(*i);
        return r;
    }
#endif

    void strncpy_term_utf8(char *dest, const char *source, size_t n)
    {
	ASSERT(util::ValidUTF8(source));

	// Must leave room for terminator
	size_t len = util::UTF8ValidSubStringLength(source, n - 1);
	strncpy(dest, source, len);
	dest[len] = 0;
    }

    void strncpy_term(std::string *dest, const char *source, size_t n)
    {
	size_t len;
	if (strlen(source) < n)
	    len = strlen(source);
	else
	    len = n;

	dest->assign(source, source + len);
    }

    void strncpy_term_utf8(std::string *dest, const char *source, size_t n)
    {
	ASSERT(util::ValidUTF8(source));

	// Must leave room for terminator
	size_t len = util::UTF8ValidSubStringLength(source, n - 1);
	dest->assign(source, source + len);
    }

    inline void xncpy(char *dest, const char *source, size_t len)
    {
	strncpy(dest, source, len);
    }

    inline void xncpy(wchar_t *dest, const wchar_t *source, size_t len)
    {
	wcsncpy(dest, source, len);
    }

    template<typename char_type>
    inline void xncpy_term_filename(char_type *dest, const std::basic_string<char_type> &source, size_t n, char_type separator)
    {
	typedef std::basic_string<char_type> string_type;
	typedef string_type::size_type size_type;

        if (source.length() >= n)
        {
            // We must do more clever truncation on a filename so we don't lose the
            // extension.
            size_type dot_position = source.find_last_of(separator);
            if (dot_position != string_type::npos)
            {
                // Of course, the extension may be more than 64 characters long too, so we must be careful.
                size_type extension_length = source.length() - dot_position;

                // We'd quite like to have at least a single character filename.
                if (extension_length > (n - 2))
                    extension_length = (n - 2);

                xncpy(dest, source.c_str(), n - 1 - extension_length);
                xncpy(dest + n - 1 - extension_length, source.c_str() + dot_position, extension_length);
                dest[n - 1] = 0;
                return;
            }
        }

        // If we've fallen through then we should do a simplistic truncation.
        xncpy(dest, source.c_str(), n - 1);
        dest[n - 1] = 0;
    }

    void strncpy_term_filename(char *dest, const std::string &source, size_t n)
    {
	xncpy_term_filename(dest, source, n, '.');
    }

    void wcsncpy_term_filename(wchar_t *dest, const std::wstring &source, size_t n)
    {
	xncpy_term_filename(dest, source, n, L'.');
    }

    // This is different since we need to ensure we don't try and plonk the
    // extension right in the middle of a multibyte sequence. In the worst
    // case we might get less characters in the string than would be possible
    // in theory (i.e. if the prefix ends in a large multibyte sequence) but
    // this is a good enough approximation.
    void strncpy_term_filename_utf8(char *dest, const std::string &source, size_t n)
    {
	ASSERT(util::ValidUTF8(source));

        if (source.length() >= n)
        {
            // We must do more clever truncation on a filename so we don't lose the
            // extension.
            std::string::size_type dot_position = source.find_last_of('.');
            if (dot_position != std::string::npos)
            {
                // Of course, the extension may be more than 64 characters long too, so we must be careful.
                size_t extension_length = source.length() - dot_position;

                // We'd quite like to have at least a single character filename.
                if (extension_length > (n - 2))
		{
		    // One character of filename and one terminator
		    extension_length = util::UTF8ValidSubStringLength(source.c_str() + dot_position, (n - 2));
		    ASSERT(dot_position + extension_length <= source.length());
		}

		size_t prefix_length = util::UTF8ValidSubStringLength(source, n - 1 - extension_length);
                memcpy(dest, source.c_str(), prefix_length);
#if DEBUG>0
		dest[prefix_length] = 0;
		ASSERT(util::ValidUTF8(dest));
#endif // DEBUG>0
                memcpy(dest + prefix_length, source.c_str() + dot_position, extension_length);
                dest[prefix_length+extension_length] = 0;
		ASSERT(util::ValidUTF8(dest));
                return;
            }
        }

	// No dots; do what we'd do without a filename
	strncpy_term_utf8(dest, source.c_str(), n);
    }

    /** Mapping table to simplified (lowercase, accent-stripped) version of
     * Unicode character (minus 160). 0 is for not-alphabetic.
     */
    static const int TABLE_START = 0x00C0;
    static const unsigned short wcsimplify_table[] = {
	/* U+00Cx */  'a', 'a', 'a', 'a',  'a', 'a',0xE6, 'c',  'e', 'e', 'e', 'e',  'i', 'i', 'i', 'i',
	/* U+00Dx */ 0xF0, 'n', 'o', 'o',  'o', 'o', 'o',   0,  'o', 'u', 'u', 'u',  'u', 'y',0xFE,0xDF,
	/* U+00Ex */  'a', 'a', 'a', 'a',  'a', 'a',0xE6, 'c',  'e', 'e', 'e', 'e',  'i', 'i', 'i', 'i',
	/* U+00Fx */ 0xF0, 'n', 'o', 'o',  'o', 'o', 'o',   0,  'o', 'u', 'u', 'u',  'u', 'y',0xFE, 'y',
	/* U+010x */  'a', 'a', 'a', 'a',  'a', 'a', 'c', 'c',  'c', 'c', 'c', 'c',  'c', 'c', 'd', 'd',
	/* U+011x */  'd', 'd', 'e', 'e',  'e', 'e', 'e', 'e',  'e', 'e', 'e', 'e',  'g', 'g', 'g', 'g',
	/* U+012x */  'g', 'g', 'g', 'g',  'h', 'h', 'h', 'h',  'i', 'i', 'i', 'i',  'i', 'i', 'i', 'i',
	/* U+013x */  'i', 'i',0x133,0x133,'j', 'j', 'k', 'k',0x138, 'l', 'l', 'l',  'l', 'l', 'l', 'l',
	/* U+014x */  'l', 'l', 'l', 'n',  'n', 'n', 'n', 'n',  'n', 'n',0x14B,0x14B,'o', 'o', 'o', 'o',
	/* U+015x */  'o', 'o',0x153,0x153,'r', 'r', 'r', 'r',  'r', 'r', 's', 's',  's', 's', 's', 's',
	/* U+016x */  's', 's', 't', 't',  't', 't', 't', 't',  'u', 'u', 'u', 'u',  'u', 'u', 'u', 'u',
	/* U+017x */  'u', 'u', 'u', 'u',  'w', 'w', 'y', 'y',  'y', 'z', 'z', 'z',  'z', 'z', 'z', 's',
    };
    static const int TABLE_END = TABLE_START
          + sizeof(wcsimplify_table)/sizeof(*wcsimplify_table);
    
    wchar_t wcsimplify(wchar_t w)
    {
	if (w<0 || w>TABLE_END)
	    return w;
	if (w < TABLE_START)
	    return isalnum(w) ? tolower(w) : 0;

	return wcsimplify_table[(unsigned int)(w - TABLE_START)];
    }
};

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
