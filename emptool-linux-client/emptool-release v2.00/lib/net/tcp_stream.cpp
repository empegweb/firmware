/* tcp_stream.cpp
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "tcp_stream.h"
#include "net/ipaddress.h"

TcpStream::TcpStream()
{
}

TcpStream::TcpStream(StreamSocket &soc)
    : m_sock(soc)
{
}

/* static */
STATUS TcpStream::Create(const std::string &host, short port, TcpStream **pp)
{
    TRACE("TcpStream::Create(host = '%s', port = %d, pp)\n", host.c_str(), port);

    ASSERT(pp);
    *pp = NULL;

    STATUS status;

    IPAddress addr;
    if (FAILED(status = addr.FromHostName(host)))
    {
	TRACE("FromHostName failed, status = 0x%x\n", PrintableStatus(status));
	return status;
    }

    IPEndPoint ep(addr, port);
    
    TcpStream *tcp = NEW TcpStream;
    if (!tcp)
	return E_OUTOFMEMORY;

    if (FAILED(status = tcp->Connect(ep)))
    {
	TRACE("tcp->Connect failed, status = 0x%x\n", PrintableStatus(status));
	delete tcp;
	return status;
    }

    *pp = tcp;

    return S_OK;
}

#if defined(WIN32)
/* static */
STATUS TcpStream::Create(const std::string &host, const std::string &service,
			 TcpStream **pp)
{
    STATUS status;

    short portNum;
    if (FAILED(status = ResolveService(service, &portNum)))
	return status;

    return Create(host, portNum, pp);
}
#endif

/* static */
STATUS TcpStream::Create(StreamSocket &soc, TcpStream **pp)
{
    *pp = NULL;

    TcpStream *tcp = NEW TcpStream(soc);
    if (!tcp)
	return E_OUTOFMEMORY;

    *pp = tcp;
    return S_OK;
}

STATUS TcpStream::Connect(const IPEndPoint &ep)
{
    TRACE("TcpStream::Connect(ep = %s:%d)\n", ep.GetIPAddress().ToString().c_str(),
	  ep.GetPortAsHost());
    STATUS status;
    
    if (FAILED(status = m_sock.Create()))
    {
	TRACE("Socket::Create failed, status = 0x%x\n", PrintableStatus(status));
	return status;
    }

    if (FAILED(status = m_sock.Connect(ep)))
    {
	TRACE("Socket::Connect failed, status = 0x%x\n", PrintableStatus(status));
	return status;
    }
    
    return S_OK;
}

STATUS TcpStream::Read(void *buffer, unsigned bytesToRead, unsigned *pBytesRead)
{
    return m_sock.Receive(buffer, bytesToRead, pBytesRead);
}

STATUS TcpStream::Write(const void *buffer, unsigned bytesToWrite, unsigned *pBytesWritten)
{
    return m_sock.Send(buffer, bytesToWrite, pBytesWritten);
}

#if defined(WIN32)
STATUS TcpStream::ResolveService(const std::string &service, short *portNum)
{
    ASSERT(portNum);

    SERVENT *s = getservbyname(service.c_str(), "tcp");
    if (!s)
	return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, WSAGetLastError());

    *portNum = ntohs(s->s_port);

    return S_OK;
}
#endif

#if 0
void TestTcpEcho()
{
    TcpStream *tcp;
    STATUS status = TcpStream::Create("localhost",
#if defined(WIN32)
				      "echo",
#else
				      7,
#endif
				      &tcp);
    ASSERT(SUCCEEDED(status));

    std::string message("Hello World!\n");
    unsigned w;
    status = tcp->Write(message.c_str(), message.size(), &w);
    ASSERT(SUCCEEDED(status));
    ASSERT(w == message.size());

    char buffer[1024];
    unsigned r;
    status = tcp->Read(buffer, sizeof(buffer), &r);
    ASSERT(SUCCEEDED(status));
    buffer[r] = 0;

    ASSERT(message == buffer);
}
#endif
