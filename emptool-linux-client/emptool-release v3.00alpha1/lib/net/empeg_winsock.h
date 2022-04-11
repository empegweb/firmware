/* empeg_winsock.h
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

#ifndef EMPEG_WINSOCK_H
#define EMPEG_WINSOCK_H 1

#if defined(WIN32)

#include "empeg_error.h"
#include "net_errors.h"
#include <winsock2.h>

/** How to make your program work without WINSOCK DLLs being present.
 ** 
 ** 1. Specify ws2_32.lib and delayimp.lib as libraries to link with.
 **
 ** 2. Type "/delayload:ws2_32.dll /delayload:wsock32.dll" on the
 **    end of the project options.
 **
 ** 3. If you used to call WSAStartup then either don't or call
 **    Winsock::Init instead.
 **
 ** 4. If you do socket stuff outside the stuff in lib/net then
 **    make sure you call Winsock::CheckPresent before you
 **    try and call anything.
 **
 ** To test, boot up a Windows 98 box in safe mode and rename
 ** wsock32.dll and ws2_32.dll to something else before running
 ** your program.
 **
 ** Mike Crowe <mcrowe@sonicblue.com> 2002/04/22
 **
 **/

class Winsock
{
    DISALLOW_COPYING(Winsock);

private:
    // The default test is just a quick comparison with zero
    enum LibraryState
    {
        NOT_CHECKED = -1,
        PRESENT = 0,
        MISSING = 1,
        DESTRUCTED = 2,
    };

    static LibraryState m_state;
    static Winsock m_instance;
    static WSADATA m_data;

    static STATUS CheckPresent2();

public:
    Winsock() {}
    ~Winsock();

    static STATUS Init();
    static STATUS Close();
    static STATUS CheckPresent()
    {
        // Quick inline comparison
        if (m_state == PRESENT)
            return S_OK;
        else
            return CheckPresent2();
    }
    static STATUS GetData(WSADATA *);
};

#endif // WIN32

#endif // EMPEG_WINSOCK_H
