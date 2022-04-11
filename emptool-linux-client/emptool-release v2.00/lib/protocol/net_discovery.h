/* net_discovery.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 *
 * (:Empeg Source Release 1.11 01-Apr-2003 18:52 rob:)
 */

#ifndef included_discovery_h
#include "discovery.h"
#endif

#ifndef INCLUDED_SOCKET_H_
#include "net/socket.h"
#endif

// UDP version of the protocol port 
#define DISCOVERY_PORT 8300

class NetworkEmpegDiscoverer : public EmpegDiscoverer
{
    DatagramSocket sock;
    bool created;

public:
    NetworkEmpegDiscoverer();
    ~NetworkEmpegDiscoverer();

    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue=0, bool *pbIgnoreFailure = NULL);

private:
    HRESULT BroadcastDiscoveryRequest();
    HRESULT WaitForDiscoveryResponses(unsigned timeout_ms, bool *pbContinue);
    bool ParseDiscoveryResponse(const std::string & s, const IPEndPoint & unit);
};

class UnicastNetworkEmpegDiscoverer : public EmpegDiscoverer
{
    IPAddress m_ip;

public:
    UnicastNetworkEmpegDiscoverer(const IPAddress & ip);
    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue = NULL, bool *pbIgnoreFailure = NULL);
};

