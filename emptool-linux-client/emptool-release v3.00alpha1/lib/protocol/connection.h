/* connection.h
 *
 * Abstraction of all the different ways a player could be connected
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.52 13-Mar-2003 18:15 rob:)
 */

#ifndef _CONNECTION_H
#define _CONNECTION_H

#ifndef CONFIG_H
#include "core/config.h"   /* for DWORD */
#endif

#include <string>

//#define REUSE_PENDING_READS

#define TRACE_CONNECTION 0

// Maximum payload sizes for serial and USB - USB is larger for more efficiency, serial is
// smaller for lower latency.
#define SERIAL_MAXPAYLOAD	4096
#define USB_MAXPAYLOAD		16384
#define TCP_MAXPAYLOAD		16384

// Maximum of all above
#define CONNECTION_MAXPAYLOAD	16384

#if !defined(WIN32)

#ifdef ECOS
#include "ecos_poll.h"
#else
#include <sys/poll.h>
#endif

#endif
#include <string>

#ifdef WIN32
#include <winsock2.h>
// We need to define this so that we can be compatible with the packets code
// that assumes UNIX style error codes.
#define ETIMEDOUT (110)

// Errors are passed back by this code but to maintain compatibility
// with the UNIX side we always use ints - Unfortunately, much code
// makes the assumption that positive values and zero are success and
// negative values are failure. Unfortunately, Windows error codes can
// be both positive and negative (when viewed as an int) so we can't
// really do this easily.

// The best way to get round the problem would probably be to define a
// platform independent type EMPEG_ERROR which can be int on UNIX and
// HRESULT on Windows. Then, define macros called SUCCEEDED, FAILED
// etc. like those used in COM to determine whether things were
// successful. Small integer Win32 error codes can be converted to
// HRESULTs using FACILITY_WIN32.

// We can then pass back small values in this type to indicate lengths
// reasonably safely - a bit like the S_TRUE and S_FALSE values in
// COM.

// We could even then write functions to convert EMPEG_ERROR variables
// to printable strings.

// Cross-platform code that needs to determine if errors are of a
// specific type could call macros like IS_TIMEOUT_ERROR(x) which
// would be better than error == TIMEOUT_ERROR because I'm sure there
// are about seventeen and a half thousand different timeout errors on
// Windows :-)

#endif

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

// IMPORTANT
// FlushReceiveBuffer should only be called _before_ sending a
// request.  Unfortunately on Windows 98 it is not possible to do a
// read with zero timeout to just empty stuff in the buffers etc so a
// read with a small timeout must be used. If you are expecting valid
// data then don't call FlushReceiveBuffers!

// Connection class which provides services to packet tx/rx
class Connection
{
    int debug_level;
    unsigned char *read_buffer;
    int read_buffer_len;
    int read_buffer_pos;

    static const int READ_BUFFER_SIZE;

public:
    Connection();
    virtual ~Connection() = 0;
    virtual STATUS Open() = 0;
    virtual void Close() = 0;
    STATUS Send(unsigned char b);

    /** These send calls always send all of it; they block until either it's
     * all gone or there's an error.
     */
    virtual STATUS Send(const unsigned char *buffer, int length) = 0;

    /** timeout > 0 block for a number of seconds
     * timeout == 0 wait for ever
     * timeout < 0 don't block at all
     */
    STATUS Receive(unsigned char *buffer, int length, int timeout_ms, DWORD & bytes_read);
    STATUS Receive(int timeout_ms, int & received);
    int PushBack(int bytes);

    /** Pause while the unit is restarted (most can ignore this, TCP needs to close and
     * reopen the socket)
     */
    virtual void Pause() {}
    virtual STATUS Unpause() { return S_OK; }

    /** What's the maximum packet size supported by this connection? */
    virtual int PacketSize() = 0;
    virtual void FlushReceiveBuffer();

    inline void SetDebugLevel(int level) {
	debug_level = level;
    }
    inline int GetDebugLevel() {
	return debug_level;
    }

#ifdef WIN32
    virtual HANDLE GetPollHandle() = 0;
#else
    virtual int GetPollDescriptor() = 0;
#endif
    bool IsDataPending() const;

    /** Returns a "fast protocol" cousin of the current connection, or NULL
     * if the current connection has no fast protocol cousin. This is a bit
     * icky, as it means tcpconnection must know about the port, but it means
     * existing users of ProtocolClient get fast connections for free.
     */
    virtual Connection *GetFastConnection();

    /** Returns true if the connection can be considered to be secure, for
     * the purposes of setting a password over it.
     */
    virtual bool IsSecure() const { return false; }

protected:

    /** This function is called to fill up our buffer.
     */
    virtual STATUS FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read) = 0;
    void EmptyReceiveBuffer();
};

class CritSect;

/******************************************************************************
 * Need eCos implementations.
 * Actually these really need splitting into separate headers please.
 */

class UsbConnection : public Connection
{
#ifdef WIN32
    static CritSect usb_cs;
    void ResetIfRequired(DWORD error);
    void ResetPipe();
    void CheckStallState(const char *when);
    bool IsResetRequired(DWORD error);
    void ResetReadEvent();
    void ResetWriteEvent();

    // Required so that we can call through the function
    // pointer.
    BOOL CancelIo();
#endif

public:
    UsbConnection(const char *device);
    virtual ~UsbConnection();
    virtual STATUS Open();
    virtual void Close();
    virtual STATUS Send(const unsigned char *buffer, int length);
    // timeout > 0 block for a number of seconds
    // timeout == 0 wait for ever
    // timeout < 0 don't block at all
    virtual int PacketSize();
    virtual bool IsSecure() const { return true; }

#ifdef WIN32
    virtual HANDLE GetPollHandle();
#else
    virtual int GetPollDescriptor();
#endif

protected:
    virtual STATUS FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read);

private:
    std::string device;

#ifdef WIN32
    HANDLE hPort;
    OVERLAPPED &overlapped_read;
    OVERLAPPED &overlapped_write;
    OVERLAPPED single_overlapped;
#ifdef REUSE_PENDING_READS
    bool read_pending;
#endif

    enum { WRITE_TIMEOUT = 20000 };

    // This is necessary since CancelIo isn't available on Windows 95 :-(
    typedef BOOL (WINAPI *FN_CANCELIO)(HANDLE);
    static BOOL bCancelIoChecked;
    static FN_CANCELIO fnCancelIo;
    static void CheckCancelIo();
#else
    // File descriptor of IO stream
    int fd;
#endif
};

class OutSocketConnection : public Connection
{
protected:
    OutSocketConnection();
    virtual ~OutSocketConnection();

public:
    virtual STATUS Open();
    virtual STATUS Send(const unsigned char *buffer, int length);
    // timeout > 0 block for a number of seconds
    // timeout == 0 wait for ever
    // timeout < 0 don't block at all
    virtual void Close();

#ifdef WIN32
    virtual HANDLE GetPollHandle();
#else
    virtual int GetPollDescriptor();
#endif

protected:
    virtual STATUS FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read);
    virtual STATUS CreateAndBind() = 0;

#ifdef WIN32
    SOCKET sock;
    enum { WRITE_TIMEOUT = 20000 };
#else
    // File descriptor of IO stream
    int fd;
#endif
};

class OutTcpConnection : public OutSocketConnection
{
public:
    OutTcpConnection(const char *host, int port);
    ~OutTcpConnection();
    virtual int PacketSize();

    virtual void Pause();
    virtual STATUS Unpause();

protected:
    virtual STATUS CreateAndBind();
    virtual Connection *GetFastConnection();

private:
    std::string host;
    int port;
    Connection *fast_connection;
};

class InSocketConnection : public Connection
{
protected:
    InSocketConnection();
    virtual ~InSocketConnection();
public:
    virtual void Close();
    virtual STATUS Open();
    virtual STATUS Send(const unsigned char *buffer, int length);
    // timeout > 0 block for a number of seconds
    // timeout == 0 wait for ever
    // timeout < 0 don't block at all
#ifdef WIN32
    virtual HANDLE GetPollHandle();
#else
    virtual int GetPollDescriptor();
#endif
    virtual int OpenPipeIfNecessary(int timeout_ms);
    int ClosePipe(); // Only need this when using the fd directly (instead of Send and FillBuffer)

protected:
    virtual STATUS CreateAndBind() = 0;
    virtual int Accept() = 0;

protected:
    virtual STATUS FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read);

#ifdef WIN32
    HANDLE hPort;
    OVERLAPPED overlapped;
    enum { WRITE_TIMEOUT = 20000 };
#else
    // File descriptor of IO streams
    int fd_accept;
    int fd_pipe;
#endif
};

class InTcpConnection : public InSocketConnection
{
public:
    InTcpConnection(int port);
    virtual int PacketSize();

protected:
    virtual STATUS CreateAndBind();
    virtual int Accept();

private:
    int port;
};

#if defined(__UNIX__)
class InUnixConnection : public Connection
{
public:
    FifoConnection(const char *file);
    virtual ~FifoConnection();
    virtual STATUS Open();
    virtual void Close();
    virtual int Send(const unsigned char *buffer, int length);

private:
    int fd;
};

class OutUnixConnection : public Connection
{
public:

private:
};
#endif

/// The Poll class can be used to register a number of connections
// using Append and wait for activity on them. Wait will return the
// same as the return value of poll(2) and then GetNextActive will
// iterate through the connections returning in turn each that has
// events pending matching the mask passed to Append.  It's only a
// thin wrapper around poll so the poll(2) manpage should help make
// sense of it.
#ifndef WIN32
class Poll
{
    Connection **connections;
    struct pollfd *pfds;
    int size; // number of connections registered.
    int progress; // progress through connections after wait.

public:
    Poll(int capacity);
    ~Poll();
    void Append(Connection *p, short events = POLLIN);
    int Wait(int timeout);
    Connection *GetNextActive();
};
#endif // not WIN32

#endif
