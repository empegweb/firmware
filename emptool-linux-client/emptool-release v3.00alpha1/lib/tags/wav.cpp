/* wav.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "wav.h"
#include "tag_errors.h"
#include "file.h"
#include "filename_to_title.h"
#include "utf8.h"
#include <stdio.h>
#define TRACE_TAGS 0

STATUS WAVTagExtractor::Create(const tstring &filename, TagExtractor **pp)
{
    WAVTagExtractor *p = NEW WAVTagExtractor(filename);
    if (!p)
	return E_OUTOFMEMORY;

    *pp = p;
    return S_OK;
}

STATUS WAVTagExtractor::ExtractTags(SeekableStream *pStm, TagExtractorObserver *pObserver)
{
    pObserver->OnExtractTag("title", util::UTF8FromT(TranslateFilenameToTitle(m_filename)).c_str());

    return IdentifyWAVStream(pStm, pObserver);
}

#if defined(WIN32)
#define snprintf _snprintf
#endif

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

#define CHAR_TO_U32LE(A, B, C, D) (((D) << 24) | ((C) << 16) | ((B) << 8) | (A))
#define CHAR_TO_U16LE(A, B) (((B) << 8) | (A))

inline STATUS ReadU32LE(Stream *bs, unsigned long *ret)
{
    unsigned char b[4];
    unsigned int result;
    STATUS rc = bs->Read(b, 4, &result);

    if ( SUCCEEDED(rc) )
    {
        if (result < 4)
            return E_ENDOFFILE;

        *ret = CHAR_TO_U32LE(b[0], b[1], b[2], b[3]);
        return S_OK;
    }
    
    return rc;
}

inline STATUS ReadU16LE(Stream *bs, unsigned short *ret)
{
    unsigned char b[2];
    unsigned int result;
    STATUS rc = bs->Read(b, 2, &result);

    if ( SUCCEEDED(rc) )
    {
        if (result < 2)
            return E_ENDOFFILE;

        *ret = CHAR_TO_U16LE(b[0], b[1]);
        return S_OK;
    }
    
    return rc;
}

STATUS WAVTagExtractor::IdentifyWAVStream(SeekableStream *stream, TagExtractorObserver *pObserver)
{
    unsigned long riff;
    STATUS status = ReadU32LE(stream, &riff);
    if (FAILED(status))
        return TAG_E_WAV_IDENTIFYFAILED;

    if (riff != CHAR_TO_U32LE('R', 'I', 'F', 'F'))
    {
        TRACEC(TRACE_TAGS, "It's not a RIFF file. 0x%08lx\n", riff);
        return TAG_E_WAV_IDENTIFYFAILED;
    }

    unsigned long total_length;
    status = ReadU32LE(stream, &total_length);
    if (FAILED(status))
        return TAG_E_WAV_IDENTIFYFAILED;

    TRACEC(TRACE_TAGS, "It claims to be %ld (0x%lx) bytes long\n", total_length, total_length);

    unsigned long wave;
    status = ReadU32LE(stream, &wave);
    if (FAILED(status))
        return TAG_E_WAV_IDENTIFYFAILED;

    if (wave != CHAR_TO_U32LE('W', 'A', 'V', 'E'))
    {
        TRACEC(TRACE_TAGS, "It's not a WAVE file. 0x%08lx\n", wave);
        return TAG_E_WAV_IDENTIFYFAILED;
    }

    bool seen_fmt = false;
    bool seen_data = false;
    unsigned long bytes_per_second;
    unsigned long data_length = 0UL;

    // OK, we've read the RIFF header. Now lets look for the sections we need.
    while (!seen_fmt || !seen_data)
    {
        unsigned long riff_name;
        status = ReadU32LE(stream, &riff_name);
        if (FAILED(status))
            return TAG_E_WAV_IDENTIFYFAILED;

        unsigned long part_length;
        status = ReadU32LE(stream, &part_length);
        if (FAILED(status))
            return TAG_E_WAV_IDENTIFYFAILED;

        switch (riff_name)
        {
        case CHAR_TO_U32LE('f', 'm', 't', ' '):
        {
            seen_fmt = true;

	    unsigned part_start;
	    status = stream->Tell(&part_start);            
	    if (FAILED(status))
		return TAG_E_WAV_IDENTIFYFAILED;

            unsigned short format_tag;
            status = ReadU16LE(stream, &format_tag);
            if (FAILED(status))
                return TAG_E_WAV_IDENTIFYFAILED;

            if (format_tag != WAVE_FORMAT_PCM)
            {
                // We only support WAVE_FORMAT_PCM files.
                TRACEC(TRACE_TAGS, "It's not a WAVE_FORMAT_PCM file.\n");
                return TAG_E_WAV_NOTPCM;
            }

            unsigned short channels;
            status = ReadU16LE(stream, &channels);
            if (FAILED(status))
                return TAG_E_WAV_IDENTIFYFAILED;

            TRACEC(TRACE_TAGS, "It uses %d channels.\n", channels);

            unsigned long sample_rate;
            status = ReadU32LE(stream, &sample_rate);
            if (FAILED(status))
                return TAG_E_WAV_IDENTIFYFAILED;


            status = ReadU32LE(stream, &bytes_per_second);
            if (FAILED(status))
                return TAG_E_WAV_IDENTIFYFAILED;

            if (bytes_per_second == 0)
            {
                TRACEC(TRACE_TAGS, "Bytes per second is zero - bad!\n");
                return TAG_E_WAV_IDENTIFYFAILED;
            }

            unsigned short block_align;
            status = ReadU16LE(stream, &block_align);
            if (FAILED(status))
                return TAG_E_WAV_IDENTIFYFAILED;

            unsigned short bits_per_sample;
            status = ReadU16LE(stream, &bits_per_sample);
            if (FAILED(status))
                return TAG_E_WAV_IDENTIFYFAILED;

            char buffer[32];
            snprintf(buffer, 32, "f%c%ld", (channels == 1) ? 'm' : 's', 
                     (sample_rate * channels * bits_per_sample)/1000);
            TRACEC(TRACE_TAGS, "bitrate tag: %s\n", buffer);
	    pObserver->OnExtractTag("bitrate", buffer);

            // And that's all we need.
            stream->SeekAbsolute(part_start + part_length);
            break;
        }

        case CHAR_TO_U32LE('d', 'a', 't', 'a'):
            seen_data = true;
            data_length = part_length;
            stream->SeekRelative(part_length);
            break;

        default:
            TRACEC(TRACE_TAGS, "Seen unknown RIFF part: 0x%08lx\n", riff_name);
            // Skip past the content of the RIFF part.
            stream->SeekRelative(part_length);
            break;
        }
    }

    // Right, if we got here then it is probably something we can play so fill in the details.
    pObserver->OnExtractTag("codec", "wave");

    ASSERT(bytes_per_second > 0);
    pObserver->OnExtractTag("duration", (static_cast<INT64>(data_length) * 1000) / bytes_per_second);

    return S_OK;
}

STATUS WAVTagExtractor::ExtractRid(SeekableStream *pStm, std::string *rid)
{
    *rid = std::string();
    return S_OK;
}

