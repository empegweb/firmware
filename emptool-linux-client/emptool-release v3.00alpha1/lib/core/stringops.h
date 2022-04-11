/* stringops.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *   Roger Lipscombe <roger@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 */

#ifndef STRINGOPS_H
#define STRINGOPS_H

#include <string>

namespace stringops
{
    class Trimmer
    {
	std::string trimmable;

    public:
	explicit Trimmer(const char *t) : trimmable(t) {}

	std::string operator()(const std::string &source)
	{
	    std::string::size_type start = source.find_first_not_of(trimmable);
	    std::string::size_type stop = source.find_last_not_of(trimmable);

	    if (start == std::string::npos)
		return std::string();
	    else if (stop == std::string::npos)
		return std::string(source, start, stop);
	    else
		return std::string(source, start, stop + 1);
	}
    };

    class WhiteSpaceTrimmer : public Trimmer
    {
    public:
	WhiteSpaceTrimmer() : Trimmer(" \t") {}
    };

    /** Swap LF for \n, CR for \r, and \ for \\ */
    std::string EscapeTagString(const std::string &value);

    // Return an uppercase version of the string
    std::string UpperCase(const std::string &value);

    // Returns the 'unhappy' fish back to tescos for a more cheery one
    std::string LowerCase(const std::string &value);
#ifdef WIN32
    std::wstring LowerCase(const std::wstring &value);
#endif

    /// Like strncpy but ensure that the resulting string is terminated.
    inline void strncpy_term(char *dest, const std::string &source, size_t n)
    {
	strncpy(dest, source.c_str(), n);
	dest[n - 1] = 0;
    }

    /// Like strncpy but ensure that the resulting string is terminated.
    inline void strncpy_term(char *dest, const char *source, size_t n)
    {
	strncpy(dest, source, n);
	dest[n - 1] = 0;
    }

    /// A bit like std::string::assign(string, n) but if the string is
    /// less than n long the result is the length of the string.
    /// Must not be used on multibyte character strings.
    void strncpy_term(std::string *dest, const char *source, size_t n);

    /// Like strncpy_term but do the right thing with UTF-8 strings.
    void strncpy_term_utf8(char *dest, const char *source, size_t n);

    inline void strncpy_term_utf8(char *dest, const std::string &source, size_t n)
    {
	strncpy_term_utf8(dest, source.c_str(), n);
    }

    /// Rather link strncpy_term but does the right things when dealing
    /// with UTF-8 strings.
    void strncpy_term_utf8(std::string *dest, const char *source, size_t n);

    inline void strncpy_term_utf8(std::string *dest, const std::string &source, size_t n)
    {
	strncpy_term_utf8(dest, source.c_str(), n);
    }

    /// Rather like strncpy_term but ensures that the extension is preserved
    /// if at all possible. Must not be used on multibyte character strings.
    void strncpy_term_filename(char *dest, const std::string &source, size_t n);

    void wcsncpy_term_filename(wchar_t *dest, const std::wstring &source, size_t n);

    /// Rather like strncpy_term_filename but does the right thing when dealing
    /// with a UTF-8 string.
    void strncpy_term_filename_utf8(char *dest, const std::string &source, size_t n);

#ifdef WIN32
    inline void wcsncpy_term(wchar_t *dest, const std::wstring &source, size_t n)
    {
        wcsncpy(dest, source.c_str(), n);
        dest[n - 1] = 0;
    }

    inline void wcsncpy_term(wchar_t *dest, const wchar_t *source, size_t n)
    {
        wcsncpy(dest, source, n);
        dest[n - 1] = 0;
    }
#endif

    /** Like "iswalnum(w) ? towlower(w) : 0" but also strips accents (at least from
     * Latin1). Unknown (currently U+0100 et seq) characters passed through
     * unaltered.
     *
     * @todo Better coverage (e.g. Latin Ext A) for products needing more
     *       than W Europe + Americas + Japan.
     * @todo Doesn't cope with locale-specific lowercasing (Turkish I/i)
     * @todo Doesn't cope with SS/eszett or other non-one-to-one mappings
     * @todo strcmp(wcsimplify()) is a fair stab at collation but is far from
     *       completely the Right Thing. Really we need our own wcsxfrm
     *       (indeed, one for each locale).
     */
    wchar_t wcsimplify(wchar_t w);
}
	    
#endif
