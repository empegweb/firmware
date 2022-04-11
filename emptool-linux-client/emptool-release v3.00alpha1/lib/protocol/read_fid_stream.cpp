/* fid_stream.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 */

#include "config.h"
#include "trace.h"
#include "read_fid_stream.h"
#include "protocolclient.h"

ReadFidStream::ReadFidStream(ProtocolClient *protocolClient, FILEID fid)
    : m_protocolClient(protocolClient), m_fid(fid), m_seekPos(0), m_maxSeek(0)
{
    m_protocolClient->StatFid(fid, reinterpret_cast<int *>(&m_maxSeek));
}

STATUS ReadFidStream::Read(void *buffer, unsigned bytesToRead, unsigned *pBytesRead)
{
    int actual_size;

    unsigned char *p = reinterpret_cast<unsigned char *>(buffer);
    STATUS status = m_protocolClient->ReadFidPartial(m_fid, p,
						     m_seekPos, bytesToRead, &actual_size);
    if (FAILED(status))
	return status;
    
    *pBytesRead = actual_size;
    return S_OK;
}

STATUS ReadFidStream::Write(const void * /*buffer*/, unsigned /*bytesToWrite*/, unsigned * /*pBytesWritten*/)
{
    return E_NOTIMPL;
}

STATUS ReadFidStream::Tell(unsigned *position)
{
    *position = m_seekPos;

    return S_OK;
}

STATUS ReadFidStream::SeekAbsolute(unsigned absolutePosition)
{
    if (absolutePosition > m_maxSeek)
	return E_FAIL;
    
    m_seekPos = absolutePosition;
    return S_OK;
}

STATUS ReadFidStream::SeekRelative(signed int relativePosition)
{
    // TODO: This is a little dodgy if we seek past the end or beginning of the stream.
    // What are the correct semantics?
    m_seekPos += relativePosition;
    if ((int) m_seekPos < 0)
	m_seekPos = 0;
    else if (m_seekPos > m_maxSeek)
	m_seekPos = m_maxSeek;
    
    return S_OK;
}

STATUS ReadFidStream::Length(unsigned *length)
{
    *length = m_maxSeek;
    return S_OK;
}

#if defined(TEST)
int main(void)
{
    /// @todo Tests
    return 0;
}
#endif // TEST
