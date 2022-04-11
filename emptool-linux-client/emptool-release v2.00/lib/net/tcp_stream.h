/* tcp_stream.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#ifndef TCP_STREAM_H
#define TCP_STREAM_H 1

#include "empeg_error.h"
#include <string>
#include "stream.h"
#include "net/socket.h"

class IPAddress;

class TcpStream : public Stream
{
    StreamSocket m_sock;

    TcpStream();
    explicit TcpStream(StreamSocket &soc);

public:
    /*!
	host can be a name, or dotted-IP.
	port is in host order.
     */
    static STATUS Create(const std::string &host, short port, TcpStream **pp);
#if defined(WIN32)
    static STATUS Create(const std::string &host, const std::string &service, TcpStream **pp);
#endif

    /*!
	Pass it an already-connected socket.
     */
    static STATUS Create(StreamSocket &soc, TcpStream **pp);

public:
    virtual STATUS Read(void *buffer, unsigned bytesToRead, unsigned *pBytesRead);
    virtual STATUS Write(const void *buffer, unsigned bytesToWrite, unsigned *pBytesWritten);

private:
#if defined(WIN32)
    static STATUS ResolveService(const std::string &service, short *portNum);
#endif

    STATUS Connect(const IPEndPoint &ep);
};

void TestTcpEcho();

#endif /* TCP_STREAM_H */
