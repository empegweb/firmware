/* InterfaceList_test.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "InterfaceList.h"
#include <stdio.h>

int main(void)
{
#ifdef WIN32
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2,0), &wsa);
#endif

    InterfaceList il = InterfaceList::GetInterfaceList();
    for (InterfaceList::const_iterator i = il.begin(); i != il.end(); ++i)
    {
	printf("flags = 0x%x\n", i->GetFlags());
	printf("addr  = %s\n", i->GetAddress().ToString().c_str());
	printf("bcast = %s\n", i->GetBroadcastAddress().ToString().c_str());
	printf("mask  = %s\n", i->GetNetmask().ToString().c_str());
	printf("\n");
    }
    
#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}
