/* file_stream.h
 *
 * A stream backed by a disk file
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.11 13-Mar-2003 18:15 rob:)
 */

#ifndef FILE_STREAM_H
#define FILE_STREAM_H			1

#include <string>

#include "types.h"
#include "empeg_tchar.h"
#include "stream.h"

// win32?
// typedef HANDLE FILE_HANDLE;

typedef int FILE_HANDLE;

class FileStream : public SeekableStream
{
protected:
    tstring m_file_name;
    unsigned m_flags;
    FILE_HANDLE m_handle;
    bool m_is_open;
    bool m_init;
    unsigned m_current_pos, m_length;

    void UpdateSeekPos();
    STATUS Init();

public:
    enum {
	FLAG_READ	= 0x0001,
	FLAG_WRITE	= 0x0002,
	FLAG_CREAT	= 0x0004,
	FLAG_EXCL	= 0x0008,
	FLAG_NOCTTY	= 0x0010,
	FLAG_TRUNC	= 0x0020,
	FLAG_APPEND	= 0x0040,
	FLAG_NONBLOCK	= 0x0080,
	FLAG_SYNC	= 0x0100,
	FLAG_NOFOLLOW	= 0x0200,
	FLAG_DIRECTORY	= 0x0400,
	FLAG_LARGEFILE	= 0x0800,
        FLAG_SEQUENTIAL = 0x1000
    };

    enum {
	MODE_DEFAULT	= 0644
    };

    FileStream();
    FileStream(FILE_HANDLE fd);
    virtual ~FileStream();

    virtual STATUS Read(void *buffer, unsigned bytes_required,
			unsigned *bytes_read);
    virtual STATUS Write(const void *buffer, unsigned bytes_available,
			 unsigned *bytes_written);
    inline STATUS Write(const std::string& s) { unsigned int u; return Write(s.c_str(), s.length(), &u); }
#if 0
    virtual STATUS ReadOffset(unsigned int offset, void *buf,
			      unsigned int bytes, unsigned int *bytesread);
#endif

    STATUS Open(const tstring &file_name,
		unsigned flags, unsigned mode = MODE_DEFAULT);
    STATUS Close();

    inline unsigned GetFlags() const { return m_flags; }
    inline FILE_HANDLE GetHandle() const { return m_handle; }

    virtual STATUS Tell(unsigned *position);

    virtual STATUS SeekAbsolute(unsigned absolutePosition);
    virtual STATUS SeekRelative(signed int relativePosition);
    virtual STATUS Length(unsigned *length);

    static STATUS Create(const TCHAR *filename, unsigned int flags, FileStream **result);
};

#endif
