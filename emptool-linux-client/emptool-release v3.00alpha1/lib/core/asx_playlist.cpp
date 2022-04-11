/* asx_playlist.cpp
 *
 * Windows Media Player XML playlists
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "asx_playlist.h"
#include "xml.h"
#include "empeg_error.h"

class ASXObserver: public util::XMLObserver
{
    bool m_inEntry; // are we somewhere inside an entry?
    std::vector<std::string> *m_entries;
    std::string m_title;

public:
    ASXObserver(std::vector<std::string> *entries)
	: m_entries(entries) {}

    std::string GetTitle() const { return m_title; }

    STATUS OnBeginEntry(const char*, const util::xmlattributes_t&);
    STATUS OnEndEntry(const char*);
    STATUS OnBeginParam(const char*, const util::xmlattributes_t&);
    STATUS OnBeginRef(const char*, const util::xmlattributes_t&);

    STATUS OnContent(const std::string&);
    STATUS OnBeginUnknown(const char*);
    STATUS OnEndUnknown(const char*);
};


STATUS ASXObserver::OnBeginEntry(const char*, const util::xmlattributes_t&)
{
    m_inEntry = true;
    return S_OK;
}

STATUS ASXObserver::OnEndEntry(const char*)
{
    m_inEntry = false;
    return S_OK;
}

STATUS ASXObserver::OnBeginParam(const char*, const util::xmlattributes_t& attr)
{
    if (m_inEntry)
	return S_OK;

    util::xmlattributes_t::const_iterator i = attr.find("name");
    if (i != attr.end())
	m_title = i->second;

    return S_OK;
}

STATUS ASXObserver::OnBeginRef(const char*, const util::xmlattributes_t& attr)
{
    util::xmlattributes_t::const_iterator i = attr.find("href");
    if (i != attr.end())
	m_entries->push_back(i->second);

    return S_OK;
}

STATUS ASXObserver::OnContent(const std::string&)
{
    return S_OK;
}

STATUS ASXObserver::OnBeginUnknown(const char*)
{
    return S_OK;
}

STATUS ASXObserver::OnEndUnknown(const char*)
{
    return S_OK;
}

static const util::xmltag_t<ASXObserver> tags[] = {
    { "entry", &ASXObserver::OnBeginEntry, &ASXObserver::OnEndEntry }, 
    { "param", &ASXObserver::OnBeginParam, NULL },
    { "ref",   &ASXObserver::OnBeginRef,   NULL },
};

STATUS ASXPlaylist::FromStream(Stream *stm)
{
    m_entries.clear();
    ASXObserver obs(&m_entries);
    util::XMLParser<ASXObserver> parser(tags, sizeof(tags)/sizeof(*tags), &obs);
    STATUS st = parser.Parse(stm);
    if (FAILED(st))
	return st;
    m_title = obs.GetTitle();
    return S_OK;
}

#if defined(TEST)
int main(void)
{
    /** @todo Some tests, perhaps? */
    return 0;
}
#endif
