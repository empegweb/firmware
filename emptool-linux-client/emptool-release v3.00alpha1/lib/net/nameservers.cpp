/* nameservers.cpp
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 */

#include "config.h"
#include "trace.h"

#include "nameservers.h"
#include "empeg_error.h"
#include <stdio.h>

/** The glibc-2.2.4 <resolv.h>, or at least <arpa/nameser_compat.h> which it
 * unconditionally includes, #defines STATUS. As you might imagine, this breaks
 * everything. So we don't include <resolv.h> until we've finished declaring
 * things that use STATUSes. This function is a wrapper for the single function
 * from <resolv.h> that we use.
 */
static void empeg_res_init();

namespace net {
    
NameServers::NameServers()
{
}

NameServers::~NameServers()
{
}

STATUS NameServers::Add(unsigned int priority, string_t servers)
{
    if (m_servers[priority] != servers)
    {
	m_servers[priority] = servers;
	return ReinitialiseDNS();
    }
    return S_OK;
}

STATUS NameServers::Remove(unsigned int priority)
{
    if (m_servers.find(priority) != m_servers.end())
    {
	m_servers.erase(priority);
	return ReinitialiseDNS();
    }
    return S_OK;
}

STATUS NameServers::ReinitialiseDNS()
{
    TRACE("Rewriting resolv.conf\n");

    FILE *f = fopen("/etc/resolv.conf", "w");
    if (!f)
    {
	TRACE_WARN("Cannot open /etc/resolv.conf for writing\n");
	return MakeErrnoStatus();
    }

    for (map_t::iterator i = m_servers.begin(); i != m_servers.end(); ++i)
    {
	const string_t& addresses = i->second;
	for (string_t::const_iterator j = addresses.begin(); 
	     j != addresses.end();
	     ++j)
	{
	    TRACE("DNS server priority %3d ip %s\n", i->first,
		  j->ToString().c_str());
	    fprintf(f, "nameserver %s\n", j->ToString().c_str());
	}
    }

    fclose(f);

    empeg_res_init();

    return S_OK;
}

} // namespace net

/* mseymour 21/02/03: Our version of eCos doesn't include the resolver
 * library. Apparently it exists in a separate DNS package CYGPKG_NS_DNS.
 */
#ifdef ECOS

static void empeg_res_init() {}

#else

#include <resolv.h> /* see comment above on empeg_res_init */

static void empeg_res_init()
{
    res_init();
}

#endif // defined(ECOS)

/* eof */
