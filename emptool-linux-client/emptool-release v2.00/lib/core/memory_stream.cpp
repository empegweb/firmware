/* memory_stream.cpp
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */

#include "trace.h"
#include "config.h"
#include "memory_stream.h"
#include "allocator.h"

#define TRACE_MEMORYSTREAM 0

MemoryStream::MemoryStream(Allocator *allocator, int granularity)
    : m_allocator(allocator), m_owned(false),
      m_buffer(NULL), m_write_index(0),
      m_size(0), m_granularity(granularity)
{
    ASSERT_PTR(allocator);
}
    
MemoryStream::~MemoryStream()
{
    if (m_owned)
	m_allocator->Free(m_buffer);
}

STATUS MemoryStream::MakeSpace(int required_size)
{
    if (m_size + m_granularity > required_size)
	required_size = m_size + m_granularity;

    TRACEC(TRACE_MEMORYSTREAM, "Allocating space for %d bytes\n", required_size);
    m_buffer = reinterpret_cast<BYTE *>(m_allocator->Realloc(m_buffer, required_size));
    if (!m_buffer)
    {
	ERROR("MemoryStream failed to allocate memory.\n");
	return E_OUTOFMEMORY;
    }
    m_size = required_size;
    TRACEC(TRACE_MEMORYSTREAM, "Size is now %d bytes\n", m_size);
    return S_OK;
}

/** It's not actually Compacting anything, it's truncating it to the correct size.
 */
BYTE *MemoryStream::CompactAndExtractPointer()
{
    BYTE *new_buffer = reinterpret_cast<BYTE *>(m_allocator->Realloc(m_buffer, m_write_index));
    if (new_buffer)
	m_buffer = new_buffer;
    
    m_owned = false;
    return m_buffer;
}
