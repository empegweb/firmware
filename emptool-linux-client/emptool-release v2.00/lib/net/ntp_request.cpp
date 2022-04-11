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

#include "config.h"
#include "trace.h"
#include "ntp_request.h"
#include "net_errors.h"

#define TRACE_NTP_REQUEST   0

#define NTP_PORT 123    // Honestly!

/* from RFC 1361
                           1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                    Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Reference Timestamp (64)                    |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Originate Timestamp (64)                    |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Receive Timestamp (64)                     |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Transmit Timestamp (64)                    |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                                                               |
      |                  Authenticator (optional) (96)                |
      |                                                               |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct ntp_packet
{
    unsigned char li_vn_mode;   // A combination of three fields
    unsigned char stratum;
    unsigned char poll;
    unsigned char prec;
    unsigned long delay;
    unsigned long disp;
    unsigned long refid;
    unsigned long reftime[2];
    unsigned long origtime[2];
    unsigned long rcvtime[2];
    unsigned long txtime[2];
};

NtpRequest::NtpRequest(const std::string &server_name, unsigned timeout) :
    m_server(server_name),
    m_timeout(timeout)
{
}

NtpRequest::~NtpRequest()
{
    Close();    
}

STATUS NtpRequest::Create()
{
    return m_socket.Create();
}

void NtpRequest::Close()
{
    m_socket.Close();
}

STATUS NtpRequest::GetUTCTime(time_t *result)
{
    STATUS status;

    IPAddress addr;
    if (FAILED(status = addr.FromHostName(m_server)))
    {
        TRACEC(TRACE_NTP_REQUEST, "FromHostName failed, status = 0x%x\n", PrintableStatus(status));
        return status;
    }

    IPEndPoint ep(addr, NTP_PORT);
    
    struct ntp_packet packet;
    
    memset ((void*)&packet, 0, sizeof (packet));
    packet.li_vn_mode = 0x0b;    // version 1, mode 3 (client)
    
    if (FAILED(status = m_socket.SendTo((void*)&packet, sizeof (packet), ep)))
        return status;
    
    int selerr = m_socket.Select (m_timeout * 1000);
    if (selerr > 0)
    {
        int bytes;
        if (FAILED (status = m_socket.ReceiveFrom((void*) &packet, sizeof (packet), &bytes, &ep)))
            return status;
        else if (bytes != sizeof (packet))
            return E_NTP_REQUEST;        
    }
    else if (selerr == 0)       // Timeout
        return E_NTP_REQUEST;
    else
        return MakeErrnoStatus();

        
    if (((packet.li_vn_mode & 0xc0) == 0xc0) ||     // Control message - must have other details
        (packet.stratum == 0) ||                    // ... unspecified or unavailable
        (packet.txtime == 0))                       // ... must have this
        return E_NTP_REQUEST;

    //
    // NTP timestamps are represented as a 64-bit unsigned fixed-
    // point number, in seconds relative to 0h on 1 January 1900. The integer
    // part is in the first 32 bits and the fraction part in the last 32 bits.
    //
    // To convert to a time_t type reference subtract the number of days between 1/1/1900 -> 1/1/1970
    //

    *result = ntohl(packet.txtime[0]) - 2208988800UL;

    TRACEC(TRACE_NTP_REQUEST, "GetUTCTime result %lu\n", *result);
    
    return S_OK;
}
