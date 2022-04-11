/* read_fid_stream.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 */

#ifndef FID_STREAM_H
#define FID_STREAM_H 1

#include "stream.h"
#include "types.h"

class ProtocolClient;
 
/** Allows reading of a FID on the player. This is not safe in a multi-
 * threaded program; for that, see LockingFidStream in model/LockingFidStream.h
 */
class ReadFidStream : public SeekableStream
{
    ProtocolClient *m_protocolClient;
    FILEID m_fid;
    unsigned m_seekPos;
    unsigned m_maxSeek;

public:
    ReadFidStream(ProtocolClient *protocolClient, FILEID fid);

// Stream
public:
    virtual STATUS Read(void *buffer, unsigned bytesToRead, unsigned *pBytesRead);
    virtual STATUS Write(const void *buffer, unsigned bytesToWrite, unsigned *pBytesWritten);

// SeekableStream
public:
    virtual STATUS Tell(unsigned *position);
    virtual STATUS SeekAbsolute(unsigned absolutePosition);
    virtual STATUS SeekRelative(signed int relativePosition);
    virtual STATUS Length(unsigned *length);
};

#endif /* FID_STREAM_H */
