/* http_stream.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.8 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H 1

#include "stream.h"

class HttpURLParser;

/*!
    Implemented on top of another stream, this removes the headers,
    unchunks chunked encoding, and (in the future) deals with
    different Transfer-Encoding types (e.g. gzip).

    You can construct it by passing a URL, or an already open stream.
    The already open stream needs to be at the start of an HTTP request
    or response.

    Clients construct one of these for each entity in a conversation.
    You Write() into it, and then Read() the response.

    If the server closes the connection, we have to reopen a socket
    on construction.

    If the server doesn't close the connection, we use a pool of kept-alive
    connections to send the request.  At least, we will do.  At some point.

    If the client is handling the socket (via the Create(Stream) variant),
    they can find out if we're at EOF because the stream is closed, or
    just because the entity is finished, by issuing a zero byte read on
    their stream.  If it returns S_FALSE, it's EOF, if it returns S_OK,
    it's not EOF.
 */
class ClientHttpStream : public Stream
{
    Stream *m_stm;			/// Where the data is coming from.
    
    // TODO: Separate these into distinct classes - one for chunked, one for other.
    bool m_isChunkedTransferEncoding;	/// Are we using chunked encoding?
    unsigned m_chunkSize;		/// What's left of the next chunk?

    bool m_isContentLength;		/// We're using Content-Length.
    unsigned m_contentLength;		/// Value of Content-Length.

    /** @todo Make the Content-Type header accessible so that this isn't
     * needed any more -- it's not an attribute of the stream.
     */
    bool m_isPlainText;                 // URL result is plain text?
    
    unsigned m_HttpResult;              // HTTP protocol result code

    bool m_streamIsMine;                // Should I delete m_stm in destructor?
    
    ClientHttpStream(Stream *stream, bool take_ownership = false);

public:
    static STATUS Create(const std::string &url, const std::string & extra_headers, ClientHttpStream **pp);
    static STATUS Create(const std::string &url, const std::string & extra_headers, Stream *stream, ClientHttpStream **pp);
    static STATUS Create(const HttpURLParser &url,
			 const std::string & extra_headers, Stream *stream,
			 ClientHttpStream **pp, bool take_ownership = false);

    static STATUS Create(const std::string &url, ClientHttpStream **pp)
    {
	return Create(url, std::string(), pp);
    }
    static STATUS Create(const std::string &url, Stream *stream, ClientHttpStream **pp)
    {
	return Create(url, std::string(), stream, pp);
    }
    static STATUS Create(const HttpURLParser &url, Stream *stream, ClientHttpStream **pp)
    {
	return Create(url, std::string(), stream, pp);
    }

    bool IsPlainText() const { return m_isPlainText; }
    bool HaveContentLength() const { return m_isContentLength; }
    unsigned ContentLength() const { return m_contentLength; }
    unsigned HttpResult() const { return m_HttpResult; }

    ~ClientHttpStream() { if (m_streamIsMine) delete m_stm; }
    
// Stream
public:
    virtual STATUS Read(void *buffer, unsigned bytesToRead, unsigned *pBytesRead);
    virtual STATUS Write(const void *buffer, unsigned bytesToWrite, unsigned *pBytesWritten);

private:
    STATUS SendRequest(const HttpURLParser &url, const std::string &extra_headers);
    STATUS ParseResponseHeader();
    std::string FormatRequestEntity(const HttpURLParser &url) const;

    bool IsChunkedTransferEncoding() const;
    STATUS ReadChunked(void *buffer, unsigned bytesToRead, unsigned *pBytesRead);
    STATUS DiscardCRLF();

    STATUS ReadUnchunked(void *buffer, unsigned bytesToRead, unsigned *pBytesRead);    
};

void TestReadClientHttpStream();
void TestReadChunkedClientHttpStream();
void TestReadSingleContentLengthClientHttpStream();
void TestReadMultipleContentLengthClientHttpStream();

#endif /* HTTP_STREAM_H */
