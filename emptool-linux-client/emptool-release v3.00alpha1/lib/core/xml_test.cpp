/* xml_test.cpp
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
#include "xml.h"
#include "stream_in_memory.h"
#include "empeg_error.h"

class MyXmlObserver: public util::XMLObserver
{
    int m_indent;

public:
    MyXmlObserver();

    STATUS OnBeginEntry(const char*, const util::xmlattributes_t&);
    STATUS OnEndEntry(const char*);
    STATUS OnBeginTitle(const char*, const util::xmlattributes_t&);
    STATUS OnEndTitle(const char*);
    STATUS OnBeginAsx(const char*, const util::xmlattributes_t&);
    STATUS OnEndAsx(const char*);

    STATUS OnContent(const std::string&);
    STATUS OnBeginUnknown(const char*);
    STATUS OnEndUnknown(const char*);
};

static const util::xmltag_t<MyXmlObserver> tags[] = {
    { "asx",   &MyXmlObserver::OnBeginAsx,   &MyXmlObserver::OnEndAsx },
//    { "entry", &MyXmlObserver::OnBeginEntry, &MyXmlObserver::OnEndEntry },
//    { "title", &MyXmlObserver::OnBeginTitle, &MyXmlObserver::OnEndTitle },
};

MyXmlObserver::MyXmlObserver()
 : m_indent(0)
{
}

STATUS MyXmlObserver::OnBeginAsx(const char*, const util::xmlattributes_t& attr)
{
    if (!attr.empty())
    {
	printf("<ASX\n");
	for (util::xmlattributes_t::const_iterator i = attr.begin();
	     i != attr.end();
	     ++i)
	{
	    printf("    '%s'='%s'\n", i->first.c_str(), i->second.c_str());
	}
	printf(">\n");
    }
    else
	printf("<ASX>\n");
    return S_OK;
}

STATUS MyXmlObserver::OnEndAsx(const char*)
{
    printf("</ASX>\n");
    return S_OK;
}

STATUS MyXmlObserver::OnContent(const std::string&s)
{
    printf("Content: '%s'\n", s.c_str());
    return S_OK;
}

STATUS MyXmlObserver::OnBeginUnknown(const char *s)
{
    printf("Unknown <%s>\n", s);
    return S_OK;
}

STATUS MyXmlObserver::OnEndUnknown(const char *s)
{
    printf("Unknown </%s>\n", s);
    return S_OK;
}

#if defined(TEST)
int main(void)
{
    MyXmlObserver obs;

    util::XMLParser<MyXmlObserver> parser(tags, sizeof(tags)/sizeof(*tags),
					  &obs);

    StreamInMemory sim;

    const char xml[] = "<ASX aubergine = \"small\">Foo<entry/></ASX>\n"
	"<ASX x=\"y\"/>";
    unsigned int wrote;
    sim.Write(xml, sizeof(xml), &wrote);
    sim.SeekAbsolute(0);

    parser.Parse(&sim);

    return 0;
}
#endif
