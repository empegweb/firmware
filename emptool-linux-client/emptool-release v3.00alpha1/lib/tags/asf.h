/* asf.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.21 13-Mar-2003 18:15 rob:)
 */

#ifndef ASF_H
#define ASF_H 1

#include "tag_extractor.h"
#include "asf_format.h"
#include <string>

class ASFTagExtractor : public TagExtractor
{
    tstring m_filename;
    bool m_foundContentDescription;

    STATUS ExtractRidSpecific(SeekableStream *pStm, unsigned int length, std::string *rid);

public:
    ASFTagExtractor(const tstring &filename)
	: m_filename(filename) { }

    /** If this fails, the value of *pp is undefined. */
    static STATUS Create(const tstring &filename, TagExtractor **pp);
    virtual STATUS ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver);
    virtual STATUS ExtractRid(SeekableStream *pStm, std::string *rid);

private:
    struct Dispatch {
	typedef STATUS (ASFTagExtractor::*PFN) (SeekableStream *pStm, const WMA_object *o, TagExtractorObserver *pObserver);

	const GUID *pGUID;
	PFN pfn;
    };
    
    static const Dispatch guid_dispatch[];

    STATUS ExtractContentDescriptionObject(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);
    STATUS ExtractContentDescriptionObject(const WMA_content_description_object *contentDescription,
				    TagExtractorObserver *pObserver);

    STATUS ExtractPropertiesObject(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);
    STATUS ExtractPropertiesObject(const WMA_properties_object *contentDescription,
				    TagExtractorObserver *pObserver);

    STATUS ExtractStreamPropertiesObject(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);

    STATUS ExtractUnknownDRM(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);
    
    STATUS ExtractTextTags(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);
    
    STATUS ExtractUnknownText2(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);

    STATUS ExtractUnknownNumbers(SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);
    
    STATUS ExtractUnknownObject (SeekableStream *pStm, const WMA_object *o,
				    TagExtractorObserver *pObserver);

    void FireExtractTextTag(TagExtractorObserver *pObserver, const utf16_string &tag,
			const utf16_string &value);

    void FireExtractDWORDTag(TagExtractorObserver *pObserver, const utf16_string &tag,
			DWORD value);

    void FireExtractTag(TagExtractorObserver *pObserver, const char *tagName,
			const utf16_string &tagValue);
    
    void FireExtractTag(TagExtractorObserver *pObserver, const char *tagName,
			DWORD tagValue);
};

#endif /* ASF_H */
