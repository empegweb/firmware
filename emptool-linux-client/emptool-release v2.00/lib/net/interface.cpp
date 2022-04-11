/* interface.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 */

#include "config.h"
#include "trace.h"
#include "interface.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>

#include <iomanip>
#include <stdio.h>

#ifdef WIN32
static char not_sure_how_to_do_this_on_Windows;
#else

IPInterface::IPInterface( const char *ifname )
    : last_error(0), ifname(ifname), sock(-1)
{
}

IPInterface::~IPInterface()
{
    if ( sock != -1 )
	close( sock );
}


    /* IP addresses (self, broadcast, netmask) */

int IPInterface::GetAddressIoctl( int which_ioctl, IPAddress *res )
{
    struct ifreq ifr;

    if ( PrepareIfr(&ifr) < 0 )
	return last_error;

    if ( ioctl( sock, which_ioctl, &ifr ) < 0 )
	return SetError();

    *res = IPAddress( (sockaddr_in*)&ifr.ifr_ifru.ifru_addr );

    return 0;
}

int IPInterface::GetIPAddress( IPAddress *res )
{
    return GetAddressIoctl( SIOCGIFADDR, res );
}

int IPInterface::GetBroadcastAddress( IPAddress *res )
{
    return GetAddressIoctl( SIOCGIFBRDADDR, res );
}

int IPInterface::GetNetMask( IPAddress *res )
{
    return GetAddressIoctl( SIOCGIFNETMASK, res );
}

int IPInterface::SetAddressIoctl( int which_ioctl, const IPAddress ip )
{
    struct ifreq ifr;

    if ( PrepareIfr(&ifr) < 0 )
	return last_error;
    
    ip.ToSockAddr( (sockaddr_in*) &ifr.ifr_ifru.ifru_addr );

    if ( ioctl( sock, which_ioctl, &ifr ) < 0 )
	return SetError();

    return 0;
}

int IPInterface::SetIPAddress( const IPAddress ip )
{
    return SetAddressIoctl( SIOCSIFADDR, ip );
}

int IPInterface::SetBroadcastAddress( const IPAddress ip )
{
    return SetAddressIoctl( SIOCSIFBRDADDR, ip );
}

int IPInterface::SetNetMask( const IPAddress ip )
{
    return SetAddressIoctl( SIOCSIFNETMASK, ip );
}

/* Get/set hardware (MAC) address */

int IPInterface::GetHardwareAddress( unsigned char buf[6] )
{
    struct ifreq ifr;

    if ( PrepareIfr(&ifr) < 0 )
	return last_error;

    if ( ioctl( sock, SIOCGIFHWADDR, &ifr ) < 0 )
	return SetError();

//    cout << "gifhwaddr: family=" << ifr.ifr_ifru.ifru_addr.sa_family << "\n";

    memcpy( buf, ifr.ifr_ifru.ifru_addr.sa_data, 6 );

    return 0;
}

int IPInterface::SetHardwareAddress( const unsigned char buf[6] )
{
    struct ifreq ifr;

    if ( PrepareIfr(&ifr) < 0 )
	return last_error;

    // We need to get the HWADDR first so the sa_family is set up right

    if ( ioctl( sock, SIOCGIFHWADDR, &ifr ) < 0 )
	return SetError();

    memcpy( ifr.ifr_ifru.ifru_addr.sa_data, buf, 6 );

    if ( ioctl( sock, SIOCSIFHWADDR, &ifr ) < 0 )
	return SetError();

    return 0;
}

int IPInterface::GetHardwareAddress(std::string *s)
{
    unsigned char raw_mac[6];
    int n = GetHardwareAddress(raw_mac);
    if (n < 0)
	return n;

    char string_mac[24];

    snprintf(string_mac, 24, "%02x:%02x:%02x:%02x:%02x:%02x",
	     raw_mac[0],
	     raw_mac[1],
	     raw_mac[2],
	     raw_mac[3],
	     raw_mac[4],
	     raw_mac[5]);

    s->assign(string_mac);
    return 0;
}

    /* Interface flags */

int IPInterface::GetFlags( unsigned int* res )
{
    struct ifreq ifr;

    if ( PrepareIfr(&ifr) < 0 )
	return last_error;

    if ( ioctl( sock, SIOCGIFFLAGS, &ifr ) < 0 )
	return SetError();

    *res = (unsigned int) ifr.ifr_ifru.ifru_flags;

    return 0;
}

int IPInterface::SetFlags( unsigned int f )
{
    struct ifreq ifr;

    if ( PrepareIfr(&ifr) < 0 )
	return last_error;

    ifr.ifr_ifru.ifru_flags = (short)f;

    if ( ioctl( sock, SIOCSIFFLAGS, &ifr ) < 0 )
	return SetError();

    return 0;
}


    /* Default routes */

int IPInterface::SetDefaultRoute()
{
    if ( PrepareIfr(0) < 0 )
	return last_error;

    struct rtentry rt;

    memset( (void*)&rt, 0, sizeof(rt) );

    rt.rt_flags = RTF_UP;
    rt.rt_dev = const_cast<char*>(ifname.c_str());

    rt.rt_dst.sa_family = AF_INET;

    // "route add default eth0"

    if ( ioctl( sock, SIOCADDRT, &rt ) < 0 )
	return SetError();

    return 0;
}

int IPInterface::SetDefaultRoute( IPAddress gateway )
{
    if ( PrepareIfr(0) < 0 )
	return last_error;

    struct rtentry rt;

    memset( (void*)&rt, 0, sizeof(rt) );

    rt.rt_flags = RTF_UP | RTF_GATEWAY;
    rt.rt_dev = const_cast<char*>(ifname.c_str());

    rt.rt_dst.sa_family = AF_INET;

    gateway.ToSockAddr( (struct sockaddr_in*) &rt.rt_gateway );

    // "route add default gw <foo> eth0"

    if ( ioctl( sock, SIOCADDRT, &rt ) < 0 )
	return SetError();

    return 0;
}


    /* Does a default route already exist? */

bool IPInterface::DefaultRouteExists()
{
    FILE *fp = fopen( "/proc/net/route", "r" );
    bool route_found = false;

    if (fp)
    {
	const int buffer_size = 100;
	char buffer[buffer_size];
	char device[10];
	unsigned long destination;
	unsigned long gateway;
	unsigned long flags;
	// Discard the headers line.
	fgets(buffer, buffer_size, fp);

	while (!feof(fp))
	{
	    // Now read in the other lines.
	    fscanf( fp, "%s %lx %lx %lx %*x %*x %*x %*x %*x %*x %*x\n",
		    device, &destination, &gateway, &flags );

	    if (destination == 0 && (flags&1) == 1)
	    {
		// We've found a default route.
		route_found = true;
		break;
	    }
	}
	fclose(fp);
    }

    return route_found;
}

std::string IPInterface::GetInterfaceNameForAddress( const IPAddress& ip )
{
    FILE *fp = fopen( "/proc/net/route", "r" );
    std::string result;

    unsigned long numericip = ip.ToNetworkLong();

    if (fp)
    {
	const int buffer_size = 100;
	char buffer[buffer_size];
	char device[10];
	unsigned long destination;
	unsigned long gateway;
	unsigned long flags;
	unsigned long mask;
	// Discard the headers line.
	fgets(buffer, buffer_size, fp);

	while (!feof(fp))
	{
	    // Now read in the other lines.
	    fscanf( fp, "%s %lx %lx %lx %*x %*x %*x %lx %*x %*x %*x\n",
		    device, &destination, &gateway, &flags, &mask );

	    if ((numericip & mask) == destination)
	    {
/*		printf("%lx & %lx = %lx, returning %s\n",
		numericip, mask, destination, device); */

		result = device;
		break;
	    }
	}
	fclose(fp);
    }

    return result;
}


    /* Utility routines */

int IPInterface::PrepareIfr( struct ifreq *ifr )
{
    if ( sock == -1 )
    {
	int newsock = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( newsock < 0 )
	    return SetError();
	sock = newsock;
    }

    if ( !ifr )
	return 0;

    memset( ifr, 0, sizeof(*ifr) );

    strncpy( ifr->ifr_name, ifname.c_str(), IFNAMSIZ );

    return 0;
}

/** Sets the class's last_error according to errno */
int IPInterface::SetError()
{
    last_error = -errno;

    return last_error;
}


    /* Test harness */

#ifdef TEST

#include <stdio.h>

void dumpfile( const char *s )
{
    FILE *f = fopen(s,"r");
    char buf[128];

    printf( "*** %s ***\n", s );

    while ( !feof(f) )
    {
	int n = fread( buf, 1, 128, f );

	fwrite( buf, 1, n, stdout );
    }

    fclose(f);
}

void print_everything( IPInterface *ipif )
{
    IPAddress ipa;

    if ( ipif->GetIPAddress( &ipa ) < 0 )
	cout << "GetIPAddress: error " << strerror( -ipif->Error() ) << "\n";
    else
	cout << "  IP Address: " << ipa.ToString() << "\n";
    
    if ( ipif->GetBroadcastAddress( &ipa ) < 0 )
		cout << "GetBroadcastAddress: error " << strerror( -ipif->Error() ) << "\n";
    else
	cout << "  Broadcast Address: " << ipa.ToString() << "\n";

    if ( ipif->GetNetMask( &ipa ) < 0 )
	cout << "GetNetMask: error " << strerror( -ipif->Error() ) << "\n";
    else
	cout << "  Net Mask: " << ipa.ToString() << "\n";

    unsigned int iff;

    if ( ipif->GetFlags( &iff ) < 0 )
	cout << "GetFlags: error " << strerror( -ipif->Error() ) << "\n";
    else
	cout << "  Flags: 0x" << setbase(16) << iff
	     << ( (iff & IFF_UP)       ? " UP" : "" )
	     << ( (iff & IFF_BROADCAST) ? " BROADCAST" : "" )
	     << ( (iff & IFF_DEBUG) ? " DEBUG" : "" )
	     << ( (iff & IFF_LOOPBACK) ? " LOOPBACK" : "" )
	     << ( (iff & IFF_POINTOPOINT) ? " POINTOPOINT" : "" )
	     << ( (iff & IFF_NOTRAILERS) ? " NOTRAILERS" : "" )
	     << ( (iff & IFF_RUNNING) ? " RUNNING" : "" )
	     << ( (iff & IFF_NOARP) ? " NOARP" : "" )
	     << ( (iff & IFF_PROMISC) ? " PROMISC" : "" )
	     << "\n";

    unsigned char hwaddr[6];

    if ( ipif->GetHardwareAddress( hwaddr ) < 0 )
	cout << "GetHardwareAddress: error " << strerror( -ipif->Error() ) << "\n";
    else
	cout << "  Hardware address: " << setbase(16)
	     << int(hwaddr[0]) << ":"
	     << int(hwaddr[1]) << ":"
	     << int(hwaddr[2]) << ":"
	     << int(hwaddr[3]) << ":"
	     << int(hwaddr[4]) << ":"
	     << int(hwaddr[5])
	     << "\n";

    dumpfile( "/proc/net/route" );
}

int main( int argc, char *argv[] )
{
    if ( argc<2 )
    {
	cout << "Usage: " << argv[0] << " <interface> [<interface>...]\n"
	     << "  e.g. " << argv[0] << " eth0\n";
	return 1;
    }

    for ( int i=1; i<argc; i++ )
    {
	IPInterface ipif( argv[i] );

	if ( ipif.Error() )
	{
	    cout << "Error initialising interface '" << argv[i] << "'"
		 << "(" << ipif.Error() << ")\n";
	}
	else
	{
	    cout << "Reading information on " << argv[i] << "\n";

	    print_everything( &ipif );
	    
	    cout << "Changing everything\n";

	    IPAddress oldip, oldbr, oldmask;
	    ipif.GetIPAddress( &oldip );
	    ipif.GetBroadcastAddress( &oldbr );
	    ipif.GetNetMask( &oldmask );

	    unsigned char macaddr[6];
	    ipif.GetHardwareAddress( macaddr );

	    unsigned int oldflags;
	    ipif.GetFlags( &oldflags );

	    if ( ipif.SetIPAddress( IPAddress("192.168.168.4") ) < 0 )
		cout << "SetIPAddress: error " << strerror( -ipif.Error() ) << "\n";

	    if ( ipif.SetBroadcastAddress( IPAddress( "192.168.255.255" ) ) < 0 )
		cout << "SetBroadcastAddress: error " << strerror( -ipif.Error() ) << "\n";
	    if ( ipif.SetNetMask( IPAddress( "255.0.0.0" ) ) < 0 )
		cout << "SetNetMask: error " << strerror( -ipif.Error() ) << "\n";

	    if ( ipif.SetFlags( IFF_PROMISC /*only*/ ) < 0 )
		cout << "SetFlags: error " << strerror( -ipif.Error() ) << "\n";

	    unsigned char newmacaddr[6];
	    memcpy( newmacaddr, macaddr, 6 );
	    newmacaddr[0] ^= 0x80;
	    if ( ipif.SetHardwareAddress( newmacaddr ) < 0 )
		cout << "SetHardwareAddress: error " << strerror( -ipif.Error() ) << "\n";

	    if ( ipif.SetDefaultRoute() < 0 )
		cout << "SetDefaultRoute: error "
		     << strerror( -ipif.Error() ) << "\n";

	    print_everything( &ipif );
	    
	    cout << "Back to normal\n";

	    ipif.SetIPAddress( oldip );
	    ipif.SetBroadcastAddress( oldbr );
	    ipif.SetNetMask( oldmask );
	    ipif.SetHardwareAddress( macaddr );
	    ipif.SetFlags( oldflags );
	    ipif.SetDefaultRoute( IPAddress(10,1,1,1) );

	    print_everything( &ipif );
	}
    }

    return 0;
}

#endif /* def TEST */

#endif /* ndef WIN32 */

/* eof */
