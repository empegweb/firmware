/* unicast_discovery.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.16 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"

#include "net_discovery.h"
#include "protocolclient.h"
#include "core/dynamic_config_file.h"

UnicastNetworkEmpegDiscoverer::UnicastNetworkEmpegDiscoverer(const IPAddress & ip)
    : m_ip(ip)
{
}

/** We've been given an explicit address to try, so we'll just hook up a connection to it,
 * and try connecting...
 */
HRESULT UnicastNetworkEmpegDiscoverer::Discover(unsigned timeout_ms, bool *pbContinue, bool* /*pbIgnoreFailure*/)
{
    // We've been given an explicit address to try, so we'll just hook up a connection to it,
    // and try connecting...
    OutTcpConnection *connection = NEW OutTcpConnection(m_ip.ToString().c_str(), PROTOCOL_TCP_PORT);
    ProtocolClient client(connection);

    STATUS status = connection->Open();
    if(SUCCEEDED(status))
    {
	if (client.IsUnitConnected())
	{
	    std::string playerType; 
	    client.GetPlayerType(&playerType);

	    std::string name;
	    std::string config;
	    status = client.GetPlayerConfiguration(&config);
	    if (SUCCEEDED(status))
	    {
		DynamicConfigFile dcf;
		dcf.FromString(config);
		
		name = dcf.GetStringValue("options", "name", GetDefaultName(playerType));
	    }
	    
	    FireDiscoveredEmpeg(DiscoveredEmpeg::Create(name, m_ip.ToString(), playerType, DiscoveredEmpeg::ctEthernet, connection));
	}
	else
	{
	    connection->Close();
	    delete connection;
	}
    }
    else
	delete connection;

    return S_OK;
}
