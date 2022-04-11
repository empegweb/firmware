/* connection_win32.cpp
 *
 * Win32-specific code in support of connecting to players over usb/serial/net
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.48 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string>
#include "interval.h"
#include "connection.h"
#include "connection_serial.h"
#include "protocol_errors.h"
#include <winioctl.h>
#include <io.h>
#include "win32_thread/CritSect.h"

#define TRACE_PROTOCOL 0

// The usb driver cannot be accessed by more than one
// thread at once, so lock it with this
CritSect UsbConnection::usb_cs;

#define IOCTL_EMPEGUSB_RESET_DEVICE CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EMPEGUSB_RESET_PIPE CTL_CODE(FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EMPEGUSB_GET_STALL_COUNT CTL_CODE(FILE_DEVICE_UNKNOWN, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

static const int INCOMPLETE_IO_RETRIES = 10;
static const int INCOMPLETE_IO_DELAY = 500;

// Use this to output all serial received to a file for debugging purposes.
//#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
static FILE *serial_log = 0;
#endif


// Because the tick count could wrap during a timeout period we need
// to be clever here.
static inline bool TimeoutPending(DWORD start_time, DWORD timeout_ms)
{
    DWORD now = GetTickCount();

    // Deal with the complex case. Like this:
    // |___t       s___| where | indicates the bounds of the type
    //                        t is the timeout time.
    //                        s is the start time.
    //                        _ is the space during which we are pending
    //                          is the space during which we are timed out.

    if (start_time + timeout_ms < start_time)
    {
	if (now < start_time && now > start_time + timeout_ms)
	    return false;
	else
	    return true;
    }

    // Deal with the simple case
    // |    s_____t    |
    return (now < start_time + timeout_ms);
}

SerialConnection::SerialConnection(const char *d, int r)
    : device(d), baud_rate(r)
{
    hPort = INVALID_HANDLE_VALUE;
}

SerialConnection::~SerialConnection()
{
    Close();
}

int SerialConnection::PacketSize()
{
    // Return maximum serial packet payload
    return SERIAL_MAXPAYLOAD;
}

// Device should be something like "\\\\.\\COM1"
STATUS SerialConnection::Open()
{
    if (hPort != INVALID_HANDLE_VALUE)
    {
	CloseHandle(hPort);
	hPort = INVALID_HANDLE_VALUE;
    }
    DCB dcb;
    hPort = CreateFile(device.c_str(), GENERIC_READ | GENERIC_WRITE,
		       0 /* No sharing */, NULL /* Default security */,
		       OPEN_EXISTING,
#ifdef USE_OVERLAPPED_SERIAL
		       FILE_FLAG_OVERLAPPED,
#else
		       0,
#endif
		       NULL /* no template */);
    if (hPort == INVALID_HANDLE_VALUE)
	return SERIAL_E_HPORT;

    if (!GetCommState(hPort, &dcb))
    {
	Close();
	return SERIAL_E_GETCOMMSTATE;
    }

    dcb.BaudRate = baud_rate;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if (!SetCommState(hPort, &dcb))
    {
	Close();
	return SERIAL_E_SETCOMMSTATE;
    }

    COMMTIMEOUTS cto;

    if (!GetCommTimeouts(hPort, &cto))
    {
	Close();
	return SERIAL_E_GETTIMEOUTS;
    }

#ifndef USE_OVERLAPPED_SERIAL
    cto.ReadIntervalTimeout = MAXDWORD;
    cto.ReadTotalTimeoutMultiplier = MAXDWORD;
    cto.ReadTotalTimeoutConstant = 200;
    cto.WriteTotalTimeoutConstant = 5000;
    cto.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(hPort, &cto))
    {
	Close();
	return SERIAL_E_SETTIMEOUTS;
    }
#endif

#ifdef USE_OVERLAPPED_SERIAL
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEvent(NULL,  TRUE /* manual reset */,
				    FALSE, /* initially non-signalled */
				    NULL);
    if (overlapped.hEvent == NULL)
    {
	Close();
	return SERIAL_E_SETOVERLAPPED;
    }
#endif

    return S_OK;
}

void SerialConnection::Close()
{
    if (hPort != INVALID_HANDLE_VALUE)
    {
	CloseHandle(hPort);
	hPort = INVALID_HANDLE_VALUE;
    }
}

// Send a block
STATUS SerialConnection::Send(const unsigned char *buffer, int length)
{
    DWORD dwTotal = 0;
    DWORD dwChunk;
    const unsigned char *towrite = buffer;
    int left = length;
#ifdef USE_OVERLAPPED_SERIAL
    while (dwTotal < static_cast<DWORD>(length))
    {
	if (!WriteFile(hPort, towrite, left, &dwChunk, &overlapped))
	{
	    if (GetLastError() == ERROR_IO_PENDING)
	    {
		// We've been deferred, wait for it.
		if (WaitForSingleObject(overlapped.hEvent, WRITE_TIMEOUT) != WAIT_OBJECT_0)
		{
		    DWORD error = GetLastError();
		    // We failed. Cancel it.
		    PurgeComm(hPort, PURGE_TXABORT);
		    //CancelIo(hPort);
		    return MakeWin32Status(error);
		}
		if (!GetOverlappedResult(hPort, &overlapped, &dwChunk, FALSE))
		{
		    DWORD error = GetLastError();
		    return MakeWin32Status(error);
		}
	    }
	    else
	    {
		DWORD error = GetLastError();
		return MakeWin32Status(error);
	    }
	}
	dwTotal += dwChunk;
	towrite += dwChunk;
	left -= dwChunk;
    }
    return 0;
#else // USE_OVERLAPPED_SERIAL
    if (!WriteFile(hPort, towrite, left, &dwChunk, NULL))
    {
	DWORD error = GetLastError();
	return MakeWin32Status(error);
    }
    else
	return S_OK;
#endif
}

// Get data from remote end
STATUS SerialConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    DWORD error = ERROR_SUCCESS;

#ifdef USE_OVERLAPPED_SERIAL
    /* If this returns true then the read succeeded immediately */
    if (!ReadFile(hPort, buffer, buffer_size, &bytes_read, &overlapped))
    {
	if (GetLastError() == ERROR_IO_PENDING)
	{
	    // We've been deferred, wait for it.
#if 0
	    if (timeout_ms < 0)
	    {
				// We've got a negative timeout so just bail out
				// with zero bytes
				//CancelIo(hPort);
		PurgeComm(hPort, PURGE_RXABORT);
		return S_OK;
	    }
#endif
	    DWORD dwTimeout;
	    // We used to set dwTimeout to 100 here since apparently
	    // we can get caught pending even if there is actually
	    // data available.
	    if (timeout < 0)
		dwTimeout = 0;
	    else
		dwTimeout = (timeout_ms) ? timeout : INFINITE;
	    switch(WaitForSingleObject(overlapped.hEvent, dwTimeout))
	    {
	    case WAIT_OBJECT_0:
		/* We succeeded */
		break;
	    case WAIT_TIMEOUT:
		/* We timed out */
		PurgeComm(hPort, PURGE_RXABORT);
		return CONN_E_TIMEDOUT;

	    case WAIT_ABANDONED:
		/* We were abandoned, did the hEvent go away? */
		error = GetLastError();
		PurgeComm(hPort, PURGE_RXABORT);
		return MakeWin32Status(error);

	    case WAIT_FAILED:
	    default:
		error = GetLastError();
		PurgeComm(hPort, PURGE_RXABORT);
		return MakeWin32Status(error);
	    }

	    if (!GetOverlappedResult(hPort, &overlapped, &dwBytes, FALSE))
		return MakeWin32Status(GetLastError());
	}
	else
	    return MakeWin32Status(GetLastError());
    }

    return S_OK;
#else // USE_OVERLAPPED_SERIAL

    Interval interval(timeout_ms);
    do
    {
	if (!ReadFile(hPort, buffer, buffer_size, &bytes_read, NULL))
	{
	    DWORD error = GetLastError();
	    return MakeWin32Status(error);
	}
    }
    while (bytes_read == 0 && interval.Pending());

    if (bytes_read == 0)
	TRACEC(TRACE_PROTOCOL, "SerialConnection::FillBuffer: leaving without reading anything.\n");

#ifdef DEBUG_SERIAL
    if (!serial_log)
    {
	serial_log = fopen("C:\\empserial.log", "a+");
    }
    if (serial_log)
    {
	fwrite(buffer, bytes_read, 1, serial_log);
	fflush(serial_log);
    }
#endif
    if (!bytes_read)
	return CONN_E_TIMEDOUT;
    else
	return S_OK;
#endif
}

HANDLE SerialConnection::GetPollHandle()
{
    return hPort;
}

// USB Connection class.

UsbConnection::FN_CANCELIO UsbConnection::fnCancelIo = NULL;
BOOL UsbConnection::bCancelIoChecked = false;

BOOL UsbConnection::CancelIo()
{
    ASSERT(hPort != INVALID_HANDLE_VALUE);
    ASSERT(fnCancelIo != NULL);
    BOOL result = fnCancelIo(hPort);

    if (!result)
        TRACEC(TRACE_PROTOCOL, "***\nCancelIo failed with result %d\n***\n", GetLastError());

    return result;
}

void UsbConnection::CheckCancelIo()
{
    if (!bCancelIoChecked)
    {
	// Kernel32 will be loaded permanently anyway so there's no harm in leaking a reference
	// count on it.
	HMODULE hModule = LoadLibrary("KERNEL32");
	ASSERT(hModule);
	if (hModule)
	{
	    fnCancelIo = (FN_CANCELIO)GetProcAddress(hModule, "CancelIo");
	}
	TRACEC(TRACE_PROTOCOL, "CancelIo is at %p\n", fnCancelIo);
	bCancelIoChecked = true;
    }
}

UsbConnection::UsbConnection(const char *d)
    : device(d), hPort(INVALID_HANDLE_VALUE), overlapped_read(single_overlapped), overlapped_write(single_overlapped)
{
    memset(&single_overlapped, 0, sizeof(single_overlapped));
    CheckCancelIo();
}

UsbConnection::~UsbConnection()
{
    Close();
}

int UsbConnection::PacketSize()
{
    // Return maximum USB packet payload
    return USB_MAXPAYLOAD;
}

STATUS UsbConnection::Open()
{
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Open() device='%s'\n", this, device.c_str());
    CritSectLock lock(usb_cs);

    if (hPort != INVALID_HANDLE_VALUE)
    {
	CloseHandle(hPort);
	hPort = INVALID_HANDLE_VALUE;
    }

    if (fnCancelIo == NULL)
    {
	// We can't work without CancelIo, better fail.
	return USB_E_NOCANCELIO;
    }

    hPort = CreateFile(device.c_str(), GENERIC_READ | GENERIC_WRITE,
		       0 /* No sharing */, NULL /* Default security */,
		       OPEN_EXISTING,
#ifndef NON_BLOCKING_READ
		       FILE_FLAG_OVERLAPPED,
#else
		       0,
#endif
		       NULL /* no template */);
    if (hPort == INVALID_HANDLE_VALUE)
    {
	DWORD d = GetLastError();
        if (d == ERROR_NO_SYSTEM_RESOURCES)
        {
            TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Open()[A] = %0x%x\n", this, USB_E_HPORT);
            return USB_E_INUSE;
        }
        else
        {
	    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Open()[A] = %0x%x, last error=%ld\n", this, USB_E_HPORT, d);
	    return USB_E_HPORT;
        }
    }

    memset(&single_overlapped, 0, sizeof(single_overlapped));
    single_overlapped.hEvent = CreateEvent(NULL, TRUE /* manual reset */,
					   FALSE, NULL);
    if (single_overlapped.hEvent == NULL)
    {
	Close();
	TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Open()[B] = %d\n", this, -73);
	return USB_E_SETOVERLAPPED;
    }

    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Open()[C] = %d\n", this, 0);

    return S_OK;
}

void UsbConnection::Close()
{
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Close()\n", this);
    CritSectLock lock(usb_cs);

    if (hPort != INVALID_HANDLE_VALUE)
    {
	CloseHandle(hPort);
	hPort = INVALID_HANDLE_VALUE;
    }

    if (single_overlapped.hEvent)
    {
	CloseHandle(single_overlapped.hEvent);
	single_overlapped.hEvent = NULL;
    }
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Close()\n", this);
}

void UsbConnection::CheckStallState(const char *when)
{
    static ULONG last_stall_count = 0;

   // First check to see if we are stalled.
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::CheckStallState()\n");
    CritSectLock lock(usb_cs);

    DWORD bytes_returned;
    ULONG stall_count;
    BOOL success;

    TRACEC(TRACE_PROTOCOL, "Calling DeviceIoControl\n");
    success = DeviceIoControl(hPort, IOCTL_EMPEGUSB_GET_STALL_COUNT, NULL, 0, &stall_count, sizeof(stall_count), &bytes_returned, &overlapped_read);
    TRACEC(TRACE_PROTOCOL, "DeviceIoControl returned %d\n", success);

    if (!success)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
	    // We've been deferred. Wait
            TRACEC(TRACE_PROTOCOL, "Waiting for overlapped event\n");
	    switch (WaitForSingleObject(overlapped_read.hEvent, 1000))
	    {
	    case WAIT_ABANDONED:
		TRACEC(TRACE_PROTOCOL, "Wait abandoned checking stall state.\n");
		success = false;
		break;
	    case WAIT_TIMEOUT:
		TRACEC(TRACE_PROTOCOL, "Wait timed out checking stall state.\n");
		success = false;
		break;
	    case WAIT_OBJECT_0:
                TRACEC(TRACE_PROTOCOL, "Getting overlapped result\n");
		success = GetOverlappedResult(hPort, &overlapped_read, &bytes_returned, FALSE);
                if (success)
                {
                    TRACEC(TRACE_PROTOCOL, "Got overlapped result: %d bytes\n", bytes_returned);
                }
                else
                {
                    TRACEC(TRACE_PROTOCOL, "GetOverlappedResult failed getting stall state. error=%d.\n", GetLastError());
                }
		break;
	    default:
	    case WAIT_FAILED:
		TRACEC(TRACE_PROTOCOL, "Wait failed getting stall state.\n\n");
		success = false;
		break;
	    }
	}
        else
        {
            TRACEC(TRACE_PROTOCOL, "Stall state ioctl failed. error=%d.\n", GetLastError());
        }
    }

    if (success)
    {
        if (bytes_returned == sizeof(stall_count))
        {
            TRACEC(TRACE_PROTOCOL, "@@ STALL COUNT is %ld %s\n", stall_count, when);

            if (stall_count > last_stall_count)
            {
                ResetPipe();
                last_stall_count = stall_count;
            }
        }
        else
        {
            TRACEC(TRACE_PROTOCOL, "Stall state returned incorrect (%ld) number of bytes\n", bytes_returned);
        }
    }
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::CheckStallState()\n");
}

inline bool UsbConnection::IsResetRequired(DWORD error)
{
    return (error == ERROR_CRC);
}

inline void UsbConnection::ResetReadEvent()
{
    TRACEC(TRACE_PROTOCOL, "Resetting read event\n");
    ResetEvent(overlapped_read.hEvent);
}

inline void UsbConnection::ResetWriteEvent()
{
    TRACEC(TRACE_PROTOCOL, "Resetting write event\n");
    ResetEvent(overlapped_write.hEvent);
}

void UsbConnection::ResetPipe()
{
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::ResetPipe()\n", this);
    CritSectLock lock(usb_cs);

    static int call_count = 0;

    ++call_count;

    ASSERT(hPort != INVALID_HANDLE_VALUE);
    BOOL success;

    // Used to reset device here but it seemed to cause things
    // to break. Just stay safe and reset the pipe.

    int ioctl_value;
    if (call_count % 2 == 0)
    {
        TRACEC(TRACE_PROTOCOL, "+++++ RESETTING USB PIPE +++++\n");
        ioctl_value = IOCTL_EMPEGUSB_RESET_PIPE;
    }
    else
    {
        TRACEC(TRACE_PROTOCOL, "+++++ RESETTING USB DEVICE +++++\n");
        ioctl_value = IOCTL_EMPEGUSB_RESET_DEVICE;
    }

    success = DeviceIoControl(hPort,
			      ioctl_value,
			      NULL,
			      0,
			      NULL,
			      0,
			      NULL,
			      &overlapped_read);

    if (!success)
    {
	if (GetLastError() == ERROR_IO_PENDING)
	{
	    // We've been deferred. Wait
	    switch (WaitForSingleObject(overlapped_read.hEvent, 1000))
	    {
	    case WAIT_ABANDONED:
		TRACEC(TRACE_PROTOCOL, "Wait abandoned resetting USB pipe.\n\n");
		success = false;
		break;
	    case WAIT_TIMEOUT:
		TRACEC(TRACE_PROTOCOL, "Wait timeout resetting USB pipe\n");
		success = false;
		break;
	    case WAIT_OBJECT_0:
		success = true;
		break;
	    default:
	    case WAIT_FAILED:
		TRACEC(TRACE_PROTOCOL, "Wait failed resetting USB pipe.\n\n");
		success = false;
		break;
	    }
	}
    }

    if (success)
    {
        DWORD bytes_returned;
        if (GetOverlappedResult(hPort, &overlapped_read, &bytes_returned, FALSE))
        {
            TRACEC(TRACE_PROTOCOL, "Successfully reset USB pipe.\n");
        }
        else
        {
            TRACEC(TRACE_PROTOCOL, "Failed to get overlapped result from resetting USB pipe. Error=%d\n", GetLastError());
            if (!CancelIo())
            {
                TRACEC(TRACE_PROTOCOL, "Failed to CancelIo. Error=%d\n", GetLastError());
            }
        }
    }
    else
    {
	DWORD status = GetLastError();
	TRACEC(TRACE_PROTOCOL, "Error %d (0x%x) occurred during reset pipe ioctl.\n", status, status);
    }

    if (call_count > 6)
    {
        TRACEC(TRACE_PROTOCOL, "Attempting to close and reopen connection\n");
        Close();
        TRACEC(TRACE_PROTOCOL, "Connection closed.\n");
        int result = Open();
        TRACEC(TRACE_PROTOCOL, "Connection reopened, result=%d (0x%x).\n", result, result);
    }
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::ResetPipe\n", this);
}

void UsbConnection::ResetIfRequired(DWORD error)
{
    CheckStallState("In ResetIfRequired");
    if (IsResetRequired(error))
	ResetPipe();
}

// Send a block
STATUS UsbConnection::Send(const unsigned char *buffer, int length)
{
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Send(buffer=%p, length=%d)\n", this, buffer, length);

    CheckStallState("Before USB Send");

    CritSectLock lock(usb_cs);
    DWORD dwTotal = 0;
    DWORD dwChunk;
    const unsigned char *towrite = buffer;
    int left = length;
    STATUS status = S_OK;
    int retry_count = 0;

    ASSERT(fnCancelIo != NULL);

    while (SUCCEEDED(status) &&
	    (dwTotal < static_cast<DWORD>(length)) &&
	    (retry_count < INCOMPLETE_IO_RETRIES))
    {
        BOOL write_result = WriteFile(hPort, towrite, left, &dwChunk, &overlapped_write);
	if (!write_result)
	{
	    if (GetLastError() == ERROR_IO_PENDING)
	    {
		TRACEC(TRACE_PROTOCOL, "USB Write is pending\n");
		// We've been deferred, wait for it.
		switch(WaitForSingleObject(overlapped_write.hEvent, WRITE_TIMEOUT))
		{
		case WAIT_OBJECT_0:
		    // OK.
		    status = S_OK;
		    break;
		case WAIT_ABANDONED:
		    TRACEC(TRACE_PROTOCOL, "USB Write: Wait abandoned.\n");
                    CancelIo();
		    ResetWriteEvent();
		    status = USB_E_WAIT_ABANDONED;
		    break;
		case WAIT_TIMEOUT:
		    TRACEC(TRACE_PROTOCOL, "USB Write: Timeout.\n");
                    CancelIo();
		    ResetWriteEvent();
		    // Things have gone a bit pear shaped. Better reset things.
		    ResetPipe();

		    status = CONN_E_TIMEDOUT;
		    break;
		case WAIT_FAILED:
		default:
		    TRACEC(TRACE_PROTOCOL, "USB Write: Wait failed.\n");
                    CancelIo();
		    ResetWriteEvent();
		    status = MakeWin32Status(GetLastError());
		    break;
		}
		if (SUCCEEDED(status) &&
		    !GetOverlappedResult(hPort, &overlapped_write, &dwChunk, FALSE))
		{
		    DWORD error = GetLastError();
    		    TRACEC(TRACE_PROTOCOL, "GetOverlappedResult failed with error %d.\n", error);
		    ResetIfRequired(error);
		    ResetWriteEvent();
                    CancelIo();
		    if (error == ERROR_IO_INCOMPLETE)
		    {
			TRACEC(TRACE_PROTOCOL, "Got an ERROR_IO_INCOMPLETE whilst writing, retrying.\n");
                        CheckStallState("after ERROR_IO_INCOMPLETE");
			Sleep(INCOMPLETE_IO_DELAY);
			dwChunk = 0;
			if (++retry_count < INCOMPLETE_IO_RETRIES)
			    status = S_OK; // Go round again.
		    }
		    else
			status = MakeWin32Status(error);
		}
	    }
	    else
	    {
		DWORD error = GetLastError();
		TRACEC(TRACE_PROTOCOL, "WriteFile failed with error %d.\n", error);
		ResetIfRequired(error);
		ResetWriteEvent();
		status = MakeWin32Status(error);
	    }
	}
	else
	{
	    TRACEC(TRACE_PROTOCOL, "USB WriteFile completed immediately\n");
	}

	if (SUCCEEDED(status))
	{
	    ResetWriteEvent();
	    dwTotal += dwChunk;
	    towrite += dwChunk;
	    left -= dwChunk;
	    TRACEC(TRACE_PROTOCOL, "USB Write chunk succeeded (maybe): %d remaining\n", left);
	}
    }

    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::Send()=%d (0x%x)\n", this, status, status);
    return status;
}

// Get data from remote end
STATUS UsbConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    TRACEC(TRACE_PROTOCOL, "UsbConnection(%p)::FillBuffer(buffer=%p, buffer_size=%d, timeout_ms=%d)\n",
		      this, buffer, buffer_size, timeout_ms);

    CheckStallState("Before USB FillBuffer");

    CritSectLock lock(usb_cs);
    // We must have at least one USB buffer full of data.
    STATUS result = S_OK;
    ASSERT(buffer_size >= 4096);
    ASSERT(fnCancelIo != NULL);
    int retry_count = 0;

    bytes_read = 0;

    while (retry_count < INCOMPLETE_IO_RETRIES)
    {
	ResetReadEvent();
	TRACEC(TRACE_PROTOCOL, "Doing ReadFile\n");
        BOOL read_result = ReadFile(hPort, buffer, buffer_size, &bytes_read, &overlapped_read);
	/* If this returns true then the read succeeded immediately */
	if (!read_result)
	{
	    if (GetLastError() != ERROR_IO_PENDING)
	    {
		// We've failed straight away. Not good.
		DWORD error = GetLastError();
		TRACEC(TRACE_PROTOCOL, "ReadFile failed with error %ld\n", error);
		result = MakeWin32Status(error);
		TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[A] = 0x%x\n", this, result);
		return result;
	    }
	}
	else
	{
	    TRACEC(TRACE_PROTOCOL, "Read succeeded immediately\n");
	    // The read succeeded immediately
	    TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[B] = 0x%x\n", this, bytes_read);
	    return S_OK;
	}

	// We've been deferred, wait for it.
	DWORD dwTimeout;

	// We can't do a zero timeout since Windows 98 will just
	// give up straight away even if data is there.
	if (timeout_ms < 0)
	    dwTimeout = INFINITE;
	else if (timeout_ms <= 100)
	    dwTimeout = 100UL;
	else
	    dwTimeout = timeout_ms;

	TRACEC(TRACE_PROTOCOL, "Read is pending, waiting with timeout %ld\n", dwTimeout);
	switch(WaitForSingleObject(overlapped_read.hEvent, dwTimeout))
	{
	case WAIT_OBJECT_0:
	    TRACEC(TRACE_PROTOCOL, "Wait succeeded. Data should be present.\n\n");
	    /* We succeeded */
	    break;
	case WAIT_TIMEOUT:
	    /* We timed out */
	    // Leave the request pending and carry on.
	    TRACEC(TRACE_PROTOCOL, "Wait timed out, cancelling read\n");
            CancelIo();
	    bytes_read = 0;
	    result = CONN_E_TIMEDOUT;
	    TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[C] = 0x%x\n", this, result);
	    return result;

	case WAIT_ABANDONED:
	{
	    TRACEC(TRACE_PROTOCOL, "Wait abandoned. Cancelling request\n");
	    /* We were abandoned, did the hEvent go away? */
            CancelIo();
	    ResetReadEvent();
	    bytes_read = 0;
	    result = USB_E_WAITABANDONED;
	    TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[D] = 0x%x\n", this, result);
	    return result;
	}

	case WAIT_FAILED:
	default:
	{
	    DWORD error = GetLastError();
	    TRACEC(TRACE_PROTOCOL, "Wait failed (%d). Cancelling request\n", error);
            CancelIo();
	    ResetReadEvent();
	    bytes_read = 0;
	    result = MakeWin32Status(error);
	    TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[E] = 0x%x\n", this, result);
	    return result;
	}

	}

	if (!GetOverlappedResult(hPort, &overlapped_read, &bytes_read, FALSE))
	{
	    DWORD error = GetLastError();

	    TRACEC(TRACE_PROTOCOL, "Failed to get overlapped result (0x%x). Cancelling request\n", error);

	    // Now cancel the request
            CancelIo();
	    ResetReadEvent();

	    bytes_read = 0;
	    if (error == ERROR_IO_INCOMPLETE)
	    {
		TRACEC(TRACE_PROTOCOL, "Get an ERROR_IO_INCOMPLETE whilst reading, retrying.\n");
		// Sleep for a while then try again.
		Sleep(INCOMPLETE_IO_DELAY);
	    }
	    else
	    {
		result = MakeWin32Status(error);
		TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[F] = 0x%x\n", this, result);
		return result;
	    }
	}
	else
	{
	    break;
	}
	retry_count++;
    }

    ResetReadEvent();

    TRACEC(TRACE_PROTOCOL, "Read succeeded: %d bytes\n\n", bytes_read);
    TRACEC(TRACE_PROTOCOL, "UsbConnection::FillBuffer(%p)[G] = 0x%x\n", this, result);
    return result; // S_OK
}

HANDLE UsbConnection::GetPollHandle()
{
    return hPort;
}

// OutSocketConnection

OutSocketConnection::OutSocketConnection()
    : sock(INVALID_SOCKET)
{
}

OutSocketConnection::~OutSocketConnection()
{
    Close();
}

void OutSocketConnection::Close()
{
    if (sock != INVALID_SOCKET)
    {
	closesocket(sock);
	sock = INVALID_SOCKET;
    }
}

/** Send a block of data
 *
 * Unlike unix calls, this write will only return when either the entire buffer
 * has been sent or an error occurrs. A partial write will cause a retry.
 */
STATUS OutSocketConnection::Send(const unsigned char *buffer, int length)
{
    // Write entire data packet: retry as necessary. We only return
    // when the entire buffer has been sent or there has been an error
    int done=0,r;

    while(done<length)
    {
	r = send(sock, (const char *)buffer+done, length - done, 0);

	if (r <= 0)
	{
	    TRACEC(TRACE_PROTOCOL, "send failed, last error = %d\n", WSAGetLastError());
	    return MakeWin32Status( WSAGetLastError() );
	}
	done+=r;
    }

    // Done OK
    return S_OK;
}

/** Get data from remote end with timeout
 *
 * if timeout>0 , it indicates a timeout in milliseconds to wait for data
 * if timeout==0, it indicates that we should read only if we can read
 *                without delay (non-blocking read, basically)
 * if timeout<0 , it indicates that we should block until we can read
 *
 * Note that in accordance with unixisms, the read will return as many bytes
 * as are available, up to the buffer size. If we reach the read() call,
 * we will get at least one byte.
 *
 * Returns -errno for an error, otherwise we return the number of bytes read.
 */
STATUS OutSocketConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    bytes_read = 0;

    if (timeout_ms >= 0)
    {
	timeval tv;

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = 1000 * (timeout_ms % 1000);

	FD_SET readfds;
	FD_ZERO(&readfds);

	FD_SET(sock, &readfds);

	int result = select(1, &readfds, NULL, NULL, &tv);
	if (result == SOCKET_ERROR )
	{
	    TRACEC(TRACE_PROTOCOL, "Select on socket failed: %ld\n", WSAGetLastError());
	    return SOCK_E_SELECT;
	}
	else if (result == 0)
        {
            TRACEC(TRACE_PROTOCOL, "Select timed out (%dms)\n", timeout_ms);
	    return CONN_E_TIMEDOUT;
        }
    }

    // Get some stuff: will block until at least one byte comes in
    //n = _read(sock, buffer, buffer_size);
    int result = recv(sock, (char *)buffer, buffer_size, 0);
    if (result == SOCKET_ERROR || result <= 0)
    {
	TRACEC(TRACE_PROTOCOL, "read on socket failed: %ld\n", WSAGetLastError());
	return SOCK_E_READ;
    }

    bytes_read = result;

    return S_OK;
}

HANDLE OutSocketConnection::GetPollHandle()
{
    return (HANDLE)sock;
}

// OutTcpConnection

STATUS OutTcpConnection::CreateAndBind()
{
    // Lookup the hostname
    struct sockaddr_in sa;

    unsigned long ia = inet_addr(host.c_str());
    if (ia == INADDR_NONE)
	return TCP_E_ADDR;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = ia;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
	TRACEC(TRACE_PROTOCOL, "socket() failed: %ld\n", WSAGetLastError());
	return TCP_E_OPEN;
    }

    // Disable Nagle algorithm for better responsiveness (we only do single
    // write()s anyway for USB reasons)
    int one = 1;
    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY,
		     (const char*)&one, sizeof(one) ) < 0 )
    {
	TRACEC(TRACE_PROTOCOL, "setsockopt() failed: %ld\n", WSAGetLastError());
	closesocket(sock);
	return TCP_E_SETSOCKOPT;
    }

    if (connect(sock, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
    {
	TRACEC(TRACE_PROTOCOL, "connect() failed: %ld\n", WSAGetLastError());
	closesocket(sock);
	return TCP_E_CONNECT;
    }

    TRACEC(TRACE_PROTOCOL, "Created connection to \'%s\' (0x%x)\n", host.c_str(), ia);

    return S_OK;
}
