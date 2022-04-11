/* serial_discovery_win32.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.13 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "serial_discovery.h"
#include "protocol/protocolclient.h"
#define EMPEG_BAUD_RATE 115200

Win32SerialEmpegDiscoverer::Win32SerialEmpegDiscoverer(int portNum)
    : portNum(portNum)
{
}

HRESULT Win32SerialEmpegDiscoverer::Discover(unsigned timeout_ms, bool *pbContinue, bool* /*pbIgnoreFailure*/)
{
    // Open a connection object on the serial port, and see if there's anything there...
    char device[25];
    sprintf(device, "\\\\.\\COM%d", portNum);
    SerialConnection *connection = NEW SerialConnection(device, EMPEG_BAUD_RATE);
    if(!connection)
	return E_OUTOFMEMORY;

    if(IsEmpegConnected(connection))
    {
	std::string playerType(GetPlayerType(connection));
	std::string name(GetEmpegName(connection, GetDefaultName(playerType)));
	char where[25];
	sprintf(where, "COM%d", portNum);
	
	bool cont = FireDiscoveredEmpeg(DiscoveredEmpeg::Create(name, where, playerType, DiscoveredEmpeg::ctSerial, connection));
	if(!cont && pbContinue)
	    *pbContinue = false;
    }
    else
	delete connection;

    return S_OK;
}
