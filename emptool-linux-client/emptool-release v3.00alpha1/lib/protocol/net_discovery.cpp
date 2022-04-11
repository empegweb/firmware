/* net_discovery.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.30 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"

#ifndef ECOS

#include <set>
#include "net_discovery.h"
#include "connection.h"
#include "packets.h"
#include "empeg_time.h"

#define TRACE_PROTOCOL 0

NetworkEmpegDiscoverer::NetworkEmpegDiscoverer()
    : created(false)
{
}

NetworkEmpegDiscoverer::~NetworkEmpegDiscoverer()
{
    if (created)
    {
        sock.Shutdown(2);
	sock.Close();
    }
}

STATUS NetworkEmpegDiscoverer::Discover(unsigned timeout_ms, bool *pbContinue, bool* /*pbIgnoreFailure*/)
{
    STATUS hr;

    if (!created)
    {
	IPEndPoint local(IPAddress::ANY, DISCOVERY_PORT);

	hr = sock.Create();
	if (FAILED(hr))
	    return hr;

	created = true;

        sock.SetReuseAddr(true);

	hr = sock.Bind(local);
	if (FAILED(hr))
	    return hr;

	int rc = sock.EnableBroadcast(true);
	if (rc < 0)
	    return E_FAIL;
    }

    hr = BroadcastDiscoveryRequest();
    if(FAILED(hr))
	return hr;

    return WaitForDiscoveryResponses(timeout_ms, pbContinue);
}

// The IP specification has it that a broadcast to 255.255.255.255
// is implementation dependent, but we make it the job of DatagramSocket
// to make sure the Right Thing is done
STATUS NetworkEmpegDiscoverer::BroadcastDiscoveryRequest()
{
    STATUS hr = sock.SendBroadcast( "?", DISCOVERY_PORT );
    if (FAILED(hr))
	return hr;

    return S_OK;
}

STATUS NetworkEmpegDiscoverer::WaitForDiscoveryResponses(unsigned timeout_ms, bool *pbContinue)
{
    std::set<IPAddress> unit_set;

    while (1)
    {
	std::string response, name, id;
	IPEndPoint unit;

	Time time_start = Time::Now();

        TRACEC(TRACE_PROTOCOL, "Selecting with timeout=%dms\n\n", timeout_ms);

        unsigned int this_timeout = timeout_ms > 1000 ? 1000 : timeout_ms; // Rebroadcast every second

	int rc = sock.Select(this_timeout);
	if (rc == 0)
	{
            TRACEC(TRACE_PROTOCOL, "Select timed out\n");

	    if (timeout_ms < this_timeout)
		timeout_ms = 0;
	    else if (timeout_ms != INFINITE)
                timeout_ms -= this_timeout;

            if (timeout_ms)
            {
                BroadcastDiscoveryRequest();
                continue;
            }

	    // Timed out.
	    return S_OK;
	}
	else if (rc < 0)
	{
            TRACEC(TRACE_PROTOCOL, "Select failed (%d)\n",rc);
	    // Error.
	    return E_FAIL;
	}

	STATUS hr = sock.ReceiveFrom( &response, 4096, &unit );

        TRACEC(TRACE_PROTOCOL, "ReceiveFrom returned %d response=%s unit=%s\n",
		  PrintableStatus(hr), response.c_str(), 
		  unit.GetIPAddress().ToString().c_str() );

	if (FAILED(hr))
	    return hr;

        // Don't report it again if it's already in the set
        if ( unit_set.insert(unit.GetIPAddress()).second )
        {
	    bool bContinue = ParseDiscoveryResponse(response, unit);
	    if(!bContinue)
	    {
    	       if(pbContinue)
    		    *pbContinue = bContinue;
    
                break;
            }
        }
        else
        {
            TRACEC(TRACE_PROTOCOL, "Found %s again\n", unit.GetIPAddress().ToString().c_str() );
        }

	unsigned taken_ms = (Time::Now() - time_start).GetAsMilliseconds();
        TRACEC(TRACE_PROTOCOL, "select took %dms\n", taken_ms);
	if (timeout_ms < taken_ms)
	    timeout_ms = 0;
	else if (timeout_ms != INFINITE)
            timeout_ms -= taken_ms;
    }

    return S_OK;
}

bool NetworkEmpegDiscoverer::ParseDiscoveryResponse(const std::string & s, const IPEndPoint & unit)
{
    std::string response(s);
    response += "\n";

    TRACEC(TRACE_PROTOCOL, "Received %s from %s\n\n", s.c_str(), unit.GetIPAddress().ToString().c_str() );

    // The string is of the format name=<name>\nid=<id>\n
    // so split it. This is nicer in Python...
    unsigned int pos;

    std::string name;
    std::string id;
    while ( (pos = response.find('\n') ) != std::string::npos )
    {
	std::string line = response.substr( 0, pos );
	response = response.substr( pos+1 );

	pos = line.find('=');

	std::string field = line.substr( 0, pos );
	std::string value = line.substr( pos+1 );

	if ( field == "name" )
	    name = value;
	if ( field == "id" )
	    id = value;
    }

    if ( name != "" && id != "" )
    {
	// unsigned int numericid = atoi(id.c_str()); // this info not used
	std::string ip(unit.GetIPAddress().ToString());
	OutTcpConnection *connection = NEW OutTcpConnection(ip.c_str(), PROTOCOL_TCP_PORT);

   	std::string playerType(GetPlayerType(connection));
	return FireDiscoveredEmpeg(DiscoveredEmpeg::Create(name, ip, playerType, DiscoveredEmpeg::ctEthernet, connection));
    }

    return true;
}

#endif // !defined(ECOS)
