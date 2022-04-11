/* connection_unix.cpp
 *
 * Unix-specific code in support of connecting to players over usb/serial
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.45 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"

#include "connection.h"
#include "connection_serial.h"
#include "time_calc.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#include <signal.h>

#include "protocol_errors.h"

#ifdef ECOS
#include "ecos_poll.h"
#endif

// extern "C" int cfmakeraw (struct termios *termios_p);

static speed_t GetBaudRateConstant(int numeric_rate)
{
    switch(numeric_rate)
    {
#define BR(X) case X: return B ## X
	BR(0);
	BR(50);
	BR(75);
	BR(110);
	BR(134);
	BR(150);
	BR(200);
	BR(300);
	BR(600);
	BR(1200);
	BR(1800);
	BR(2400);
	BR(4800);
	BR(9600);
	BR(19200);
	BR(38400);
	BR(57600);
	BR(115200);
	BR(230400);
	BR(460800);
#ifdef B500000
	BR(500000);
	BR(576000);
	BR(921600);
	BR(1000000);
	BR(1152000);
	BR(1500000);
	BR(2000000);
#ifdef B2500000
	BR(2500000);
	BR(3000000);
	BR(3500000);
	BR(4000000);
#endif
#endif
#undef BR
    default:
	return B115200;
    }
}

SerialConnection::SerialConnection(const char *d, int r)
    : device(d), baud_rate(r), raw(false), pl(d)
{
    // Initialise fd
    fd=-1;
}

void SerialConnection::SetBaudRate(int rate)
{
    // Can only be done when the port isn't open.
    ASSERT(fd < 0);
    baud_rate = rate;
}

int SerialConnection::PacketSize()
{
    // Return maximum serial packet payload
    return SERIAL_MAXPAYLOAD;
}

void SerialConnection::Raw()
{
    if (!raw)
    {
	// Set speed
	struct termios t;
	tcgetattr(fd,&t);

	int change_flow = 0;
	if(t.c_iflag & IXON) {
	    change_flow = 1;
	    if(GetDebugLevel()>0)
		fprintf(stderr, "Flow control problem: XON is turned on\n");
	}
	if(t.c_iflag & IXOFF) {
	    change_flow = 1;
	    if(GetDebugLevel()>0)
		fprintf(stderr, "Flow control problem: XOFF is turned on\n");
	}
	if(t.c_cflag & CRTSCTS) {
	    change_flow = 1;
	    if(GetDebugLevel()>0)
		fprintf(stderr, "Flow control problem: CRTSCTS is turned on\n");
	}

	if(change_flow) {
	    t.c_iflag &= ~(IXON | IXOFF);
	    t.c_cflag &= ~CRTSCTS;
	    if(GetDebugLevel()>0)
		fprintf(stderr, "Flow control turned off\n");
	}

	// Save old settings
	oldtermio=t;

#ifndef ECOS
	cfmakeraw(&t);
#else
	// cfmakeraw would do this...
	t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	t.c_oflag &= ~OPOST;
	t.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	t.c_cflag &= ~(CSIZE|PARENB);
	t.c_cflag |= CS8;
#endif

	cfsetispeed(&t,GetBaudRateConstant(baud_rate));
	cfsetospeed(&t,GetBaudRateConstant(baud_rate));
	// make sure we don't screw up the last bits of in/out
	usleep(5000);
	tcsetattr(fd,TCSAFLUSH,&t);

	raw = true;
    }
}

void SerialConnection::Cooked()
{
    if (raw)
    {
	// dont stomp over last in/out
	tcsetattr(fd,TCSAFLUSH,&oldtermio);
	raw = false;
    }
}

STATUS SerialConnection::Open()
{
    // Close device if it's open already and not STDx
    if (fd >= 3)
    {
	close(fd);
	fd = -1;
    }

    if (!SUCCEEDED(pl.Lock()))
	return SERIAL_E_BUSY;

    // Try to open device
    if ((fd = open(device.c_str(),O_RDWR))<0)
	return SERIAL_E_OPEN;

    // In case we're talking to the empeg make it switch to protocol mode (^Y)
    //Send((unsigned char *)ENABLE_PROTOCOL_SEQUENCE, sizeof(ENABLE_PROTOCOL_SEQUENCE));
    Raw();
    FlushReceiveBuffer();
    return S_OK;
}

STATUS SerialConnection::OpenCooked()
{
    // Close device if it's open already and not STDx
    if (fd >= 3)
    {
	close(fd);
	fd = -1;
    }

    // Try to open device
    if ((fd = open(device.c_str(),O_RDWR)) < 0)
        return SERIAL_E_OPEN;

    // Set speed
    struct termios t;
    tcgetattr(fd,&t);
    cfsetispeed(&t,GetBaudRateConstant(baud_rate));
    cfsetospeed(&t,GetBaudRateConstant(baud_rate));
    tcsetattr(fd,TCSAFLUSH,&t);

    return S_OK;
}

STATUS SerialConnection::OpenAs(int new_fd)
{
    // Close device if it's open already and not STDx
    if (fd >= 3)
    {
	close(fd);
	fd = -1;
    }
    
    ASSERT( new_fd >= 0 ); // no broken fds

    fd = new_fd;

    return S_OK;
}

void SerialConnection::Close()
{
    // Not STDx streams
    if (fd>=3) {
	// Restore old settings
	pl.Unlock();
	Cooked();
	usleep(5000);
	close(fd);
	fd = -1;
    }
}

SerialConnection::~SerialConnection()
{
    Close();
}

#if 0
// Single-byte send
int SerialConnection::Send(unsigned char b)
{
    unsigned char c=b;

    if (write(fd,&c,1)<=0)
	return -1;

    return 0;
}
#endif

/** Send a block of data
 *
 * Unlike unix calls, this write will only return when either the entire buffer
 * has been sent or an error occurrs. A partial write will cause a retry.
 */
STATUS SerialConnection::Send(const unsigned char *buffer, int length)
{
    // Write entire data packet: retry as necessary. We only return
    // when the entire buffer has been sent or there has been an error
    int done = 0, r;

    while(done < length)
    {
	if ((r = write(fd, buffer + done, length - done)) <= 0)
	    return MakeErrnoStatus();
	done += r;
    }

    // Done OK
    return S_OK;
}

/** Get data from remote end with timeout
 *
 * if timeout>0 , it indicates a timeout in milliseconds to wait for data
 * if timeout==0, it indicates that we should read only if we can read
 *                without delay (non-blocking read, basically)
 * if timeout<0 , it indicates that we should block until we can read
 *
 * Note that in accordance with unixisms, the read will return as many bytes
 * as are available, up to the buffer size. If we reach the read() call,
 * we will get at least one byte.
 *
 * Returns -errno for an error, otherwise we return the number of bytes read.
 */
STATUS SerialConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    bytes_read = 0;

    int n;
    if (timeout_ms >= 0)
    {
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	n = poll(&pfd, 1, timeout_ms);
	if (n < 0)
	    return MakeErrnoStatus();
	if (!n)
	    return CONN_E_TIMEDOUT;
    }

    // Get some stuff: will block until at least one byte comes in
    n = read(fd, buffer, buffer_size);
    if (n < 0)
	return MakeErrnoStatus();
#if 0
    else if (n)
    {
	printf("---Filled %d bytes\n", n);
	for(int i = 0; i < n; i++)
	{
	    if (isprint(buffer[i]))
		printf("%c", buffer[i]);
	    else
		printf("\\x%02x", buffer[i]);
	}
	printf("\n---end.\n");
    }
#endif

    bytes_read = n;

    return S_OK;
}

int SerialConnection::GetPollDescriptor()
{
    return fd;
}

UsbConnection::UsbConnection(const char *d)
    : device(d)
{
    // Initialise fd
    fd=-1;
}

int UsbConnection::PacketSize()
{
    // Return maximum USB packet payload
    return USB_MAXPAYLOAD;
}

STATUS UsbConnection::Open()
{
    // Close device if it's open already
    if (fd >= 0)
    {
	close(fd);
	fd = -1;
    }

    // Try to open device
    if ((fd = open(device.c_str(),O_RDWR)) < 0)
        return USB_E_OPENDEVICE;

    FlushReceiveBuffer();
    return S_OK;
}

void UsbConnection::Close()
{
    if (fd>=0) {
	close(fd);
	fd = -1;
    }
}

UsbConnection::~UsbConnection()
{
    Close();
}

/** Send a block of data
 *
 * Unlike unix calls, this write will only return when either the entire buffer
 * has been sent or an error occurrs. A partial write will cause a retry.
 */
STATUS UsbConnection::Send(const unsigned char *buffer, int length)
{
    // Write entire data packet: retry as necessary. We only return
    // when the entire buffer has been sent or there has been an error
    int done = 0, r;

    while(done < length)
    {
	if ((r = write(fd, buffer + done, length - done)) <= 0)
	    return MakeErrnoStatus();
	done += r;
    }

    // Done OK
    return S_OK;
}

// Get data from remote end with timeout
//
// if timeout>0 , it indicates a timeout in milliseconds to wait for data
// if timeout==0, it indicates that we should read only if we can read
//                without delay (non-blocking read, basically)
// if timeout<0 , it indicates that we should block until we can read
//
// Note that in accordance with unixisms, the read will return as many bytes
// as are available, up to the buffer size. If we reach the read() call,
// we will get at least one byte.
//
// Returns -errno for an error, otherwise we return the number of bytes read.
//
STATUS UsbConnection::FillBuffer(unsigned char *buffer, int buffer_size, int timeout_ms, DWORD & bytes_read)
{
    bytes_read = 0;

    int n;
    if (timeout_ms >= 0)
    {
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	n = poll(&pfd, 1, timeout_ms);
	if (n < 0)
	    return MakeErrnoStatus();
	if (!n)
	    return CONN_E_TIMEDOUT;
    }

    // Get some stuff: will block until at least one byte comes in
    n = read(fd, buffer, buffer_size);
    if (n < 0)
	return MakeErrnoStatus();

    bytes_read = n;

    return S_OK;
}

int UsbConnection::GetPollDescriptor()
{
    return fd;
}

Poll::Poll(int capacity)
{
    connections = NEW (Connection *)[capacity];
    pfds = NEW struct pollfd[capacity];
    size = 0;
    progress = 0;
}

Poll::~Poll()
{
    delete[] connections;
    delete[] pfds;
}

void Poll::Append(Connection *p, short events)
{
    connections[size] = p;
    pfds[size].fd = p->GetPollDescriptor();
    pfds[size].events = events;
    pfds[size].revents = 0;
    size++;
}

int Poll::Wait(int timeout)
{
    int result = 0;

    for(int i = 0; i < size; i++)
    {
	pfds[i].fd = connections[i]->GetPollDescriptor();
    }

    progress = 0;

    // First check whether any of the connections have pending data in
    // their buffers
    for(int i =0; i < size; i++)
    {
	if (connections[i]->IsDataPending())
	    result++;
    }

    if (!result)
    {
	result = poll(pfds, size, timeout * 1000);
    }
    return result;
}

Connection *Poll::GetNextActive()
{
    while (progress < size)
    {
	if (connections[progress]->IsDataPending() || pfds[progress].revents)
	{
	    return connections[progress++];
	}
	progress++;
    }
    return NULL;
}

