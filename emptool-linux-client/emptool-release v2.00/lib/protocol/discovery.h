/* lib/protocol/discovery.h
 *
 * Discovering empeg-car units on serial/USB/net
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.20 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *  Peter Hartley <peter@empeg.com>
 *  Mike Crowe <mac@empeg.com>
 *  Roger Lipscombe <roger@empeg.com>
 */

#ifndef DISCOVERY_H
#define DISCOVERY_H 1

#include <string>
#include <vector>
#include "net/socket.h"
#include "net/ipaddress.h"
#include "core/refcount.h"

struct EmpegDiscovererObserver;
class Connection;

/** This is what we're looking for
 */
class DiscoveredEmpeg : public CountedObject
{
public:
    enum ConnectionType {
	ctUnknown   = 0,
	ctSerial    = 1,
	ctUsb	    = 2,
	ctEthernet  = 3,
    };

public:
    typedef CountedPointer<DiscoveredEmpeg> Pointer;
    static Pointer Create(const std::string & name, const std::string & location, const std::string & playerType, ConnectionType connectionType, Connection *connection);

    std::string GetName() const { return m_name; }
    std::string GetLocation() const { return m_location; }
    std::string GetPlayerType() const { return m_playerType; }
    ConnectionType GetConnectionType() const { return m_connectionType; }
    Connection *GetConnection() { return m_connection; }

    bool operator==(const DiscoveredEmpeg & other) const
    {
	return m_name == other.m_name &&
		m_location == other.m_location &&
		m_connectionType == other.m_connectionType;
    }

    ~DiscoveredEmpeg();

private:
    std::string m_name;
    std::string m_location;
    std::string m_playerType;
    ConnectionType m_connectionType;
    Connection *m_connection;

private:
    DiscoveredEmpeg(const std::string & name, const std::string & location, const std::string & playerType, ConnectionType connectionType, Connection *connection)
	: m_name(name), m_location(location), m_playerType(playerType), m_connectionType(connectionType), m_connection(connection)
    {
    }

    DiscoveredEmpeg(const DiscoveredEmpeg & rhs);   // prevent copying - no implementation.
};

typedef DiscoveredEmpeg::Pointer DiscoveredEmpegPtr;

/** Base class for the other Discoverers
 */
class EmpegDiscoverer
{
    EmpegDiscovererObserver *observer;

public:
    EmpegDiscoverer();
    virtual ~EmpegDiscoverer() {}
    void Attach(EmpegDiscovererObserver *o);

    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue = NULL, bool *pbIgnoreFailure = NULL) = 0;

    struct DefaultNames
    {
	std::string empeg_car1;
	std::string empeg_car2;
	std::string rio_car;
	std::string jupiter;
	std::string unknown;
    };
    inline void SetDefaultNames(DefaultNames names) { m_defaultNames = names; }

protected:
    bool IsEmpegConnected(Connection *connection);
    std::string GetEmpegName(Connection *connection, const std::string &defaultName);
    std::string GetDefaultName(const std::string &playerType);
    std::string GetPlayerType(Connection *connection);
    DefaultNames m_defaultNames;
    bool FireDiscoveredEmpeg(DiscoveredEmpegPtr discoveredEmpeg);
};

/** Observer interface.  Called for each unit found.
 */
struct EmpegDiscovererObserver
{
    // return true to keep looking.
    virtual bool OnDiscoveredEmpeg(DiscoveredEmpegPtr discoveredEmpeg) = 0;

    virtual ~EmpegDiscovererObserver() {}
};

/** Used to composite multiple discoverers.
 */
class CompoundEmpegDiscoverer : public EmpegDiscoverer, public EmpegDiscovererObserver
{
    typedef std::vector<EmpegDiscoverer *> DiscovererVec;
    DiscovererVec discoverers;

public:
    void AddDiscoverer(EmpegDiscoverer *empegDiscoverer);

// EmpegDiscoverer
public:
    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue = NULL, bool *pbIgnoreFailure = NULL);

// EmpegDiscovererObserver
public:
    virtual bool OnDiscoveredEmpeg(DiscoveredEmpegPtr discoveredEmpeg)
    { return FireDiscoveredEmpeg(discoveredEmpeg); }
};

#endif /* DISCOVERY_H */
