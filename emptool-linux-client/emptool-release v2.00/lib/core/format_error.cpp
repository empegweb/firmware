/* format_error.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "empeg_error.h"
#include "var_string.h"

#ifdef WIN32
static std::string RemoveTrailingCrLf(const std::string &s)
{
    // If the string ends in a CR/LF pair, ditch them.
    std::string::size_type pos = s.rfind("\r\n");
    if (pos == std::string::npos)
	return s;

    return std::string(s.begin(), s.begin()+pos);
}

std::string FormatErrorMessage(STATUS status)
{
    void *pBuf = NULL;
    std::string str;

    if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			status,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &pBuf,
			0,
			NULL) != 0)
    {
	str = (LPSTR)pBuf;
    }
    else
    {
	// If there are no messages compiled into the module, the above fails, even
	// though the message could be FORMAT_MESSAGE_FROM_SYSTEM'ed.  So we'll try it again.
	if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
		    status,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		    (LPTSTR) &pBuf,
		    0,
		    NULL) != 0)
	{
	    str = (LPSTR)pBuf;
	}
    }

    if (pBuf)
	LocalFree(pBuf);

    return RemoveTrailingCrLf(str);
}

std::string FormatErrorMessageEx(STATUS status)
{
    std::string s(FormatErrorMessage(status));
    if (!s.empty())
	return s;

    char temp[20];
    sprintf(temp, "Error 0x%08x", status);

    return std::string(temp);
}
#else
std::string FormatErrorMessageEx(STATUS status)
{
    return FormatErrorMessage(status);
}

std::string FormatErrorMessage(STATUS st)
{
    if (SUCCEEDED(st))
	return "[<OK>]";
    return VarString::Printf("[<Error %x>]", PrintableStatus(st));
}
#endif
