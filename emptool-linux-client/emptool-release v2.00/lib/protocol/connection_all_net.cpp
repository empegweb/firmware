/* connection_all_net.cpp
 *
 * Network connection code common to Win32 and Unix
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"

#include "connection.h"

STATUS OutSocketConnection::Open()
{
    // Close device if it's open already
    Close();

    STATUS result;
    if (FAILED(result = CreateAndBind()))
	return result;

    FlushReceiveBuffer();
    return S_OK;
}

OutTcpConnection::OutTcpConnection(const char *h, int p)
    : host(h), port(p), fast_connection(NULL)
{
}

// TCP must disconnect/reconnect when player restarts
void OutTcpConnection::Pause()
{
    Close();
    if ( fast_connection )
    {
	fast_connection->Close();
	delete fast_connection;
	fast_connection = NULL;
    }
}

STATUS OutTcpConnection::Unpause()
{
    return Open();
}

OutTcpConnection::~OutTcpConnection()
{
    delete fast_connection;
}

int OutTcpConnection::PacketSize()
{
    // Return maximum USB packet payload
    return TCP_MAXPAYLOAD;
}

Connection *OutTcpConnection::GetFastConnection()
{
    if ( !fast_connection )
    {
	fast_connection = NEW OutTcpConnection(host.c_str(),port+1);
	if (FAILED(fast_connection->Open()))
	{
//	    printf("Opening fast connection failed\n");
	    delete fast_connection;
	    fast_connection = NULL;
	}
//	printf("Opening fast connection succeeded\n");
    }
    return fast_connection;
}
