/* stream_in_memory.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "stream_in_memory.h"

#if !defined(WIN32)
#include <unistd.h>
#endif

#include "empeg_error.h"

#define TRACE_STREAM		0

#define ARBITRARY_LIMITATION	(128*1024*1024)

StreamInMemory::StreamInMemory()
    : m_pos(0)
{
}

STATUS StreamInMemory::Read(void *buffer, unsigned bytesToRead,
			    unsigned *pBytesRead)
{
    if(m_pos >= m_buffer.size()) {
	*pBytesRead = 0;
	return S_FALSE;
    }
    
    if(m_pos + bytesToRead > m_buffer.size())
	bytesToRead = m_buffer.size() - m_pos;
    
    memcpy(buffer, (void*)(m_buffer.data() + m_pos), bytesToRead);
    *pBytesRead = bytesToRead;
    m_pos += bytesToRead;
    return S_OK;
}

STATUS StreamInMemory::Write(const void *buffer, unsigned bytesToWrite,
			     unsigned *pBytesWritten)
{
    if(m_pos + bytesToWrite > m_buffer.size())
	Resize(m_pos + bytesToWrite);

    // allows zero length writes to expand the stream length
    if(bytesToWrite > 0)
    {
	// We've done the resize ourselves so we can cheat here
	memcpy((char*)(m_buffer.data() + m_pos), buffer, bytesToWrite);
    }
    
    *pBytesWritten = bytesToWrite;
    m_pos += bytesToWrite;
    return S_OK;
}

STATUS StreamInMemory::Tell(unsigned *position)
{
    *position = m_pos;
    return S_OK;
}

STATUS StreamInMemory::SeekAbsolute(unsigned absolutePosition)
{
    // absolutely anywhere is valid
    m_pos = absolutePosition;
    return S_OK;
}

STATUS StreamInMemory::SeekRelative(signed int relativePosition)
{
    m_pos += relativePosition;
    if(int(m_pos) < 0)
	m_pos = 0;
    return S_OK;
}

STATUS StreamInMemory::Length(unsigned *length)
{
    *length = m_buffer.size();
    return S_OK;
}

STATUS StreamInMemory::Resize(unsigned len)
{
    TRACEC(TRACE_STREAM, "Resize: %u\n", len);
    if(len > ARBITRARY_LIMITATION) {
	TRACE_WARN("Resize rather large, methinks (%u bytes)\n", len);
	ASSERT(false);
    }
    m_buffer.resize(len);
    return S_OK;
}

STATUS StreamInMemory::Set(unsigned start, unsigned len, unsigned char pattern)
{
    STATUS status;
    if(start + len >= m_buffer.size()) {
	if(FAILED(status = Resize(start + len)))
	    return status;
    }
    // Again, we've done the resize ourselves so we can cheat here
    memset((char*)(m_buffer.data() + start), pattern, len);
    return S_OK;
}

void StreamInMemory::RemoveFromFront(unsigned bytes)
{
    // Can't remove more than the buffer's actual size
    if(bytes > m_buffer.size())
	bytes = m_buffer.size();
    
    // Rather than doing a hideously inefficient copy, let's move it.
    // Use memmove because these are overlapping regions.
    // 0 <= bytes <= m_buffer.size(), therefore:
    // destination is valid, provided m_buffer.size() > 0,
    //   if m_buffer.size() == 0, then bytes == 0 anyway.
    // source is valid, provided m_buffer.size() >= bytes, which holds true.
    // amount is valid, provided m_buffer.size() >= bytes, which holds true.
    memmove((char *) m_buffer.data(),	// dest (start)
	    m_buffer.data() + bytes,	// from (bytes)
	    m_buffer.size() - bytes);	// amount (from bytes to end)

    // Now manually resize the buffer.
    // m_buffer.size() >= bytes, so this is always 0 or greater.
    m_buffer.resize(m_buffer.size() - bytes);

    // 0 <= m_pos <= original_buffer_size
    // If m_pos was inside the bit we removed, set it to start of stream.
    if(m_pos < bytes)
	m_pos = 0;
    else
    {
	// Otherwise move m_pos back by the amount we removed under its feet.
	// Because m_pos >= bytes, m_pos remains 0 or positive.
	m_pos -= bytes;
    }
    // You are the weakest precondition. Goodbye.
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
