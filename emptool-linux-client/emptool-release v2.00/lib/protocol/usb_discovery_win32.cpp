/* usb_discovery_win32.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *   Roger Lipscombe <roger@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.12 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "usb_discovery.h"
#include "enum_usb_devices_win32.h"
#include "connection.h"

HRESULT Win32UsbEmpegDiscoverer::Discover(unsigned timeout_ms, bool *pbContinue, bool* /*pbIgnoreFailure*/)
{
    CEmpegUsbDevices devices;
    
    if (devices.GetCount() == 0)
	return S_OK;

    for(CEmpegUsbDevices::const_iterator i = devices.begin(); i != devices.end(); ++i)
    {
	// Put together a Connection object to the device, and hand it out.
	UsbConnection *connection = NEW UsbConnection(i->c_str());
	if(!connection)
	    return E_OUTOFMEMORY;

	if(IsEmpegConnected(connection))
	{
	    std::string playerType(GetPlayerType(connection));
	    std::string name(GetEmpegName(connection, GetDefaultName(playerType)));
	    FireDiscoveredEmpeg(DiscoveredEmpeg::Create(name, "USB", playerType, DiscoveredEmpeg::ctUsb, connection));
	}
	else
	    delete connection;
    }

    return S_OK;
}
