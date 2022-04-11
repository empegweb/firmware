/* InterfaceList_unix.cpp
 *
 * Unix implementation of IP interface listing
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.14 01-Apr-2003 18:52 rob:)
 */

#include "InterfaceList.h"
#include "socket.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <ctype.h>

InterfaceList InterfaceList::GetInterfaceList()
{
    //
    // Don't use SIOCGIFCONF - it won't list the interfaces unless they're up (i.e. IFF_UP flag is on)
    //
    // This is useless if you're actually using this list to turn the IFF_UP flags on!
    //
    // The only sure-fire way to do this (at the moment) is to read the proc file :-(
    //

    InterfaceList ifs;

    StreamSocket s;
    STATUS status;
    if(FAILED(status = s.Create()))
        return ifs;

    std::vector<std::string> if_names;
    
    FILE *cmdfile = fopen ("/proc/net/dev", "r");
    if (cmdfile != NULL)
    {
        char buffer[ 512 ];

        while (fgets (buffer, sizeof (buffer), cmdfile))
        {
            std::string name = "";            
            for (unsigned i = 0; (i < sizeof (buffer)) && (buffer[ i ]); i++)
                if (isspace(buffer[ i ]))       // ignore spaces
                {
                }
                else if (buffer[ i ] == ':')    // this marks end of interface name - hopefully!
                {
                    if_names.push_back(name);
                    break;
                }
                else 
                    name += buffer[ i ];        // must be part of the interface name                
        }
        fclose (cmdfile);
    }
    else        
        if_names.push_back("eth0");             // Nice ...
    
    for (unsigned i = 0; i < if_names.size();i++)
    {
        struct ifreq ifr; 
        memset(&ifr, 0, sizeof(ifr)); 

        strncpy (ifr.ifr_name, if_names[ i ].c_str(), IFNAMSIZ);

        if (ioctl (s.GetFD(), SIOCGIFINDEX, &ifr) == 0)
        {        
            int index = ifr.ifr_ifindex;
            
            ioctl( s.GetFD(), SIOCGIFFLAGS, &ifr );
            unsigned int flags = ifr.ifr_ifru.ifru_flags;

            ioctl( s.GetFD(), SIOCGIFADDR, &ifr );
            IPAddress addr((struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr);

            ioctl( s.GetFD(), SIOCGIFBRDADDR, &ifr );
            IPAddress broadcast((struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr);

            ioctl( s.GetFD(), SIOCGIFNETMASK, &ifr );
            IPAddress netmask((struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr);

            ifs.push_back(InterfaceInfo(flags, addr, broadcast, netmask, 
                if_names[ i ], index));
        }
    }

    s.Close();
    return ifs;
}

