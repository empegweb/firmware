/* InterfaceList_win32.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.8 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "InterfaceList.h"
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "socket.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

InterfaceList InterfaceList::GetInterfaceList()
{
    InterfaceList ifs;

    StreamSocket soc;
    HRESULT hr;
    if(FAILED(hr = soc.Create()))
	return ifs;

    DWORD bytesReturned = 0;
    char buffer[8192];
    int result = soc.WSAIoctl(SIO_GET_INTERFACE_LIST,	// socket, ioctl
			NULL, 0,				// in
			buffer, sizeof(buffer),		// out
			&bytesReturned,			// how much?
			NULL, NULL);			// overlapped completion
    if(result == SOCKET_ERROR)
	return ifs;

    // Filter out loopback and PPP.
    for(const char *p = buffer; p < buffer + bytesReturned; p += sizeof(INTERFACE_INFO))
    {
	const INTERFACE_INFO *info = (const INTERFACE_INFO *)p;
	
	// We might want the interface...
	ifs.push_back(InterfaceInfo(info->iiFlags,
		IPAddress((struct sockaddr_in *)&info->iiAddress),
		IPAddress((struct sockaddr_in *)&info->iiBroadcastAddress),
		IPAddress((struct sockaddr_in *)&info->iiNetmask), "", 0));
    }

    return ifs;
}
