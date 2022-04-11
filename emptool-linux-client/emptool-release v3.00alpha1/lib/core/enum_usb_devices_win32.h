/* enum_usb_devices_win32.h
 *
 * (C) 1998-2000 empeg ltd, http://www.empeg.com
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 */

#ifndef ENUM_USB_DEVICES_WIN32_H
#define ENUM_USB_DEVICES_WIN32_H

#if defined(WIN32)
#include <vector>
#include <string>
#include <windows.h>
#include "empeg_status.h"
#include "empeg_tchar.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class UsbDeviceEnumeratorCallback
{
public:
    virtual STATUS OnDevice(const tstring &device) = 0;
};

// We have to be rather sneaky here since some of the symbols we're using
// aren't available on NT4.0 or Windows 95. We have to really dynamically
// link with the symbols.

class UsbDeviceEnumeratorWin32
{
    void FindDevices();
    void AppendDevice(const std::string &str);

public:
    UsbDeviceEnumeratorWin32();
    virtual ~UsbDeviceEnumeratorWin32();

    STATUS EnumDevices(LPCGUID guid, UsbDeviceEnumeratorCallback *callback);

    static bool IsUsbSupported();
};

#endif // defined(WIN32)

#endif // ENUM_USB_DEVICES_WIN32_H
