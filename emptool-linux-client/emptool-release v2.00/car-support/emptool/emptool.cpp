/* emptool.cpp
 * 
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Main program of emptool command-line uploader
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.17 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "emptool.h"
#ifdef WIN32
#include "EmpegUsbDevices.h"
#endif
#include <string>
#include <list>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "protocolclient.h"
#include "discovery.h"
#include "net_discovery.h"
#include "playerdb.h"
#include "empconsole.h"
#include "numerics.h"

#include "version.h"
#include "empeg_error.h"
// As in linux/Documentation/devices.txt

#define DEV_MAJOR_SERIAL	4
#define DEV_MINOR_SERIAL_FROM	64
#define DEV_MINOR_SERIAL_TO	255
#define DEV_MAJOR_CALLOUT	5
#define DEV_MINOR_CALLOUT_FROM	64
#define DEV_MINOR_CALLOUT_TO	255

// Character major 240 is for local/experimental use (we use it for USB)
#define DEV_MAJOR_USB		240
#define DEV_MINOR_USB_FROM	0
#define DEV_MINOR_USB_TO	7
	    	    
const int TIMEOUT = 15;
 
typedef std::list<std::string> StringList;

void usage();
int rebuild_database(ProtocolClient *client);
static Connection *choose_device();

int main(int argc, char *argv[])
{
    std::string device;
    
    int use_usb = -1;
    int do_usage = 0, do_restart = 0, do_reboot = 0, do_quit = 0, do_rebuild = 0, do_check = 0;
    int non_default_timeout = 0;
    int debug = 0;
    int timeout = TIMEOUT;
    int wait_player = 0;
    int i;

    for(i=1; i<argc; i++) {
	if(argv[i][0] == '-') {
	    if(argv[i][1] == '-') {
		if(!strcmp(argv[i], "--help")) do_usage = 1;
		else if(!strcmp(argv[i], "--timeout")) {
		    if(i+1 >= argc) do_usage = 1;
		    else {
			timeout = atol(argv[++i]);
			non_default_timeout = 1;
		    }
		}
		else if(!strcmp(argv[i], "--check")) do_check = 1;
		else if(!strcmp(argv[i], "--restart")) do_restart = 1;
		else if(!strcmp(argv[i], "--reboot")) do_reboot = 1;
		else if(!strcmp(argv[i], "--rebuild")) do_rebuild = 1;
		else if(!strcmp(argv[i], "--wait")) wait_player = 1;
		else if(!strcmp(argv[i], "--quit")) do_quit = 1;
		else if(!strcmp(argv[i], "--serial")) use_usb = 0;
		else if(!strcmp(argv[i], "--usb")) use_usb = 1;
		else if(!strcmp(argv[i], "--debug")) {
		    if(i+1 >= argc) do_usage = 1;
		    else {
			debug = atol(argv[++i]);
			if((debug < 0) || (debug > 4)) do_usage = 1;
		    }
		}
		else {
		    printf("Unknown option %s\n", argv[i]);
		    do_usage = 1;
		    break;
		}
	    }
	    else {
		int arg2 = i + 1;
		for(int j=1; argv[i][j]; j++) {
		    switch(argv[i][j]) {
		    case 'h':
			do_usage = 1;
			break;

		    case 't':
			if(arg2 >= argc) do_usage = 1;
			else timeout = atol(argv[arg2++]);
			break;
			
		    case 'c':
			do_check = 1;
			break;
			
		    case 'r':
			do_restart = 1;
			break;
			
		    case 'b':
			do_reboot = 1;
			break;
			
		    case 'd':
			do_rebuild = 1;
			break;

		    case 'w':
			wait_player = 1;
			break;
			
		    case 'q':
			do_quit = 1;
			break;

		    case 's':
			use_usb = 0;
			break;

		    case 'u':
			use_usb = 1;
			break;
			
		    default:
			printf("Unknown option -%c\n", argv[i][j]);
			do_usage = 1;
			break;
		    }
		}
		i = arg2 - 1;
	    }
	}
	else {
	    if(device != "") {
		do_usage = 1;
		break;
	    }
	    device = argv[i];
	}
    }

    // start up a progress thingy
    ConsoleProgress progress;

    if(do_quit + do_check + do_restart + do_rebuild + do_reboot > 1) {
	do_usage = 1;
    }

    if(do_usage) {
	usage();
	return 1;
    }
    
    Connection *connection = NULL;

    bool host = false; // to make error message nicer


    if ( device == "" )
	connection = choose_device();
    else
    {
	struct stat statbuf;
	if(stat(device.c_str(), &statbuf) < 0) {
	    // Not a file? Try it as a hostname
	    connection = NEW OutTcpConnection( device.c_str(),
					       PROTOCOL_TCP_PORT );
	    host = true;
	}
	else
	{
	    if(use_usb == -1) {
		if(!S_ISCHR(statbuf.st_mode)) {
		    fprintf(stderr, "Not a character device - \"%s\"\n", device.c_str() );
		    return 1;
		}
		int dmajor, dminor;
		dmajor = (statbuf.st_rdev >> 8) & 255;
		dminor = (statbuf.st_rdev) & 255;
		if(dmajor == DEV_MAJOR_SERIAL) {
		if((dminor >= DEV_MINOR_SERIAL_FROM) &&
		   (dminor <= DEV_MINOR_SERIAL_TO)) use_usb = 0;
		}
		else if(dmajor == DEV_MAJOR_CALLOUT) {
		    if((dminor >= DEV_MINOR_CALLOUT_FROM) &&
		       (dminor <= DEV_MINOR_CALLOUT_TO)) use_usb = 0;
		}
		else if(dmajor == DEV_MAJOR_USB) {
		    if((dminor >= DEV_MINOR_USB_FROM) &&
		       (dminor <= DEV_MINOR_USB_TO)) use_usb = 1;
		}
		if(use_usb == -1) {
		    fprintf(stderr, "Unknown device character %d,%d - assuming serial buffering\n",
			    dmajor, dminor);
		    use_usb = 0;
		}
	    }
	    if(use_usb)
		connection = NEW UsbConnection(device.c_str());
	    else {
		SerialConnection *serial = NEW SerialConnection( device.c_str(), 
								 115200 );
		serial->Raw();
		connection = serial;
	    }
	}
    }

    if(debug) connection->SetDebugLevel(debug);
    
    if(FAILED(connection->Open())) {
	if ( host )
	    progress.Error(Numerics::System_connection_error,
			   "Failed to connect to host %s, errno:%d\n",
			   device.c_str(), errno);
	else
	    progress.Error(Numerics::System_connection_error,
			   "Failed to open device %s, errno:%d\n",
			   device.c_str(), errno);
	return 1;
    }
    
    if(do_quit) {
	if(!wait_player) {
	    progress.Log(Numerics::Quitting_player,
			 "Quitting player (blind)\n");
	    Request r(connection);
	    r.Quit();
	    r.Quit();	// one for luck
	    return 0;
	}
	progress.TaskStart(Numerics::Quitting_player, "Quitting player");
	progress.TaskUpdate(0, timeout);
	Request r(connection);
	int i;
	for(i=0; i<timeout; i++) {
	    if ( FAILED(r.Quit()) )
		break;
	    progress.TaskUpdate(i, timeout);
	    sleep(1);
	}
	if(i == timeout) {
	    progress.Error(Numerics::Connection_timeout,
			   "No response\n");
	    return 1;
	}
	progress.TaskUpdate(timeout, timeout);
	return 0;
    }

    ProtocolClient client(connection);

    if(non_default_timeout) {
	client.SetMaximumRetryCount(timeout);
    }
    
    progress.TaskStart(Numerics::Checking_connection, "Checking connection");
    progress.TaskUpdate(0, timeout);

    for(i=0; i<timeout; i++) {
	if(client.IsUnitConnected()) break;
	progress.TaskUpdate(i, timeout);
	sleep(1);
    }
    if(i == timeout) {
	progress.Error(Numerics::Connection_timeout,
		       "Unit not found, check cabling and setup\n");
	return 1;
    }
    progress.TaskUpdate(timeout, timeout);

    // check for protocol version
    short major, minor;
    STATUS result = client.GetProtocolVersion(&major, &minor);
    if (SUCCEEDED(result)) 
    {
        if(major > PROTOCOL_VERSION_MAJOR) 
        {
            printf("Protocol version of emptool (%d) is too old for your player (%d)\n",
                   PROTOCOL_VERSION_MAJOR, major);
            printf("Please obtain an updated emptool\n");
            return 1;
        }
        else if(major < PROTOCOL_VERSION_MAJOR) 
        {
            printf("Protocol version of your player (%d) is too old for emptool (%d)\n",
                   major, PROTOCOL_VERSION_MAJOR);
            printf("Please obtain an upgrade for your player\n");
            return 1;
        }
    }
    else
    {
        printf ("%s\n", FormatErrorMessage(result).c_str());
    }
    
    // just do a check
    if(do_check) return 0;
    
    if(do_restart) {
	printf("Restarting player");
	fflush(stdout);
	Request r(connection);
	int i;
	for(i=0; i<timeout; i++) {
	    if ( FAILED(r.SendCommand(COM_RESTART, 1)) )
		 break;
	    printf(".");
	    fflush(stdout);
	    sleep(1);
	}
	if(i == timeout) {
	    printf("no response\n");
	    return 1;
	}
	printf("\nDone\n");
	if(wait_player) {
	    printf("Waiting for player to restart");
	    fflush(stdout);
	    // bloody multithreading
	    sleep(2);
	    int i;
	    for(i=0; i<timeout; i++) {
		if(client.IsUnitConnected()) break;
		printf(".");
		fflush(stdout);
		sleep(1);
	    }
	    if(i == timeout) {
		printf("timed out\n");
		return 1;
	    }
	    printf("\nFound player\n");
	}
	return 0;
    }
    else if(do_reboot) {
	printf("Rebooting player");
	fflush(stdout);
	Request r(connection);
	int i;
	for(i=0; i<timeout; i++) {
	    if ( FAILED(r.SendCommand(COM_RESTART,2)) )
		break;
	    printf(".");
	    fflush(stdout);
	    sleep(1);
	}
	if(i == timeout) {
	    printf("no response\n");
	    return 1;
	}
	printf("\nDone\n");
	if(wait_player) {
	    printf("Waiting for player to reboot");
	    fflush(stdout);
	    // bloody multithreading
	    sleep(2);
	    int i;
	    for(i=0; i<timeout; i++) {
		if(client.IsUnitConnected()) break;
		printf(".");
		fflush(stdout);
		sleep(1);
	    }
	    if(i == timeout) {
		printf("timed out\n");
		return 1;
	    }
	    printf("\nFound player\n");
	}
	return 0;
    }
    else if(do_rebuild) {
	PlayerDatabase db(client, &progress);
	
	if( FAILED(db.RebuildDatabase()) ) {
	    printf("Failed to rebuild database\n");
	    return 1;
	}
	else return 0;
    }
    else {
	PlayerDatabase db(client, &progress);
	
	ConsoleInterface iface(db);
	
	if(debug) iface.SetDebug(debug);
	if(!iface.Init()) return 1;
	
	while(iface.ProcessLine());
    }

    /* This is done when the DiscoveredEmpeg pointer goes away */
    //delete connection;

    printf("Connection closed\n");

    return 0;
}

void usage()
{
    printf("emptool version " EMPTOOL_VERSION "\n"
	   "\n"
	   "Usage:\n"
	   "  emptool [options] [<device>]\n"
	   "\n"
	   "Options:\n"
	   "\n"
	   "-h, --help                This text\n"
	   "-t, --timeout <retries>   Connection check polling retries\n"
	   "-c, --check               Just check connection, 1 retry/second\n"
	   "-r, --restart             Restart (cycle) the player\n"
	   "-b, --reboot              Reboot (power) the player\n"
	   "-d, --rebuild             Rebuild player database\n"
	   "-w, --wait                Wait for player to restart/reboot\n"
	   "-q, --quit                Quit developer image players to shell\n"
	   "--debug <0-3>             Lots of debug output\n"
	   "--serial                  Override to use serial buffer size\n"
	   "--usb                     Override to use USB buffer size\n"
	   "\n"
	   "<device> is the device to connect with (e.g /dev/ttyS0) or its network address\n"
	   "(e.g. 10.0.0.3).\n"
	   "\n"
	   "If <device> is left out, emptool searches for one on the network.\n\n" );
}

class NetDeviceLocator : public EmpegDiscovererObserver
{
    Connection *conn;
    bool found;

public:
    NetDeviceLocator() : found(false)
    {
    }

    bool Found() const
    {
	return found;
    }
    Connection *GetConnection() const
    {
	ASSERT(conn);
	return conn;
    }

    virtual bool OnDiscoveredEmpeg( DiscoveredEmpegPtr );
};

bool NetDeviceLocator::OnDiscoveredEmpeg( DiscoveredEmpegPtr dep )
{
    printf("empegcar '%s' (ID number %d) found at %s\n",
	   dep->GetName().c_str(), 0, dep->GetLocation().c_str());

    found = true;
    conn = dep->GetConnection();

    // stop the dep going away when we delete the discoverer
    static DiscoveredEmpegPtr sdep = dep;

    return false;
}

static Connection *choose_device()
{
    printf( "Scanning Ethernet for an empegcar...\n" );

    NetDeviceLocator locator;
    NetworkEmpegDiscoverer d;

    d.Attach( &locator );

    HRESULT hr = d.Discover(10000); // 10s timeout

    if (FAILED(hr))
    {
	printf( "Looking for network players gave error 0x%x\n",
		PrintableStatus(hr) );
	exit(1);
    }

    if (!locator.Found())
    {
	printf( "Cannot find an empegcar on Ethernet\n" );
	exit(1);
    }

    Connection *result = locator.GetConnection();

    printf( "Connection is %p\n", result );

    return result;
}
