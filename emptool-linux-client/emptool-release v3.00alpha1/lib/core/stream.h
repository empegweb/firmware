/* stream.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.16 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#ifndef STREAM_H
#define STREAM_H 1

#ifndef EMPEG_STATUS_H
#include "empeg_status.h"
#endif

/*!
    Abstract base class for a whole bundle of different stream types.
 */
class Stream
{
public:
    virtual ~Stream() {}

    /*!
	Failures are returned with a failure HRESULT.  The content of the
	buffer is undefined.  The number of bytes returned is undefined.
	
	Returning S_OK with zero bytes is valid.  It does not mean EOF.
	It might mean that there's no data available yet.
	
	Returning S_FALSE denotes EOF.  There might be some data in the
	buffer.  Calling Read again will return S_FALSE again, and return
	zero bytes.

	If the number of bytes read is less than bytesToRead, the remainder
	of the buffer is undefined.

	pBytesRead is not allowed to be NULL.

	Attempting to read zero bytes is valid.  It'll return S_OK with zero bytes
	if the stream is not at EOF.  It'll return S_FALSE with zero bytes if the
	stream is at EOF.
     */
    virtual STATUS Read(void *buffer, unsigned bytesToRead,
			unsigned *pBytesRead) = 0;

    virtual STATUS Write(const void *buffer, unsigned bytesToWrite,
			 unsigned *pBytesWritten) = 0;

    /** Keeps on calling Read until it's all arrived or error or EOF. Returns
     * S_FALSE only if EOF before ANY bytes read (otherwise E_ENDOFFILE).
     */
    STATUS EnsureRead(void *buffer, unsigned bytesToRead);

    /** Keeps on calling Write until it's all gone or error. This is the same
     * as Write for disk files, but Writes are not Ensured on sockets or
     * devices.
     */
    STATUS EnsureWrite(const void *buffer, unsigned bytesToWrite);
};

/*!
    Analogous to /dev/null, except that reads are invalid, writes go nowhere.
 */
class NullStream : public Stream
{
public:
    virtual STATUS Read(void * /*buffer*/, unsigned /*bytesToRead*/, unsigned * /*pBytesRead*/);

    virtual STATUS Write(const void * /*buffer*/, unsigned bytesToWrite, unsigned * pBytesWritten);
};

class SeekableStream : public Stream
{
public:
    /** Returns the current seek position of the stream.
     */
    virtual STATUS Tell(unsigned *position) = 0;

    /** Set the stream pointer, relative to the start of the stream.
     * It can't be positioned before the start of the stream.
     * In some implementations, it can be positioned after the end of the stream.
     * Reading in this situation is an error.  Writing will cause
     * (where applicable) the stream to be extended.
     */
    virtual STATUS SeekAbsolute(unsigned absolutePosition) = 0;

    /** Set the stream pointer, relative to the current position.
     */
    virtual STATUS SeekRelative(signed int relativePosition) = 0;

    /** Return the length of the stream.  It's possible that the
     * underlying implementation doesn't know the length of the
     * stream. 
     */
    virtual STATUS Length(unsigned *length) = 0;

#if 0
    /** This leaves the read position of the stream intact. */
    virtual STATUS ReadOffset(unsigned /*absoluteOffset*/, void * /*pBuffer*/,
			      unsigned /*bytesToRead*/, unsigned * /*bytesRead*/)
	{ return E_NOTIMPL; }
#endif
};

#endif /* STREAM_H */
