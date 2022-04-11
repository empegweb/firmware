/* lib/protocol/discovery.cpp
 *
 * Discovering empeg-car units on serial/USB/net
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.38 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *  Peter Hartley <peter@empeg.com>
 *  Roger Lipscombe <roger@empeg.com>
 */

/* Finds units on serial and USB (under Win32),
 * and on the network (via a broadcast).
 *
 * You can also compose a bunch of Discoverer objects with the
 * CompoundEmpegDiscoverer class.
 *
 * Different types of EmpegDiscoverer are in other files.
 */

#include "config.h"
#include "trace.h"

#include "discovery.h"

#include <errno.h>

#include "net/socket.h"
#include "net/InterfaceList.h"
#include "core/dynamic_config_file.h"
#include "protocolclient.h"
#include "empeg_time.h"

EmpegDiscoverer::EmpegDiscoverer()
    : observer(NULL)
{
    /** These defaults can be deleted after the clone tool has been changed
    to pass these names in
    */
    m_defaultNames.empeg_car1 = "empeg car";
    m_defaultNames.empeg_car2 = "empeg car";
    m_defaultNames.rio_car = "Rio Car";
    m_defaultNames.jupiter = "Rio HSX-109";
    m_defaultNames.unknown = "Unknown Player";
}

void EmpegDiscoverer::Attach(EmpegDiscovererObserver *o)
{
    ASSERT(o);
    ASSERT(!observer);
    observer = o;
}

bool EmpegDiscoverer::FireDiscoveredEmpeg(DiscoveredEmpegPtr discoveredEmpeg)
{
    ASSERT(observer);
    return observer->OnDiscoveredEmpeg(discoveredEmpeg);
}

std::string EmpegDiscoverer::GetDefaultName(const std::string &playerType)
{
    if (playerType == "empeg-car-1")
	return m_defaultNames.empeg_car1;
    else if (playerType == "empeg-car-2")
	return m_defaultNames.empeg_car2;
    else if (playerType == "rio-car")
	return m_defaultNames.rio_car;
    else if (playerType == "jupiter")
	return m_defaultNames.jupiter;
    else
	return m_defaultNames.unknown;
}

std::string EmpegDiscoverer::GetEmpegName(Connection *connection, const std::string &defaultName)
{
    std::string s(defaultName);

    ProtocolClient client(connection);
    if(SUCCEEDED(connection->Open()))
    {
	std::string config;
	if (SUCCEEDED(client.GetPlayerConfiguration(&config)))
	{
	    DynamicConfigFile dcf;
	    dcf.FromString(config);

	    dcf.GetStringValue("options", "name", &s);
	}
	
	connection->Close();
    }

    return s;
}

std::string EmpegDiscoverer::GetPlayerType(Connection *connection)
{
    std::string type = "unknown";

    ProtocolClient client(connection);
    if(SUCCEEDED(connection->Open()))
    {
	// Drop the retry count - it gets boring otherwise.
	client.SetMaximumRetryCount(4);
	client.GetPlayerType(&type);
	connection->Close();	
    }

    return type;
}
    

bool EmpegDiscoverer::IsEmpegConnected(Connection *connection)
{
    ProtocolClient client(connection);
    if(FAILED(connection->Open()))
	return false;

    bool b = client.IsUnitConnected();
    
    connection->Close();
    return b;
}

void CompoundEmpegDiscoverer::AddDiscoverer(EmpegDiscoverer *empegDiscoverer)
{
    ASSERT(empegDiscoverer);
    empegDiscoverer->Attach(this);
    discoverers.push_back(empegDiscoverer);
}

STATUS CompoundEmpegDiscoverer::Discover(unsigned timeout_ms, bool *pbContinue, bool *pbIgnoreFailure)
{
    if (discoverers.empty())
	return S_OK;

    do
    {
	Time startTime = Time::Now();
    
	unsigned individual_timeout_ms = timeout_ms / discoverers.size();
	for (DiscovererVec::const_iterator i = discoverers.begin(); i != discoverers.end(); ++i)
	{
	    EmpegDiscoverer *d = *i;
	    STATUS hr = d->Discover(individual_timeout_ms, pbContinue);
	    if (FAILED(hr) && pbIgnoreFailure && !*pbIgnoreFailure)
		return hr;

	    if (pbContinue && !*pbContinue)
		break;
	}

	unsigned int elapsedTime = (Time::Now() - startTime).GetAsMilliseconds();
	if (elapsedTime < timeout_ms)   // ensure it never goes negative -- it's unsigned.
	    timeout_ms -= elapsedTime;
	else
	    timeout_ms = 0;
    } while (timeout_ms > 0 && (!pbContinue || *pbContinue));

    return S_OK;
}

DiscoveredEmpegPtr DiscoveredEmpeg::Create(const std::string &name, const std::string &location,
					   const std::string &playerType, ConnectionType connectionType,
					   Connection *connection)
{
    DiscoveredEmpeg *p = NEW DiscoveredEmpeg(name, location, playerType, connectionType, connection);

    return DiscoveredEmpegPtr(p);
}

DiscoveredEmpeg::~DiscoveredEmpeg()
{
    delete m_connection;
}


#ifdef TEST

class MyObserver : public DiscoveryObserver
{
    virtual bool OnDiscover(const std::string name, unsigned int id, IPAddress addr);
};

bool MyObserver::OnDiscover(const std::string name, unsigned int id, IPAddress addr)
{
    cout << "empeg '" << name << "' id=" << id << " is at " <<
	addr.ToString() << "\n";
    return true;
}    

int main( int argc, char *argv[] )
{
    MyObserver o;
    Discovery d(&o);

    d.Discover(10000);

    return 0;
}

#endif

/* eof */
