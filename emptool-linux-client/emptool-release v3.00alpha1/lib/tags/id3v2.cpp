/* id3v2.cpp
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.25 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#define TRACE_TAGS 0
#include "id3.h"
#include "tag_errors.h"
#include "stringpred.h"

#ifndef LOBYTE
#define LOBYTE(w) ((w) & 0xff)
#endif

#ifndef HIBYTE
#define HIBYTE(w) (((w) & 0xff00) >> 8)
#endif

STATUS ID3V2TagParser::Parse(SeekableStream *pStm, 
			     ID3V2FrameObserver *observer, unsigned *offset)
{
    STATUS status;
    ID3V2_Tag v2;
    
    if ((FAILED(status = ParseHeader(pStm, &v2, offset))))
        return status;
    
    unsigned int idlen, sizelen, flagslen, headerlen;
    if (v2.minor_version <= 2)
    {
	idlen = 3;
	sizelen = 3;
	flagslen = 0;
	headerlen= 6;
    }
    else
    {
	idlen = 4;
	sizelen = 4;
	flagslen = 2;
	headerlen = 10;
    }

    bool frame_size_unsynchronised = (v2.minor_version > 3);
    unsigned bytesRead;
    unsigned char sizebuf[4];

    for(;;)
    {
	// Right, now go looking for ID3 frames in the file -- we can't
	// easily copy them into memory, because they're woefully unaligned.
	ID3V2_Frame frame;
	memset(&frame, 0, sizeof(frame));
	pStm->Read(frame.frameID, idlen, &bytesRead);

	// Skip padding.
	if (frame.frameID[0] == 0)
	{
	    TRACEC(TRACE_TAGS, "Found an 0 byte, end of tags\n");
	    break;
	}

	/* We convert ID3v2.2 to 2.3 here because (a) it simplifies the
	 * upstream code and (b) we only write v2.3 anyway
	 */
	if (idlen == 3)
	{
	    memcpy(frame.frameID,
		   ID3TagExtractor::ConvertThreeCCtoFourCC(frame.frameID), 4);
	}

	pStm->Read(sizebuf, sizelen, &bytesRead);

	if (frame_size_unsynchronised)
	{
	    // Watch out for broken (negative) sizes
	    for (unsigned int i=0; i<sizelen; i++)
	    {
		if (sizebuf[i] > 127)
		{
		    TRACEC(TRACE_TAGS, "Bogus tag, aborting\n");
		    return S_OK;
		}
	    }
	    
	    if (sizelen == 4)
		frame.size = ss32_to_cpu(sizebuf);
	    else
		frame.size = ss24_to_cpu(sizebuf);
	}
	else
	{
	    if (sizelen == 4)
		frame.size = p32_to_cpu(sizebuf);
	    else
		frame.size = p24_to_cpu(sizebuf);
	}

	if (flagslen)
	    pStm->Read(&frame.flags, flagslen, &bytesRead);

	// Save the seek position - processing the frame may move it...
	unsigned pos;
	pStm->Tell(&pos);

	if ((pos+frame.size) > *offset || frame.size <= 0)
	{
	    TRACEC(TRACE_TAGS, "Bogus frame size (%d +%d > %d)\n",
		   pos, frame.size, *offset);
	    break;
	}

	TRACEC(TRACE_TAGS, "v2.%c Frame at %d\n", '0' + v2.minor_version, pos-headerlen);

	if (frame.size > 0)
	{
	    observer->OnFrame(pStm, &frame);
	}

	// Figure out where the next frame is coming from...
	pStm->SeekAbsolute(pos + frame.size);
	if (pos + frame.size == *offset)
	{
	    TRACEC(TRACE_TAGS,"Got to end of tags at %d\n", *offset);
	    break;
	}
    }
    
    return S_OK;
}

STATUS ID3V2TagParser::ParseHeader(SeekableStream *pStm, ID3V2_Tag *v2, unsigned *offset)
{
    STATUS status;
    if (FAILED(status = pStm->SeekAbsolute(0)))
        return status;

    unsigned bytesRead;
    unsigned char sizebuf[4];

    pStm->Read(v2->fileIdentifier, sizeof(v2->fileIdentifier), &bytesRead);
    pStm->Read(&v2->minor_version, sizeof(v2->minor_version), &bytesRead);
    pStm->Read(&v2->revision, sizeof(v2->revision), &bytesRead);
    pStm->Read(&v2->flags, sizeof(v2->flags), &bytesRead);
    pStm->Read(sizebuf, sizeof(v2->size), &bytesRead);
    if (bytesRead != sizeof(v2->size))
    return TAG_E_ID3_TOOSHORT;  // Failed to read enough data for the V2 tag

    v2->size = ss32_to_cpu(sizebuf);

    if (memcmp(v2->fileIdentifier, "ID3", 3) != 0)
    {
        TRACEC(TRACE_TAGS, "It's not an ID3V2 tag\n");
        return TAG_E_ID3_NOTV2; // It's not an ID3V2 tag
    }
    
    TRACEC(TRACE_TAGS, "ID3v2 version: ID3v2->%d.%d\n", v2->minor_version, v2->revision);
    TRACEC(TRACE_TAGS, "ID3v2 flags:   %02x\n", v2->flags);
    UINT32 tagSize = v2->size + 10; // Size doesn't include 10-byte header
    TRACEC(TRACE_TAGS, "ID3v2 size:    %d\n", tagSize);
    *offset = tagSize;

    if (v2->minor_version > ID3V2_VERSION)
        return TAG_E_ID3_TOONEW;   // Don't understand this version - too new

    return S_OK;
}

void ID3TagExtractor::OnFrame(SeekableStream *pStm, 
			      const ID3V2_Frame *frameHeader)
{
    // Skip padding.
    if (frameHeader->frameID[0] == 0)
	return;

    std::string frameID(stringpred::FromFixedBuffer(frameHeader->frameID,
						    sizeof(frameHeader->frameID)));
    TRACEC(TRACE_TAGS, "  Frame ID:    %s (%s)\n",
	   frameID.c_str(), DescribeFrameID(frameHeader->frameID));
    TRACEC(TRACE_TAGS, "  Frame size:  %d\n", frameHeader->size);
    TRACEC(TRACE_TAGS, "  Frame flags: %04x\n", frameHeader->flags);
    TRACEC(TRACE_TAGS, "\n");
    
    // Based on the frame ID, dispatch it to another function.
    for (const FrameIDLookup *p = frameLookup; p->frameID[0]; ++p)
    {
	if (memcmp(p->frameID, frameHeader->frameID, 4) == 0)
	{
	    FrameIDLookup::PFN f = p->pfn;
	    if (f)
		(m_collect->*f)(pStm, frameHeader, frameHeader->size);

	    break;
	}
    }
}

const ID3TagExtractor::FrameIDLookup ID3TagExtractor::frameLookup[] = {
    { "AENC", "CRA", "Audio encryption", &CollectFrames::OnUnknownFrame, },
    { "APIC", "PIC", "Attached picture", &CollectFrames::OnUnknownFrame, },
    { "ASPI", "",    "Audio seek point index", &CollectFrames::OnUnknownFrame, },

    { "COMM", "COM", "Comments", &CollectFrames::OnCommentFrame, },
    { "COMR", "",    "Commercial frame", &CollectFrames::OnUnknownFrame, },

    { "ENCR", "",    "Encryption method registration", &CollectFrames::OnUnknownFrame, },
    { "EQUA", "EQU", "Equalisation", &CollectFrames::OnUnknownFrame, },	// (deprecated in ID3v2.4.0)
    { "EQU2", "",    "Equalisation (2)", &CollectFrames::OnUnknownFrame, },
    { "ETCO", "ETC", "Event timing codes", &CollectFrames::OnUnknownFrame, },

    { "GEOB", "GEO", "General encapsulated object", &CollectFrames::OnUnknownFrame, },
    { "GRID", "",    "Group identification registration", &CollectFrames::OnUnknownFrame, },

    { "IPLS", "IPL", "Involved people list", &CollectFrames::OnUnknownFrame, },	// (deprecated in ID3v2.4.0)

    { "LINK", "LNK", "Linked information", &CollectFrames::OnUnknownFrame, },

    { "MCDI", "MCI", "Music CD identifier", &CollectFrames::OnUnknownFrame, },
    { "MLLT", "MLL", "MPEG location lookup table", &CollectFrames::OnUnknownFrame, },

    { "OWNE", "",    "Ownership frame", &CollectFrames::OnUnknownFrame, },

    { "PRIV", "",    "Private frame", &CollectFrames::OnUnknownFrame, },
    { "PCNT", "CNT", "Play counter", &CollectFrames::OnUnknownFrame, },
    { "POPM", "POP", "Popularimeter", &CollectFrames::OnUnknownFrame, },
    { "POSS", "",    "Position synchronisation frame", &CollectFrames::OnUnknownFrame, },

    { "RBUF", "BUF", "Recommended buffer size", &CollectFrames::OnUnknownFrame, },
    { "RVAD", "RVA", "Relative volume adjustment", &CollectFrames::OnUnknownFrame, },	// (deprecated in ID3v2.4.0)
    { "RVA2", "",    "Relative volume adjustment (2)", &CollectFrames::OnUnknownFrame, },
    { "RVRB", "REV", "Reverb", &CollectFrames::OnUnknownFrame, },

    { "SEEK", "",    "Seek frame", &CollectFrames::OnUnknownFrame, },
    { "SIGN", "",    "Signature frame", &CollectFrames::OnUnknownFrame, },
    { "SYLT", "SLT", "Synchronised lyric/text", &CollectFrames::OnUnknownFrame, },
    { "SYTC", "STC", "Synchronised tempo codes", &CollectFrames::OnUnknownFrame, },

    { "TALB", "TAL", "Album/Movie/Show title", &CollectFrames::OnTextFrame, },
    { "TBPM", "TBP", "BPM (beats per minute)", &CollectFrames::OnTextFrame, },
    { "TCOM", "TCM", "Composer", &CollectFrames::OnTextFrame, },
    { "TCON", "TCO", "Content type", &CollectFrames::OnTextFrame, },
    { "TCOP", "TCR", "Copyright message", &CollectFrames::OnTextFrame, },
    { "TDAT", "",    "Date", &CollectFrames::OnTextFrame, },	// (deprecated in ID3v2.4.0)
    { "TDEN", "",    "Encoding time", &CollectFrames::OnTextFrame, },
    { "TDLY", "TDY", "Playlist delay", &CollectFrames::OnTextFrame, },
    { "TDOR", "",    "Original release time", &CollectFrames::OnTextFrame, },
    { "TDRC", "",    "Recording time", &CollectFrames::OnTextFrame, },
    { "TDRL", "",    "Release time", &CollectFrames::OnTextFrame, },
    { "TDTG", "",    "Tagging time", &CollectFrames::OnTextFrame, },
    { "TENC", "TEN", "Encoded by", &CollectFrames::OnTextFrame, },
    { "TEXT", "TXT", "Lyricist/Text writer", &CollectFrames::OnTextFrame, },
    { "TFLT", "TFT", "File type", &CollectFrames::OnTextFrame, },
    { "TIME", "TIM", "Time", &CollectFrames::OnTextFrame, },	// (deprecated in ID3v2.4.0)
    { "TIPL", "",    "Involved people list", &CollectFrames::OnTextFrame, },
    { "TIT1", "TT1", "Content group description", &CollectFrames::OnTextFrame, },
    { "TIT2", "TT2", "Title/songname/content description", &CollectFrames::OnTextFrame, },
    { "TIT3", "TT3", "Subtitle/Description refinement", &CollectFrames::OnTextFrame, },
    { "TKEY", "TKE", "Initial key", &CollectFrames::OnTextFrame, },
    { "TLAN", "TLA", "Language(s)", &CollectFrames::OnTextFrame, },
    { "TLEN", "TLE", "Length", &CollectFrames::OnTextFrame, },
    { "TMCL", "",    "Musician credits list", &CollectFrames::OnTextFrame, },
    { "TMED", "TMT", "Media type", &CollectFrames::OnTextFrame, },
    { "TMOO", "",    "Mood", &CollectFrames::OnTextFrame, },
    { "TOAL", "TOT", "Original album/movie/show title", &CollectFrames::OnTextFrame, },
    { "TOFN", "TOF", "Original filename", &CollectFrames::OnTextFrame, },
    { "TOLY", "TOL", "Original lyricist(s)/text writer(s)", &CollectFrames::OnTextFrame, },
    { "TOPE", "TOA", "Original artist(s)/performer(s)", &CollectFrames::OnTextFrame, },
    { "TORY", "TOR", "Original release year", &CollectFrames::OnTextFrame, },	// (deprecated in ID3v2.4.0)
    { "TOWN", "",    "File owner/licensee", &CollectFrames::OnTextFrame, },
    { "TPE1", "TP1", "Lead performer(s)/Soloist(s)", &CollectFrames::OnTextFrame, },
    { "TPE2", "TP2", "Band/orchestra/accompaniment", &CollectFrames::OnTextFrame, },
    { "TPE3", "TP3", "Conductor/performer refinement", &CollectFrames::OnTextFrame, },
    { "TPE4", "TP4", "Interpreted, remixed, or otherwise modified by", &CollectFrames::OnTextFrame, },
    { "TPOS", "TPA", "Part of a set", &CollectFrames::OnTextFrame, },
    { "TPRO", "",    "Produced notice", &CollectFrames::OnTextFrame, },
    { "TPUB", "TPB", "Publisher", &CollectFrames::OnTextFrame, },
    { "TRCK", "TRK", "Track number/Position in set", &CollectFrames::OnTextFrame, },
    { "TRDA", "TRD", "Recording dates", &CollectFrames::OnTextFrame, },	// (deprecated in ID3v2.4.0)
    { "TRSN", "",    "Internet radio station name", &CollectFrames::OnTextFrame, },
    { "TRSO", "",    "Internet radio station owner", &CollectFrames::OnTextFrame, },
    { "TSIZ", "TSI", "Size", &CollectFrames::OnTextFrame, },  // (deprecated in ID3v2.4.0)
    { "TSOA", "",    "Album sort order", &CollectFrames::OnTextFrame, },
    { "TSOP", "",    "Performer sort order", &CollectFrames::OnTextFrame, },
    { "TSOT", "",    "Title sort order", &CollectFrames::OnTextFrame, },
    { "TSRC", "TRC", "ISRC (international standard recording code)", &CollectFrames::OnTextFrame, },
    { "TSSE", "TSS", "Software/Hardware and settings used for encoding", &CollectFrames::OnTextFrame, },
    { "TSST", "",    "Set subtitle", &CollectFrames::OnTextFrame, },
    { "TXXX", "TXX", "User defined text information frame", &CollectFrames::OnUnknownFrame, },
    { "TYER", "TYE", "Year", &CollectFrames::OnTextFrame, }, // (deprecated in ID3v2.4.0)

    { "UFID", "UFI", "Unique file identifier", &CollectFrames::OnUnknownFrame, },
    { "USER", "",    "Terms of use", &CollectFrames::OnUnknownFrame, },
    { "USLT", "ULT", "Unsynchronised lyric/text transcription", &CollectFrames::OnUnknownFrame, },

    { "WCOM", "WCM", "Commercial information", &CollectFrames::OnUnknownFrame, },
    { "WCOP", "WCP", "Copyright/Legal information", &CollectFrames::OnUnknownFrame, },
    { "WOAF", "WAF", "Official audio file webpage", &CollectFrames::OnUnknownFrame, },
    { "WOAR", "WAR", "Official artist/performer webpage", &CollectFrames::OnUnknownFrame, },
    { "WOAS", "WAS", "Official audio source webpage", &CollectFrames::OnUnknownFrame, },
    { "WORS", "",    "Official Internet radio station homepage", &CollectFrames::OnUnknownFrame, },
    { "WPAY", "",    "Payment", &CollectFrames::OnUnknownFrame, },
    { "WPUB", "WPB", "Publishers official webpage", &CollectFrames::OnUnknownFrame, },
    { "WXXX", "WXX", "User defined URL link frame", &CollectFrames::OnUnknownFrame, },
    { "", "", NULL, NULL, },
};

const char *ID3TagExtractor::DescribeFrameID(const char *frameID)
{
    for (const FrameIDLookup *p = frameLookup; p->frameID[0]; ++p)
    {
	if (memcmp(p->frameID, frameID, 4) == 0)
	{
	    return p->frameDescription;
	}
    }

    return "-unknown-";
}

const char *ID3TagExtractor::ConvertThreeCCtoFourCC(const char *threecc)
{
    for (const FrameIDLookup *p = frameLookup; p->frameID[0]; ++p)
	if (memcmp(p->frameThreeCC, threecc, 3) == 0)
	    return p->frameID;
    return "TXXX";
}
