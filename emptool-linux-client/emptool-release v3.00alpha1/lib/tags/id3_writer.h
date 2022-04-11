/* id3_writer.h
 *
 * (C) 2002 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 13-Mar-2003 18:15 rob:)
 */

#ifndef ID3_WRITER_H
#define ID3_WRITER_H

#ifndef TAG_WRITER_H
#include "tag_writer.h"
#endif
#ifndef ID3_H
#include "id3.h"
#endif
#ifndef ID3_H
#include "id3v1_format.h"
#endif

#include <string>
#include <set>
#include <vector>

class FileStream;

namespace tags {

class ID3TagWriter: public TagWriter, public ID3V2FrameObserver
{
    tstring m_filename;
    std::set<std::string> m_my_frameids;
    typedef std::vector<std::string> frames_t;
    frames_t m_my_frames;
    frames_t m_existing_frames;

    /* Which parts of ID3v1 should we overwrite? */
    enum {
	GOT_TITLE = 1,
	GOT_ARTIST = 2,
	GOT_ALBUM = 4,
	GOT_YEAR = 8,
	GOT_COMMENT = 0x10,
	GOT_GENRE = 0x20,
	GOT_TRACKNR = 0x40
    };
    unsigned int m_v1flags;
    ID3V1_Tag m_v1;
    int m_gap_policy;
    int m_frame_policy;

    ID3TagWriter(const tstring& filename) 
	: m_filename(filename), m_v1flags(0), m_gap_policy(0), m_frame_policy(0) {}

    static std::string Unsynchronise(const std::string&, bool *was_changed);
    static std::string UnsynchroniseInteger(int);
    static std::string PlainInteger(int);
    static std::string AddFrameHeader(std::string frame,
				      std::string frameID,
				      bool was_unsync);
    STATUS RewriteID3V1IfPresent();

 public:
    static STATUS Create(const tstring& filename, TagWriter **ppWriter);

    enum gap_policy {
	ALWAYS,	    // Leave a bit of space after the ID3v2 tag if rewriting (default)
	PRESERVE,   // Use exact gap if rewriting, otherwise leave gap alone
	NEVER       // Always rewrite with exactly the right gap
    };
    void SetGapPolicy(gap_policy g) { m_gap_policy = g; }

    enum frame_policy {
	KEEP,       // Keep frames we don't know about (e.g. album art) (default)
	DISCARD     // Discard frames we don't know about
    };
    void SetFramePolicy(frame_policy f) { m_frame_policy = f; }

    // Being a TagWriter
    STATUS Write();
    STATUS SetTag(const char *tagname, const char *tagValue);

    // Being an ID3V2FrameObserver
    void OnFrame(SeekableStream *pStm, const ID3V2_Frame *frame);

    std::string StringFrame(const std::string&, const char *which_tag);
    std::string StringFrameNotZero(const std::string&, const char *which_tag);
    std::string PrivateFrame(const std::string&, const char *which_tag);
    std::string TocFrame(const std::string&, const char *which_tag);
    std::string CommentFrame(const std::string&, const char *which_tag);
};

}; // namespace tags

#endif
