/* InterfaceList_test.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

//#include "config.h"
#include "trace.h"
#include "InterfaceList.h"
#include <stdio.h>
#include <net/if.h>

int main(void)
{
#ifdef WIN32
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2,0), &wsa);
#endif

    InterfaceList il = InterfaceList::GetInterfaceList();
    for (InterfaceList::const_iterator i = il.begin(); i != il.end(); ++i)
    {
        printf("name = %s\n", i->GetName().c_str());
        printf("index = %d\n", i->GetIndex());
        unsigned int flags = i->GetFlags();
        printf("flags = 0x%x\n", flags);
        if (IFF_UP            & flags)
            printf ("  IFF_UP            Interface is running.\n");
        if (IFF_BROADCAST     & flags)
            printf ("  IFF_BROADCAST     Valid broadcast address set.\n");
        if (IFF_DEBUG         & flags)
            printf ("  IFF_DEBUG         Internal debugging flag.\n");
        if (IFF_LOOPBACK      & flags)
            printf ("  IFF_LOOPBACK      Interface is a loopback interface.\n");
        if (IFF_POINTOPOINT   & flags)
            printf ("  IFF_POINTOPOINT   Interface is a point-to-point link.\n");
        if (IFF_RUNNING       & flags)
            printf ("  IFF_RUNNING       Resources allocated.\n");
        if (IFF_NOARP         & flags)
            printf ("  IFF_NOARP         No arp protocol, L2 destination address not set.\n");
        if (IFF_PROMISC       & flags)
            printf ("  IFF_PROMISC       Interface is in promiscuous mode.\n");
        if (IFF_NOTRAILERS    & flags)
            printf ("  IFF_NOTRAILERS    Avoid use of trailers.\n");
        if (IFF_ALLMULTI      & flags)
            printf ("  IFF_ALLMULTI      Receive all multicast packets.\n");
        if (IFF_MASTER        & flags)
            printf ("  IFF_MASTER        Master of a load balancing bundle.\n");
        if (IFF_SLAVE         & flags)
            printf ("  IFF_SLAVE         Slave of a load balancing bundle.\n");
        if (IFF_MULTICAST     & flags)
            printf ("  IFF_MULTICAST     Supports multicast\n");
        if (IFF_PORTSEL       & flags)
            printf ("  IFF_PORTSEL       Is able to select media type via ifmap.\n");
        if (IFF_AUTOMEDIA     & flags)
            printf ("  IFF_AUTOMEDIA     Auto media selection active.\n");
//      if (IFF_DYNAMIC       & flags)
//          printf ("  IFF_DYNAMIC       The addresses are lost when the  interface  goes down.\n");
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
