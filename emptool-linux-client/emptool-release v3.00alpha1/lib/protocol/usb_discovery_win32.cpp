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
 * (:Empeg Source Release 1.13 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "usb_discovery.h"
#include "enum_usb_devices_win32.h"
#include "connection.h"

#include <initguid.h>

// {248F0D00-0F88-11d3-9129-00104B62B7D4}
DEFINE_GUID(GUID_CLASS_EMPEGCAR, 
    0x248f0d00, 0xf88, 0x11d3, 0x91, 0x29, 0x0, 0x10, 0x4b, 0x62, 0xb7, 0xd4);

STATUS Win32UsbEmpegDiscoverer::OnDevice(const std::string &device)
{
    // Put together a Connection object to the device, and hand it out.
    UsbConnection *connection = NEW UsbConnection(device.c_str());
    if (connection)
    {
        if(IsEmpegConnected(connection))
        {
    	    std::string playerType(GetPlayerType(connection));
    	    std::string name(GetEmpegName(connection, GetDefaultName(playerType)));
    	    FireDiscoveredEmpeg(DiscoveredEmpeg::Create(name, "USB", playerType, DiscoveredEmpeg::ctUsb, connection));
        }
        else
	    delete connection;

        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

HRESULT Win32UsbEmpegDiscoverer::Discover(unsigned timeout_ms, bool *pbContinue, bool* /*pbIgnoreFailure*/)
{
    UsbDeviceEnumeratorWin32 enumerator;

    enumerator.EnumDevices(&GUID_CLASS_EMPEGCAR, this);
    return S_OK;
}
