/* socket.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.30 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#ifndef INCLUDED_SOCKET_H
#define INCLUDED_SOCKET_H

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib")
#endif

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

#ifdef WIN32

class WinSockInitialise
{
public:
    WinSockInitialise(WORD requestedVersion)
	: m_result(WSAStartup(requestedVersion, &m_data))
    {
    }

    ~WinSockInitialise()
    {
	WSACleanup();
    }

    operator int() { return m_result; }

private:
    int	    m_result;
    WSADATA m_data;
};

#else /* WIN32 */
    #include "types.h"
    #include "empeg_error.h"
    typedef STATUS HRESULT;
    typedef int SOCKET;
#endif

#include "ipaddress.h"

class Socket
{
protected:
    SOCKET m_soc;

protected:
    Socket(SOCKET soc);

public:
    Socket();

    STATUS Create(int af, int type, int protocol);
    STATUS Bind(const IPEndPoint &endpoint);
    void Shutdown( int how ); // SD_*, usually SD_BOTH
    void Close();

    bool IsOpen() const;

    int Select(int timeoutMS) const;
    int EnableBroadcast(bool b);
    int SetNonBlocking(bool b);
    STATUS BindToDevice(const char *ifname);
    STATUS SetReuseAddr(bool);
    IPEndPoint GetLocalEndPoint() const;

#ifndef WIN32
    STATUS GetBroadcastAddress(IPAddress *brdaddr);
#endif
    
#ifdef WIN32
    int AsyncSelect(HWND hwnd, UINT message, LONG networkEvents);
    int EventSelect(HANDLE hEvent, LONG networkEvents);
    int WSAIoctl(DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer,
		    LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned,
		    LPWSAOVERLAPPED lpOverlapped = NULL, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine = NULL) const;

    // You didn't see this:
    operator SOCKET() { return m_soc; }
#endif
    // You didn't see this either:
    int GetFD() const { return m_soc; }
};
    
class StreamSocket : public Socket
{
    StreamSocket(SOCKET soc)
	: Socket(soc)
    {
    }
    
public:
    StreamSocket() { /* nothing */ }
    StreamSocket(const StreamSocket & other)
	: Socket(other)
    {
    }

    STATUS Create();
    STATUS Create(const IPEndPoint &localEndpoint);
    STATUS Connect(const IPEndPoint &remoteEndpoint);

    STATUS Listen(int backlog = SOMAXCONN) const;

#if 0
    int Receive(char *buffer, int count) const;
    int Send(const char *buffer, int count) const;
#endif

    void SetNoDelay(bool nodelay=true) const;

    STATUS Send(const void *buffer, size_t len, size_t *pSent);  // blocks until all gone or error
    STATUS Receive(void *buffer, size_t len, size_t *pReceived); // returns as soon as any read

    STATUS SetReuseAddr(bool reuseAddr = true);
    StreamSocket Accept(bool *pGotOne = NULL) const;
};

class DatagramSocket : public Socket
{
    DatagramSocket(const DatagramSocket & other);
    const DatagramSocket & operator=(const DatagramSocket & other);

    STATUS SendTo2( const void*, size_t, const IPEndPoint& ) const;

public:
    DatagramSocket() { /* nothing */ }

    STATUS Create();
    STATUS Create(const IPEndPoint &endpoint);

    // Don't try and use these for broadcasting; use the specific SendBroadcast
    // calls instead
    STATUS SendTo(const void *msg, int count, const IPEndPoint & dest) const
	{ return SendTo2( msg, count, dest ); }
    STATUS SendTo(const std::string & s, const IPEndPoint & dest) const
	{ return SendTo2( s.data(), s.length(), dest ); }

    STATUS SendBroadcast( const std::string& s, short portAsHost ) const
	{ return SendBroadcast( s.data(), s.length(), portAsHost ); }
    STATUS SendBroadcast( const void*, size_t count, short portAsHost ) const;

    STATUS ReceiveFrom(void *msg, int bufsize, int *bytesRead, IPEndPoint *endpoint) const;
    STATUS ReceiveFrom(std::string *str, int max_size, IPEndPoint *source) const;
};

#endif /* INCLUDED_SOCKET_H_ */
