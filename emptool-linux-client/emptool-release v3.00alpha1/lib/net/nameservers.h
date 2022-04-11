/* nameservers.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 */

#ifndef NAMESERVERS_H
#define NAMESERVERS_H

#include <map>
#include <string>

#ifndef IPADDRESS_H
#include "ipaddress.h"
#endif

#include "singleton.h"

namespace net {

    /** Maintains the list of currently-available nameservers, as indicated by
     * DHCP and/or PPP servers. Handles arrival and departure of interfaces.
     * This works by rewriting /etc/hosts.conf, so, in order for this to work
     * on a player, /etc/hosts.conf must be (a symlink to) somewhere writable.
     */
    class NameServers : public Singleton<NameServers>
    {
	typedef std::basic_string<IPAddress> string_t;
	typedef std::map<unsigned int,string_t> map_t;
	map_t m_servers;

	STATUS ReinitialiseDNS();
	
    public:
	NameServers();
	~NameServers();

	/** Add a collection of nameservers, at a given priority. Lower numbers
	 * are higher priority (i.e. asked first). This function rewrites
	 * /etc/hosts.conf and calls res_init, so don't call it on your PC
	 * unless you know what you're doing.
	 */
	STATUS Add(unsigned int priority, string_t servers);

	/** Remove all nameservers at this priority level (e.g. when ppp goes
	 * away). This function rewrites /etc/hosts.conf and calls res_init, 
	 * so don't call it on your PC unless you know what you're doing.
	 */
	STATUS Remove(unsigned int priority);

	/** Suggested priorities for Add and Remove */
	enum {
	    PPP_PRIORITY = 0,
	    ETHERNET_PRIORITY = 100 /* plus your interface number */
	};

	friend NameServers *GetNameServers();
    };

    inline NameServers *GetNameServers() { return NameServers::GetInstance(); }

}; // namespace net

#endif
