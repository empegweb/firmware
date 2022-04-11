/* memory_stream.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 */

#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H 1

#include "types.h"
#include "empeg_error.h"
#include <string>

class Allocator;

class MemoryStream
{
    Allocator *m_allocator;
    bool m_owned;
    BYTE *m_buffer;
    int m_write_index;
    int m_size;
    int m_granularity;

    STATUS MakeSpace(int required_size);
    
public:
    MemoryStream(Allocator *allocator, int granularity = 512);
    ~MemoryStream();

    STATUS Append(const BYTE *begin, const BYTE *end);
    STATUS Append(const BYTE *p, int n);
    STATUS Append(BYTE b, int count = 1);
    STATUS Append(const std::string &);

    BYTE *CompactAndExtractPointer();
    int GetSize() const;
};

inline STATUS MemoryStream::Append(const BYTE *b, const BYTE *e)
{
    return Append(b, e-b);
}

inline STATUS MemoryStream::Append(const std::string &s)
{
    return Append(reinterpret_cast<const BYTE *>(s.data()), s.length());
}

inline int MemoryStream::GetSize() const
{
    return m_write_index;
}

// These are run a lot but only in a few places in the code and it
// makes rebuilding a database a bit faster so it is worth making them
// inline.
inline STATUS MemoryStream::Append(const BYTE *begin, int count)
{
    ASSERT(m_write_index <= m_size);
    if (m_write_index + count > m_size)
    {
	STATUS status = MakeSpace(m_write_index + count);
	if (FAILED(status))
	    return status;
    }
    
    memcpy(m_buffer + m_write_index, begin, count);
    m_write_index += count;
    return S_OK;
}

inline STATUS MemoryStream::Append(BYTE b, int count)
{
    if (m_write_index + count > m_size)
    {
	STATUS status = MakeSpace(m_write_index + count);
	if (FAILED(status))
	    return status;
    }
    while (count--)
	m_buffer[m_write_index++] = b;
    
    ASSERT(m_write_index <= m_size);
    
    return S_OK;
}


#endif
