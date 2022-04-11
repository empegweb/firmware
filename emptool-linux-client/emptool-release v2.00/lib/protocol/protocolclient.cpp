/* protocolclient.cpp
 *
 * High-level interface to the player protocol
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.126 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "protocolclient.h"

#ifdef WIN32
 #include <time.h>
 #include <crtdbg.h>
#else
 #include <stdio.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <sys/time.h>
#endif
#include "core/empeg_time.h"
#include "core/empeg_error.h"
#include "protocol_errors.h"
#include "file_stream.h"
#include <algorithm>
#include <string.h>
#include "NgLog/NgLog.h"
#include "connection.h"
#include "fids.h"
#include "dyndata_format.h"
#include "var_string.h"
#include "simple_allocator.h"

#define TRACE_PROTOCOL 0
LOG_COMPONENT(PROTOCOL)

const int ProtocolClient::DEFAULT_MAX_RETRIES = 32;

// We'll retry on out of memory in the hope that some can be found from somewhere. It's worth a try.

#ifdef WIN32
inline bool WINDOWS_RETRY_ERROR(STATUS x)
{
    return (x == MakeWin32Status(ERROR_CRC));
}
#else
inline bool WINDOWS_RETRY_ERROR(STATUS)
{
    return false;
}
#endif

static bool RETRY_ERROR(STATUS x)
{
    return (LOCAL_NAK_STATUS(x) || REMOTE_NAK_STATUS(x)
	    || WINDOWS_RETRY_ERROR(x)
	    || x == MakeErrnoStatus(ETIMEDOUT)
	    || x == MakeErrnoStatus(EWRONGPACKET)
	    || x == MakeErrnoStatus(LINUX_ENOMEM)
            || x == CONN_E_TIMEDOUT);
}

ProtocolClient::ProtocolClient(Connection *p)
    : max_retries(DEFAULT_MAX_RETRIES), connection(p), observer(NULL),
      m_protocol_version_major(-1), m_protocol_version_minor(-1)
{
    ASSERT(connection != NULL);
}

ProtocolClient::~ProtocolClient()
{
}

STATUS ProtocolClient::CheckProtocolVersion()
{
    if ( m_protocol_version_major < 0  // assume OK until proven guilty
	 || m_protocol_version_major == PROTOCOL_VERSION_MAJOR )
	return S_OK;

    if ( m_protocol_version_major > PROTOCOL_VERSION_MAJOR )
	return E_NEWVERSION;

    return E_OLDVERSION;
}

STATUS ProtocolClient::StatFid(FILEID fid, int *psize)
{
    LOG_ENTER("ProtocolClient::StatFid(fid=0x%x, psize=%p)", fid, psize);
    ASSERT(psize);
    STATUS result;

    result = CheckProtocolVersion();
    if ( FAILED(result) )
	return result;

    int retries = 0;
    Request r(connection);
    TRACE("Starting to do StatFID\n");
    do
    {
	result = r.StatFID(fid, psize);
	if ( FAILED(result) )
	{
	    DoReportResult(result);
	    ErrorAction(result, retries);
	}

	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    if ( SUCCEEDED(result) )
	LOG_LEAVE("ProtocolClient::StatFid() = %d", *psize);
    else
	LOG_LEAVE("ProtocolClient::StatFid: error 0x%x\n",
		  PrintableStatus(result) );
    return result;
}

#ifdef FAKE_AS_LOCAL_FILES
int ProtocolClient::ReadFidToMemory(FILEID fid, BYTE **ppdest, int *psize)
{
    const char *filename;
    switch (fid)
    {
    case FID_VERSION:
	filename = "T:\\var\\version";
	break;
    case FID_PLAYLISTDATABASE:
	filename = "T:\\var\\playlists";
	break;
    case FID_TAGINDEX:
	filename = "T:\\var\\tags";
	break;
    case FID_STATICDATABASE:
	filename = "T:\\var\\database";
	break;
    case FID_DYNAMICDATABASE:
	filename = "T:\\var\\dynamic";
	break;
    default:
	filename = "";
	break;
    }
    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
			      OPEN_EXISTING,
			      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			      NULL);

    if (hFile == INVALID_HANDLE_VALUE)
	return MakeWin32Status(GetLastError());

    // Find out how big the file is.
    DWORD size = GetFileSize(hFile, NULL);
    if (size == 0xFFFFFFFF)
    {
	DWORD error = GetLastError();
	VERIFY(CloseHandle(hFile));
	return MakeWin32Status(error);
    }

    // zero-length fids should allocate 1 byte?
    BYTE *buffer = NULL;
    if(size == 0)
	buffer = reinterpret_cast<BYTE *>(EMALLOC(1));
    else
	buffer = reinterpret_cast<BYTE *>(EMALLOC(size));

    ASSERT(buffer);
    *ppdest = buffer;

    int result;
    DWORD bytes_read;
    if (::ReadFile(hFile, buffer, size, &bytes_read, NULL))
    {
	ASSERT(bytes_read == size);
	*psize = size;
	result = 0;
    }
    else
    {
	DWORD error = GetLastError();
	result = MakeWin32Status(error);
    }
    VERIFY(CloseHandle(hFile));
    return result;
}
#endif

STATUS ProtocolClient::ReadFidPartial(FILEID fid, BYTE *buffer, int offset,
				      int required_size, int *actual_size)
{
    LOG_ENTER("ProtocolClient::ReadFidPartial(0x%x, %p, %d, %p)", fid,
		      buffer, required_size, actual_size);
    Request r(connection);

    if (required_size > connection->PacketSize())
	required_size = connection->PacketSize();

    STATUS result;

    int retries = 0;
    do
    {
	result = r.ReadFID(fid, offset, required_size, buffer, actual_size);
	DoReportResult(result);
	++retries;
	if ( FAILED(result) )
	{
	    LOG_INFO("ReadFid(fid=0x%x, offset=0x%x, size=0x%x) failed with error 0x%x, retries=%d\n",
		      fid, offset, required_size, PrintableStatus(result),
		     retries);
	    ErrorAction(result, retries);
	}
    } while (RETRY_ERROR(result) && retries < max_retries);

    LOG_LEAVE("ProtocolClient::ReadFidPartial() returns 0x%x",
	      PrintableStatus(result));
    return result;
}


ProtocolClient::FidInMemory::~FidInMemory()
{
    m_pAlloc->Free(m_buffer);
    delete m_pAlloc;
}

/** The same as ReadFidToMemory, except that it doesn't make a call to StatFID
 * before it starts.  This means that we can't report accurate progress.  To
 * get around this, we use a Zeno's paradox style thing.
 */
STATUS ProtocolClient::ReadFidToMemory2(FILEID fid, FidInMemory **ppfm)
{
    LOG_ENTER("ProtocolClient::ReadFidToMemory2(0x%x)", fid);

    *ppfm = NULL;
    FidInMemory *f = NEW FidInMemory(NEW SimpleAllocator);

    Request r(connection);
    BYTE *buffer = NEW BYTE[connection->PacketSize()];

    STATUS result = S_OK;

    int report_size = 8 * 1024;	    // arbitrary units
    int offset = 0;
    int nbytes;

    int report_offset = 0;
    DoReportProgress(Read, report_offset, report_size);
    do
    {
	int retries = 0;
	do
	{
	    result = r.ReadFID(fid, offset, connection->PacketSize(), buffer,
			       &nbytes);
	    DoReportResult(result);
	    retries++;
	    if (FAILED(result))
	    {
		LOG_INFO("ReadFid(fid=0x%x, offset=0x%x, size=0x%x) failed with error 0x%x, retries=%d\n",
			 fid, offset, connection->PacketSize(),
			 PrintableStatus(result), retries);
		ErrorAction(result, retries);
	    }
	} while (RETRY_ERROR(result) && retries < max_retries);

	if (FAILED(result))
	    break;

	f->Append(buffer, nbytes);

	r.NewRequest();	// next packet id
	offset += nbytes;

	// We're lying about the size, so we'll do a Zeno's Paradox (only with smaller steps)
	// and see what that looks like...
	DoReportProgress(Read, report_offset, report_size);
	report_offset += (report_size - report_offset) / 32;
    } while (nbytes == connection->PacketSize());

    delete [] buffer;

    if (FAILED(result))
	return result;

    DoReportProgress(Read, report_size, report_size);
    *ppfm = f;

    result = S_OK;

    LOG_LEAVE("ProtocolClient::ReadFidToMemory() = %d",
	      PrintableStatus(result));

    return result;
}

#ifndef FAKE_AS_LOCAL_FILES
STATUS ProtocolClient::ReadFidToMemory(FILEID fid, BYTE **ppdest, int *psize)
{
    LOG_ENTER("ProtocolClient::ReadFidToMemory(0x%x, %p, %p)", fid,
		      ppdest, psize);
    STATUS result;
    int size;
    Request r(connection);
    DoReportProgress(Stat, 0, 1);

    ASSERT( ppdest );
    ASSERT( psize );

    result = CheckProtocolVersion();
    if ( FAILED(result) )
	return result;

    int retries = 0;
    do
    {
	result = r.StatFID(fid, &size);
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);

	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    if ( FAILED(result) )
    {
	LOG_LEAVE("ProtocolClient::ReadFidToMemory()\' = 0x%x",
		  PrintableStatus(result));
	return result;
    }

    r.NewRequest();	// we're doing more stuff

    DoReportProgress(Stat, 1, 1);

    ASSERT(size >= 0);
    DoReportProgress(Read, 0, size);

    // zero-length fids should allocate 1 byte?
    BYTE *buffer = NULL;
    if(size == 0)
	buffer = reinterpret_cast<BYTE *>(EMALLOC(1));
    else
	buffer = reinterpret_cast<BYTE *>(EMALLOC(size));

    ASSERT(buffer);
    *ppdest = buffer;

    int offset = 0;
    int nbytes;

    do
    {
	retries = 0;
	do
	{
	    result = r.ReadFID(fid, offset, connection->PacketSize(), buffer,
			       &nbytes);
	    DoReportResult(result);
	    retries++;
	    if (FAILED(result))
	    {
		LOG_INFO("ReadFid(fid=0x%x, offset=0x%x, size=0x%x) failed with error 0x%x, retries=%d\n",
			 fid, offset, connection->PacketSize(),
			 PrintableStatus(result), retries);
		ErrorAction(result, retries);
	    }
	} while (RETRY_ERROR(result) && retries < max_retries);

	if (FAILED(result))
	    break;

	r.NewRequest();	// next packet id
	offset += nbytes;
	buffer += nbytes;
	DoReportProgress(Read, offset, size);
    } while (nbytes == connection->PacketSize());

    if (FAILED(result))
    {
	ASSERT(*ppdest);
	EFREE(*ppdest);
	*ppdest = NULL;
    }
    else
    {
	DoReportProgress(Read, size, size);
	*psize = offset;
	result = S_OK;
    }
    LOG_LEAVE("ProtocolClient::ReadFidToMemory() = %d",
	      PrintableStatus(result));
    return result;
}
#endif

STATUS ProtocolClient::PrepareFid(FILEID fid, int size)
{
    LOG_ENTER("ProtocolClient::PrepareFid(fid=0x%x, size=%d)",
		      fid, size);
    Request r(connection);

    STATUS result;
    int retries = 0;
    do
    {
	result = r.PrepareFID(fid, size);
	DoReportResult(result);

	// Only flush if something went wrong.
	if (FAILED(result))
	    ErrorAction(result, retries);

	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    LOG_LEAVE("ProtocolClient::PrepareFid() = 0x%x", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::WriteFidFromMemory(FILEID fid, const BYTE *psource,
					  int size)
{
    LOG_ENTER("ProtocolClient::WriteFidFromMemory(fid=0x%x, psource=%p, size=%d)",
		      fid, psource, size);
    Request r(connection);

    STATUS result;

    int max_progress = size;
    DoReportProgress(Prepare, 0, max_progress);

    // First prepare the file
    int prepare_retries = 0;
    do
    {
	result = r.PrepareFID(fid, size);
	DoReportResult(result);

	// Only flush if something went wrong.
	if (FAILED(result))
	    ErrorAction(result, prepare_retries);

	++prepare_retries;
    } while (RETRY_ERROR(result) && prepare_retries < max_retries);

    if (FAILED(result))
    {
	LOG_LEAVE("ProtocolClient::WriteFidFromMemory() = %x",
		  PrintableStatus(result));
	return result;
    }

    r.NewRequest();	// more stuff to do

    int offset = 0;
    const BYTE *buffer = psource;
    while (offset < size)
    {
	int write_size = size - offset;
	if (write_size > connection->PacketSize())
	    write_size = connection->PacketSize();
	int write_retries = 0;
	do
	{
	    result = r.WriteFID(fid, offset, write_size, buffer);
	    DoReportResult(result);
	    if (FAILED(result))
	    {
		LOG_INFO("WriteFID(fid=0x%x, offset=0x%x, size=0x%x) failed with error %d\n",
			  fid, offset, write_size, PrintableStatus(result));
		ErrorAction(result, write_retries);
	    }

	    write_retries++;
	} while (RETRY_ERROR(result) && write_retries < max_retries);
	if (FAILED(result))
	    break;
	r.NewRequest();
	offset += write_size;
	buffer += write_size;
	DoReportProgress(Write, offset, max_progress);
    }

    if (SUCCEEDED(result))
    {
	DoReportProgress(Write, max_progress, max_progress);
	result = S_OK;
    }

    LOG_LEAVE("ProtocolClient::WriteFidFromMemory() = %d",
	      PrintableStatus(result));
    return result;
}

// egcs has no auto_ptr otherwise we ought to use that instead.
template<class T>
class AutoDeleteBuffer
{
    T *data;
public:
    AutoDeleteBuffer(T *p) : data(p) {}
    ~AutoDeleteBuffer() { EFREE(data); }
};

#ifndef WIN32
using std::min;
#endif

STATUS ProtocolClient::WriteFidFromFile(FILEID fid, const char *filename)
{
    LOG_ENTER("ProtocolClient::WriteFidFromFile(fid=0x%x, filename=%s)",
		      fid, filename);

    BYTE *buffer = reinterpret_cast<BYTE *>(EMALLOC(connection->PacketSize()));
    // Use an AutoDeleteBuffer so that the buffer is always deleted no matter how we exit.
    AutoDeleteBuffer<BYTE> auto_delete_buffer(buffer);

    // Reset progress report
    DoReportProgress(Prepare, 0, 0);

    FileStream file;
    STATUS result = file.Open(filename, FileStream::FLAG_READ | FileStream::FLAG_SEQUENTIAL);

    if (FAILED(result))
	return result;

    // Find out how big the file is.
    unsigned int size;
    result = file.Length( &size );

    if (FAILED(result))
        return result;

    int max_progress = size;
    DoReportProgress(Prepare, 0, max_progress);
    Request r(connection);

    int retries = 0;

#if DEBUG>0
    Time startTime = Time::Now();
#endif

    do
    {
	result = r.PrepareFID(fid, size);
	if (FAILED(result))
	{
	    LOG_INFO("Failed to PrepareFID(0x%x, 0x%x) - result=0x%x",
		     fid, size, PrintableStatus(result));
	    ErrorAction(result, retries);
	}
	DoReportResult(result);
	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    if (FAILED(result))
    {
	LOG_LEAVE("ProtocolClient::WriteFidFromFile(A)\' = %d",
		  PrintableStatus(result));
	return result;
    }

    Connection *fc = connection->GetFastConnection();
    if (fc)
    {
        LOG_INFO("Attempting fastwrite(%u)\n", size);

        TCPFastHeader fh;
        fh.operation = 0;
        fh.fid = fid;
        fh.offset = 0;
        fh.size = size;

        result = fc->Send((const unsigned char *)&fh, sizeof(fh));
        if (FAILED(result))
        {
            LOG_ERROR("Writing fast-protocol header failed\n");
	    LOG_LEAVE("ProtocolClient::WriteFidFromFile(H) = %x",
		      PrintableStatus(result));
            return result;
        }

        unsigned int lump = connection->PacketSize();
        size_t nbytes;
        unsigned int remain = size;

        /* Read from the file and squirt out to the socket as quickly as
         * possible. We would use TransmitFile, but Win95 doesn't have it.
         */
        result = S_OK;

        do {
            result = file.Read(buffer, min(lump,remain), &nbytes);
            if (FAILED(result))
                break;

            result = fc->Send(buffer, nbytes);
            if (FAILED(result))
                break;

            remain -= nbytes;

            DoReportProgress(Write, size - remain, max_progress);
        } while ( remain > 0 );

        if ( remain > 0 )
        {
            LOG_WARN("Fastwrite has %u bytes remaining\n", remain);
        }
    }
    else
    {
        r.NewRequest();	// more stuff!

        unsigned int read_size;
        int offset = 0;
        do
        {
            result = file.Read(buffer, connection->PacketSize(), &read_size);

            if (FAILED(result))
	    {
	        read_size = 0;
	    }

	    if (read_size)
	    {
	        BYTE *write_buffer = buffer;
	        retries = 0;
	        do
	        {
    		    result = r.WriteFID(fid, offset, (int)read_size, write_buffer);
		    DoReportResult(result);
		    if (FAILED(result))
		    {
        	        LOG_INFO("WriteFID(fid=0x%x, offset=0x%x, size=0x%x) failed with error %d\n",
				 fid, offset, read_size,
				 PrintableStatus(result));
		        ErrorAction(result, retries);
		    }
		    retries++;
	        } while (RETRY_ERROR(result) && retries < max_retries);

	        if (FAILED(result))
	        {
    		    read_size = 0;
		    break;
	        }
	    }
	    r.NewRequest();
	    offset += read_size;
	    DoReportProgress(Write, offset, max_progress);
        }
        while (read_size == static_cast<DWORD>(connection->PacketSize()));
    }

#if DEBUG>0
    unsigned long elapsedTime = (Time::Now() - startTime).GetAsMilliseconds();
    INT64 tenBytesPerSec = ((INT64)(size * 100)) / elapsedTime;
    LOG_INFO("WriteFID(%d bytes) took %ldms (%d.%02dK/s)\n", size, elapsedTime,
	     (int)(tenBytesPerSec/100), (int)(tenBytesPerSec%100) );
#endif

    if (SUCCEEDED(result))
    {
	DoReportProgress(Write, max_progress, max_progress);
    }
    LOG_LEAVE("ProtocolClient::WriteFidFromFile(C) = 0x%x",
	      PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::WriteFidPartial(FILEID fid, const BYTE *psource,
				       int pos, int size)
{
    LOG_ENTER("ProtocolClient::WriteFidPartial(fid=0x%x, psource=%p, pos=%d, size=%d)",
		      fid, psource, pos, size);
    Request r(connection);

    STATUS result = S_OK;
    int offset = 0;
    const BYTE *buffer = psource;
    while (offset < size)
    {
	int write_size = size - offset;
	if (write_size > connection->PacketSize())
	    write_size = connection->PacketSize();
	int retries = 0;
	do
	{
	    result = r.WriteFID(fid, pos + offset, write_size, buffer);
	    DoReportResult(result);
	    if (FAILED(result))
	    {
		LOG_INFO("WriteFID(fid=0x%x, offset=0x%x, size=0x%x) failed with error %x\n",
			  fid, pos + offset, write_size,
			 PrintableStatus(result));
		ErrorAction(result, retries);
	    }

	    retries++;
	} while (RETRY_ERROR(result) && retries < max_retries);
	if (FAILED(result))
	    break;
	r.NewRequest();
	offset += write_size;
	buffer += write_size;
    }

    LOG_LEAVE("ProtocolClient::WriteFidPartial() = %x",
	      PrintableStatus(result));
    return result;
}

void ProtocolClient::FreeFidMemory(BYTE **ppdest)
{
    LOG_ENTER("ProtocolClient::FreeFidMemory(ppdest=%p)",ppdest);
    LOG_INFO("*ppdest=%p", *ppdest);
    ASSERT(ppdest);
    EFREE(*ppdest);
    *ppdest = NULL;
    LOG_LEAVE("ProtocolClient::FreeFidMemory()");
}

STATUS ProtocolClient::CheckDatabaseAvailability()
{
    LOG_ENTER("ProtocolClient::CheckDatabaseAvailability()");
    STATUS result;
    int size = 0;

    result = CheckProtocolVersion();
    if ( FAILED(result) )
	return result;

    result = StatFid(FID_TAGINDEX, &size);
    if (SUCCEEDED(result))
    {
	LOG_INFO("Tag index size is %d bytes\n", size);

	if (size < 16)
	{
	    LOG_INFO("Tag database is suspiciously small. Failing");

	    // Pretend it doesn't exist
	    result = MakeErrnoStatus(LINUX_ENOENT);
	}
    }
    else
    {
	LOG_INFO("Tag index stat failed: %d", PrintableStatus(result));
    }
    if (SUCCEEDED(result))
    {
	result = StatFid(FID_STATICDATABASE, &size);
	if (SUCCEEDED(result))
	{
	    LOG_INFO("Static database size is %d bytes\n", size);
            // A completely empty database is around ten bytes.
	    if (size < 10)
	    {
		LOG_INFO("Static database is suspiciously small. Failing.");
		// Pretend it doesn't exist
		result = MakeErrnoStatus(LINUX_ENOENT);
	    }
	}
	else
	{
	    LOG_INFO("Static database stat failed: %d\n", PrintableStatus(result));
	}
    }
    if (SUCCEEDED(result))
    {
	result = StatFid(FID_PLAYLISTDATABASE, &size);
	if (SUCCEEDED(result))
	{
	    LOG_INFO("Playlist database size is %d bytes\n", size);
	}
	else
	{
	    LOG_INFO("Playlist database stat failed: %d\n", PrintableStatus(result));
	}
    }
    LOG_LEAVE("ProtocolClient::CheckDatabaseAvailability() = %d", PrintableStatus(result));
    return result;
}

/** @todo This code has been copied to model/database_parser.{cpp,h}.
 * Unfortunately, emptool (and currently emplode) are still using it.
 * Changing emplode is easy.  Changing emptool is hard.
 */
STATUS ProtocolClient::RetrieveTagIndex(TuneDatabaseObserver *db_observer)
{
    LOG_ENTER("ProtocolClient::RetrieveTagIndex(db_observer=%p)", db_observer);
    STATUS result;
    BYTE *tag_buffer;
    int tag_buffer_size;

    result = CheckProtocolVersion();
    if ( FAILED(result) )
	return result;

    result = ReadFidToMemory(FID_TAGINDEX, &tag_buffer, &tag_buffer_size);
    if (SUCCEEDED(result))
    {
	if (tag_buffer_size <= 16)
	{
	    // Hmm, that's suspiciously short.
	    LOG_INFO("Tags file is suspiciously short (%d bytes), failing gracefully.", tag_buffer_size);

	    // Pretend it isn't there
	    result = MakeErrnoStatus(LINUX_ENOENT);

	    // Fix a leak...
	    FreeFidMemory(&tag_buffer);
	}
    }

    if (SUCCEEDED(result))
    {
	// Now iterate over the list calling the observer for each entry.

	const char *ptr = reinterpret_cast<const char *>(tag_buffer);
	const char *end = reinterpret_cast<const char *>(ptr + tag_buffer_size);

	int tag_index = 0;
	while (ptr < end)
	{
	    // pdh 13-Jun-00: memchr not strchr, it won't be 0-terminated
	    const char *eos = reinterpret_cast<const char*>( memchr(ptr, '\n', end-ptr) );
	    if (eos == NULL)
		break;
	    std::string tag_name(ptr, eos);

	    // If we're zero length then it's just a placeholder for a deleted
	    // tag. Ignore it and skip to the next one
	    if (tag_name.length() > 0)
	    {
		result = db_observer->TagIndexAction(tag_index, tag_name.c_str());
		if (FAILED(result))
		    break;
	    }

	    // Skip to next string
	    ptr = eos + 1;
	    tag_index++;
	}
	FreeFidMemory(&tag_buffer);
    }

    LOG_LEAVE("ProtocolClient::RetrieveTagIndex() = %d", PrintableStatus(result));

    return result;
}

STATUS ProtocolClient::DeleteDatabases()
{
    LOG_ENTER("ProtocolClient::DeleteDatabases()");
    STATUS result;

    result = DeleteFid(FID_STATICDATABASE);
    if (SUCCEEDED(result))
    {
	result = DeleteFid(FID_TAGINDEX);
    }
    if (SUCCEEDED(result))
    {
	result = DeleteFid(FID_PLAYLISTDATABASE);
    }
    LOG_LEAVE("ProtocolClient::DeleteDatabases() = %d", PrintableStatus(result));
    return result;
}

typedef struct {
    size_t version;
    size_t itemsize;
    BYTE data[1]; // or more
} dynamic_database;

STATUS ProtocolClient::RetrieveDatabases(TuneDatabaseObserver *db_observer)
{
    LOG_ENTER("ProtocolClient::RetrieveDatabases(db_observer=%p)", db_observer);

    // First request the static database to memory.
    STATUS result;

    result = CheckProtocolVersion();
    if ( FAILED(result) )
	return result;

    dynamic_database *dynamic_buffer;
    int dynamic_buffer_size;

    ProtocolClient::FidInMemory *pFidInMemory;
    result = ReadFidToMemory2(FID_STATICDATABASE, &pFidInMemory);
    if (FAILED(result))
    {
	LOG_LEAVE("ProtocolClient::RetrieveDatabases()\' = %d", PrintableStatus(result));
	return result;
    }

    result = ReadFidToMemory(FID_DYNAMICDATABASE, (BYTE**)&dynamic_buffer,
			     &dynamic_buffer_size);
    if (FAILED(result))
    {
	/* We don't care too much */
	dynamic_buffer = NULL;
	dynamic_buffer_size = 0;
    }

    result = S_OK;
    int static_pos = 0;
    int db_index = 0;

    const BYTE *static_buffer = pFidInMemory->GetBuffer();
    int static_buffer_size = pFidInMemory->GetSize();
    while (static_pos < static_buffer_size)
    {
	DynamicData dd;
	BYTE *dyn = NULL;

	if ( dynamic_buffer )
	{
	    size_t srcitemsize = dynamic_buffer->itemsize;

	    dyn = dynamic_buffer->data
		  + db_index * srcitemsize;

	    if ( ( dyn + srcitemsize - (BYTE*)dynamic_buffer )
		 <= dynamic_buffer_size )
	    {
		ConvertDynamicData( dyn, srcitemsize, dynamic_buffer->version,
				    &dd );
	    }
	    else
		dyn = NULL;
	}

	if (static_buffer[static_pos] != 0xFF)
	    result = db_observer->DatabaseEntryAction(MakeFid(db_index, FIDTYPE_TUNE),
						      static_buffer + static_pos,
						      dyn ? &dd : NULL );
	if (result != S_OK)
	    break;

	// Skip to the next record
	while (static_buffer[static_pos] != 0xFF)
	{
	    ASSERT(static_pos < static_buffer_size);
	    static_pos++;
	    static_pos += static_buffer[static_pos] + 1;
	}
	static_pos++;
	db_index++;
    }
    delete pFidInMemory;
    FreeFidMemory((BYTE**)&dynamic_buffer);
    LOG_LEAVE("ProtocolClient::RetrieveDatabases() = %d", PrintableStatus(result));
    return result;
}

bool ProtocolClient::IsUnitConnected()
{
    LOG_ENTER("ProtocolClient::IsUnitConnected()");
    Request r(connection);
    bool result = SUCCEEDED(r.Ping());

    if (result)
    {
	r.GetProtocolVersion(&m_protocol_version_major,
			      &m_protocol_version_minor);
    }

    LOG_LEAVE("ProtocolClient::IsUnitConnected()=%d", result);
    return result;
}

STATUS ProtocolClient::WaitForUnitConnected(int timeout)
{
    LOG_ENTER("ProtocolClient::WaitForUnitConnected(timeout=%d)", timeout);

    connection->Pause();
    bool opened = false;

    if (FAILED(connection->Unpause()))
    {
//	TRACE("reconnect failed (%d)\n", rc );
    }
    else
	opened = true;

    Request r(connection);

    STATUS result = S_OK;
    time_t start = time(NULL);
    do
    {
	int offset = time(NULL) - start;
	if (offset > timeout)
	    offset = timeout;
	DoReportProgress(Waiting, offset, timeout);

	if ( !opened )
	{
            if (FAILED(connection->Unpause()))
	    {
//		TRACE("reconnect failed (%d)\n", rc );
	    }
	    else
	    {
//		TRACE("reconnect succeeded\n");
		opened = true;
	    }
	}
	else
	{
	    result = r.Ping();

	    if ( SUCCEEDED(result) )
	    {
		r.GetProtocolVersion( &m_protocol_version_major,
				      &m_protocol_version_minor );
	    }
	}
    } while ( (!opened || RETRY_ERROR(result))
	      && time(NULL) < start + timeout);

    DoReportProgress(Waiting, timeout, timeout);
    LOG_LEAVE("ProtocolClient::WaitForUnitConnected()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::EnableWrite(bool b)
{
    LOG_ENTER("ProtocolClient::EnableWrite(b=%d)", b);
    Request r(connection);
    DoReportProgress(Remount, 0, 1);

    STATUS result;
    int retries = 0;
    do
    {
	result = r.Mount(b ? 1 : 0);
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);
	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    DoReportProgress(Remount, 1, 1);
    LOG_LEAVE("ProtocolClient::EnableWrite()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::RebuildPlayerDatabase(int options)
{
    LOG_ENTER("ProtocolClient::RebuildPlayerDatabase(options=%d)", options);
    Request r(connection);

    RequestProgress request_progress(this, Rebuild);
    if(observer)
	r.SetObserver(&request_progress);

    //DoReportProgress(Rebuild, 0, 1);

    STATUS result;
    int retries = 0;
    do
    {
	result = r.BuildAndSaveDatabase(options);
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);
	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    DoReportProgress(Rebuild, 1, 1);
    LOG_LEAVE("ProtocolClient::RebuildPlayerDatabase()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::DeleteFid(FILEID fid, int mask)
{
    LOG_ENTER("ProtocolClient::DeleteFid(fid=%d, mask=0x%x)", fid, mask);
    Request r(connection);
    DoReportProgress(Delete, 0, 1);

    STATUS result;
    int retries = 0;
    do
    {
	result = r.DeleteFID(fid, mask);
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);
	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    DoReportProgress(Delete, 1, 1);

    LOG_LEAVE("ProtocolClient::DeleteFid()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::GetDiskInfo(DiskSpaceInfo *info, int count)
{
    ASSERT(count > 0);

    LOG_ENTER("ProtocolClient::GetDiskInfo(info=%p, count=%d)", info, count);

    memset(reinterpret_cast<void *>(info), 0, count * sizeof(DiskInfo));

    STATUS result;

    result = CheckProtocolVersion();
    if ( FAILED(result) )
	return result;

    int retries = 0;
    Request r(connection);
    DoReportProgress(DiskInfo, 0, 1);

    do
    {
	if (count == 1)
	{
	    result = r.FreeSpace(&info[0].size, &info[0].space, &info[0].block_size,
				 NULL, NULL, NULL);
	}
	else
	{
	    result = r.FreeSpace(&info[0].size, &info[0].space, &info[0].block_size,
				 &info[1].size, &info[1].space, &info[1].block_size);
	}
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);
	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    DoReportProgress(DiskInfo, 1, 1);

    LOG_INFO("Disk 0: size=%ld, space=%ld, block_size=%ld",
	     (long)info[0].size, (long)info[0].space, (long)info[0].block_size);
    LOG_INFO("Disk 1: size=%ld, space=%ld, block_size=%ld",
	     (long)info[1].size, (long)info[1].space, (long)info[1].block_size);

    LOG_LEAVE("ProtocolClient::GetDiskInfo() = %d", PrintableStatus(result));
    return result;
}

#define XTRACE LOG_INFO

STATUS ProtocolClient::Fsck(const char *drive, unsigned int *pflags)
{
    LOG_ENTER("ProtocolClient::Fsck(drive='%s')", drive);
    Request r(connection);

    ASSERT(pflags);
    *pflags = 0;

    RequestProgress request_progress(this, Check);
    if(observer) r.SetObserver(&request_progress);

    int retries = 0;
    STATUS result;
    unsigned int fsck_flags;
    do
    {
	connection->FlushReceiveBuffer();
	result = r.Fsck(drive,0,&fsck_flags);
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);
	++retries;
    }
    while (RETRY_ERROR(result) && retries < max_retries);

    // Did it fail outright?
    if (SUCCEEDED(result))
    {
	bool bad = false;

	XTRACE("FSCK result: %d", fsck_flags);
	if (fsck_flags & 1)
	{
	    XTRACE("FSCK corrected some errors on drive %s", drive);
	}
	if (fsck_flags & 2)
	{
	    XTRACE("FSCK thinks the system should be rebooted on drive %s", drive);
	    bad = true;
	}
	if (fsck_flags & 4)
	{
	    XTRACE("FSCK left stuff uncorrected on drive %s", drive);
	    bad = true;
	}
	if (fsck_flags & 8)
	{
	    XTRACE("FSCK reported operational error on drive %s", drive);
	    bad = true;
	}
	if (fsck_flags & 16)
	{
	    XTRACE("FSCK report usage or syntax error on drive %s", drive);
	    bad = true;
	}
	if (fsck_flags & 128)
	{
	    XTRACE("FSCK reported shared library error on drive %s", drive);
	    bad = true;
	}
	if (fsck_flags & 512)
	{
	    XTRACE("FSCK reported disk is not present on drive %s", drive);
	}

	if (bad)
	    result = E_PROTOCOL_FSCK;

	*pflags = fsck_flags;
    }
    LOG_LEAVE("ProtocolClient::Fsck()=%x", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::CheckMedia( unsigned *pfsck_flags )
{
    LOG_ENTER("ProtocolClient::CheckMedia()");
    STATUS result;
    //DoReportProgress(Check, 0, 2);

    result = Fsck("/dev/hda4", pfsck_flags);
    if (FAILED(result))
	return result;

    //DoReportProgress(Check, 1, 2);

    result = Fsck("/dev/hdc4", pfsck_flags);
    if (FAILED(result))
	return result;

    //DoReportProgress(Check, 2, 2);
    LOG_LEAVE("ProtocolClient::CheckMedia()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::SendCommand(int command, int parameter0, int parameter1,
				   const char *parameter2)
{
    LOG_ENTER("ProtocolClient::SendCommand(command=%d, p0=%d, p1=%d, p2=%p", command, parameter0, parameter1, parameter2);
    Request r(connection);
    STATUS result;
    int retries = 0;
    do
    {
	connection->FlushReceiveBuffer();
	result = r.SendCommand(command, parameter0, parameter1, parameter2);
	DoReportResult(result);
	if (FAILED(result))
	    ErrorAction(result, retries);
	++retries;
    } while (RETRY_ERROR(result) && retries < max_retries);

    LOG_LEAVE("ProtocolClient::SendCommand()=%d", PrintableStatus(result));
    return result;
}

/** Make sure you've called srand at some point before this. */
STATUS ProtocolClient::PlayFidList(const vecFids &fids, int mode)
{
    typedef std::vector<std::string> vecGroups;
    vecGroups fid_groups;
    std::string fid_group = "";
    const size_t max_size_of_fid_group = sizeof(CommandRequestPacket().parameter2);
    const int max_length_of_single_fid = 10;	// Characters in representation eg.",2970"
    int collective_id = ::rand();

    for (std::vector<FILEID>::const_iterator fid_i = fids.begin();
	 fid_i != fids.end(); ++fid_i)
    {
	if (fid_group.length() >= max_size_of_fid_group - max_length_of_single_fid)
	{
	    // Collect the group string and recycle it
	    fid_groups.push_back(fid_group);
	    fid_group = "";
	}

	FILEID this_fid = *fid_i;
	if (fid_group.length() == 0)
	    fid_group = VarString::Printf("<%x>", collective_id);
	else
	    fid_group += ',';
        fid_group += VarString::Printf("%x", this_fid);
	ASSERT(fid_group.length() < max_size_of_fid_group);
    }

    // Now send out collected groups followed by out latest one.
    // On getting the latest one, the player
    // should collate the lot & start playing them
    STATUS result = S_OK;
    for (vecGroups::const_iterator group_i = fid_groups.begin();
	 group_i != fid_groups.end() && !FAILED(result); ++group_i)
    {
	std::string collected_group = *group_i;
	result = SendCommand(COM_BUILDMULTFIDS, 0, 0, collected_group.c_str());
    }

    if (!FAILED(result))
	result = SendCommand(COM_PLAYMULTFIDS, 0, mode, fid_group.c_str());

    return result;
}

STATUS ProtocolClient::RestartUnit(bool auto_slumber, bool wait_for_completion)
{
    int param = RESTART_UNIT;
    if (auto_slumber)
        param |= RESTART_IN_SLUMBER;
    return InternalRestart(param, wait_for_completion);
}

STATUS ProtocolClient::RestartPlayer(bool auto_slumber, bool wait_for_completion)
{
    int param = RESTART_PLAYER;
    if (auto_slumber)
        param |= RESTART_IN_SLUMBER;
    return InternalRestart(param, wait_for_completion);
}

STATUS ProtocolClient::InternalRestart(int param, bool wait_for_completion)
{
    LOG_ENTER("ProtocolClient::InternalRestart(param=%d, wait_for_completion=%d)", param, wait_for_completion);

    STATUS result = SendCommand(COM_RESTART, param);
    if (SUCCEEDED(result))
    {
	LOG_LEAVE("Restart command sent, waiting for player to begin responding\n");
	// Just in case the player is still responding we don't want to talk to it on the
	// way down so wait a little while before talking to it.

        if (wait_for_completion)
        {
#ifdef WIN32
            Sleep(1500); // 1.5 seconds.
#else
	    sleep(2); // 2 seconds
#endif
	    result = WaitForUnitConnected();
        }
    }

    LOG_LEAVE("ProtocolClient::InternalRestart()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::LockUI(bool lock)
{
    LOG_ENTER("ProtocolClient::LockUI(%d)", lock);
    STATUS result = SendCommand(COM_LOCKUI, lock ? true : false);
    LOG_LEAVE("ProtocolClient::LockUI()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::SetSlumber(bool slumber)
{
    LOG_ENTER("ProtocolClient::SetSlumber(%d)", slumber);
    STATUS result = SendCommand(COM_SLUMBER, slumber ? true : false);
    LOG_LEAVE("ProtocolClient::SetSlumber()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::PlayFid(FILEID fid, int mode)
{
    LOG_ENTER("ProtocolClient::PlayFid(0x%x)", fid);
    STATUS result = SendCommand(COM_PLAYFID, fid, mode);
    LOG_LEAVE("ProtocolClient::PlayFid()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::PlayFids(const vecFids &fids, int mode)
{
    LOG_ENTER("ProtocolClient::PlayFids(0x%x)", fids.size());
    STATUS result = PlayFidList(fids, mode);
    LOG_LEAVE("ProtocolClient::PlayFids()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::SetPlayState(int pp)
{
    LOG_ENTER("ProtocolClient::SetPlayState(0x%x)", pp);
    STATUS result = SendCommand(COM_PLAYSTATE, pp);
    //printf("Result=%d\n", result);
    LOG_LEAVE("ProtocolClient::SetPlayState()=%d", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::GrabScreen(unsigned char *screen)
{
    LOG_ENTER("ProtocolClient::GrabScreen(screen=%p)", screen);
    Request r(connection);

    // Don't bother to retry this :-)
    STATUS result = r.GrabScreen(0,screen);
    LOG_LEAVE("ProtocolClient::GrabScreen()=%x", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::InitiateSession(const char *host_description, const char *password, UINT32 *session_cookie, std::string *failure_reason)
{
    LOG_ENTER("ProtocolClient::InitiateSession");
    Request r(connection);

    STATUS result = r.InitiateSession(host_description, password, session_cookie, failure_reason);
    LOG_LEAVE("ProtocolClient::InitiateSession");
    return result;
}

STATUS ProtocolClient::SessionHeartbeat(UINT32 session_cookie)
{
    LOG_ENTER("ProtocolClient::SessionHeartbeat");
    Request r(connection);

    STATUS result = r.SessionHeartbeat(session_cookie);
    LOG_LEAVE("ProtocolClient::SessionHeartbeat");
    return result;
}

STATUS ProtocolClient::TerminateSession(UINT32 session_cookie)
{
    LOG_ENTER("ProtocolClient::TerminateSession");
    Request r(connection);

    STATUS result = r.TerminateSession(session_cookie);
    LOG_LEAVE("ProtocolClient::TerminateSession");
    return result;
}

STATUS ProtocolClient::NotifySyncComplete()
{
    LOG_ENTER("ProtocolClient::NotifySyncComplete");
    Request r(connection);

    STATUS result = r.NotifySyncComplete();
    LOG_LEAVE("ProtocolClient::NotifySyncComplete");
    return result;
}

STATUS ProtocolClient::GetPlayerType(std::string *type)
{
    LOG_ENTER("ProtocolClient::GetPlayerType");
    char *buffer = NULL;
    int length;
    STATUS result = ReadFidToMemory(FID_PLAYERTYPE,
				    reinterpret_cast<BYTE **>(&buffer),
				    &length);
    if (SUCCEEDED(result))
	*type = std::string(buffer, length - 1);

    FreeFidMemory(reinterpret_cast<BYTE **>(&buffer));

    LOG_LEAVE("ProtocolClient::GetPlayerType()=%x", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::GetVersionInfo(PlayerVersionInfo *info)
{
    LOG_ENTER("ProtocolClient::GetVersionInfo(info=%p)", info);
    ASSERT(info);
    char *buffer = NULL;
    int length;
    STATUS result = ReadFidToMemory(FID_VERSION,
				    reinterpret_cast<BYTE **>(&buffer),
				    &length);
    if (SUCCEEDED(result))
    {
	char temp_protocol[8];
	char *dests[3];
	const int lengths[3] = { sizeof(info->version), sizeof(info->beta), sizeof(temp_protocol) };
	const int max_count = 3;

	dests[0] = info->version;
	dests[1] = info->beta;
	dests[2] = temp_protocol;

	int start = 0, end = 0;
	int count = 0;
	while (end < length)
	{
	    if (buffer[end] == '\n' && count < max_count)
	    {
		buffer[end] = 0;
		strncpy(dests[count], buffer + start, lengths[count]);
		count++;
		start = end + 1;
	    }
	    end++;
	}
	if (end > start && count < max_count)
	{
	    int len = end - start;
	    if (lengths[count] < len)
		len = lengths[count];
	    strncpy(dests[count], buffer + start, len);
	    dests[count][len] = 0;
	    count++;
	}
	// FIXME: result = count;
	while (count < max_count)
	{
	    *dests[count++] = 0;
	}
	info->protocol = atoi(temp_protocol);
	LOG_INFO("Connected player version information:");
	LOG_INFO("   Version:  %s", info->version);
	LOG_INFO("   Beta:     %s", info->beta);
	LOG_INFO("   Protocol: %d", info->protocol);
    }
    FreeFidMemory(reinterpret_cast<BYTE **>(&buffer));
    LOG_LEAVE("ProtocolClient::GetVersionInfo()=%x", PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::GetProtocolVersion(short *major, short *minor)
{
    LOG_ENTER("ProtocolClient::GetProtocolVersion(major=%p, minor=%p)",
	      major, minor);

    STATUS result = S_OK;

    if ( m_protocol_version_major < 0 )
    {
	// We need to do a ping to get the info.
	Request r(connection);
	result = r.Ping();
	if (SUCCEEDED(result))
	{
	    result = r.GetProtocolVersion(&m_protocol_version_major,
					  &m_protocol_version_minor);
	}
    }

    if ( SUCCEEDED(result) )
    {
	*major = m_protocol_version_major;
	*minor = m_protocol_version_minor;
    }

    LOG_LEAVE("ProtocolClient::GetProtocolVersion()=%d",
	      PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::GetPlayerConfiguration(std::string *s)
{
    LOG_ENTER("ProtocolClient::GetPlayerConfiguration(s=%p)", s);
    BYTE *data;
    int size;

    STATUS result = ReadFidToMemory(FID_CONFIGFILE, &data, &size);
    if (SUCCEEDED(result))
    {
	s->assign(reinterpret_cast<char *>(data), size);
	FreeFidMemory(&data);
    }
    else if (result == MakeErrnoStatus(LINUX_ENOENT)
	     || PrintableStatus(result) == -LINUX_ENOENT) // TODO: remove this.  It's for backwards compatibility.
    {
	// The file doesn't exist.
	LOG_INFO("The player configuration file config.ini didn't exist, nomatter.");
	s->assign("");
	result = S_OK;
    }

    LOG_LEAVE("ProtocolClient::GetPlayerConfiguration() = %d",
	      PrintableStatus(result));
    return result;
}

STATUS ProtocolClient::SetPlayerConfiguration(const std::string &s)
{
    LOG_ENTER("ProtocolClient::SetPlayerConfiguration()");
    STATUS result = WriteFidFromMemory(FID_CONFIGFILE,
				   reinterpret_cast<const BYTE *>(s.data()),
				       s.length());
    LOG_LEAVE("ProtocolClient::SetPlayerConfiguration() = %d",
	      PrintableStatus(result));
    return result;
}

void ProtocolClient::DoReportResult(STATUS result)
{
    static const struct { STATUS s; Warning w; } sw[] = {
	{ MakeErrnoStatus(ETIMEDOUT),          LOCAL_TIMEOUT },
	{ MakeErrnoStatus(ENAK),               LOCAL_ACKFAIL },
	{ MakeErrnoStatus(EWRONGPACKET),       LOCAL_WRONGPACKET },
	{ MakeErrnoStatus(ENAK_CRC),           LOCAL_CRCFAIL },
	{ MakeErrnoStatus(ENAK_DROPOUT),       LOCAL_DROPOUT },
	{ MakeErrnoStatus(EREMOTENAK),         REMOTE_ACKFAIL },
	{ MakeErrnoStatus(EREMOTENAK_CRC),     REMOTE_CRCFAIL },
	{ MakeErrnoStatus(EREMOTENAK_DROPOUT), REMOTE_DROPOUT }
    };

#if DEBUG >= 2
    static const char *const errors[] = {
	"ETIMEDOUT",
	"ENAK",
	"EWRONGPACKET",
	"ENAK_CRC",
	"ENAK_DROPOUT",
	"EREMOTENAK",
	"EREMOTENAK_CRC",
	"EREMOTENAK_DROPOUT"
    };
#endif

    for ( size_t i=0; i<sizeof(sw)/sizeof(sw[0]); i++ )
    {
	if ( result == sw[i].s )
	{
#if DEBUG >= 2
	    TRACE("%d %s\n", i, errors[i]);
#endif
	    DoReportWarning(sw[i].w);
	    break;
	}
    }
}

bool ProtocolClient::FileNotFoundStatus(STATUS result)
{
    return result == MakeErrnoStatus(LINUX_ENOENT)
	|| result == MakeErrnoStatus(LINUX_EIO)
	|| result == MakeErrnoStatus(LINUX_ENXIO);
}

void ProtocolClient::ErrorAction(STATUS result, int retries)
{
    if (RETRY_ERROR(result))
    {
	// It's a retry error, if we've retried a few times then start waiting.
	while (retries > 3)
	{
#ifdef WIN32
	    LOG_INFO("Sleeping for a while due to excessive retries.");
	    ::Sleep(1000);
#else
	    sleep(1);
#endif
	    --retries;
	}
    }
    // Make sure we flush after we wait so that we throw away anything that happened
    // in the meantime.
    connection->FlushReceiveBuffer();
}

static std::string trim(const std::string &str)
{
    typedef std::string string_type;
    const std::string strWhite(" \t");

    string_type::const_iterator i = str.begin();

    while ((i != str.end()) && (strWhite.find(*i) != string_type::npos))
	    i++;

    if(i == str.end())
	return string_type();

    string_type::const_reverse_iterator j = str.rbegin();
    while ((j != str.rend()) && (strWhite.find(*j) != string_type::npos))
	j++;

    if(j == str.rend())
	return string_type();

    return string_type(i, j.base());
}

STATUS ProtocolClient::GetPlayerIdentity(PlayerIdentity *id)
{
    BYTE *buffer;
    int size;

    // Make sure we start with a clean slate.
    memset(id, 0, sizeof(PlayerIdentity));

    STATUS result = ReadFidToMemory(FID_ID, &buffer, &size);
    if (SUCCEEDED(result))
    {
	// We've got the thing, now parse it.

	//Get each line.

	const char *p = reinterpret_cast<char *>(buffer);
	const char *end = p + size;

	const char *line_start = p;

	while (true)
	{
	    if (*p == '\n' || p == end)
	    {
		if (p != line_start)
		{
		    std::string line(line_start, p);
		    TRACE("Got a line: %s\n", line.c_str());

		    std::string::size_type colon = line.find_first_of(':');
		    if (colon != std::string::npos)
		    {
			std::string field = trim(std::string(line, 0, colon));
			std::string value = trim(std::string(line, colon + 1, std::string::npos));

			TRACE("Field=%s, value=%s\n", field.c_str(), value.c_str());

			if (field == "hwrev")
			    id->hwrev = atoi(value.c_str());
			else if (field == "serial")
			    id->serial = atoi(value.c_str());
			else if (field == "build")
			    id->build = strtol(value.c_str(), NULL, 16);
			else if (field == "id")
			{
			    // TODO: This is of the form %08x-%08x-%08x-%08x, parse it here!
			}
			else if (field == "ram")
			{
			    id->ram = atoi(value.c_str());
			}
			else if (field == "flash")
			{
			    id->flash = atoi(value.c_str());
			}
		    }
		}
    		if (p == end)
    		    break;
		line_start = p + 1;
	    }

	    ++p;
	}
	FreeFidMemory(&buffer);
	return S_OK;
    }
    else
    {
	TRACE("Failed to get ID result=0x%08x.\n", PrintableStatus(result));
    }
    return result;
}

void ProtocolClient::RequestProgress::ProgressAction(Request *req, int stage, int stage_maximum,
						     int current, int maximum, char *string_info)
{
    UNUSED(req);
    UNUSED(string_info);
/*
    printf("ProgressAction(), stage:%d/%d progress:%d/%d\n",
	   stage, stage_maximum, current, maximum);
*/

    if(client->GetObserver() == NULL) return;

    int progress;
    int total = maximum * stage_maximum;
    if((stage != stage_maximum) || (current != maximum))
	progress = (stage - 1) * maximum + current;
    else progress = total;

//    printf("ProgressAction(), ReportProgress(%d, %d)\n", progress, total);

    if (progress > total)
    {
	//	ASSERT(false);
	// no, the remote end could kill us if it's wrong, not us
	progress = total;
    }

    client->GetObserver()->ReportProgress(client, activity, progress, total);
}

const char *ProtocolClient::DescribeWarning(Warning w)
{
    switch (w)
    {
    case LOCAL_TIMEOUT:
        return "Local timeout";
    case LOCAL_ACKFAIL:
        return "Local acknowledge failure";
    case LOCAL_WRONGPACKET:
        return "Local packet mismatch";
    case LOCAL_CRCFAIL:
        return "Local CRC failure";
    case LOCAL_DROPOUT:
        return "Local dropout";

    case REMOTE_TIMEOUT:
        return "Remote timeout";
    case REMOTE_ACKFAIL:
        return "Remote acknowledge failure";
    case REMOTE_WRONGPACKET:
        return "Remote packet mismatch";
    case REMOTE_CRCFAIL:
        return "Remote CRC failure";
    case REMOTE_DROPOUT:
        return "Remote dropout";
    default:
        return "Unknown warning";
    }
}
