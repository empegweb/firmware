/* ogg.cpp
 * 
 * TagExtractor for Ogg Vorbis files
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
#include "ogg.h"
#include "utf8.h"
#include "filename_to_title.h"
#include "rid.h"
#include "vorbis/ivorbisfile.h"
#include "stringops.h"

#define TRACE_OGGEXTRACT 1

namespace tags {

STATUS OggTagExtractor::Create(const tstring& filename, TagExtractor **pp)
{
    *pp = NEW OggTagExtractor(filename);
    return S_OK;
}

static size_t ogg_read_func(void *ptr, size_t size, size_t nmemb,
			    void *handle)
{
    SeekableStream *stm = (SeekableStream*)handle;

    unsigned int nread = 0;
    STATUS st = stm->Read(ptr, size*nmemb, &nread);

    if (FAILED(st))
    {
	TRACEC(TRACE_OGGEXTRACT, "ogg_read_func failed (%x)\n",
	       PrintableStatus(st));
    }
    return nread/size;
}

static int ogg_seek_func(void *handle, ogg_int64_t offset, int whence)
{
    SeekableStream *stm = (SeekableStream*)handle;
    STATUS st;
    switch (whence)
    {
    case SEEK_SET:
	st = stm->SeekAbsolute((unsigned) offset);
	break;
    case SEEK_CUR:
	st = stm->SeekRelative((int) offset);
	break;
    default: // SEEK_END, we hope
    {
	unsigned int len = 0;
	stm->Length(&len);
	st = stm->SeekAbsolute(len + (int)offset);
    }
	break;
    }

    if (FAILED(st))
    {
	TRACEC(TRACE_OGGEXTRACT, "ogg_seek_func(%d) failed (%x)\n", whence,
	       PrintableStatus(st));
	return -1;
    }
    return 0;
}

static int ogg_close_func(void*)
{
    // We don't actually want Ogg to close our stream for us
    return 0;
}

static long ogg_tell_func(void *handle)
{
    SeekableStream *stm = (SeekableStream*)handle;
    unsigned int pos = 0;
    STATUS st = stm->Tell(&pos);
    if (FAILED(st))
    {
	TRACEC(TRACE_OGGEXTRACT, "ogg_seek_func failed (%x)\n",
	       PrintableStatus(st));
    }
    return (long)pos;
}

static const ov_callbacks ogg_stream_callbacks = {
    &ogg_read_func,
    &ogg_seek_func,
    &ogg_close_func,
    &ogg_tell_func
};

STATUS OggTagExtractor::ExtractTags(SeekableStream *stm, 
				    TagExtractorObserver *teo)
{
    OggVorbis_File ovf;

    int rc = ov_open_callbacks((void*)stm, &ovf, NULL, 0, ogg_stream_callbacks);
    if (rc < 0)
    {
	TRACEC(TRACE_OGGEXTRACT, "%s is not an ogg (%d)\n",
	       m_filename.c_str(), rc);
	return E_CORRUPTED;
    }

    vorbis_comment *vc = ov_comment(&ovf, -1);

    if (!vc)
    {
	TRACEC(TRACE_OGGEXTRACT, "No vorbis comment\n");
	teo->OnExtractTag("title", 
			  util::UTF8FromT(TranslateFilenameToTitle(m_filename)).c_str());
    }
    else
    {
	char **ptr = vc->user_comments;

	while (*ptr)
	{
	    char *eq = strchr(*ptr, '=');
	    if (eq)
	    {
		std::string name(*ptr, eq);
		name = stringops::LowerCase(name);

		if (name == "album")
		    name = "source";
		if (name == "tracknumber")
		    name = "tracknr";
		if (name == "version")
		    name = "subtitle";
		teo->OnExtractTag(name.c_str(), eq+1); // Already in UTF-8
	    }
	    else
	    {
		TRACEC(TRACE_OGGEXTRACT, "Funny comment line '%s'\n", *ptr);
	    }
	    ptr++;
	}
    }

    /* Must set these must-be-correct tags AFTER reading arbitrary tags from
     * the vorbis_comment.
     */
    teo->OnExtractTag("codec", "vorbis");
    teo->OnExtractTag("offset", 0);
    teo->OnExtractTag("trailer", 0);

    vorbis_info *vi = ov_info(&ovf, -1);
    if (!vi)
    {
	TRACEC(TRACE_OGGEXTRACT, "No vorbis_info\n");
    }
    else
    {
	std::string bitrate;
	teo->OnExtractTag("samplerate", vi->rate);

	if (vi->bitrate_upper == vi->bitrate_lower
	    && vi->bitrate_upper > 0)
	{
	    // CBR
	    bitrate = VarString::Printf("f%c%ld",
					(vi->channels > 1) ? 's' : 'm',
					vi->bitrate_upper/1000);
	}
	else if (vi->bitrate_nominal > 0)
	{
	    // VBR
	    bitrate = VarString::Printf("v%c%ld",
					(vi->channels > 1) ? 's' : 'm',
					vi->bitrate_nominal/1000);
	}
	else if (vi->bitrate_upper > 0 && vi->bitrate_lower > 0)
	{
	    // VBR, guess average
	    bitrate = VarString::Printf("v%c%ld",
					(vi->channels > 1) ? 's' : 'm',
					(vi->bitrate_upper+vi->bitrate_lower)/2000);
	}
	// else we don't know

	if (!bitrate.empty())
	    teo->OnExtractTag("bitrate", bitrate.c_str());
    }

    ogg_int64_t t_total = ov_time_total(&ovf, -1);
    teo->OnExtractTag("duration", t_total);

    /** Now calculate the unique ID. Like WMA, we can't easily omit the tags
     * from the calculation :-(
     */
    unsigned int length;
    if (SUCCEEDED(stm->Length(&length)))
    {
	std::string rid;
    STATUS st = ExtractRidSpecific(stm, length, &rid);
	if (FAILED(st))
	    return st;
	teo->OnExtractTag("rid", rid.c_str());
    }

    ov_clear(&ovf);
    
    TRACEC(TRACE_OGGEXTRACT, "Extraction apparently successful\n");

    return S_OK;
}

STATUS OggTagExtractor::ExtractRid(SeekableStream *pStm, std::string *rid)
{
    STATUS status;
    unsigned int length;
    if (FAILED(status = pStm->Length(&length)))
        return status;
    return ExtractRidSpecific(pStm, length, rid);
}

STATUS OggTagExtractor::ExtractRidSpecific(SeekableStream *pStm, unsigned int length, std::string *rid)
{
    return CalculateRid(pStm, 0, length, rid);
}

}; // namespace tags
