/* interface.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 */

#ifndef included_interface_h
#define included_interface_h

#ifndef IP_H
#include "ipaddress.h"
#endif

#include <string>

struct ifreq;

/** Encapsulate an IP-capable network interface, eg. IPInterface("eth0").
 * Unix only.
 */
class IPInterface
{
    int last_error;
    std::string ifname;
    int sock;

    DISALLOW_COPYING(IPInterface);
    
 public:
    explicit IPInterface( const char *ifname );
    ~IPInterface();

    // Returns -errno, or 0 if last operation was successful
    int Error() { return last_error; }

    // all these return 0 for success or -errno

    int GetIPAddress( IPAddress * );
    int SetIPAddress( const IPAddress );

    int GetBroadcastAddress( IPAddress * );
    int SetBroadcastAddress( const IPAddress );

    int GetNetMask( IPAddress * );
    int SetNetMask( const IPAddress );

    int GetHardwareAddress( unsigned char buf[6] );
    int SetHardwareAddress( const unsigned char buf[6] );
        // SetHardwareAddress will return -EBUSY if the interface is up
        // use SetFlags( <not including IFF_UP> ) to take it down first

    int GetHardwareAddress(std::string *);

    int GetFlags( unsigned int *iff_flags ); // see IFF_* in <net/if.h>
    int SetFlags( unsigned int );

    int SetDefaultRoute( IPAddress gateway );
    int SetDefaultRoute(); // no gateway ("but send 255.255.255.255 thisaway")

    bool DefaultRouteExists();

    static std::string GetInterfaceNameForAddress( const IPAddress& );

 protected:
    int PrepareIfr( struct ifreq* );
    int SetError();
    int SetAddressIoctl( int which_ioctl, const IPAddress );
    int GetAddressIoctl( int which_ioctl, IPAddress * );
};

#endif

/* eof */
