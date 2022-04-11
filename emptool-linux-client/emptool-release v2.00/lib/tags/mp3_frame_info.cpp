/* mp3_frame_info.cpp
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11.2.2 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "mp3_frame_info.h"
#include "tag_extractor.h"
#include <stdio.h>
#include "stream.h"
#include "protocol/fids.h"

/// @todo: This code _badly_ needs a rewrite. It is a mountain of hacks. We probably want to borrow
/// the code from mp3_decoder.cpp (and maybe share it) or look at mp3chop.

#define FIND_SYNC_BEFORE_BYTES_COUNT (192 * 1024) /* 192K */

STATUS FrameInfoExtract::Extract(int dataOffset, TagExtractorObserver *pObserver)
{
    /* pdh 18-Sep-00: changed to check for CBR first, as only the CBR check
     * searches for the header (i.e. skips ID3v2).
     */
    m_seekPos = 0;
    m_length = 0;
    STATUS rc = m_stream->Length(&m_length);
    if (FAILED(rc))
	return rc;

    /* ral 17-Dec-01: We've seen some files, created with Musicmatch, and then "fixed"
     * with MP3 Tag Studio, which have the Xing VBR header at the start, followed by a
     * couple of bogus frames.  This causes us to skip the VBR header.
     *
     * We'll have a quick stab at reading it from immediately after the ID3v2 tag.
     */
    m_seekPos = dataOffset;
    bool got_vbr_header = false;
    if (GetXingHeader())
    {
	got_vbr_header = true;
	ParseXingHeader(pObserver);
    }

#if SUPPORT_VBRI
    m_seekPos = dataOffset;
    if (!got_vbr_header && GetVBRIHeader())
    {
	got_vbr_header = true;
	ParseVBRIHeader(pObserver);
    }
#endif

    m_seekPos = dataOffset;
    rc = ParseMP3Frames(got_vbr_header, pObserver);
    if (FAILED(rc))
        return rc;

    /* pdh 31-Aug-01: just in case there was any junk after the id3v2 tag, 
     * don't believe the dataOffset passed in -- instead, use the position
     * where the CBR search found sync.
     */
    m_seekPos = m_first_header;

    // Try for a VBR header...
    if (!got_vbr_header && GetXingHeader())
	ParseXingHeader(pObserver);

    return S_OK;
}

#if SUPPORT_VBRI
bool FrameInfoExtract::GetVBRIHeader()
{
    memset(&m_vbriHeader, 0, sizeof(VBRIHeader));

    UINT32 shift_register = 0;
    shift_register =  GetByte() << 24;
    shift_register |= GetByte() << 16;
    shift_register |= GetByte() << 8;
    shift_register |= GetByte();
    
    m_vbriHeader.mpeg_audio_version_id =    ((shift_register & 0x00180000) >> 3) >> 16;
    m_vbriHeader.sample_rate_index =	    ((shift_register & 0x00000c00) >> 2) >> 8;
    m_vbriHeader.channel_mode =		    ((shift_register & 0x000000c0) >> 6);

    int layer_description =	((shift_register & 0x00060000) >> 1) >> 16;
    int bitrate_index =		((shift_register & 0x0000f000) >> 4) >> 8;
    
    if (m_vbriHeader.channel_mode != 0)
	return false;

    int layer_number = 4 - layer_description;
    if (FigureMP3Bitrate(m_vbriHeader.mpeg_audio_version_id,
			    layer_number, bitrate_index) != 160000)
	return false;

    m_seekPos += 0x20;

    if (GetByte() != 'V') return false;    // fail
    if (GetByte() != 'B') return false;    // header not found
    if (GetByte() != 'R') return false;
    if (GetByte() != 'I') return false;

    m_vbriHeader.version  = GetWord();
    m_vbriHeader.quality  = GetByte() << 8;
    m_vbriHeader.quality |= GetByte();
    m_vbriHeader.bytes    = GetWord();
    m_vbriHeader.frames   = GetWord();

    return true;       // success
}

/** @bug The "1152" is wrong for MPEG-2 and -2.5 layer 3 -- does anyone
 *       generate VBRI headers on such (ultra-low-bitrate) files? Fix by
 *       copying what's done for Xing headers.
 */
bool FrameInfoExtract::ParseVBRIHeader(TagExtractorObserver *pObserver)
{
    // No divide by 0's!
    int length_ms = 0;

    UINT32 samplerate = FigureMP3Samplerate(m_vbriHeader.mpeg_audio_version_id,
					    m_vbriHeader.sample_rate_index);

    if (samplerate != 0)
	length_ms = ((INT64)m_vbriHeader.frames * (INT64)1152 * (INT64)1000) / samplerate;
    
    char bitrate[32];
    FigureAverageBitrateFromVBRI(length_ms, bitrate, 32);
    if (pObserver != NULL)
    {
	pObserver->OnExtractTag("duration", length_ms);
	pObserver->OnExtractTag("bitrate", bitrate);
	pObserver->OnExtractTag("samplerate", samplerate);
    }

    return true;
}
#endif

// get Xing header data    
bool FrameInfoExtract::GetXingHeader()
{
    /* pdh 17-Oct-02: we must get the whole header, as we need to know the
     * frame size (and thus the mpeg version and layer numbers), see
     * ParseXingHeader().
     */
    if (!GetMP3Sync(4))
	return false;

    int h_id = m_mp3Header.mpeg_version_code & 1;
    int h_sr_index = m_mp3Header.samplerate_code;
    int h_mode = m_mp3Header.channelmode_code;

    m_seekPos += FigureXingHeaderOffset(h_id, h_mode);

    char xing[4];
    xing[0] = GetByte();
    xing[1] = GetByte();
    xing[2] = GetByte();
    xing[3] = GetByte();

    if (memcmp(xing, "Xing", 4) != 0)
	return false;

    m_xingHeader.h_id = h_id;
    m_xingHeader.samprate = m_mp3Header.samplerate;

    // get flags
    int head_flags = GetWord();
    m_xingHeader.flags = head_flags;

    if (head_flags & FRAMES_FLAG)
	m_xingHeader.frames = GetWord();

    if (head_flags & BYTES_FLAG)
	m_xingHeader.bytes = GetWord();

    if (head_flags & TOC_FLAG)
    {
	for(int i=0; i<100; i++)
	    m_xingHeader.toc[i] = GetByte();
    }

    m_xingHeader.vbr_scale = 1;
    if (head_flags & VBR_SCALE_FLAG)
	m_xingHeader.vbr_scale = GetWord();

#ifdef PRINT_VBR_TOC
    for(int i = 0; i < 100; i++)
    {
	if ((i % 10) == 0)
	    printf("\n");
	printf(" %3d", m_xingHeader.toc[i]);
    }
#endif

    return true;       // success
}

int FrameInfoExtract::FigureXingHeaderOffset(int h_id, int h_mode)
{
    if (h_id)	// mpeg1
    {        
	if (h_mode != 3)
	    return 32;
	else
	    return 17;
    }
    else	// mpeg2
    {      
	if (h_mode != 3)
	    return 17;
	else
	    return 9;
    }
}

bool FrameInfoExtract::ParseXingHeader(TagExtractorObserver *pObserver)
{
    // No divide by 0's!
    int length_ms;
    if ((m_xingHeader.samprate / 1000) != 0)
    {
	const int mpeg_version = m_mp3Header.mpeg_version_code;
	const int layer_number = m_mp3Header.layer_number;
	int frame_len;

	if (layer_number == 1 || mpeg_version == 3)
	    frame_len = 1152;
	else
	{
	    // Half-size frames in MPEG2 & 2.5, layers 2 & 3
	    frame_len = 576;
	}

	length_ms = (m_xingHeader.frames * frame_len) / (m_xingHeader.samprate / 1000);
    }
    else
    {
	length_ms = 0;
    }
    
    char bitrate[32];
    FigureAverageBitrateFromXing(length_ms, bitrate, 32);
    if (pObserver != NULL)
    {
	pObserver->OnExtractTag("duration", length_ms);
	pObserver->OnExtractTag("bitrate", bitrate);
	pObserver->OnExtractTag("samplerate", m_xingHeader.samprate);
    }

    return true;
}

#ifdef _MSC_VER
#define empeg_snprintf _snprintf
#else
#define empeg_snprintf snprintf
#endif

#if SUPPORT_VBRI
void FrameInfoExtract::FigureAverageBitrateFromVBRI(int length_ms, char *buffer, int bufflen)
{
    // Note the average bitrate
    if ((length_ms / 1000) != 0)
    {
	empeg_snprintf(buffer, bufflen, "v%c%d",
	    (m_mp3Header.channelmode_code == 3) ? 'm' : 's',
	    (m_vbriHeader.bytes * 8) / length_ms);
    }
    else
    {
	// Unknown bitrate: use bitrate of first header
	empeg_snprintf(buffer, bufflen, "v%c%d",
	    (m_mp3Header.channelmode_code == 3) ? 'm' : 's',
	    m_mp3Header.bitrate / 1000);
    }
    buffer[31] = '\0';
}
#endif

void FrameInfoExtract::FigureAverageBitrateFromXing(int length_ms, char *buffer, int bufflen)
{
    // Note the average bitrate
    if ((length_ms / 1000) != 0)
    {
	empeg_snprintf(buffer, bufflen, "v%c%ld",
	    (m_mp3Header.channelmode_code == 3) ? 'm' : 's',
	    (m_xingHeader.bytes * 8) / length_ms);
    }
    else
    {
	// Unknown bitrate: use bitrate of first header
	empeg_snprintf(buffer, bufflen, "v%c%d",
	    (m_mp3Header.channelmode_code == 3) ? 'm' : 's',
	    m_mp3Header.bitrate / 1000);
    }
    buffer[31] = '\0';
}

STATUS FrameInfoExtract::ParseMP3Frames(bool got_xing_header, TagExtractorObserver *pObserver)
{
    if (m_length < 4)
	return E_INVALID_MP3;

    // It's a standard MP3 file.  Look for sync in the first 192Kb (3 lots of 64).
    unsigned start_pos = Tell();
    unsigned end_pos = start_pos + FIND_SYNC_BEFORE_BYTES_COUNT;

    // There is no point in finding sync right at the end of the file.
    if (end_pos > (m_length - 4))
        end_pos = m_length - 4;

    while (Tell() < end_pos)
    {
	if (GetMP3Sync(65536))
	{
	    // We got sync
	    int first_sample_rate = m_mp3Header.samplerate;
	    m_first_header = m_seekPos - 4;

	    // To be valid, the next frame should be the right distance away.
	    Skip(m_mp3Header.length - 4);
	    if (GetMP3Sync(4))
	    {
		// Looks good so far.  Do the sample rates match?
		if (first_sample_rate == m_mp3Header.samplerate)
		{
		    // They do.  There's a good chance that this is valid data.
		    // Note down the bitrate and sample rate.
		    char br[32];
		    sprintf(br, "f%c%d",  (m_mp3Header.channelmode_code == 3) ? 'm' : 's', m_mp3Header.bitrate / 1000);

		    // Work out length
		    int length_ms = 0;
		    if ((m_mp3Header.bitrate / 1000) != 0)
		    {
			length_ms = ((m_length - m_first_header) * 8) / (m_mp3Header.bitrate / 1000);
		    }

		    if (pObserver != NULL)
		    {
			if (!got_xing_header)
			{
			    pObserver->OnExtractTag("bitrate", br);
			    pObserver->OnExtractTag("samplerate", m_mp3Header.samplerate);
			    pObserver->OnExtractTag("duration", length_ms);
			}
			
			// As this is the first frame, this is really where we
			// should be streaming from
			pObserver->OnExtractTag("offset", m_first_header);
			

			if (m_mp3Header.copyright)
			    pObserver->OnExtractTag("copyright", "yes");
			else
			    pObserver->OnExtractTag("copyright", "no");

			if (m_mp3Header.original)
			    pObserver->OnExtractTag("copy", "no");
			else
			    pObserver->OnExtractTag("copy", "yes");
			
		    }
		    
		    return S_OK;
		}
	    }
            else
            {
                // Go back to before but skip a byte so we don't revisit the same sync.
                Seek(m_first_header + 1);
            }
	}
	else
	{
	    // Back up a bit, just in case we managed to straddle the sync...
	    Skip(-3);
	}
    }

    return E_INVALID_MP3;
}

bool FrameInfoExtract::GetMP3Sync(int distance)
{
    // Search until end of file or distance bytes
    if ((m_seekPos + distance) > m_length)
	distance = (m_length - m_seekPos);

    const int bound = m_seekPos + distance;

    // Nothing in the shift register yet
    m_shiftReg = 0;

    // On an empty drive, you can seek forever...
    while((int) m_seekPos < bound)
    {
	int byte = GetByte();
	
	if ( byte < 0 )
	    return false;

	m_shiftReg = (m_shiftReg << 8) | byte;
	
	// Got sync yet?
	if ((m_shiftReg & 0xffe00000) == 0xffe00000)
	{
	    if (GetMP3Header() > 0)
	        return true;
	}
    }

    return false;
}	

void FrameInfoExtract::Skip(int howfar)
{
    if ((m_seekPos + howfar) < m_length)
    {
	m_seekPos += howfar;

	// The shift register will be junk
	m_shiftReg = 0;
    }
}

void FrameInfoExtract::Seek(unsigned pos)
{
    if (pos > m_length)
        pos = m_length;

    m_seekPos = pos;

    // The shift register will be junk
    m_shiftReg = 0;
}

int FrameInfoExtract::GetByte(void)
{
    // Fill buffer as necessary
    if (((int) m_seekPos < m_buffer_start) || ((int) m_seekPos - m_buffer_start) >= m_buffer_length)
    {
	// Valid?
	if (m_seekPos >= m_length)
	{
	    // Simulate reading zeros beyond the end
	    m_seekPos++;
	    return 0;
	}

	// Refill it
	m_buffer_start = m_seekPos;
	if (FAILED(m_stream->SeekAbsolute(m_seekPos)))
	    return -1;

	STATUS rc = m_stream->Read(m_buffer, sizeof(m_buffer), 
				   (unsigned int*)&m_buffer_length);
	if (FAILED(rc))
	    return -1;
    }

    return(m_buffer[(m_seekPos++ - m_buffer_start)]);
}

// See: http://www.dv.co.yu/mpgscript/mpeghdr.htm

int FrameInfoExtract::GetMP3Header()
{
    m_mp3Header.layer_code = (m_shiftReg & 0x60000) >> 17;
    
    // Invalid layer?
    if (m_mp3Header.layer_code == 0)
	return 0;

    m_mp3Header.mpeg_version_code = (m_shiftReg & 0x180000) >> 19;
    
    // Reserved version code
    if (m_mp3Header.mpeg_version_code == 1) 
	return 0;

    m_mp3Header.bitrate_code = (m_shiftReg & 0xf000) >> 12;

    // We don't do free format or invalid bitrates
    if (m_mp3Header.bitrate_code == 0 || m_mp3Header.bitrate_code == 0xf)
	return 0;
    
    m_mp3Header.samplerate_code = (m_shiftReg & 0xc00) >> 10;

    // We don't do bad sample rates
    if (m_mp3Header.samplerate_code == 0x3)
	return 0;
    
    /* 0=stereo, 1=js, 2=dual channel, 3=mono */
    m_mp3Header.channelmode_code = (m_shiftReg & 0xc0) >> 6;

    m_mp3Header.layer_number = 4 - m_mp3Header.layer_code;
    m_mp3Header.padded = (m_shiftReg & 0x200) >> 9;

    m_mp3Header.copyright = (m_shiftReg & 0x8) >> 3;
    m_mp3Header.original =  (m_shiftReg & 0x4) >> 2;

    m_mp3Header.bitrate = FigureMP3Bitrate(m_mp3Header.mpeg_version_code, m_mp3Header.layer_number, m_mp3Header.bitrate_code);
    m_mp3Header.samplerate = FigureMP3Samplerate(m_mp3Header.mpeg_version_code, m_mp3Header.samplerate_code);

    if (m_mp3Header.layer_number == 1)
    {
	// MPEG 1, 2 & 2.5 layer 1
	m_mp3Header.length = (12 * m_mp3Header.bitrate) / m_mp3Header.samplerate + m_mp3Header.padded * 4;
    }
    else if (m_mp3Header.mpeg_version_code == 3)
    {
	// MPEG 1 layers 2 & 3
	m_mp3Header.length = (144 * m_mp3Header.bitrate) / m_mp3Header.samplerate + m_mp3Header.padded;
    }
    else
    {
	// MPEG 2 & 2.5 layers 2 & 3
	
	// Frames are smaller here...
	m_mp3Header.length = (72 * m_mp3Header.bitrate) / m_mp3Header.samplerate + m_mp3Header.padded;
    }

    return m_mp3Header.length;
}

int FrameInfoExtract::FigureMP3Bitrate(int mpeg_version_code, int layer_number, int bitrate_code)
{
    static const short bitrate_table[2][3][16] =
    {
	// MPEG2 & 2.5
	{
	    // Layer 1
	    {
		0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1
	    },
	    // Layer 2
	    {
		0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1
	    },
	    // Layer 3
	    {
		0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1
	    }
	},
	// MPEG1
	{
	    // Layer 1
	    {
		0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1
	    },
	    // Layer 2
	    {
		0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1
	    },
	    // Layer 3
	    {
		0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1
	    }
	}
    };

    return 1000 * bitrate_table[mpeg_version_code & 1]
				[layer_number - 1]
				[bitrate_code];
}

int FrameInfoExtract::FigureMP3Samplerate(int mpeg_version_code, int samplerate_code)
{
    static const int samplerate_table[4][4] =
    {
        // MPEG Version 2.5
        {
	    11025, 12000, 8000, -1
        },
        // Reserved
        {
	    -1, -1, -1, -1
        },
        // MPEG Version 2
        {
	    22050, 24000, 16000, -1
        },
        // MPEG Version 1
        {
	    44100, 48000, 32000, -1
        }
    };

    return samplerate_table[mpeg_version_code]
			    [samplerate_code];
}

unsigned long FrameInfoExtract::GetWord()
{
    unsigned int w;

    w  = GetByte() << 24;
    w |= GetByte() << 16;
    w |= GetByte() << 8;
    w |= GetByte();
    
    return w;
}

STATUS FrameInfoExtract::MarkCopyFlag(bool *changed)
{
    STATUS status = Extract(0, NULL);               // Test file is a valid MP3 firstly
    
    if (FAILED(status))
        return status;
        
    *changed = false;

    if (m_mp3Header.original != 1)                  // Check 'original' flag of 2nd frame
        return S_OK;                                // Leave now if not set

    unsigned i;
    char write_buf[ 4 ];
        
    m_seekPos = m_first_header;
    while (GetMP3Sync(4))
    {
        if (m_mp3Header.original == 1)              // Iff marked as an original mp3
        {        
            m_stream->SeekAbsolute(m_seekPos - 4);  // Seek back to start of header

            m_shiftReg &= ~(1UL << 2);              // Clear 'original' flag
    
            write_buf[ 0 ] = (m_shiftReg >> 24) & 0xff; // Write header in correct order
            write_buf[ 1 ] = (m_shiftReg >> 16) & 0xff;
            write_buf[ 2 ] = (m_shiftReg >> 8) & 0xff;
            write_buf[ 3 ] = m_shiftReg & 0xff;
            
            if (FAILED(status = m_stream->Write(write_buf, sizeof (write_buf), &i)))
                return status;                      // And write back to the file
            *changed = true;
        }        
                
        Skip(m_mp3Header.length - 4);               // Next header should be a fixed distance away
    }    
    
    return S_OK;
}
