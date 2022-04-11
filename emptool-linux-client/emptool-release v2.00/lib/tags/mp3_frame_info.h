/* mp3_frame_info.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#ifndef INCLUDED_MP3FRAMEINFO_H_
#define INCLUDED_MP3FRAMEINFO_H_

#include "vbr_header.h"
#include <string>
#include <string.h>
#include "types.h"
#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

#define SUPPORT_VBRI 1

class SeekableStream;
class TagExtractorObserver;

class FrameInfoExtract
{
    SeekableStream *m_stream;

    /** @todo Implement SeekableStreamCache */
    unsigned char m_buffer[32768];
    int m_buffer_start;
    int m_buffer_length;

    int m_first_header;
    unsigned m_seekPos;
    unsigned m_dataOffset;
    unsigned m_length;
    unsigned long m_shiftReg;

    struct mp3_header
    {
	int layer_code;
	int mpeg_version_code;
	int bitrate_code;
	int samplerate_code;
	int channelmode_code;
	int layer_number;
	int padded;
	int bitrate;
	int samplerate;
	int length;
	int copyright;
	int original;
    };

#if SUPPORT_VBRI
    struct VBRIHeader {
	UINT32 version;
	UINT16 quality;
	UINT32 bytes;
	UINT32 frames;

	// We don't need the rest.
	/*UINT16 toc_size;
	UINT32 unknown;
	UINT16 stride;
	// UINT16 toc[toc_size];*/

	/* We fill these in ourselves: */
	int mpeg_audio_version_id;
	int sample_rate_index;
	int channel_mode;
    };
#endif

    /** @todo Move these out of the class... */
    mp3_header m_mp3Header;
    xing_vbr_header m_xingHeader;
#if SUPPORT_VBRI
    VBRIHeader m_vbriHeader;
#endif

public:
    explicit FrameInfoExtract(SeekableStream *stream)
    : m_stream(stream), m_buffer_start(0), m_buffer_length(0), m_first_header(0),
      m_seekPos(0), m_dataOffset(0), m_length(0), m_shiftReg(0)
    {
	memset(&m_mp3Header, 0, sizeof(mp3_header));
	memset(&m_xingHeader, 0, sizeof(xing_vbr_header));
#if SUPPORT_VBRI
	memset(&m_vbriHeader, 0, sizeof(VBRIHeader));
#endif
    }

    STATUS Extract(int dataOffset, TagExtractorObserver *pObserver);
    STATUS MarkCopyFlag(bool *changed);
    
private:
    void Skip(int howfar);
    void Seek(unsigned pos);
    
    /** Returns a byte (0..255) from the stream, or -1 for error.
     *
     * @todo Return a status.
     */
    int GetByte();
    unsigned long GetWord();
    unsigned Tell() const { return m_seekPos; }
    bool EndOfFile() const { return m_seekPos >= m_length; }

    bool GetXingHeader();
    bool ParseXingHeader(TagExtractorObserver *pObserver);
    int FigureXingHeaderOffset(int h_id, int h_mode);
    void FigureAverageBitrateFromXing(int length_ms, char *buffer, int bufflen);

#if SUPPORT_VBRI
    bool GetVBRIHeader();
    bool ParseVBRIHeader(TagExtractorObserver *pObserver);
    void FigureAverageBitrateFromVBRI(int length_ms, char *buffer, int bufflen);
#endif

    STATUS ParseMP3Frames(bool got_xing_header, TagExtractorObserver *pObserver);
    bool GetMP3Sync(int distance);
    int GetMP3Header();
    int FigureMP3Bitrate(int mpeg_version_code, int layer_number, int bitrate_code);
    int FigureMP3Samplerate(int mpeg_version_code, int samplerate_code);
};

#endif /* INCLUDED_MP3FRAMEINFO_H_ */
