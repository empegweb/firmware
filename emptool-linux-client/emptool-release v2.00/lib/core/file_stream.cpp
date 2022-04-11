/* file_stream.cpp
 *
 * A stream backed by a disk file
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.9 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "file_stream.h"

#include <stdio.h>

#ifdef WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <direct.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <limits.h>
 #include <sys/types.h>
 #include <io.h>
#else
 #include <unistd.h>
 #include <limits.h>
 #include <fcntl.h>
 #include <sys/param.h>
 #include <sys/types.h>
#endif

FileStream::FileStream()
    : m_file_name(),
      m_flags(0),
      m_handle(-1),
      m_is_open(false),
      m_init(false),
      m_current_pos(0),
      m_length(0)
{
}

FileStream::FileStream(int fd)
    : m_file_name(),
      m_flags(0),
      m_handle(fd),
      m_is_open(true),
      m_init(false),
      m_current_pos(0),
      m_length(0)
{
}

FileStream::~FileStream()
{
    if(m_is_open)
	VERIFY(SUCCEEDED(Close()));
}

void FileStream::UpdateSeekPos()
{
    ASSERT(m_is_open);

    off_t pos = lseek(m_handle, 0, SEEK_CUR);
    if(pos == (off_t) -1)
        WARN("lseek failed: %m\n");
    else
        m_current_pos = (unsigned) pos;
}

STATUS FileStream::Init()
{
    // initialise m_current_pos
    UpdateSeekPos();

    // initialise m_length
    struct stat statbuf;
    if(fstat(m_handle, &statbuf))
    {
        WARN("fstat failed: %m\n");
        return MakeErrnoStatus();
    }
    m_length = statbuf.st_size;

    m_init = true;

    return S_OK;
}

STATUS FileStream::Open(const std::string &file_name,
			unsigned flags, unsigned mode)
{
    ASSERT(!m_is_open);

    m_file_name = file_name;
    m_flags = flags;

    unsigned openflags = 0;

    switch(m_flags & (FLAG_READ | FLAG_WRITE))
    {
    case 0:
	WARN("Can't open a file without read/write flag\n");
	return E_NOTIMPL;

    case FLAG_READ:
	openflags |= O_RDONLY;
	break;

    case FLAG_WRITE:
	openflags |= O_WRONLY;
	break;

    case FLAG_READ | FLAG_WRITE:
	openflags |= O_RDWR;
    }

    if(m_flags & FLAG_CREAT)
	openflags |= O_CREAT;
    if(m_flags & FLAG_EXCL)
	openflags |= O_EXCL;
    if(m_flags & FLAG_TRUNC)
	openflags |= O_TRUNC;
    if(m_flags & FLAG_APPEND)
	openflags |= O_APPEND;

#ifdef WIN32
    openflags |= O_BINARY;
    if(m_flags & FLAG_SEQUENTIAL)
        openflags |= O_SEQUENTIAL;
#else
    if(m_flags & FLAG_NOCTTY)
	openflags |= O_NOCTTY;
    if(m_flags & FLAG_NONBLOCK)
	openflags |= O_NONBLOCK;
    if(m_flags & FLAG_SYNC)
	openflags |= O_SYNC;
    if(m_flags & FLAG_NOFOLLOW)
	openflags |= O_NOFOLLOW;
    if(m_flags & FLAG_DIRECTORY)
	openflags |= O_DIRECTORY;
    if(m_flags & FLAG_LARGEFILE)
	openflags |= O_LARGEFILE;
#endif

    m_handle = open(file_name.c_str(), openflags, mode);
    if(m_handle < 0)
    {
	WARN("Failed to open file \"%s\": %m\n", file_name.c_str());
	return MakeErrnoStatus();
    }

    m_is_open = true;
    return S_OK;
}

STATUS FileStream::Close()
{
    ASSERT(m_is_open);

    if(close(m_handle))
    {
	WARN("Failed to close file \"%s\": %m\n", m_file_name.c_str());
	return MakeErrnoStatus();
    }
    m_handle = -1;
    m_is_open = false;
    return S_OK;
}

STATUS FileStream::Read(void *buffer, unsigned bytes_required,
			unsigned *bytes_read)
{
    ASSERT(m_is_open);

    int n = read(m_handle, buffer, bytes_required);
    if(n < 0)
    {
	WARN("Failed to read from file \"%s\": %m\n", m_file_name.c_str());
	return MakeErrnoStatus();
    }
    // n >= 0
    *bytes_read = n;
    m_current_pos += n;

    if ((unsigned) n != bytes_required)
	return S_FALSE;

    return S_OK;
}

STATUS FileStream::Write(const void *buffer, unsigned bytes_available,
			 unsigned *bytes_written)
{
    ASSERT(m_is_open);

    int n = write(m_handle, buffer, bytes_available);
    if(n < 0)
    {
	WARN("Failed to write to file \"%s\": %m\n", m_file_name.c_str());
	return MakeErrnoStatus();
    }
    *bytes_written = n;
    m_current_pos += n;
    return S_OK;
}

STATUS FileStream::Tell(unsigned *position)
{
    ASSERT(m_is_open);
    if(!m_init)
    {
        STATUS status;
        if(FAILED(status = Init()))
            return status;
    }

    *position = m_current_pos;
    return S_OK;
}

STATUS FileStream::SeekAbsolute(unsigned absolutePosition)
{
    ASSERT(m_is_open);

    if(!m_init)
    {
        STATUS status;
        if(FAILED(status = Init()))
            return status;
    }

    if(absolutePosition != m_current_pos)
    {
        off_t result = lseek(m_handle, absolutePosition, SEEK_SET);
        if(result == (off_t) -1)
            return MakeErrnoStatus();
        else
            m_current_pos = result;
    }

    return S_OK;
}

STATUS FileStream::SeekRelative(signed int relativePosition)
{
    ASSERT(m_is_open);

    // already there?
    if(!relativePosition)
        return S_OK;

    if(!m_init)
    {
        STATUS status;
        if(FAILED(status = Init()))
            return status;
    }

    off_t new_pos = lseek(m_handle, relativePosition, SEEK_CUR);
    if(new_pos == (off_t) -1)
        return MakeErrnoStatus();

    m_current_pos = new_pos;
    return S_OK;
}

STATUS FileStream::Length(unsigned *length)
{
    ASSERT(m_is_open);

    if(!m_init)
    {
        STATUS status;
        if(FAILED(status = Init()))
            return status;
    }

    *length = m_length;
    return S_OK;
}

STATUS FileStream::Create( const char *filename, unsigned int flags, FileStream **result )
{
    *result = NEW FileStream();
    if(*result)
        return (*result)->Open(filename, flags);

    return S_FALSE;
}
