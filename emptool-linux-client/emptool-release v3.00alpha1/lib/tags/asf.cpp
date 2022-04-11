/* asf.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.50 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#define INITGUID
#include "asf.h"
#include "tag_errors.h"
#if defined(WIN32)
 #include <malloc.h>
#endif
#include "asf_format.h"
#include "stringpred.h"
#include "file.h"
#include "var_string.h"
#include "rid.h"
#include <stdio.h>
#include "filename_to_title.h"
#include "utf8.h"

#define TRACE_TAGS 0

// Note that if this is enabled then we'll trace UTF-8 strings, if it's disabled then
// we'll trace ISO-8859-1 strings.
#define ENABLE_UTF8 1

STATUS ASFTagExtractor::Create(const tstring &filename, TagExtractor **pp)
{
    ASFTagExtractor *p = NEW ASFTagExtractor(filename);
    if (!p)
	return E_OUTOFMEMORY;

    *pp = p;
    return S_OK;
}

const ASFTagExtractor::Dispatch ASFTagExtractor::guid_dispatch[] =
{
    { &GUID_WMA_content_description_object, &ASFTagExtractor::ExtractContentDescriptionObject, },
    { &GUID_WMA_properties_object, &ASFTagExtractor::ExtractPropertiesObject, },
    { &GUID_WMA_unknown_drm_1, &ASFTagExtractor::ExtractUnknownDRM, },
    { &GUID_WMA_text_tags, &ASFTagExtractor::ExtractTextTags, },
    { &GUID_WMA_stream_properties_object, &ASFTagExtractor::ExtractStreamPropertiesObject, },

#if defined(ARCH_PC) && DEBUG>0
    { &GUID_WMA_unknown_text_2, &ASFTagExtractor::ExtractUnknownText2, },
    { &GUID_WMA_unknown_numbers_1, &ASFTagExtractor::ExtractUnknownNumbers, },
    { &GUID_WMA_clock_object, NULL, },
    { &GUID_WMA_data_section_object, NULL, },
#endif
};

#if ENABLE_UTF8
// Native is UTF-8
inline std::string ConvertUTF16ToNative(const utf16_string &s)
{
    return util::UTF8FromUTF16(s);
}
#else
// Native is ISO-8859-1
inline std::string ConvertUTF16ToNative(const utf16_string &s)
{
    return util::Latin1FromUTF16(s);
}
#endif // !ENABLE_UTF8

STATUS ASFTagExtractor::ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver)
{
    pObserver->OnExtractTag("codec", "wma");

    WMA_header_object headerObject;
    
    STATUS status = pStm->SeekAbsolute(0);
    if (FAILED(status))
	return status;

    unsigned bytesRead;
    status = pStm->Read(&headerObject, sizeof(headerObject), &bytesRead);
    if (FAILED(status))
	return status;

    if ((headerObject.o.guid != GUID_WMA_header_object) ||
	(headerObject.reserved1 != 0x01) ||
	(headerObject.reserved2 != 0x02))
    {
	return TAG_E_WMA_NOTASF;
    }
    
    TRACEC(TRACE_TAGS, "ASF Header Size:   %d\n", (unsigned int)headerObject.o.size);
    TRACEC(TRACE_TAGS, "Number of headers: %d\n", headerObject.number_headers);
    
    /// @todo Come up with somewhere sensible to put this
    m_foundContentDescription = false;
    UINT32 cObjects = 0;
    while (cObjects <= headerObject.number_headers)
    {
	unsigned pos;
	if (FAILED(status = pStm->Tell(&pos)))
	    return status;

	TRACEC(TRACE_TAGS, "Tell() = %d\n", pos);
	
	WMA_object o;
	if (FAILED(status = pStm->Read(&o, sizeof(WMA_object), &bytesRead)))
	    return status;

	TRACEC(TRACE_TAGS, "o.size = %" LONGLONGFMT "x\n", o.size);
	o.size = le64_to_cpu(o.size);
	TRACEC(TRACE_TAGS, "o.size = %" LONGLONGFMT "x\n", o.size);
	
	// Dispatch based on the GUID....
	TRACEC(TRACE_TAGS, " GUID: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
			    o.guid.Data1, o.guid.Data2, o.guid.Data3,
			    o.guid.Data4[0], o.guid.Data4[1],
			    o.guid.Data4[2], o.guid.Data4[3], o.guid.Data4[4],
			    o.guid.Data4[5], o.guid.Data4[6], o.guid.Data4[7]);
	
	bool recognised = false;
	for (size_t i = 0; i < COUNTOF(guid_dispatch); ++i)
	{
	    if (o.guid == *(guid_dispatch[i].pGUID))
	    {
		Dispatch::PFN f = guid_dispatch[i].pfn;
		if (f)
		{
		    STATUS status = (this->*f)(pStm, &o, pObserver);
		    if (FAILED(status))
			return status;
		}

		recognised = true;
		break;
	    }
	}

#if defined(ARCH_PC) && DEBUG>0
	if (!recognised)
	    ExtractUnknownObject(pStm, &o, pObserver);
#endif

	// Skip to the next item, based on the size.
	TRACEC(TRACE_TAGS, "o.size = %" LONGLONGFMT "x\n", o.size);
	pos += (UINT32)o.size;
	TRACEC(TRACE_TAGS, "Seeking to %d\n", pos);
	pStm->SeekAbsolute(pos);
	++cObjects;
    }

    if (!m_foundContentDescription)
    {
	// We need to fake out the title.
	pObserver->OnExtractTag("title", util::UTF8FromT(TranslateFilenameToTitle(m_filename)).c_str());
    }

    pObserver->OnExtractTag("offset", 0);

    /** Now calculate the unique ID. We cannot easily, as we do for MP3, omit
     * metadata from the calculation :-(
     */
    unsigned int length;
    if (SUCCEEDED(pStm->Length(&length)))
    {
	std::string rid;
    STATUS st = ExtractRidSpecific(pStm, length, &rid);
	if (FAILED(st))
	    return st;
	pObserver->OnExtractTag("rid", rid.c_str());
    }

    return S_OK;
}

STATUS ASFTagExtractor::ExtractRid(SeekableStream *pStm, std::string *rid)
{
    STATUS status;
    unsigned int length;
    if (FAILED(status = pStm->Length(&length)))
        return status;
    return ExtractRidSpecific(pStm, length, rid);
}

STATUS ASFTagExtractor::ExtractRidSpecific(SeekableStream *pStm, unsigned int length, std::string *rid)
{
    return CalculateRid(pStm, 0, length, rid);
}

STATUS ASFTagExtractor::ExtractContentDescriptionObject (
    SeekableStream *pStm,
    const WMA_object *o,
    TagExtractorObserver *pObserver)
{
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));

    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
				o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }

    status = ExtractContentDescriptionObject(
	reinterpret_cast<const WMA_content_description_object *>(pBuffer),
	pObserver);
    delete[] pBuffer;
    return status;
}

STATUS ASFTagExtractor::ExtractPropertiesObject (
    SeekableStream *pStm,
    const WMA_object *o,
    TagExtractorObserver *pObserver)
{
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));

    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
				o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }

    status = ExtractPropertiesObject(
	reinterpret_cast<const WMA_properties_object *>(pBuffer),
	pObserver);
    delete[] pBuffer;
    return status;
}

STATUS ASFTagExtractor::ExtractContentDescriptionObject (
    const WMA_content_description_object *contentDescription,
    TagExtractorObserver *pObserver)
{
    // OK, the values live at the end of the structure.  Let's go find them.
    const char *pTitle = reinterpret_cast<const char *>(contentDescription + 1);
    const char *pAuthor = pTitle + contentDescription->title_len;
    const char *pCopyright = pAuthor + contentDescription->author_len;
    const char *pDescription = pCopyright + contentDescription->copyright_len;
    const char *pRating = pDescription + contentDescription->description_len;
    
    utf16_string title;
    if (contentDescription->title_len)
	title = utf16_string(reinterpret_cast<const UTF16CHAR *>(pTitle));

    // We _must_ have a title.
    if (!title.empty())
	FireExtractTag(pObserver, "title", title);
    else
	pObserver->OnExtractTag("title", util::UTF8FromT(TranslateFilenameToTitle(m_filename)).c_str());
    
    utf16_string author;
    if (contentDescription->author_len)
    {
	author = utf16_string(reinterpret_cast<const UTF16CHAR *>(pAuthor));
	FireExtractTag(pObserver, "artist", author);
    }
    
    utf16_string copyright;
    if (contentDescription->copyright_len)
    {
	copyright = utf16_string(reinterpret_cast<const UTF16CHAR *>(pCopyright));
	/* ignore it */
    }
    
    utf16_string description;
    if (contentDescription->description_len)
    {
	description = utf16_string(reinterpret_cast<const UTF16CHAR *>(pDescription));
	FireExtractTag(pObserver, "comment", description);
    }
    
    utf16_string rating;
    if (contentDescription->rating_len)
    {
	rating = utf16_string(reinterpret_cast<const UTF16CHAR *>(pRating));
	/* ignore it */
    }
    
    m_foundContentDescription = true;

    return S_OK;
}

#define CONVERT_100NS_TO_MS ((INT64) 10000)
#define FILETIME_SECOND ((INT64) 10000000)
#define FILETIME_MINUTE (60 * FILETIME_SECOND)
#define FILETIME_HOUR   (60 * FILETIME_MINUTE)
#define FILETIME_DAY    (24 * FILETIME_HOUR)

#if defined(WIN32)  // FILETIME shenanigans are too much hassle on anything else
static void DumpPropertiesObject(const WMA_properties_object *properties)
{
    TRACEC(TRACE_TAGS, "  multimedia_stream_id: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
			properties->multimedia_stream_id.Data1, properties->multimedia_stream_id.Data2, properties->multimedia_stream_id.Data3,
			properties->multimedia_stream_id.Data4[0], properties->multimedia_stream_id.Data4[1],
			properties->multimedia_stream_id.Data4[2], properties->multimedia_stream_id.Data4[3], properties->multimedia_stream_id.Data4[4],
			properties->multimedia_stream_id.Data4[5], properties->multimedia_stream_id.Data4[6], properties->multimedia_stream_id.Data4[7]);
    
    TRACEC(TRACE_TAGS, "  total_size: %" LONGLONGFMT "u bytes\n", properties->total_size);
    
    // FILETIME created;
    {
	FILETIME ft;
	ft.dwLowDateTime = (UINT32) (properties->created & 0xFFFFFFFF);
	ft.dwHighDateTime = (UINT32) (properties->created >> 32);
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);
	TRACEC(TRACE_TAGS, "  created: %4d-%02d-%02d  %d:%02d:%02d.%03d\n",
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }

    TRACEC(TRACE_TAGS, "  num_interleave_packets: %" LONGLONGFMT "u\n", properties->num_interleave_packets);

    // UINT64 play_duration_100ns;
    {
	/** @todo This'll break if the track is more than 24 hours long. */
	FILETIME ft;
	ft.dwLowDateTime = (UINT32) (properties->play_duration_100ns & 0xFFFFFFFF);
	ft.dwHighDateTime = (UINT32) (properties->play_duration_100ns >> 32);
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);
	TRACEC(TRACE_TAGS, "  play_duration: %d:%02d:%02d.%03d\n",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }

    // UINT64 send_duration_100ns;
    {
	/** @todo This'll break if the track is more than 24 hours long. */
	FILETIME ft;
	ft.dwLowDateTime = (UINT32) (properties->send_duration_100ns & 0xFFFFFFFF);
	ft.dwHighDateTime = (UINT32) (properties->send_duration_100ns >> 32);
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);
	TRACEC(TRACE_TAGS, "  send_duration: %d:%02d:%02d.%03d\n",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }

    TRACEC(TRACE_TAGS, "  preroll: %" LONGLONGFMT "u ms\n", properties->preroll_ms);
    TRACEC(TRACE_TAGS, "  flags: %x\n", properties->flags);
    TRACEC(TRACE_TAGS, "  min_interleave_packet_size: %u bytes\n", properties->min_interleave_packet_size);
    TRACEC(TRACE_TAGS, "  max_interleave_packet_size: %u bytes\n", properties->max_interleave_packet_size);
    TRACEC(TRACE_TAGS, "  maximum_bit_rate: %u bps\n", properties->maximum_bit_rate);
}
#endif

inline UINT32 Convert100nsToMilliseconds(UINT64 x)
{
    return (x) / CONVERT_100NS_TO_MS;
}

STATUS ASFTagExtractor::ExtractPropertiesObject (
    const WMA_properties_object *properties,
    TagExtractorObserver *pObserver)
{
//    DumpPropertiesObject(properties);
    
    // We want duration in ms
    UINT32 duration_ms = Convert100nsToMilliseconds(properties->play_duration_100ns);
    
    // According to some piece of documentation I found, we have to
    // adjust the duration by 'preroll'.  Which, in true MS fashion is in
    // completely different units from the rest.  Fortunately, we _want_
    // the result in milliseconds.
    UINT32 adjusted_duration_ms = duration_ms - properties->preroll_ms;
    pObserver->OnExtractTag("duration", adjusted_duration_ms);

    /// We want bitrate, as xyNNN where x = f or v, y = s or m, and NNN is in Kbps.
    std::string bitrate(VarString::Printf("fs%d", properties->maximum_bit_rate / 1000));
    pObserver->OnExtractTag("bitrate", bitrate.c_str());

    return S_OK;
}

struct WAVEFORMATEX {
    UINT16 wFormatTag;
    UINT16 nChannels;
    UINT32 nSamplesPerSec;
    UINT32 nAvgBytesPerSec;
    UINT16 nBlockAlign;
    UINT16 wBitsPerSample;
    UINT16 cbSize; 
};

STATUS ASFTagExtractor::ExtractStreamPropertiesObject (
    SeekableStream *pStm, const WMA_object *o,
    TagExtractorObserver *pObserver)
{
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));
    
    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
	o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }

    const WMA_stream_properties_object *p = reinterpret_cast<const WMA_stream_properties_object *>(pBuffer);
    if (p->stream_type == GUID_WMA_stream_type_audio)
    {
	// Then it's followed by a WAVEFORMATEX, go poking around in it.
	const WAVEFORMATEX *fmtx = reinterpret_cast<const WAVEFORMATEX *>(pBuffer + sizeof(WMA_stream_properties_object));
	pObserver->OnExtractTag("samplerate", fmtx->nSamplesPerSec);
    }

    delete[] pBuffer;
    return S_OK;
}

STATUS ASFTagExtractor::ExtractUnknownDRM (
    SeekableStream *pStm, const WMA_object *o,
    TagExtractorObserver *pObserver)
{
    UNUSED(pStm);
    UNUSED(o);

    // We can't read it, but it does mean that the file is (probably) DRM.
    pObserver->OnExtractTag("drm", "msasf");
    return S_OK;
}

static bool IsTextTag(const utf16_string &tag)
{
    // The only tag (as far as we know) that's not text is the track number.
//    if (ConvertUTF16To8859_1(tag.c_str()) == "WM/Track")
    if (tag == util::UTF16FromUTF8("WM/Track"))
	return false;

    return true;
}

STATUS ASFTagExtractor::ExtractTextTags (
    SeekableStream *pStm, const WMA_object *o,
    TagExtractorObserver *pObserver)
{
    TRACEC(TRACE_TAGS, "        GUID_WMA_unknown_text_1\n");
    
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));
    
    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
	o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }
    
    const char *p = reinterpret_cast<const char *>(pBuffer);
    p += sizeof(WMA_object);
    UINT16 num_tags = *(UINT16 *)p;
    p += sizeof(UINT16);
    
    TRACEC(TRACE_TAGS, "  num_tags: %d\n", num_tags);
    
    for (UINT16 i = 0; i < num_tags; ++i)
    {
	UINT16 tag_size = *(UINT16 *)p;
	p += sizeof(UINT16);
	utf16_string tag(reinterpret_cast<const UTF16CHAR *>(p));
	p += tag_size;
	
	UINT16 unknown = *(UINT16 *)p;
	p += sizeof(UINT16);
	UNUSED(unknown);
	
	if (IsTextTag(tag))
	{
	    UINT16 value_size = *(UINT16 *)p;
	    p += sizeof(UINT16);
	    utf16_string value(reinterpret_cast<const UTF16CHAR *>(p));
	    p += value_size;
	    
	    FireExtractTextTag(pObserver, tag, value);
	}
	else
	{
	    UINT16 value_size = *(UINT16 *)p;
	    p += sizeof(UINT16);
	    DWORD value = *(DWORD *)p;
	    p += value_size;

	    FireExtractDWORDTag(pObserver, tag, value);
	}
    }

    delete[] pBuffer;
    return S_OK;
}

void ASFTagExtractor::FireExtractDWORDTag(TagExtractorObserver *pObserver, const utf16_string &tag, DWORD value)
{
    // This doesn't work on gcc 'cos its std::basic_string::c_str() implementation is hosed.
    TRACEC(TRACE_TAGS, "   tag: %hs = %ld\n",
	ConvertUTF16ToNative(tag.c_str()).c_str(), value);

    std::string tagName(ConvertUTF16ToNative(tag));

    // Map from the WMA tag to the one we care about...
    if (tagName == "WM/Track")
    {
	// WM/Track appears to be zero-based.
	FireExtractTag(pObserver, "tracknr", value+1);
	FireExtractTag(pObserver, "file_id", value+1);
    }
    else
	TRACE_WARN("Don't know about this tag.\n");
}

void ASFTagExtractor::FireExtractTextTag(TagExtractorObserver *pObserver, const utf16_string &tag, const utf16_string &value)
{
    // This doesn't work on gcc 'cos its std::basic_string::c_str() implementation is hosed.
    TRACEC(TRACE_TAGS, "   tag: %hs = %s\n",
	ConvertUTF16ToNative(tag.c_str()).c_str(),
	ConvertUTF16ToNative(value.c_str()).c_str());

    std::string tagName(ConvertUTF16ToNative(tag));

    // Map from the WMA tag to the one we care about...
    if (tagName == "WM/AlbumTitle")
	FireExtractTag(pObserver, "source", value);
    else if (tagName == "WM/Genre")
	FireExtractTag(pObserver, "genre", value);
    else if (tagName == "WM/Year")
	FireExtractTag(pObserver, "year", value);
    else if (tagName == "WM/Track")
    {
	// It's still zero based even though it's a string!
#if WCHAR_MAX > USHRT_MAX
	// It's a UTF16 string - Linux can't use wcstol on it. This
	// should really be SystemFromUTF16 but that doesn't exist so
	// we cheat - it's only numbers after all!
	std::string narrow = util::Latin1FromUTF16(value);
	DWORD numeric_value = strtol(narrow.c_str(), NULL, 10);
#else // WCHAR_MAX <= USHRT_MAX
	DWORD numeric_value = wcstol(value.c_str(), NULL, 10);
#endif // WCHART_MAX <= USHRT_MAX
	FireExtractTag(pObserver, "tracknr", numeric_value + 1);
	FireExtractTag(pObserver, "file_id", numeric_value + 1);
    }
    /// @todo: Handle WM/TrackNumber - see MS docs before you do.
}


STATUS ASFTagExtractor::ExtractUnknownText2 (
					     SeekableStream *pStm, const WMA_object *o,
					     TagExtractorObserver *pObserver)
{
    UNUSED(pObserver);
    
    TRACEC(TRACE_TAGS, "        GUID_WMA_unknown_text_2\n");
    
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));
    
    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
	o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }
    
    const char *p = reinterpret_cast<const char *>(pBuffer);
    p += sizeof(WMA_object);
    
    GUID unknown_guid;
    memcpy(&unknown_guid, p, sizeof(GUID));
    p += sizeof(GUID);
    
    TRACEC(TRACE_TAGS, "  unknown_guid: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
	unknown_guid.Data1, unknown_guid.Data2, unknown_guid.Data3,
	unknown_guid.Data4[0], unknown_guid.Data4[1],
	unknown_guid.Data4[2], unknown_guid.Data4[3], unknown_guid.Data4[4],
	unknown_guid.Data4[5], unknown_guid.Data4[6], unknown_guid.Data4[7]);

    const UINT32 unknown = *(UINT32 *)p;
    UNUSED(unknown);

    p += sizeof(UINT32);
    
    TRACEC(TRACE_TAGS, "  unknown: %d (0x%x)\n", unknown, unknown);
    
    UINT16 num_strings = *(UINT16 *)p;
    p += sizeof(UINT16);
    
    TRACEC(TRACE_TAGS, "  num_strings: %d\n", num_strings);
    
    for (UINT16 i = 0; i < num_strings; ++i)
    {
	UINT16 size = *(UINT16 *)p;
	p += sizeof(UINT16);

	UTF16CHAR *text = (UTF16CHAR *)p;
        UNUSED(text);

	p += size * sizeof(UTF16CHAR);
	
	TRACEC(TRACE_TAGS, "   string %d: %hs\n", i, ConvertUTF16ToNative(text).c_str());
    }

    delete[] pBuffer;
    return S_OK;
}

STATUS ASFTagExtractor::ExtractUnknownNumbers (
					       SeekableStream *pStm, const WMA_object *o,
					       TagExtractorObserver *pObserver)
{
    UNUSED(pObserver);
    
    TRACEC(TRACE_TAGS, "        GUID_WMA_unknown_numbers_1\n");
    
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));
    
    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
	o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }
    
    const WMA_unknown_numbers_1 *p = reinterpret_cast<const WMA_unknown_numbers_1 *>(pBuffer);
    if (p->unknown1 != 0x0001 || p->unknown2 != 0x0001 || p->unknown3 != 0xfc88 || p->unknown4 != 0x0000)
    {
	TRACEC(TRACE_TAGS, "  !! Differs from expected value (not that we know what the value means)\n");
	
	TRACEC(TRACE_TAGS, "  unknown1: 0x%04x (expected 0x0001)\n", p->unknown1);
	TRACEC(TRACE_TAGS, "  unknown2: 0x%04x (expected 0x0001)\n", p->unknown2);
	TRACEC(TRACE_TAGS, "  unknown3: 0x%04x (expected 0xfc88)\n", p->unknown3);
	TRACEC(TRACE_TAGS, "  unknown4: 0x%04x (expected 0x0000)\n", p->unknown4);
    }

    delete[] pBuffer;
    return S_OK;
}

STATUS ASFTagExtractor::ExtractUnknownObject (
					      SeekableStream *pStm, const WMA_object *o,
					      TagExtractorObserver *pObserver)
{
    UNUSED(pObserver);
    
    BYTE *pBuffer = NEW BYTE[o->size];
    memcpy(pBuffer, o, sizeof(WMA_object));
    
    unsigned bytesRead;
    STATUS status = pStm->Read(pBuffer + sizeof(WMA_object),
	o->size - sizeof(WMA_object), &bytesRead);
    if (FAILED(status))
    {
	delete[] pBuffer;
	return status;
    }

    delete[] pBuffer;
    return S_OK;
}

void ASFTagExtractor::FireExtractTag(TagExtractorObserver *pObserver,
				     const char *tagName, const utf16_string &tagValue)
{
    std::string s(ConvertUTF16ToNative(tagValue));
    pObserver->OnExtractTag(tagName, s.c_str());
}

void ASFTagExtractor::FireExtractTag(TagExtractorObserver *pObserver,
				     const char *tagName, DWORD tagValue)
{
    pObserver->OnExtractTag(tagName, tagValue);
}
