/* enum_usb_devices_win32.cpp: implementation of the CEmpegUsbDevices class.
 *
 * (C) 1998-2000 empeg ltd, http://www.empeg.com
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *   Roger Lipscombe <roger@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 */

#include "config.h"
#include "trace.h"
#include "enum_usb_devices_win32.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <setupapi.h>

#ifndef TRACE
#define TRACE printf
#endif

#include <initguid.h>

// {248F0D00-0F88-11d3-9129-00104B62B7D4}
DEFINE_GUID(GUID_CLASS_EMPEGCAR, 
    0x248f0d00, 0xf88, 0x11d3, 0x91, 0x29, 0x0, 0x10, 0x4b, 0x62, 0xb7, 0xd4);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define FINDPROC(A) if ((A = (P ## A)GetProcAddress(hModule, STRIZE(A))) == NULL) available = FALSE

class SetupAPI
{
    typedef BOOL(WINAPI *PSetupDiGetDeviceInterfaceDetail)(HDEVINFO  DeviceInfoSet,
	PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData,
	PSP_DEVICE_INTERFACE_DETAIL_DATA  DeviceInterfaceDetailData, 
	DWORD  DeviceInterfaceDetailDataSize,
	PDWORD  RequiredSize, 
	PSP_DEVINFO_DATA  DeviceInfoData);
    
    
    typedef BOOL (WINAPI *PSetupDiEnumDeviceInterfaces)(HDEVINFO  DeviceInfoSet,
	PSP_DEVINFO_DATA  DeviceInfoData, 
	LPGUID  InterfaceClassGuid,
	DWORD  MemberIndex,
	PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData);
    
    typedef HDEVINFO (WINAPI *PSetupDiGetClassDevsA)(LPGUID  ClassGuid, 
	PCSTR  Enumerator, 
	HWND  hwndParent, 
	DWORD  Flags);
    
    typedef BOOL (WINAPI *PSetupDiDestroyDeviceInfoList)(HDEVINFO DeviceInfoSet);
    
    HMODULE hModule;
    bool available;
    
public:
    PSetupDiEnumDeviceInterfaces SetupDiEnumDeviceInterfaces;
    PSetupDiGetDeviceInterfaceDetail SetupDiGetDeviceInterfaceDetail;
    PSetupDiGetClassDevsA SetupDiGetClassDevsA;
    PSetupDiDestroyDeviceInfoList SetupDiDestroyDeviceInfoList;
    
    bool IsAvailable()
    {
	return available;
    }
    
    SetupAPI()
    {
	memset(this, 0, sizeof(*this));
	hModule = LoadLibrary("SETUPAPI.dll");
	if (hModule)
	{
	    available = TRUE;
	    // We got the library, that should work no matter what
	    // we're running on.
	    FINDPROC(SetupDiEnumDeviceInterfaces);
	    FINDPROC(SetupDiGetDeviceInterfaceDetail);
	    FINDPROC(SetupDiGetClassDevsA);
	    FINDPROC(SetupDiDestroyDeviceInfoList);
	    
	    if (!SetupDiEnumDeviceInterfaces
		|| !SetupDiGetDeviceInterfaceDetail
		|| !SetupDiGetClassDevsA
		|| !SetupDiDestroyDeviceInfoList)
	    {
		// We haven't got some of the stuff we need - assume we have none
		// of it just in case
		SetupDiEnumDeviceInterfaces = NULL;
		SetupDiGetDeviceInterfaceDetail = NULL;
		SetupDiGetClassDevsA = NULL;
		SetupDiDestroyDeviceInfoList = NULL;
		available = FALSE;
	    }
	}
    }
    
    ~SetupAPI()
    {
	FreeLibrary(hModule);
    }
};

static SetupAPI api;

CEmpegUsbDevices::CEmpegUsbDevices()
{
    if (api.IsAvailable())
	FindDevices();
}

CEmpegUsbDevices::~CEmpegUsbDevices()
{
    
}

static std::string GetDeviceFilename(HDEVINFO hardwareDeviceInfo, 
				     PSP_INTERFACE_DEVICE_DATA deviceInfoData)
{
    PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    HANDLE								 hOut = INVALID_HANDLE_VALUE;
    
    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    api.SetupDiGetInterfaceDeviceDetail (
	hardwareDeviceInfo,
	deviceInfoData,
	NULL, // probing so no output buffer yet
	0, // probing so output buffer length of zero
	&requiredLength,
	NULL); // not interested in the specific dev-node
    
    
    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;
    
    functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);
    
    //
    // Retrieve the information from Plug and Play.
    //
    if (!api.SetupDiGetInterfaceDeviceDetail(
	hardwareDeviceInfo,
	deviceInfoData,
	functionClassDeviceData,
	predictedLength,
	&requiredLength,
	NULL))
    {
	free(functionClassDeviceData);
        return "";
    }
    
    std::string strResult = functionClassDeviceData->DevicePath;
    free(functionClassDeviceData);
    return strResult;
}

void CEmpegUsbDevices::FindDevices()
{
    ULONG numberDevices;
    HANDLE hOut = INVALID_HANDLE_VALUE;
    HDEVINFO                 hardwareDeviceInfo;
    SP_INTERFACE_DEVICE_DATA deviceInfoData;
    ULONG                    i;
    BOOLEAN                  done;
    //   PUSB_DEVICE_DESCRIPTOR	*UsbDevices = &usbDeviceInst;
    
    //   *UsbDevices = NULL;
    numberDevices = 0;
    LPGUID pGuid = const_cast<LPGUID>(&GUID_CLASS_EMPEGCAR);
    
    //
    // Open a handle to the plug and play dev node.
    // SetupDiGetClassDevs() returns a device information set that contains info on all 
    // installed devices of a specified class.
    //
    hardwareDeviceInfo = api.SetupDiGetClassDevs(pGuid,
	NULL, // Define no enumerator (global)
	NULL, // Define no
	(DIGCF_PRESENT | // Only Devices present
	DIGCF_INTERFACEDEVICE)); // Function class devices.
    
    //
    // Take a wild guess at the number of devices we have;
    // Be prepared to realloc and retry if there are more than we guessed
    //
    numberDevices = 4;
    done = FALSE;
    deviceInfoData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);
    
    i=0;
    while (!done)
    {
	// SetupDiEnumDeviceInterfaces() returns information about device interfaces 
	// exposed by one or more devices. Each call returns information about one interface;
	// the routine can be called repeatedly to get information about several interfaces
	// exposed by one or more devices.
	
	if (api.SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
	    0, // We don't care about specific PDOs
	    pGuid, i, &deviceInfoData))
	{
	    std::string s = GetDeviceFilename(hardwareDeviceInfo, &deviceInfoData);
	    m_devices.push_back(s);
	} 
	else 
	{
	    if (ERROR_NO_MORE_ITEMS == GetLastError())
	    {
		done = TRUE;
		break;
	    }
	}
	i++;
    }
    
    // SetupDiDestroyDeviceInfoList() destroys a device information set 
    // and frees all associated memory.
    
    api.SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
}

std::string CEmpegUsbDevices::GetDummy() const
{
    // We need to return a device name that is guaranteed not to exist.
    // The simplest way to do this is probably to use the device GUID to
    // generate a sensible looking device name based on our GUID. The
    // only thing we do know is that none of our devices are present
    // so all device names based on our GUID can't exist either :-)
    
    // We end up with something like this:
    //  \\.\0000000000000000#{00873fdf-61a8-11d1-aa5e-00c04fb1728b}
    
    // Yes, this is a hack. It would probably be better to deal with having
    // no other devices using some other method but this will work
    // for now.
    
    
    
    WCHAR wguid_buffer[64];
    char guid_buffer[96];
    char device_buffer[128];
    
    StringFromGUID2(GUID_CLASS_EMPEGCAR, wguid_buffer, sizeof(wguid_buffer)/sizeof(WCHAR));
    WideCharToMultiByte(GetACP(), 0, wguid_buffer, -1, 
	guid_buffer, sizeof(guid_buffer), NULL, NULL);
    
    sprintf(device_buffer, "\\\\.\\0000000000000000#%s", guid_buffer);
    return device_buffer;
}

bool CEmpegUsbDevices::IsUsbSupported()
{
    return api.IsAvailable();
}
