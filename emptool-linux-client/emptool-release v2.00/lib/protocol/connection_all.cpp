/* connection_all.cpp
 *
 * Connection code common to Win32 and Unix
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.41 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#else
#include <stdio.h>
#endif
#include "connection.h"
#include <stdio.h>
#include <ctype.h>

#include "NgLog/NgLog.h"
#if DEBUG>0 || !defined(ARCH_EMPEG)
LOG_COMPONENT(CONNECTION)
#endif
    
#ifdef ARCH_EMPEG
// We don't really need a huge buffer - the kernel will buffer for us.
const int Connection::READ_BUFFER_SIZE = 4096;
#else
// On PCs with lots of memory it is worth having a larger buffer methinks
const int Connection::READ_BUFFER_SIZE = 65536;
#endif    
    
Connection::Connection()
    : debug_level(0), read_buffer(NULL), read_buffer_len(0),
      read_buffer_pos(0)
{
    read_buffer = NEW unsigned char[READ_BUFFER_SIZE];
}

Connection::~Connection()
{
    delete []read_buffer;
}

// Get data from remote end with timeout
//
// if timeout>0 , it indicates a timeout in milliseconds to wait for data
// if timeout==0, it indicates that we should read only if we can read
//                without delay (non-blocking read, basically)
// if timeout<0 , it indicates that we should block until we can read
//
// Note that in accordance with unixisms, the read will return as many bytes
// as are available, up to the buffer size. If we reach the read() call,
// we will get at least one byte.
//
// Modified so if a NULL buffer is passed in it will just throw away the bytes
// instead.
STATUS Connection::Receive(unsigned char *buffer, int length, int timeout_ms, DWORD & bytes_read)
{
    TRACEC(TRACE_CONNECTION,"Connection::Receive(buffer=%p, length=%d, timeout_ms=%d)",
		      buffer, length, timeout_ms);

    STATUS result = S_OK;
    if (read_buffer_pos >= read_buffer_len)
    {
	TRACEC(TRACE_CONNECTION, "No data available - we need to fill the buffer\n");

	result = FillBuffer(read_buffer, READ_BUFFER_SIZE, timeout_ms, bytes_read);

        TRACEC(TRACE_CONNECTION, "Fill buffer result: %d\n", PrintableStatus(result));

	if (SUCCEEDED(result))
	{
	    read_buffer_pos = 0;
	    read_buffer_len = bytes_read;
	}
    }

    if (SUCCEEDED(result))
    {
	int read_count = read_buffer_len - read_buffer_pos;

	TRACEC(TRACE_CONNECTION, "There are %d bytes available in the buffer\n", read_count);

	if (read_count > length)
	    read_count = length;

	TRACEC(TRACE_CONNECTION, "We will read %d of them.\n", read_count);

	ASSERT(read_buffer_pos + read_count <= read_buffer_len);

	if (buffer)
	    memcpy(buffer, read_buffer + read_buffer_pos, read_count);

	read_buffer_pos += read_count;
	bytes_read = read_count;

	TRACEC(TRACE_CONNECTION, "We've read %ld bytes\n", bytes_read);
    }

    TRACEC(TRACE_CONNECTION, "Connection::Receive() = %d (0x%x)\n", PrintableStatus(result), PrintableStatus(result));

    return result;
}


// Get single byte from remote
STATUS Connection::Receive(int timeout_ms, int & received)
{
    unsigned char b = 0;
    DWORD bytes_read;
    STATUS result;
    if (FAILED(result = Receive(&b, 1, timeout_ms, bytes_read)))
	return result;

    received = b;

    return S_OK;
}


void Connection::EmptyReceiveBuffer()
{
    read_buffer_pos = read_buffer_len = 0;
}

void Connection::FlushReceiveBuffer()
{
    STATUS result;
    DWORD bytes_read;
    do
    {
#ifdef _DEBUG
        for(int i = read_buffer_pos; i < read_buffer_len; ++i)
        {
	    // This is clearly a hack.  Technically, we know nothing about PSOH down here.
            if (read_buffer[i] == 2)
            {
                LOG_WARN("Throwing away a PSOH!");
            }
        }
#endif
#if !defined(ARCH_EMPEG)
        if (read_buffer_pos != read_buffer_len)
        {
            const int PER_LINE = 16;
            const unsigned char *p = read_buffer + read_buffer_pos;
            const unsigned char *end = read_buffer + read_buffer_len;

            LOG_INFO("Connection::FlushReceiveBuffer throwing away...");

            while (p < end)
            {
                char buffer[256];
                char *output = buffer;

                int i;
	        int count = end - p;
	        if (count > PER_LINE)
	            count = PER_LINE;
	        for(i = 0; i < count; i++)
	        {
                    output += sprintf(output, "%02x ", p[i]);
	        }
	        for(i = count; i <= PER_LINE; i++)
                {
                    strcat(output, "   ");
                    output += 3;
                }

	        for(i = 0; i < count; i++)
	        {
                    output += sprintf(output, "%c", isprint(p[i]) ? p[i] : '.');
	        }
                LOG_INFO("%p: %s", p, buffer);
	        p += PER_LINE;
            }
        }
#endif
	// Make it look like our buffer is empty.
	read_buffer_pos = read_buffer_len = 0;

	// Fill it up again if we can
	result = FillBuffer(read_buffer, READ_BUFFER_SIZE, 0 /* No timeout */, bytes_read);

    } while (SUCCEEDED(result) && bytes_read > 0);
}

int Connection::PushBack(int bytes)
{
    ASSERT(bytes >= 0);
    if (bytes > 0)
    {
	if (read_buffer_pos < bytes)
	    bytes = read_buffer_pos;

	read_buffer_pos -= bytes;
	TRACEC(TRACE_CONNECTION, "Pushed back %d bytes - new position is %d\n", bytes, read_buffer_pos);

	ASSERT_EX(read_buffer_pos >= 0, "read_buffer_pos=%d\n", read_buffer_pos);
	ASSERT_EX(read_buffer_pos <= read_buffer_len, "read_buffer_pos=%d, read_buffer_len=%d\n",
		  read_buffer_pos, read_buffer_len);
    }
    return bytes;
}

bool Connection::IsDataPending() const
{
    return (read_buffer_pos < read_buffer_len);
}

// Single-byte send
STATUS Connection::Send(unsigned char b)
{
    return Send(&b, 1);
}

// Reimplemented in OutTcpConnection
Connection *Connection::GetFastConnection()
{
    return NULL;
}
