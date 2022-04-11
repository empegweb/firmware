/* connection_serial.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef CONNECTION_SERIAL_H
#define CONNECTION_SERIAL_H

#ifndef CONNECTION_H
#include "connection.h"
#endif

#ifndef WIN32	// Not required on Win32
#include <termios.h>
#include "core/serial.h"
#endif

class SerialConnection : public Connection
{
public:
    SerialConnection(const char *device, int baud_rate);
    virtual ~SerialConnection();

    STATUS OpenCooked();
    STATUS OpenAs(int new_fd);
    virtual STATUS Open();
    virtual void Close();
    virtual STATUS Send(const unsigned char *buffer, int length);

    // timeout > 0 block for a number of seconds
    // timeout == 0 wait for ever
    // timeout < 0 don't block at all
    //virtual int Receive(unsigned char *buffer, int length, int timeout);
    //virtual int Receive(int timeout);
    virtual int PacketSize();
    //virtual void FlushReceiveBuffer();
    void SetBaudRate(int rate);
    virtual bool IsSecure() const { return true; }

#ifdef WIN32
    virtual HANDLE GetPollHandle();
#else
    virtual int GetPollDescriptor();
    void Raw();
    void Cooked();
#endif

protected:
    virtual STATUS FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms,  DWORD & bytes_read);

private:
    std::string device;
    int baud_rate;
    bool raw;

#ifdef WIN32
    HANDLE hPort;
#ifdef USE_OVERLAPPED_SERIAL
    OVERLAPPED overlapped;
#endif
    enum { WRITE_TIMEOUT = 20000 };
#else
    // File descriptor of IO stream
    int fd;

    // Old settings
    struct termios oldtermio;

    // Lock on serial port
    PortLock pl;
#endif
};

#endif
