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
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#ifndef USB_DISCOVERY_H
#define USB_DISCOVERY_H

#include "discovery.h"

#if defined(WIN32)
class Win32UsbEmpegDiscoverer : public EmpegDiscoverer
{
public:
    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue, bool *pbIgnoreFailure = NULL);
};
#endif /* defined(WIN32) */

#endif
