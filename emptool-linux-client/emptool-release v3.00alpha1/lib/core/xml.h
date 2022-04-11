/* xml.h
 *
 * Simple XML parser
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 */

#ifndef XML_H
#define XML_H

#include <string>
#include <map>

#ifndef EMPEG_STATUS_H
#include "empeg_status.h"
#endif

class Stream;

namespace util
{

typedef std::map<std::string, std::string> xmlattributes_t;

template <class observer_t>
struct xmltag_t
{
    const char *tag;
    STATUS (observer_t::*begin_fn)(const char *tag, const xmlattributes_t&);
    STATUS (observer_t::*end_fn)(const char *tag);
};

class XMLObserver
{
 public:
    virtual ~XMLObserver() {}
    virtual STATUS OnContent(const std::string& content) = 0;
    virtual STATUS OnBeginUnknown(const char *tag) = 0;
    virtual STATUS OnEndUnknown(const char *tag) = 0;
};

class XMLParserBase
{
    std::string m_buffer;
    STATUS ReadData(Stream *stm);
    STATUS DoTag(const std::string& tag, const xmlattributes_t& attr, 
		 bool end_tag);
    
 protected:
    typedef xmltag_t<XMLObserver> DummyTag;

    XMLObserver *m_observer;
    const DummyTag *m_tags;
    unsigned int m_nTags;

    XMLParserBase(const DummyTag *ptags, unsigned int ntags,
		  XMLObserver *observer)
	: m_observer(observer), m_tags(ptags), m_nTags(ntags) {}

    virtual ~XMLParserBase() {}

    virtual STATUS TagBegin(const DummyTag*, const char *tagtext,
			    const xmlattributes_t&) = 0;
    virtual STATUS TagEnd(const DummyTag*, const char *tagtext) = 0;

 public:
    STATUS Parse(Stream *stm);
};

/** Lightweight (inlinable) templated class based on the untemplated parent.
 * We cast the observer and tag structure passed in, to generic (wrong) types,
 * but then we cast them back again (inside this class) before using them.
 * Nobody outside this class sees anything nontypesafe.
 */
template <class T>
class XMLParser: public XMLParserBase
{
 public:
    XMLParser(const xmltag_t<T> *tags, unsigned int ntags, T *observer)
	: XMLParserBase((const DummyTag*)tags, ntags, observer)
	{}

 private:
    /** Given an xmltag_t of the wrong type, cast it to the right type and call
     * the member function pointer it contains.
     *
     * This function looks atrocious so your code doesn't have to.
     */
    STATUS TagBegin(const DummyTag *tag, const char *tagtext,
		    const xmlattributes_t& attr)
	{ return ((T*)m_observer->*(((xmltag_t<T>*)tag)->begin_fn))(tagtext, attr); }
    STATUS TagEnd(const DummyTag *tag, const char *tagtext)
	{ return ((T*)m_observer->*(((xmltag_t<T>*)tag)->end_fn))(tagtext); }
};

}; // namespace util

#endif
