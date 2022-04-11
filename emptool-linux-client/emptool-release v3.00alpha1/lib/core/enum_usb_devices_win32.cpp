/* enum_usb_devices_win32.cpp: implementation of the UsbDeviceEnumeratorWin32 class.
 *
 * (C) 1998-2000 empeg ltd, http://www.empeg.com
 *
 * (:Empeg Source Release 1.11 13-Mar-2003 18:15 rob:)
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
#include <tchar.h>
#include "enum_usb_devices_win32.h"
#include "empeg_error.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <setupapi.h>

#define TRACE_USBENUM 0

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
	LPCGUID  InterfaceClassGuid,
	DWORD  MemberIndex,
	PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData);

#if defined(UNICODE)
    typedef HDEVINFO (WINAPI *PSetupDiGetClassDevsW)(LPCGUID  ClassGuid, 
	PCSTR  Enumerator, 
	HWND  hwndParent, 
	DWORD  Flags);
#else
    typedef HDEVINFO (WINAPI *PSetupDiGetClassDevsA)(LPCGUID  ClassGuid, 
	PCSTR  Enumerator, 
	HWND  hwndParent, 
	DWORD  Flags);
#endif
    
    typedef BOOL (WINAPI *PSetupDiDestroyDeviceInfoList)(HDEVINFO DeviceInfoSet);
    
    HMODULE hModule;
    bool available;
    
public:
    PSetupDiEnumDeviceInterfaces SetupDiEnumDeviceInterfaces;
    PSetupDiGetDeviceInterfaceDetail SetupDiGetDeviceInterfaceDetail;
#if defined(UNICODE)
    PSetupDiGetClassDevsW SetupDiGetClassDevsW;
#else
    PSetupDiGetClassDevsA SetupDiGetClassDevsA;
#endif
    PSetupDiDestroyDeviceInfoList SetupDiDestroyDeviceInfoList;
    
    bool IsAvailable() const { return available; }
    
    SetupAPI();
    ~SetupAPI();
};

SetupAPI::SetupAPI()
{
    TRACEC(TRACE_USBENUM, "Enter SetupAPI constructor.\n");
    memset(this, 0, sizeof(*this));
    hModule = LoadLibrary(_T("SETUPAPI.dll"));
    if (hModule)
    {
	available = TRUE;
	// We got the library, that should work no matter what
	// we're running on.
	FINDPROC(SetupDiEnumDeviceInterfaces);
	FINDPROC(SetupDiGetDeviceInterfaceDetail);
#if defined(UNICODE)
	FINDPROC(SetupDiGetClassDevsW);
#else
	FINDPROC(SetupDiGetClassDevsA);
#endif
	FINDPROC(SetupDiDestroyDeviceInfoList);
	
	if (!SetupDiEnumDeviceInterfaces
	    || !SetupDiGetDeviceInterfaceDetail
#if defined(UNICODE)
	    || !SetupDiGetClassDevsW
#else
	    || !SetupDiGetClassDevsA
#endif
	    || !SetupDiDestroyDeviceInfoList)
	{
	    // We haven't got some of the stuff we need - assume we have none
	    // of it just in case
	    SetupDiEnumDeviceInterfaces = NULL;
	    SetupDiGetDeviceInterfaceDetail = NULL;
#if defined(UNICODE)
	    SetupDiGetClassDevsW = NULL;
#else
	    SetupDiGetClassDevsA = NULL;
#endif
	    SetupDiDestroyDeviceInfoList = NULL;
	    available = FALSE;
	}
    }
    TRACEC(TRACE_USBENUM, "Leave SetupAPI constructor, hModule=%p.\n", hModule);
}

SetupAPI::~SetupAPI()
{
    TRACEC(TRACE_USBENUM, "Enter SetupAPI destructor, hModule=%p.\n", hModule);
    if (hModule)
	FreeLibrary(hModule);
    TRACEC(TRACE_USBENUM, "Leave SetupAPI destructor.\n");
}


static SetupAPI api;

UsbDeviceEnumeratorWin32::UsbDeviceEnumeratorWin32()
{
}

UsbDeviceEnumeratorWin32::~UsbDeviceEnumeratorWin32()
{
    
}

/** This can't be a static member of UsbDeviceEnumeratorWin32 unfortunately because its
 ** interface contains stuff we'd rather keep out of the header file. Isn't C++ wonderful.
 **/
static tstring GetDeviceFilename(HDEVINFO hardwareDeviceInfo, 
				     PSP_INTERFACE_DEVICE_DATA deviceInfoData)
{
    PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
    ULONG requiredLength = 0;
    tstring strResult;
   
    TRACEC(TRACE_USBENUM, "Enter GetDeviceFilename\n");

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    // Will always fail with ERROR_INSUFFICIENT_BUFFER
    //
    api.SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
	deviceInfoData,
	NULL, // probing so no output buffer yet
	0, // probing so output buffer length of zero
	&requiredLength,
	NULL); // not interested in the specific dev-node

    functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (api.SetupDiGetDeviceInterfaceDetail(
	hardwareDeviceInfo,
	deviceInfoData,
	functionClassDeviceData,
	requiredLength,
	NULL, // we don't need to get the length back this time
	NULL))
    {
        strResult = functionClassDeviceData->DevicePath;
    }
    else
    {
        TRACEC(TRACE_USBENUM, "SetupDiGetDeviceInterfaceDetail failed(2).\n");
    }
    free(functionClassDeviceData);

    TRACEC(TRACE_USBENUM, "Leave GetDeviceFilename() = '%s'\n", strResult.c_str());
    return strResult;
}

STATUS UsbDeviceEnumeratorWin32::EnumDevices(LPCGUID guid, UsbDeviceEnumeratorCallback *callback)
{
    TRACEC(TRACE_USBENUM, "Enter UsbDeviceEnumeratorWin32::EnumDevices\n");

    if (!api.IsAvailable())
    {
        TRACE_WARN("API is not available, can't do it.\n");
        TRACEC(TRACE_USBENUM, "Leave UsbDeviceEnumeratorWin32::EnumDevices() = E_NOTIMPL\n");
        return E_NOTIMPL;
    }

    STATUS status;

    //
    // Open a handle to the plug and play dev node.
    // SetupDiGetClassDevs() returns a device information set that contains info on all 
    // installed devices of a specified class.
    //
    HDEVINFO hardwareDeviceInfo = api.SetupDiGetClassDevs(guid,
	NULL, // Define no enumerator (global)
	NULL, // Define no window
	(DIGCF_PRESENT | // Only Devices present
	DIGCF_INTERFACEDEVICE)); // Function class devices.

    if (hardwareDeviceInfo != INVALID_HANDLE_VALUE)
    {
        //
        // Take a wild guess at the number of devices we have;
        // Be prepared to realloc and retry if there are more than we guessed
        //
        SP_INTERFACE_DEVICE_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
    
        ULONG i=0;
        bool done = false;
        while (!done)
        {
	    // SetupDiEnumDeviceInterfaces() returns information about device interfaces 
	    // exposed by one or more devices. Each call returns information about one interface;
	    // the routine can be called repeatedly to get information about several interfaces
	    // exposed by one or more devices.
	    
	    if (api.SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
	        NULL, // We don't care about specific PDOs
	        guid, i, &deviceInfoData))
	    {
	        tstring s = GetDeviceFilename(hardwareDeviceInfo, &deviceInfoData);
                if (s.length())
                {
                    TRACEC(TRACE_USBENUM, "Calling callback with a device called '%s'\n", s.c_str());
                    status = callback->OnDevice(s);
                    if (FAILED(status))
                        done = true;
                }
	    } 
	    else 
	    {
                DWORD last_error = GetLastError();
                // We might have run out, we might have failed, either way bail out.
                if (last_error == ERROR_NO_MORE_ITEMS)
                    status = S_OK;
                else
                    status = MakeWin32Status(last_error);

	        done = TRUE;
	    }
	    i++;
        }
    
        // SetupDiDestroyDeviceInfoList() destroys a device information set 
        // and frees all associated memory.
    
        if (!api.SetupDiDestroyDeviceInfoList(hardwareDeviceInfo))
        {
            TRACEC(TRACE_USBENUM, "Failed to free up everything.\n");
            ASSERT(false);
        }
    }
    else
    {
        status = MakeWin32Status(GetLastError());
    }

    TRACEC(TRACE_USBENUM, "Enter UsbDeviceEnumeratorWin32::EnumDevices() = 0x%08x\n", PrintableStatus(status));
    return status;
}

bool UsbDeviceEnumeratorWin32::IsUsbSupported()
{
    return api.IsAvailable();
}
