/* empeg_winsock.cpp
 *
 * Wrappers for starting up and closing down Winsock.
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see
 * file COPYING), unless you possess an alternative written licence
 * from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "empeg_winsock.h"
#include <winsock2.h>

#if defined(WIN32)

#define FACILITY_VISUALCPP  ((LONG)0x6d)
#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

Winsock::LibraryState Winsock::m_state = NOT_CHECKED;
Winsock Winsock::m_instance;
WSADATA Winsock::m_data;

Winsock::~Winsock()
{
    // Will only close it if we started it.
    Close();
}

STATUS Winsock::Init()
{
    STATUS status;
    DWORD required_version = MAKEWORD(2, 0);

    __try
    {
        LONG result = WSAStartup(required_version, &m_data);
        if (result == 0)
            status = S_OK;
        else
            status = MakeWin32Status(result);
    }
    __except((GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND))
             || (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND)))
    {
        fprintf(stderr, "Could not load Winsock DLL\n");
        status = WINSOCK_E_NOTPRESENT;
    }

    if (SUCCEEDED(status))
        m_state = PRESENT;
    else
        m_state = MISSING;

    return status;
}

STATUS Winsock::Close()
{
    STATUS hr = CheckPresent();
    if (FAILED(hr))
        return hr;

    WSACleanup();
    m_state = DESTRUCTED;

    return S_OK;
}

STATUS Winsock::CheckPresent2()
{
    switch (m_state)
    {
    case NOT_CHECKED:
        return Init();

    case PRESENT:
        // Should never have got here then.
        ASSERT(false);
        return S_OK;

    case MISSING:
        return WINSOCK_E_NOTPRESENT;

    case DESTRUCTED:
        // Eeek we've been destroyed.
        ASSERT(false);
        return WINSOCK_E_NOTPRESENT;

    default:
        ASSERT(false);
        return E_FAIL;
    }
}

STATUS Winsock::GetData(WSADATA *data)
{
    STATUS status = CheckPresent();
    if (FAILED(status))
        return status;

    *data = m_data;
    return S_OK;
}

#endif // WIN32
