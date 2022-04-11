/* xml.cpp
 *
 * Simple XML parser
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "xml.h"
#include "stream.h"
#include "stringpred.h"
#include "stringops.h"
#include "empeg_error.h"

#define TRACE_XML 0

enum {
    CONTENT = 0,
    BEGIN_TAG,
    IN_TAG,
    IN_END_TAG,
    IN_ATTRWS,
    IN_ATTR,
    IN_ATTR_WAITEQ,
    IN_ATTR_WAITVALUE,
    IN_ATTR_VALUE
};

static const char tagchars[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
".-_:";

/** Simple XML parser, doesn't cope with entities
 *
 * @todo Character entities
 */
STATUS util::XMLParserBase::Parse(Stream *stm)
{
    int state = CONTENT;
    unsigned int pos = 0;
    std::string current_tag;
    std::string current_attr;
    xmlattributes_t attrs;
    bool end_tag = false;

    for (;;)
    {
	STATUS st = ReadData(stm);

	if (FAILED(st) || pos == m_buffer.size())
	{
	    TRACEC(TRACE_XML, "End of stream in state %d\n", state);
	    return st;
	}

	while (pos < m_buffer.size())
	{
	    //TRACEC(TRACE_XML, "state %d pos=%d sz=%d\n", state, pos, m_buffer.size());

	    switch (state)
	    {
		/*  "edededed<foo>"
		 *     ^
		 */
	    case CONTENT:
	    {
		unsigned int find_lt = m_buffer.find('<', pos);
		if (find_lt == std::string::npos)
		{
		    pos = m_buffer.size();
		    break;
		}

		if (find_lt>0)
		{
		    std::string content(m_buffer,0,find_lt);
		    m_observer->OnContent(content);
		}
		m_buffer.erase(0, find_lt+1);
		pos = 0;
		state = BEGIN_TAG;
		break;
	    }

	    /*  "ededed<foo>"
	     *          ^
	     */
	    case BEGIN_TAG:
		attrs.clear();
		if (m_buffer[pos] == '/')
		{
		    m_buffer.erase(0,1);
		    pos = 0;
		    end_tag = true;
		}
		else
		    end_tag = false;

		state = IN_TAG;
		break;

		/*  "ededed<foo>"
		 *           ^
		 */
	    case IN_TAG:
	    {
		unsigned int find_end = m_buffer.find_first_not_of(tagchars,
								   pos);
		if (find_end == std::string::npos)
		{
		    pos = m_buffer.size();
		    break;
		}
		current_tag = stringops::LowerCase(std::string(m_buffer, 0, find_end));
		TRACEC(TRACE_XML, "Tag '%s'\n", current_tag.c_str());
		m_buffer.erase(0, find_end);
		pos = 0;
		state = IN_ATTRWS;
		break;
	    }

	    /*  "ededed<foo bar=aubergine>"
	     *             ^
	     */
	    case IN_ATTRWS:
		if (m_buffer[pos] == '>')
		{
		    st = DoTag(current_tag, attrs, end_tag);
		    if (FAILED(st))
			return st;
		    m_buffer.erase(0,pos+1);
		    pos = 0;
		    state = CONTENT;
		}
		else if (m_buffer[pos] == '/')
		{
		    st = DoTag(current_tag, attrs, end_tag);
		    if (FAILED(st))
			return st;
		    m_buffer.erase(0,pos+1);
		    pos = 0;
		    end_tag = true;
		    // stay in ATTRWS ready for the '>'
		    break;
		}
		else if (strchr(tagchars, m_buffer[pos]))
		{
		    // Beginning of an attribute
		    m_buffer.erase(0,pos);
		    pos = 0;
		    state = IN_ATTR;
		}
		else
		{
		    // Skip (probably ws)
		    pos++;
		}
		break;

		/*  "eded<foo bar=aubergine>"
		 *             ^
		 */
	    case IN_ATTR:
	    {
		unsigned int find_end = m_buffer.find_first_not_of(tagchars);
		if (find_end == std::string::npos)
		{
		    pos = m_buffer.size();
		    break;
		}
		current_attr = std::string(m_buffer, 0, find_end);
		m_buffer.erase(0,find_end+1);
		pos = 0;
		state = IN_ATTR_WAITEQ;
		break;
	    }

	    /* "eded<foo bar = aubergine>"
	     *              ^
	     * The XML specification doesn't allow ws here, but all Microsoft's
	     * example ASXes have some. Grrr.
	     */
	    case IN_ATTR_WAITEQ:
		if (isspace(m_buffer[pos]))
		{
		    pos++;
		    break;
		}
		else if (m_buffer[pos] == '=')
		{
		    m_buffer.erase(0,pos+1);
		    pos = 0;
		    state = IN_ATTR_WAITVALUE;
		    break;
		}

		state = IN_ATTRWS;
		// Do not advance 'pos'
		break;
		
		/* eded<foo bar = "aubergine">
		 *               ^
		 */
	    case IN_ATTR_WAITVALUE:
		if (isspace(m_buffer[pos]))
		{
		    pos++;
		    break;
		}
		else if (m_buffer[pos] == '\"')
		{
		    m_buffer.erase(0,pos+1);
		    pos = 0;
		    state = IN_ATTR_VALUE;
		    break;
		}
		state = IN_ATTRWS;
		break;

		/* eded<foo bar = "aubergine"> 
		 *                   ^
		 */
	    case IN_ATTR_VALUE:
	    {
		unsigned int find_end = m_buffer.find('\"');
		if (find_end == std::string::npos)
		{
		    pos = m_buffer.size();
		    break;
		}
		
		std::string value(m_buffer,0,find_end);
		TRACEC(TRACE_XML, "  <%s> = <%s>\n", current_attr.c_str(), value.c_str());
		attrs[stringops::LowerCase(current_attr)] = value;
		m_buffer.erase(0,find_end+1);
		pos = 0;
		state = IN_ATTRWS;
		break;
	    }
	    default:
		ASSERT(false);
		break;
	    }
	}
    }
}

STATUS util::XMLParserBase::ReadData(Stream *stm)
{
    const int LUMP = 2048;

    char buffer[LUMP];

    unsigned int nread;

    STATUS st = stm->Read(buffer, LUMP, &nread);
    if (FAILED(st))
	return st;

    if (!nread)
	return S_FALSE;

    m_buffer.append(buffer, nread);

    return S_OK;
}

namespace util {

class TagEqPredicate
{
    const char *m_tag;
public:
    TagEqPredicate(const char *tag) : m_tag(tag) {}
    bool operator()(const xmltag_t<XMLObserver>& t)
	{
	    return !empeg_stricmp(m_tag, t.tag);
	}
};

};

STATUS util::XMLParserBase::DoTag(const std::string& tag,
				  const xmlattributes_t& attr, bool end_tag)
{
    TagEqPredicate teq(tag.c_str());

    const DummyTag *ptr = std::find_if(m_tags, m_tags + m_nTags, teq);
		
    if (ptr == m_tags + m_nTags)
    {
	// Not found
	if (end_tag)
	    return m_observer->OnEndUnknown(tag.c_str());

	return m_observer->OnBeginUnknown(tag.c_str());
    }

    if (end_tag)
    {
	if (!ptr->end_fn)
	    return S_OK;
	return TagEnd(ptr, tag.c_str());
    }
    
    if (!ptr->begin_fn)
	return S_OK;

    return TagBegin(ptr, tag.c_str(), attr);
}
