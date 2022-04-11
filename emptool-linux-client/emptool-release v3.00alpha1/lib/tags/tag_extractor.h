/* tag_extractor.h
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.17 13-Mar-2003 18:15 rob:)
 */

#ifndef TAG_EXTRACTOR_H
#define TAG_EXTRACTOR_H 1

#include "empeg_error.h"
#include "stream.h"
#include "types.h"
#include "empeg_tchar.h"
#include "var_string.h"
#include <map>

class TagExtractorObserver;
 
/** Extract the tags from media files.  It's actually implemented
 * in terms of derived classes ID3TagExtractor, ASFTagExtractor,
 * etc.. There's a virtual constructor/factory idiom going on
 * here -- TagExtractor::Create looks at the filename and creates
 * the most relevant derived class.
 *
 * Both Create and ExtractTags take the filename.  Create uses it to:
 * - figure out which extractor to use
 * - figure out the default title
 *
 * ExtractTags uses it to create a SeekableStream, which is passed to
 * the other overload of ExtractTags.
 *
 * @todo If we could use the SeekableStream to figure out which
 * extractor to use, we'd only need to pass the default title.  This would
 * make it a bit more sensible to use.
 *
 * It is possible for ExtractTags to call the observer multiple times with
 * the same tag but a different value. In these circumstances the last 
 * value returned is more likely to be correct. :-)
 *
 */
class TagExtractor
{
public:
    virtual ~TagExtractor();
    virtual STATUS ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver) = 0;
    virtual STATUS ExtractTags(const tstring &filename, TagExtractorObserver *pObserver);

    virtual STATUS ExtractRid(SeekableStream *pStm, std::string *rid) = 0;
    virtual STATUS ExtractRid(const tstring &filename, std::string *rid);
    
public:
    static STATUS Create(const tstring &filename, const tstring &suffix, TagExtractor **ppExtractor);
};

class TagExtractorObserver
{
public:
    virtual ~TagExtractorObserver() {}
    virtual void OnExtractTag(const char *tagName, const UTF8CHAR *tagValue) = 0;
    virtual void OnExtractTag(const char *tagName, int tagValue) = 0;
};

class TagExtractToMap: public TagExtractorObserver,
		       public std::map<std::string, utf8_string>
{
public:
    void OnExtractTag(const char *name, const UTF8CHAR *value)
    {
	(*this)[name] = value;
    }
    void OnExtractTag(const char *name, int value)
    {
	(*this)[name] = VarString::Printf("%d", value);
    }
};

#endif /* TAG_EXTRACTOR_H */
