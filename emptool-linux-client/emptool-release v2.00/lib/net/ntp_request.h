/* IpAddrTable.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Richard Nixon <rnixon@riohome.com>
 */

#ifndef NTP_REQUEST_H
#define NTP_REQUEST_H

#include "socket.h"

class NtpRequest
{
private:
    std::string m_server;
    unsigned m_timeout;
    
    DatagramSocket m_socket;
    
public:
    NtpRequest(const std::string &server_name, unsigned timeout = 10);

    ~NtpRequest();

    STATUS Create();

    void Close();

    STATUS GetUTCTime(time_t *result);    
    
    void SetTimeout(unsigned timeout) { m_timeout = timeout; }
};

#endif // NTP_REQUEST_H
