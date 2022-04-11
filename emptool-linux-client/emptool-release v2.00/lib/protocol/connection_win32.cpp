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
 * (:Empeg Source Release 1.44.2.1 01-Apr-2003 18:52 rob:)
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
#include "protocol_errors.h"
#include "NgLog/NgLog.h"
LOG_COMPONENT(Connection)
#include <winioctl.h>
#include <io.h>
#include "win32_thread/CritSect.h"

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
	LOG_INFO("SerialConnection::FillBuffer: leaving without reading anything.");

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
        LOG_INFO("***\nCancelIo failed with result %d\n***", GetLastError());

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
	LOG_INFO("CancelIo is at %p", fnCancelIo);
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
    LOG_ENTER("UsbConnection(%p)::Open() device='%s'", this, device.c_str());
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
            LOG_LEAVE("UsbConnection(%p)::Open()[A] = %0x%x", this, USB_E_HPORT);
            return USB_E_INUSE;
        }
        else
        {
	    LOG_LEAVE("UsbConnection(%p)::Open()[A] = 0x%x, last error=%ld (0x%ld)", this, USB_E_HPORT, d, d);
	    return USB_E_HPORT;
        }
    }

    memset(&single_overlapped, 0, sizeof(single_overlapped));
    single_overlapped.hEvent = CreateEvent(NULL, TRUE /* manual reset */,
					   FALSE, NULL);
    if (single_overlapped.hEvent == NULL)
    {
	Close();
	LOG_LEAVE("UsbConnection(%p)::Open()[B] = %d", this, -73);
	return USB_E_SETOVERLAPPED;
    }

    LOG_LEAVE("UsbConnection(%p)::Open()[C] = %d", this, 0);

    return S_OK;
}

void UsbConnection::Close()
{
    LOG_ENTER("UsbConnection(%p)::Close()", this);
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
    LOG_LEAVE("UsbConnection(%p)::Close()", this);
}

void UsbConnection::CheckStallState(const char *when)
{
    static ULONG last_stall_count = 0;

   // First check to see if we are stalled.
    LOG_ENTER("UsbConnection(%p)::CheckStallState()");
    CritSectLock lock(usb_cs);

    DWORD bytes_returned;
    ULONG stall_count;
    BOOL success;

    LOG_INFO("Calling DeviceIoControl");
    success = DeviceIoControl(hPort, IOCTL_EMPEGUSB_GET_STALL_COUNT, NULL, 0, &stall_count, sizeof(stall_count), &bytes_returned, &overlapped_read);
    LOG_INFO("DeviceIoControl returned %d", success);

    if (!success)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
	    // We've been deferred. Wait
            LOG_INFO("Waiting for overlapped event");
	    switch (WaitForSingleObject(overlapped_read.hEvent, 1000))
	    {
	    case WAIT_ABANDONED:
		LOG_INFO("Wait abandoned checking stall state.");
		success = false;
		break;
	    case WAIT_TIMEOUT:
		LOG_INFO("Wait timed out checking stall state.");
		success = false;
		break;
	    case WAIT_OBJECT_0:
                LOG_INFO("Getting overlapped result");
		success = GetOverlappedResult(hPort, &overlapped_read, &bytes_returned, FALSE);
                if (success)
                {
                    LOG_INFO("Got overlapped result: %d bytes", bytes_returned);
                }
                else
                {
                    LOG_INFO("GetOverlappedResult failed getting stall state. error=%d.", GetLastError());
                }
		break;
	    default:
	    case WAIT_FAILED:
		LOG_INFO("Wait failed getting stall state.\n");
		success = false;
		break;
	    }
	}
        else
        {
            LOG_INFO("Stall state ioctl failed. error=%d.", GetLastError());
        }
    }

    if (success)
    {
        if (bytes_returned == sizeof(stall_count))
        {
            LOG_INFO("@@ STALL COUNT is %ld %s", stall_count, when);

            if (stall_count > last_stall_count)
            {
                ResetPipe();
                last_stall_count = stall_count;
            }
        }
        else
        {
            LOG_INFO("Stall state returned incorrect (%ld) number of bytes", bytes_returned);
        }
    }
    LOG_LEAVE("UsbConnection(%p)::CheckStallState()");
}

inline bool UsbConnection::IsResetRequired(DWORD error)
{
    return (error == ERROR_CRC);
}

inline void UsbConnection::ResetReadEvent()
{
    LOG_INFO("Resetting read event");
    ResetEvent(overlapped_read.hEvent);
}

inline void UsbConnection::ResetWriteEvent()
{
    LOG_INFO("Resetting write event");
    ResetEvent(overlapped_write.hEvent);
}

void UsbConnection::ResetPipe()
{
    LOG_ENTER("UsbConnection(%p)::ResetPipe()", this);
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
        LOG_INFO("+++++ RESETTING USB PIPE +++++");
        ioctl_value = IOCTL_EMPEGUSB_RESET_PIPE;
    }
    else
    {
        LOG_INFO("+++++ RESETTING USB DEVICE +++++");
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
		LOG_INFO("Wait abandoned resetting USB pipe.\n");
		success = false;
		break;
	    case WAIT_TIMEOUT:
		LOG_INFO("Wait timeout resetting USB pipe");
		success = false;
		break;
	    case WAIT_OBJECT_0:
		success = true;
		break;
	    default:
	    case WAIT_FAILED:
		LOG_INFO("Wait failed resetting USB pipe.\n");
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
            LOG_INFO("Successfully reset USB pipe.");
        }
        else
        {
            LOG_INFO("Failed to get overlapped result from resetting USB pipe. Error=%d", GetLastError());
            if (!CancelIo())
            {
                LOG_INFO("Failed to CancelIo. Error=%d", GetLastError());
            }
        }
    }
    else
    {
	DWORD status = GetLastError();
	LOG_INFO("Error %d (0x%x) occurred during reset pipe ioctl.", status, status);
    }

    if (call_count > 6)
    {
        LOG_INFO("Attempting to close and reopen connection");
        Close();
        LOG_INFO("Connection closed.");
        int result = Open();
        LOG_INFO("Connection reopened, result=%d (0x%x).", result, result);
    }
    LOG_LEAVE("UsbConnection(%p)::ResetPipe", this);
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
    LOG_ENTER("UsbConnection(%p)::Send(buffer=%p, length=%d)", this, buffer, length);

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
		LOG_INFO("USB Write is pending");
		// We've been deferred, wait for it.
		switch(WaitForSingleObject(overlapped_write.hEvent, WRITE_TIMEOUT))
		{
		case WAIT_OBJECT_0:
		    // OK.
		    status = S_OK;
		    break;
		case WAIT_ABANDONED:
		    LOG_INFO("USB Write: Wait abandoned.");
                    CancelIo();
		    ResetWriteEvent();
		    status = USB_E_WAIT_ABANDONED;
		    break;
		case WAIT_TIMEOUT:
		    LOG_INFO("USB Write: Timeout.");
                    CancelIo();
		    ResetWriteEvent();
		    // Things have gone a bit pear shaped. Better reset things.
		    ResetPipe();

		    status = CONN_E_TIMEDOUT;
		    break;
		case WAIT_FAILED:
		default:
		    LOG_INFO("USB Write: Wait failed.");
                    CancelIo();
		    ResetWriteEvent();
		    status = MakeWin32Status(GetLastError());
		    break;
		}
		if (SUCCEEDED(status) &&
		    !GetOverlappedResult(hPort, &overlapped_write, &dwChunk, FALSE))
		{
		    DWORD error = GetLastError();
    		    LOG_INFO("GetOverlappedResult failed with error %d.", error);
		    ResetIfRequired(error);
		    ResetWriteEvent();
                    CancelIo();
		    if (error == ERROR_IO_INCOMPLETE)
		    {
			LOG_INFO("Got an ERROR_IO_INCOMPLETE whilst writing, retrying.");
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
		LOG_INFO("WriteFile failed with error %d.", error);
		ResetIfRequired(error);
		ResetWriteEvent();
		status = MakeWin32Status(error);
	    }
	}
	else
	{
	    LOG_INFO("USB WriteFile completed immediately");
	}

	if (SUCCEEDED(status))
	{
	    ResetWriteEvent();
	    dwTotal += dwChunk;
	    towrite += dwChunk;
	    left -= dwChunk;
	    LOG_INFO("USB Write chunk succeeded (maybe): %d remaining", left);
	}
    }

    LOG_LEAVE("UsbConnection(%p)::Send()=%d (0x%x)", this, status, status);
    return status;
}

// Get data from remote end
STATUS UsbConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    LOG_ENTER("UsbConnection(%p)::FillBuffer(buffer=%p, buffer_size=%d, timeout_ms=%d)",
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
	LOG_INFO("Doing ReadFile");
        BOOL read_result = ReadFile(hPort, buffer, buffer_size, &bytes_read, &overlapped_read);
	/* If this returns true then the read succeeded immediately */
	if (!read_result)
	{
	    if (GetLastError() != ERROR_IO_PENDING)
	    {
		// We've failed straight away. Not good.
		DWORD error = GetLastError();
		LOG_INFO("ReadFile failed with error %ld", error);
		result = MakeWin32Status(error);
		LOG_LEAVE("UsbConnection::FillBuffer(%p)[A] = 0x%x", this, result);
		return result;
	    }
	}
	else
	{
	    LOG_INFO("Read succeeded immediately");
	    // The read succeeded immediately
	    LOG_LEAVE("UsbConnection::FillBuffer(%p)[B] = 0x%x", this, bytes_read);
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

	LOG_INFO("Read is pending, waiting with timeout %ld", dwTimeout);
	switch(WaitForSingleObject(overlapped_read.hEvent, dwTimeout))
	{
	case WAIT_OBJECT_0:
	    LOG_INFO("Wait succeeded. Data should be present.\n");
	    /* We succeeded */
	    break;
	case WAIT_TIMEOUT:
	    /* We timed out */
	    // Leave the request pending and carry on.
	    LOG_INFO("Wait timed out, cancelling read");
            CancelIo();
	    bytes_read = 0;
	    result = CONN_E_TIMEDOUT;
	    LOG_LEAVE("UsbConnection::FillBuffer(%p)[C] = 0x%x", this, result);
	    return result;

	case WAIT_ABANDONED:
	{
	    LOG_INFO("Wait abandoned. Cancelling request");
	    /* We were abandoned, did the hEvent go away? */
            CancelIo();
	    ResetReadEvent();
	    bytes_read = 0;
	    result = USB_E_WAITABANDONED;
	    LOG_LEAVE("UsbConnection::FillBuffer(%p)[D] = 0x%x", this, result);
	    return result;
	}

	case WAIT_FAILED:
	default:
	{
	    DWORD error = GetLastError();
	    LOG_INFO("Wait failed (%d). Cancelling request", error);
            CancelIo();
	    ResetReadEvent();
	    bytes_read = 0;
	    result = MakeWin32Status(error);
	    LOG_LEAVE("UsbConnection::FillBuffer(%p)[E] = 0x%x", this, result);
	    return result;
	}

	}

	if (!GetOverlappedResult(hPort, &overlapped_read, &bytes_read, FALSE))
	{
	    DWORD error = GetLastError();

	    LOG_INFO("Failed to get overlapped result (0x%x). Cancelling request", error);

	    // Now cancel the request
            CancelIo();
	    ResetReadEvent();

	    bytes_read = 0;
	    if (error == ERROR_IO_INCOMPLETE)
	    {
		LOG_INFO("Get an ERROR_IO_INCOMPLETE whilst reading, retrying.");
		// Sleep for a while then try again.
		Sleep(INCOMPLETE_IO_DELAY);
	    }
	    else
	    {
		result = MakeWin32Status(error);
		LOG_LEAVE("UsbConnection::FillBuffer(%p)[F] = 0x%x", this, result);
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

    LOG_INFO("Read succeeded: %d bytes\n", bytes_read);
    LOG_LEAVE("UsbConnection::FillBuffer(%p)[G] = 0x%x", this, result);
    return result; // S_OK
}

HANDLE UsbConnection::GetPollHandle()
{
    return hPort;
}

#ifdef SUPPORT_TCP_PROTOCOL

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
	    LOG_WARN("send failed, last error = %d", WSAGetLastError());
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
	    LOG_INFO("Select on socket failed: %ld", WSAGetLastError());
	    return SOCK_E_SELECT;
	}
	else if (result == 0)
        {
            LOG_DEBUG("Select timed out (%dms)", timeout_ms);
	    return CONN_E_TIMEDOUT;
        }
    }

    // Get some stuff: will block until at least one byte comes in
    //n = _read(sock, buffer, buffer_size);
    int result = recv(sock, (char *)buffer, buffer_size, 0);
    if (result == SOCKET_ERROR || result <= 0)
    {
	LOG_INFO("read on socket failed: %ld", WSAGetLastError());
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
	LOG_WARN("socket() failed: %ld", WSAGetLastError());
	return TCP_E_OPEN;
    }

    // Disable Nagle algorithm for better responsiveness (we only do single
    // write()s anyway for USB reasons)
    int one = 1;
    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY,
		     (const char*)&one, sizeof(one) ) < 0 )
    {
	LOG_WARN("setsockopt() failed: %ld", WSAGetLastError());
	closesocket(sock);
	return TCP_E_SETSOCKOPT;
    }

    if (connect(sock, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
    {
	LOG_WARN("connect() failed: %ld", WSAGetLastError());
	closesocket(sock);
	return TCP_E_CONNECT;
    }

    LOG_INFO("Created connection to \'%s\' (0x%x)", host.c_str(), ia);

    return S_OK;
}

#endif // SUPPORT_TCP_PROTOCOL
