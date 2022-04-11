/* stream_in_memory.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef STREAM_IN_MEMORY_H
#define STREAM_IN_MEMORY_H		1

#include <string>

#include "stream.h"

class StreamInMemory : public SeekableStream
{
    std::string m_buffer;
    unsigned m_pos;
    
public:
    StreamInMemory();

    virtual STATUS Read(void *buffer, unsigned bytesToRead,
			unsigned *pBytesRead);
    virtual STATUS Write(const void *buffer, unsigned bytesToWrite,
			 unsigned *pBytesWritten);
    virtual STATUS Tell(unsigned *position);
    virtual STATUS SeekAbsolute(unsigned absolutePosition);
    virtual STATUS SeekRelative(signed int relativePosition);
    virtual STATUS Length(unsigned *length); 
    
    STATUS Resize(unsigned len);
    inline STATUS Clear(unsigned start, unsigned len) {
	return Set(start, len, 0);
    }
    // automatically grows if required
    STATUS Set(unsigned start, unsigned len, unsigned char pattern);
    //inline char *GetBuffer() { return m_buffer.data(); }
    inline const char *GetBuffer() const { return m_buffer.data(); }
    inline unsigned GetLength() const { return m_buffer.size(); }
    void RemoveFromFront(unsigned bytes);
};

#endif
