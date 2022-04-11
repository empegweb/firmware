/* format_error.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.14 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "empeg_error.h"
#include "var_string.h"
#include "utf8.h"

#ifdef WIN32
static tstring RemoveTrailingCrLf(const tstring &s)
{
    // If the string ends in a CR/LF pair, ditch them.
    tstring::size_type pos = s.rfind(_T("\r\n"));
    if (pos == tstring::npos)
	return s;

    return tstring(s.begin(), s.begin()+pos);
}

tstring FormatErrorMessage(STATUS status)
{
    void *pBuf = NULL;
    tstring str;

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
	str = (LPTSTR)pBuf;
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
	    str = (LPTSTR)pBuf;
	}
    }

    if (pBuf)
	LocalFree(pBuf);

    return RemoveTrailingCrLf(str);
}

tstring FormatErrorMessageEx(STATUS status)
{
    tstring s(FormatErrorMessage(status));
    if (!s.empty())
	return s;

    if (STATUS_SEVERITY(status) == SEV_WIN32_ERROR
	&& STATUS_COMPONENT(status) == COMPONENT_ERRNO)
    {
	int which = STATUS_RESULT(status);

	tstring r(util::TFromSystem(strerror(which)));
	return r;
    }

    TCHAR temp[20];
    _sntprintf(temp, 20, _T("Error 0x%08x"), PrintableStatus(status));

    return tstring(temp);
}
#else
std::string FormatErrorMessageEx(STATUS status)
{
    if (STATUS_SEVERITY(status) == SEV_ERROR
	&& STATUS_COMPONENT(status) == COMPONENT_ERRNO)
    {
	int which = STATUS_RESULT(status);
	return std::string(strerror(which));
    }

    return FormatErrorMessage(status);
}

std::string FormatErrorMessage(STATUS st)
{
    if (SUCCEEDED(st))
	return "[<OK>]";
    return VarString::Printf("[<Error %x>]", PrintableStatus(st));
}
#endif

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
