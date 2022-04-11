/* id3_writer.cpp
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.25 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"
#include "id3_writer.h"
#include "id3.h"
#include "file.h"
#include "genre.h"
#include "file_stream.h"
#include "stringpred.h"
#include "utf8.h"
#include <numeric>
#include <stdio.h>

#define TRACE_TAGW 0

#define ENABLE_UTF8 1

using namespace tags;

STATUS ID3TagWriter::Create(const tstring& filename, TagWriter **ppWriter)
{
    *ppWriter = NEW ID3TagWriter(filename);
    return S_OK;
}

/** Care is needed here. This list must match the list in the tag extractor
 * (id3.cpp) for round-tripping purposes, but we mustn't blat extended tags
 * (such as TIT1/TIT3/TPE2/TSOA) that we don't understand yet.
 *
 * It's not worth writing tags that are trivially derivable from the content,
 * such as samplerate or codec, but it is worth writing ones that might take
 * effort to find out, such as duration and BPM.
 */
static const struct {
    const char *tagname;
    const char *id3tag;
    std::string (ID3TagWriter::*pFrameFn)(const std::string&, const char*);
} tagmap[] = {
    { "artist",   "TPE1", &ID3TagWriter::StringFrame },
    { "source",   "TALB", &ID3TagWriter::StringFrame },
    { "title",    "TIT2", &ID3TagWriter::StringFrame },
    { "year",     "TYER", &ID3TagWriter::StringFrameNotZero },
    { "comment",  "COMM", &ID3TagWriter::CommentFrame },
    { "genre",    "TCON", &ID3TagWriter::StringFrame },
    { "tracknr",  "TRCK", &ID3TagWriter::StringFrameNotZero },
    { "bpm",      "TBPM", &ID3TagWriter::StringFrameNotZero },
    { "duration", "TLEN", &ID3TagWriter::StringFrame }, // both in ms luckily
    { "subtitle", "TIT3", &ID3TagWriter::StringFrame },
    { "composer", "TCOM", &ID3TagWriter::StringFrame },
    { "conductor","TPE3", &ID3TagWriter::StringFrame },
    { "remixed",  "TPE4", &ID3TagWriter::StringFrame },
    { "partofset","TPOS", &ID3TagWriter::StringFrame },
    { "partname", "TSST", &ID3TagWriter::StringFrame },
    { "isrc",     "TSRC", &ID3TagWriter::StringFrame },
    { "originalartist", "TOPE", &ID3TagWriter::StringFrame },
    { "lyricist", "TEXT", &ID3TagWriter::StringFrame },
    { "key",      "TKEY", &ID3TagWriter::StringFrame },
    { "language", "TLAN", &ID3TagWriter::StringFrame },
    { "sourcemedia", "TMED", &ID3TagWriter::StringFrame },
    { "mood",     "TMOO", &ID3TagWriter::StringFrame },
    { "sourcesort", "TSOA", &ID3TagWriter::StringFrame },
    { "artistsort", "TSOP", &ID3TagWriter::StringFrame },
    { "titlesort", "TSOT", &ID3TagWriter::StringFrame },
//    { "file_id",  "TRCK", &ID3TagWriter::PrivateFrame },
//    { "rid",      "TRID", &ID3TagWriter::PrivateFrame }, // or UFID frame??
//    { "cd_toc",   "MCID", &ID3TagWriter::TocFrame },
};

STATUS ID3TagWriter::SetTag(const char *tagname, const char *tagvalue)
{
    for (unsigned int i=0; i<sizeof(tagmap)/sizeof(tagmap[0]); ++i)
    {
	if (!strcmp(tagname, tagmap[i].tagname))
	{
	    const char *which_tag = tagmap[i].id3tag;
	    std::string frame_contents = (this->*tagmap[i].pFrameFn)(tagvalue,
								     which_tag);
	    /* frame_contents may be empty at this stage, but we record the
	     * fact anyway, in order not to reanimate old tags from the file
	     * in the set-it-to-blank case.
	     */
	    m_my_frameids.insert(which_tag);
	    if (!frame_contents.empty())
		m_my_frames.push_back(frame_contents);
	    break;
	}
    }

    /* ID3v1 needs local-encoding versions of the tags (aaargggh!) */
    std::string local = util::SystemFromUTF8(tagvalue);
    
    if (!strcmp(tagname, "artist"))
    {
	/* For once strncpy does the right thing! */
	strncpy(m_v1.artist, local.c_str(), ID3V1_FIELD_LENGTH);
	m_v1flags |= GOT_ARTIST;
    }
    else if (!strcmp(tagname, "title"))
    {
	strncpy(m_v1.title, local.c_str(), ID3V1_FIELD_LENGTH);
	m_v1flags |= GOT_TITLE;
    }
    else if (!strcmp(tagname, "comment"))
    {
	unsigned int field_len = ID3V1_FIELD_LENGTH;
	if (m_v1flags & GOT_TRACKNR)
	    field_len -= 2;
	strncpy(m_v1.comment, local.c_str(), field_len);
	m_v1flags |= GOT_COMMENT;
    }
    else if (!strcmp(tagname, "source"))
    {
	strncpy(m_v1.album, local.c_str(), ID3V1_FIELD_LENGTH);
	m_v1flags |= GOT_ALBUM;
    }
    else if (!strcmp(tagname, "year"))
    {
	strncpy(m_v1.year, local.c_str(), 4);
	m_v1flags |= GOT_YEAR;
    }
    else if (!strcmp(tagname, "tracknr"))
    {
	m_v1.comment[28] = '\0';
	m_v1.comment[29] = (char)atoi(tagvalue);
	m_v1flags |= GOT_TRACKNR;
    }
    else if (!strcmp(tagname, "genre"))
    {
	int genre_id = GenreList::Find(util::TFromUTF8(tagvalue).c_str());
	if (genre_id >= 0)
	    m_v1.genre = genre_id;
	else
	    m_v1.genre = '\xFF';
	m_v1flags |= GOT_GENRE;
    }

    return S_OK;
}

std::string ID3TagWriter::StringFrame(const std::string& contents, 
				      const char* tag)
{
    if (contents.empty())
	return std::string();

    // This string is specifically a string of octets
    std::string frame = contents;
    char encoding = ID3V2_TEXT_ENCODING_ISO8859_1;

#if ENABLE_UTF8
    // UTF8 tags were only added in id3v2.4, so we only bother with
    // Latin1 or UTF16
    switch (util::UTF8Classify(frame.c_str()))
    {
    case util::NOT_UTF8:
    case util::ASCII: break; // assume already OK
    case util::ISO_8859_1:
	/** If we're treating Latin1 ID3v2 input as local, then we mustn't
	 * write Latin1 as Latin1 otherwise we'd confuse ourselves on reread.
	 */
	if (!ShouldTreatLatin1AsLocal())
	{
	    frame = util::Latin1FromUTF8(frame); 
	    break;
	}
	/* else fall through */
    case util::FULL_UNICODE:
    {
	encoding = ID3V2_TEXT_ENCODING_UNICODE;
	utf16_string us = util::UTF16FromUTF8(frame.c_str());
	us = util::UTF16EnsureBOM(us);
	frame = std::string();
	frame.reserve(us.length()*2);
	for (unsigned int i=0; i<us.length(); ++i)
	{
	    UTF16CHAR uch = us[i];
	    frame += (char)(uch & 0xFF);
	    frame += (char)((uch>>8) & 0xFF);
	}
	break;
    }
    };
#endif

    bool was_unsynchronised;
    frame = Unsynchronise(frame, &was_unsynchronised);

    // Text encoding
    frame = encoding + frame;
    return AddFrameHeader(frame, tag, was_unsynchronised);
}
 
/** @todo UTF8 issues */
std::string ID3TagWriter::CommentFrame(const std::string& contents, 
				       const char* tag)
{
    if (contents.empty())
	return std::string();

    std::string frame = contents;
    char encoding = ID3V2_TEXT_ENCODING_ISO8859_1;

#if ENABLE_UTF8
    // UTF8 tags were only added in id3v2.4, so we only bother with
    // Latin1 or UTF16
    switch (util::UTF8Classify(frame.c_str()))
    {
    case util::NOT_UTF8:
    case util::ASCII: break; // assume already OK
    case util::ISO_8859_1: frame = util::Latin1FromUTF8(frame); break;
    case util::FULL_UNICODE:
    {
	encoding = ID3V2_TEXT_ENCODING_UNICODE;
	utf16_string us = util::UTF16FromUTF8(frame.c_str());
	us = util::UTF16EnsureBOM(us);
	frame = std::string();
	frame.reserve(us.length()*2);
	for (unsigned int i=0; i<us.length(); ++i)
	{
	    UTF16CHAR uch = us[i];
	    frame += (char)(uch & 0xFF);
	    frame += (char)((uch>>8) & 0xFF);
	}
	break;
    }
    };
#endif

    bool was_unsynchronised;
    frame = Unsynchronise(frame, &was_unsynchronised);

    /** Text encoding, language (ISO-639-2, "XXX" if unknown), zero-terminated title
     */
    frame = "xXXXx" + frame;
    frame[0] = encoding;
    frame[4] = '\0';
    return AddFrameHeader(frame, tag, was_unsynchronised);
}
 
std::string ID3TagWriter::AddFrameHeader(std::string frame,
					 std::string frameID, bool was_unsync)
{
    unsigned int frame_data_size = frame.length();

    // Frame format flags
    char flags = 0;
    if (was_unsync)
	flags |= 2;
    frame = flags + frame;

    // Frame status flags
    frame = '\0' + frame;

    /** pdh 24-Oct-02 Plain, not unsynchronised, in ID3v2.3 (which is what we
     * claim to be)
     */
    frame = frameID + PlainInteger(frame_data_size) + frame;

    return frame;
}

std::string ID3TagWriter::StringFrameNotZero(const std::string& s,
					     const char *tag)
{
    if (s == "0")
	return std::string();
    return StringFrame(s, tag);
}

#if 0
std::string ID3TagWriter::PrivateFrame(const std::string& s, const char *tag)
{
    if (s.empty() || s == "0")
	return std::string();

    bool was_unsynchronised;

    std::string frame = "http://empeg.com/id3priv/#";
    frame += tag;
    frame += '\0';
    frame += Unsynchronise(s, &was_unsynchronised);

    return AddFrameHeader(frame, "PRIV", was_unsynchronised);
}
#endif

std::string ID3TagWriter::Unsynchronise(const std::string& s, 
					bool *was_changed)
{
    if (s.find('\xFF') == std::string::npos)
    {
	*was_changed = false;
	return s;
    }
    *was_changed = true;

    std::string result;
    result.reserve(s.length());
    for (unsigned int i=0; i<s.length(); ++i)
    {
	int c = (unsigned char)s[i];
	result += c;
	if (c == 0xFF)
	{
	    result += '\0';
	}
    }
    return result;
}

std::string ID3TagWriter::UnsynchroniseInteger(int i)
{
    std::string r;
    r =  (char)((i>>21) & 0x7F);
    r += (char)((i>>14) & 0x7F);
    r += (char)((i>>7)  & 0x7F);
    r += (char)( i      & 0x7F);
    return r;
}

std::string ID3TagWriter::PlainInteger(int i)
{
    std::string r;
    r =  (char)((i>>24) & 0xFF);
    r += (char)((i>>16) & 0xFF);
    r += (char)((i>>8)  & 0xFF);
    r += (char)( i      & 0xFF);
    return r;
}

/** Called for each ID3v2 frame that's in the file to start with */
void ID3TagWriter::OnFrame(SeekableStream *pStm, const ID3V2_Frame *frame)
{
    std::string frameID(stringpred::FromFixedBuffer(frame->frameID,
						    sizeof(frame->frameID)));

#if 0
    if (frameID == "PRIV")
    {
	/** @todo: deal with private frame */
	TRACEC(TRACE_TAGW, "Private frame\n");
	return;
    }
#endif

    if (m_my_frameids.find(frameID) != m_my_frameids.end())
    {
	TRACEC(TRACE_TAGW, "Discarding existing %s frame\n", frameID.c_str());
	return; // Ignore frame types we've written one of
    }

    TRACEC(TRACE_TAGW, "Keeping existing %s frame\n", frameID.c_str());

    // Size is the frame *contents* size and doesn't include the header
    char *buffer = NEW char[frame->size+10];
    memcpy(buffer, frame->frameID, 4);
    /** pdh 24-Oct-02: Plain, not unsynchronised, integer in ID3v2.3 */
    std::string sizestr = PlainInteger(frame->size);
    memcpy(buffer+4, sizestr.c_str(), 4);
    buffer[8] = frame->flags >> 8;
    buffer[9] = frame->flags & 0xff;

    unsigned int bytesread;
    pStm->Read(buffer + 10, frame->size, &bytesread);

    m_existing_frames.push_back(std::string(buffer,bytesread+10));
    delete[] buffer;
}

/** If the file has an ID3V1 tag, update it. If not, don't bother to write one.
 */
STATUS ID3TagWriter::RewriteID3V1IfPresent()
{
    FileStream fs;

    STATUS st = fs.Open(m_filename,
			FileStream::FLAG_READ | FileStream::FLAG_WRITE);
    if (FAILED(st))
	return st;

    unsigned int file_len;
    if (SUCCEEDED(fs.Length(&file_len))
	&& file_len >= 128
	&& SUCCEEDED(fs.SeekAbsolute(file_len - 128)))
    {
	char buf[4];
	buf[3] = '\0';
	unsigned int bytes_read = 0;
	if (SUCCEEDED(fs.Read(buf, 3, &bytes_read))
	    && bytes_read == 3
	    && !memcmp(buf, "TAG", 3))
	{
	    ID3V1_Tag old_v1;
	    fs.Read(&old_v1.title, 125, &bytes_read);

	    if (m_v1flags & GOT_TITLE)
		memcpy(old_v1.title, m_v1.title, sizeof(old_v1.title));
	    if (m_v1flags & GOT_ARTIST)
		memcpy(old_v1.artist, m_v1.artist, sizeof(old_v1.artist));
	    if (m_v1flags & GOT_ALBUM)
		memcpy(old_v1.album, m_v1.album, sizeof(old_v1.album));
	    if (m_v1flags & GOT_YEAR)
		memcpy(old_v1.year, m_v1.year, sizeof(old_v1.year));
	    if (m_v1flags & GOT_GENRE)
		old_v1.genre = m_v1.genre;
	    if (m_v1flags & GOT_COMMENT)
	    {
		if (old_v1.comment[28] == 0) // Old tag had tracknr
		    memcpy(old_v1.comment, m_v1.comment, 28);
		else
		    memcpy(old_v1.comment, m_v1.comment, 30);
	    }
	    if (m_v1flags & GOT_TRACKNR)
	    {
		old_v1.comment[28] = 0;
		old_v1.comment[29] = m_v1.comment[29];
	    }
	    if (SUCCEEDED(fs.SeekAbsolute(file_len-125)))
	    {
		STATUS st = fs.Write(&old_v1.title, 125, &bytes_read);
		if (FAILED(st))
		{
		    TRACE_WARN("Couldn't rewrite id3v1\n");
		}
		return st;
	    }
	}
    }

    return S_OK;
}

/** Ho boy.
 *
 * The Plan:
 *     (1) Open file, read existing ID3v2 frames, find length of tag
 *     (2) Form list of existing ID3v2 frames we're preserving
 *     (3) Stick them on the end of our ID3v2 frames to form the ID3v2 data
 *     (4) Does this data fit in the existing ID3v2 tag space? If so:
 *           (4a) Add padding to exactly fit existing space
 *           (4b) Rewrite that much of the file
 *     (5) If it doesn't fit (e.g. no previous v2 tag)
 *           (5a) Create a new file, write the tag to it, plus some padding
 *           (5b) Copy rest of original file into new file
 *           (5c) Delete original file, rename new to original
 *     (6) Write ID3v1 tag to file
 *  [[ (7) Rewrite Lyrics200 tag if present? ]]
 *
 * Strictly speaking, we know about ID3v2.4 (or at least, the authors of this
 * code have read the spec); however, we claim to only know about 2.3 as
 * Winamp doesn't grok tags with a 2.4 header.
 */
STATUS ID3TagWriter::Write()
{
    FileStream fs;

    /* Step 1: Open the file
     */
    STATUS st = fs.Open(m_filename,
			FileStream::FLAG_READ | FileStream::FLAG_WRITE);
    if (FAILED(st))
	return st;

    /* Step 2: Read the existing frames, work out which ones we're keeping */
    unsigned int old_v2_size = 0;
    ID3V2TagParser::Parse(&fs, this, &old_v2_size);

    TRACEC(TRACE_TAGW,
	   "Attempting rewrite %lu my frames, %lu existing frames, %lu bytes\n",
	   (unsigned long) m_my_frames.size(),
	   (unsigned long) m_existing_frames.size(),
	   (unsigned long) old_v2_size);

    /* Step 3: Construct entire ID3v2 tag as string */
    std::string tag;
    unsigned int len=0;
    for (frames_t::const_iterator i = m_my_frames.begin();
	 i != m_my_frames.end();
	 ++i)
    {
	len += i->length();
	tag += *i;
    }

    if (m_frame_policy != DISCARD)
    {
	for (frames_t::const_iterator ii = m_existing_frames.begin();
	    ii != m_existing_frames.end();
	    ++ii)
	{
	    len += ii->length();
	    tag += *ii;
	}
    }

    TRACEC(TRACE_TAGW, "Require %lu (%lu) bytes\n",
	   (unsigned long) (tag.length() + 10),
	   (unsigned long) (len+10));

    if (tag.length() + 10 <= old_v2_size && m_gap_policy != NEVER)
    {
	/* Step 4: Thank heavens -- it fits */

	/* Step 4a: Pad */
	tag.resize(old_v2_size - 10, '\0');

	/* Note that the tag size, as opposed to the frame size, IS
	 * unsynchronised even in ID3v2.3.
	 */
	tag = UnsynchroniseInteger(tag.length()) + tag;
	tag = '\0' + tag; // ID3v2 flags
	tag = '\0' + tag; // ID3v2 revision
	tag = '\3' + tag; // ID3v2 minor version
	tag = "ID3" + tag;
	ASSERT(tag.length() == old_v2_size);

	/* Step 4b: Rewrite file */
	fs.SeekAbsolute(0);
	unsigned int bytes_written;
	st = fs.Write(tag.data(), old_v2_size, &bytes_written);
	if (FAILED(st))
	    return st;
	
	if (bytes_written < old_v2_size)
	    return E_ENDOFFILE;

	TRACEC(TRACE_TAGW, "File rewritten in-place\n");
    }
    else
    {
	/* Step 5: Bother. It's the difficult case. */

	tstring new_filename = m_filename + _T(".new");
	FileStream newfs;
	st = newfs.Open(new_filename,
			FileStream::FLAG_WRITE | FileStream::FLAG_READ
			| FileStream::FLAG_CREAT | FileStream::FLAG_TRUNC
			| FileStream::FLAG_SEQUENTIAL);
	if (FAILED(st))
	    return st;

	if (m_gap_policy == ALWAYS)
	{
	    // Leave plenty of gap in case we rewrite again
	    tag.resize(tag.length()+512, '\0');
	}
	else
	{
	    // Ensure at least 1 byte padding for unsync purposes
	    tag.resize(tag.length()+1, '\0');
	}
	unsigned int len = tag.length();
	tag = "ID3   " + UnsynchroniseInteger(len) + tag;
	tag[3] = '\3'; // ID3v2 minor version
	tag[4] = '\0'; // ID3v2 revision
	tag[5] = '\0'; // ID3v2 flags

	newfs.SeekAbsolute(0);
	unsigned int bytes_written;
	st = newfs.Write(tag.data(), tag.length(), &bytes_written);
	if (FAILED(st))
	{
	    TRACE_ERROR("Couldn't open new file '%s'\n", new_filename.c_str());
	    newfs.Close();
	    util::DeleteFile(new_filename.c_str());
	    return st;
	}

	fs.SeekAbsolute(old_v2_size);

	const int BUFFER_SIZE=65536;
	char *copy_buffer = NEW char[BUFFER_SIZE];
	std::auto_ptr<char> delete_me(copy_buffer);
	unsigned int total=0;

	for (;;)
	{
	    unsigned int chunk;
	    st= fs.Read(copy_buffer, BUFFER_SIZE, &chunk);
	    if (SUCCEEDED(st))
		st = newfs.Write(copy_buffer, chunk, &chunk);

	    total += chunk;

	    if (FAILED(st) || st == S_FALSE || chunk == 0)
		break;
	}
	if (FAILED(st))
	{
	    TRACE_ERROR("Couldn't copy file (%x)\n", PrintableStatus(st));
	    newfs.Close();
	    util::DeleteFile(new_filename.c_str());
	    return st;
	}

	TRACEC(TRACE_TAGW, "Rewrote %u bytes\n", total);

	/* OK, we've written a new file. Now flim-flam it into place */
	newfs.Close();
	fs.Close();
	st = util::DeleteFile(m_filename.c_str());
	if (FAILED(st))
	{
	    TRACE_ERROR("Couldn't remove file (%x)\n", PrintableStatus(st));
	    util::DeleteFile(new_filename.c_str());
	    return st;
	}

	st = util::RenameFile(new_filename.c_str(), m_filename.c_str());
	if (FAILED(st))
	{
	    /* This is the only situation in which we're really stuffed, but
	     * hopefully it shouldn't actually arise in practice.
	     */
	    TRACE_ERROR("Disaster, can't rename new to old (%x)\n", PrintableStatus(st));
	    return st;
	}

	/* OK, reopen the new file for ID3v1 purposes */
	st = fs.Open(m_filename, FileStream::FLAG_READ
		     | FileStream::FLAG_WRITE);
	if (FAILED(st))
	{
	    TRACE_ERROR("Can't reopen new file (%x)\n", PrintableStatus(st));
	    return st;
	}
    }
    
    fs.Close();

    /* Does the file have a V1 tag? If so, update it; if not, we don't bother
     * to write one.
     */
    RewriteID3V1IfPresent();

    return S_OK;
}
