/* usb_discovery.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.8 13-Mar-2003 18:15 rob:)
 */

#ifndef USB_DISCOVERY_H
#define USB_DISCOVERY_H

#include "discovery.h"

#if defined(WIN32)
#include "enum_usb_devices_win32.h"

class Win32UsbEmpegDiscoverer : public EmpegDiscoverer, public UsbDeviceEnumeratorCallback
{
protected:
    virtual STATUS OnDevice(const std::string &device);

public:
    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue, bool *pbIgnoreFailure = NULL);
};
#endif /* defined(WIN32) */

#endif
