/* packets.cpp
 *
 * Individual protocol packets
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Hugo Fiennes <hugo@empeg.com>
 *   Mike Crowe <mac@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.92.2.1 01-Apr-2003 18:52 rob:)
 */

#include "core/config.h"
#include "core/trace.h"
#ifdef WIN32
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "stringpred.h"
#include "packets.h"
#include "connection.h"
#include "crc.h"
#include "interval.h"
#include "protocol_errors.h"

#include "NgLog/NgLog.h"

#define TRACE_PROTOCOL 0
LOG_COMPONENT(PROTOCOL)

// Constants
#define PSOH		2	// Not really SOH, but we use it as SOH.
                                // 1 is used during unit upgrading, so best to steer clear.

#define MAX_WRONG_PACKETS 16

//UINT32 Request::id = 42;

Mutex RequestIdentifierFactory::m_lock_id("request_id_factory");
UINT32 RequestIdentifierFactory::m_id = 42;

/** We don't want bytes that are ASCII controls (1..31), so use a 28-bit ID
 * and intersperse it into the 0 bits of 0x80808080.
 */
UINT32 RequestIdentifierFactory::Reserve()
{
    MutexLock lock(m_lock_id);
    UINT32 result =  (m_id &      0x7F) 
	          + ((m_id &    0x3F80) << 8)
	          + ((m_id &  0x1FC000) << 16)
	          + ((m_id & 0xFE00000) << 24)
	          + 0x80808080;
    m_id++;
    return result;
}

void Request::NewRequest()
{
    m_packet_id = RequestIdentifierFactory::Reserve();
    LOG_INFO("NEW request, id=0x%x", m_packet_id);
}

// Not to be used except for debugging.
void Request::SetRequest(UINT32 i)
{
    LOG_INFO("Setting request to 0x%x", i);
    m_packet_id = i;
}



// Request sending class
Request::Request(Connection *c)
    : state(0), substate(0), crc(0), sentcrc(0),
      bufptr(NULL), buffer(NULL),
      protocol_version_minor(0),
      protocol_version_major(-1),
      observer(NULL)
{
    ASSERT(c != NULL);
    // Save connection
    connection=c;

    // Get buffer memory
    buffer_size = c->PacketSize()+256;
    buffer=NEW BYTE[buffer_size];
    ASSERT(buffer != NULL);

    // Set up packet header pointer
    header=(EmpegPacketHeader*)buffer;

    //connection->FlushReceiveBuffer();
    // Not sure why this ever got removed but it is essential!
    NewRequest();
}

Request::~Request()
{
    // Free buffer
    delete[] buffer;
}

/** Send a packet.  The passed in parameter is the header of the packet.
 * We build the packet and send it in one go.
 * Doing it all in one write is a very good idea with USB, as it avoids short
 * packets before the end of the transfer, hence giving better throughput.
 * The data copy time is as nothing compared to the USB latency. I think.
 *
 * The outgoing packet consists of:
 *
 *  PSOH|EmpegPacketHeader|char[h.datasize]|crc16
 *
 * The CRC16 of the packet excludes the data_size member.
 */
STATUS Request::Send(EmpegPacketHeader *tosend) const
{
    // Make CRC
    unsigned short pktcrc = CRC::CRC16(((unsigned char*)tosend) + 2, tosend->datasize + 1 + 1 + 4);

    LOG_INFO("Sending response.");
    BYTE buf[USB_MAXPAYLOAD+256],*b=buf;

    LOG_INFO("Sending packet of type=%d opcode=%d with id=0x%x", 
        tosend->type, tosend->opcode, tosend->packet_id);
    
    // Packet starts with PSOH
    *b++ = PSOH;
    unsigned packet_length = sizeof(EmpegPacketHeader) + tosend->datasize;
    memcpy(b, tosend, packet_length);
    b += packet_length;
    *b++ = (pktcrc & 0xff);
    *b++ = (pktcrc >> 8);

    //LOG_DEBUG_HEX((const char *)buffer, b - buffer);

    // Send whole packet with one write
    STATUS st = connection->Send(buf, b - buf);

    // Most of our callers ignore errors, so make sure someone notices
    if (FAILED(st))
    {
	WARN("Send failed result = 0x%x\n", PrintableStatus(st));
    }

    return st;
}

// Send a progress response
void Request::SendProgress(ProgressResponsePacket* progress) const
{
    // Send a progress response, fill in length etc
    progress->header.type=OPTYPE_PROGRESS;
    progress->header.datasize=sizeof(ProgressResponsePacket)-sizeof(EmpegPacketHeader);

    // Ignore error
    Send(&progress->header);
}

// Ping remote end
STATUS Request::Ping()
{
    LOG_INFO("Request::Ping");
    char set_raw[2] = { 25, 13 };
    STATUS result;

    if(FAILED(result = connection->Send((BYTE *)set_raw, 2)))
    {
	LOG_INFO("Request::Ping(A) = 0x%x\n", PrintableStatus(result));
	return result;
    }

    // Make request & send it making sure we don't accidentally send 
    // any INTR characters
    PingRequestPacket request;

    request.header.type=OPTYPE_REQUEST;	// this is 0
    request.header.opcode=OP_PING;		// this is 0
    request.header.datasize=0;

    while (true)
    {
	/** @todo This is twisted -- surely it'd be easier to
	 * do some bit-twiddling in NewRequest to ensure that it never suggested
	 * an ID we didn't like?
	 */
	unsigned char *p = (unsigned char *) &m_packet_id;
	if((p[0] && (p[0] < 32)) ||
	   (p[1] && (p[1] < 32)) ||
	   (p[2] && (p[2] < 32)) ||
	   (p[3] && (p[3] < 32))) {
            // Eeek, we could do with a new packet ID here
            NewRequest();
	    continue;
	}

	request.header.packet_id=m_packet_id;

	// Make CRC
        unsigned short newcrc=CRC::CRC16(((unsigned char*)&request)+2,1+1+4);

	p = (unsigned char *) &newcrc;
	if((p[0] && (p[0] < 32)) || (p[1] && (p[1] < 32))) {
            NewRequest();
	    continue;
	}
	break;
    }

    Flush();

    result = Send(&request.header);

    if (FAILED(result))
    {
	LOG_INFO("Request::Ping(B) = 0x%x\n", PrintableStatus(result));
	return result;
    }

    PingResponsePacket *response;
    // Wait for reply
    result=WaitForReply((EmpegPacketHeader **)&response,m_packet_id);

    // Timed out?
    if (FAILED(result))
    {
	LOG_INFO("Request::Ping(C) = 0x%x\n", PrintableStatus(result));
	return result;
    }

    if (response->header.datasize >= sizeof(response->version_major) + sizeof(response->version_minor))
    {
	protocol_version_major = response->version_major;
	protocol_version_minor = response->version_minor;
    }
    else
    {
	protocol_version_major = 0;
	protocol_version_minor = 0;
    }

    LOG_INFO("Request::Ping(D) = OK\n");
    return S_OK;
}

// Force remote end to quit
STATUS Request::Quit()
{
    char set_raw[2] = { 25, 13 };
    STATUS result;
    if (FAILED(result = connection->Send((unsigned char *) set_raw, 2)))
	return result;

    // Make request & send it
    EmpegPacketHeader h,*r;

    h.type=OPTYPE_REQUEST;
    h.opcode=OP_QUIT;
    h.packet_id=m_packet_id;
    h.datasize=0;

    Flush();
    if (FAILED(result = Send(&h)))
	return result;

    // Wait for reply
    return WaitForReply(&r,m_packet_id);
}

// Remount ro/rw
STATUS Request::Mount(int readwrite)
{
    // Make request & send it
    MountRequestPacket p;
    MountResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_MOUNT;
    p.header.packet_id = m_packet_id;
    p.header.datasize=sizeof(MountRequestPacket)-sizeof(EmpegPacketHeader);
    p.partition=0;
    p.mode=readwrite;

    Flush();

    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    // Timed out?
    if (FAILED(result))
	return result;

    // Return error
    return r->response;
}

// Get size of file
STATUS Request::StatFID(int fid, int *psize)
{
    // Make request & send it
    StatRequestPacket p;
    StatResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_STATFID;
    p.header.packet_id=m_packet_id;
    p.header.datasize=sizeof(StatRequestPacket)-sizeof(EmpegPacketHeader);
    p.file_id=fid;

    // This causes everything to be slow.
    //Flush();
    STATUS result = Send(&p.header);

    if (FAILED(result))
	return result;

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    LOG_INFO("StatFID: wfr returned 0x%x remote returned 0x%x\n",
	     PrintableStatus(result),
             r ? PrintableStatus(r->result) : 0 );

    // Timed out?
    if (FAILED(result))
	return result;

    ASSERT(r);

    if (FAILED(r->result))
	return r->result;

    if ( psize )
	*psize = r->file_size;

    return S_OK;
}

// Transfer a bufferful of file data TO the empeg
STATUS Request::WriteFID(int fid, int offset, int size, const BYTE *buf)
{
    LOG_INFO("Request::WriteFid(fid=0x%x, offset=0x%x, size=0x%x, buf=%p)",
		      fid, offset, size, buf);
    // Make request & send it
    ASSERT(size <= connection->PacketSize());

    char txbuffer[USB_MAXPAYLOAD+256];
    TransferRequestPacket *t=(TransferRequestPacket*)txbuffer;
    t->header.opcode=OP_WRITEFID;
    t->header.datasize=sizeof(TransferRequestPacket)-sizeof(EmpegPacketHeader)+size;

	// Request-specifics
    t->file_id=fid;
    t->chunk_offset=offset;
    t->chunk_size=size;
    memcpy(t->data,buf,size);

    int nbytes;

    STATUS result = GenericWrite(&t->header, &nbytes);

    LOG_INFO("Request::WriteFid() = %d (0x%x)", nbytes, PrintableStatus(result));
    if (FAILED(result))
	return result;

    if ( nbytes != size )
	return MakeErrnoStatus(EIO);

    return S_OK;
}

STATUS Request::GenericWrite(EmpegPacketHeader *h, int *pnbytes)
{
    LOG_INFO("Request::GenericWrite(h=%p)", h);

    // Fill in packet header
    h->type=OPTYPE_REQUEST;
    h->packet_id=m_packet_id;

    // Send the packet
    STATUS result = Send(h);
    if (FAILED(result))
    {
	LOG_INFO("Request::GenericWrite(A) = 0x%x", PrintableStatus(result));
	return result;
    }

    if(connection->GetDebugLevel()>=2)
	fprintf(stderr, "Sent packet: id %x\n", m_packet_id);

    // Wait for reply
    TransferResponsePacket *r;
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
    {
	LOG_INFO("Request::GenericWrite(B) = 0x%x", PrintableStatus(result));
	return(result);
    }

    // NAK? We shouldn't get these any more.
    ASSERT(result != MakeErrnoStatus(NAK));

    // Why don't we do this any more? MAC 1999/07/18
    // Check ID is right & that chunk start was correct
    //if (r->header.packet_id!=id ||
    //    r->chunk_offset!=t.chunk_offset) return(-EWRONGPACKET);

    // Return number of bytes transferred

    if (FAILED(r->result))
    {
	LOG_INFO("Request::GenericWrite(D) = 0x%x",
		 PrintableStatus(result));
	return r->result;
    }

    *pnbytes = r->nbytes;
    LOG_INFO("Request::GenericWrite(C) = bytes=%d, result = S_OK", r->nbytes);
    return S_OK;
}

// Transfer a bufferful of file data FROM the empeg
STATUS Request::ReadFID(int fid, int offset, int size, BYTE *buf,
			int *pnbytes)
{
    ASSERT(size <= connection->PacketSize());
    // Make request & send it
    TransferRequestPacket t;
    t.header.type=OPTYPE_REQUEST;
    t.header.opcode=OP_READFID;
    t.header.packet_id=m_packet_id;
    t.header.datasize=sizeof(TransferRequestPacket)-sizeof(EmpegPacketHeader);

    // Request-specifics
    t.file_id=fid;
    t.chunk_offset=offset;
    t.chunk_size=size;
    STATUS result = Send(&t.header);

    if (FAILED(result))
	return result;

    // Wait for reply
    TransferResponsePacket *r;
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
	return(result);

    if (FAILED(r->result))
	return r->result;

    if (r->nbytes > size)
    {
	// Wierder and wierder.
	ERROR("Chunk size response is bigger than the amount we asked for.\n");
	return MakeErrnoStatus(EIO);
    }

    if ( r->header.datasize
	 != r->nbytes + sizeof(TransferResponsePacket) - sizeof(EmpegPacketHeader) )
    {
	ERROR("Packet header size (%d) disagrees with calculated packet size (%d)\n",
	      r->header.datasize,
	      r->nbytes + sizeof(TransferResponsePacket)
	                - sizeof(EmpegPacketHeader) );
	return MakeErrnoStatus(EIO);
    }

    // Copy data into local buffer
    memcpy(buf,r->data,r->nbytes);

    if ( pnbytes )
	*pnbytes = r->nbytes;

    // Return number of bytes transferred
    return S_OK;
}

// Prepare a file for download TO the empeg
STATUS Request::PrepareFID(int fid, int size)
{
    // Make request & send it
    PrepareRequestPacket t;
    t.header.type=OPTYPE_REQUEST;
    t.header.opcode=OP_PREPAREFID;
    t.header.packet_id=m_packet_id;
    t.header.datasize=sizeof(PrepareRequestPacket)-sizeof(EmpegPacketHeader);

    // Request-specifics
    t.file_id=fid;
    t.file_size=size;

    // Don't damage performance on Windows.
    //Flush();
    STATUS result = Send(&t.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    TransferResponsePacket *r;
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
	return result;

    if (r->nbytes != size) {
	if(connection->GetDebugLevel()>=1)
	    fprintf(stderr, "PrepareFID. Unable to write the whole file. Managed %d of %d.",
			r->nbytes, size);
    }

    // Return number of bytes prepared
    if (FAILED(r->result))
	return r->result;

    ASSERT(r->nbytes == size);

    if (r->nbytes != size)
    {
	LOG_ERROR("Prepare didn't prepare the expected number of bytes. expected=%d, actual=%d\n",
		  size, r->nbytes);
	return MakeErrnoStatus(EIO);
    }
    return S_OK;
}

// Delete file(s) from the empeg
STATUS Request::DeleteFID(int fid, unsigned int mask)
{
    // Make request & send it
    DeleteRequestPacket t;
    t.header.type=OPTYPE_REQUEST;
    t.header.opcode=OP_DELETEFID;
    t.header.packet_id=m_packet_id;
    t.header.datasize=sizeof(DeleteRequestPacket)-sizeof(EmpegPacketHeader);

    // Request-specifics
    t.file_id=fid;
    t.id_mask=mask;

    // Don't damage performance.
    //Flush();
    STATUS result = Send(&t.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    DeleteResponsePacket *r;
    result=WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    // Timed out?
    if (FAILED(result))
	return(result);

    // Return result
    return(r->result);
}

// Rebuild status database & save to disk
STATUS Request::BuildAndSaveDatabase(int options)
{
    // Make request & send it
    RebuildRequestPacket p;
    RebuildResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_REBUILD;
    p.header.packet_id=m_packet_id;
    p.header.datasize=sizeof(RebuildRequestPacket)-sizeof(EmpegPacketHeader);
    p.operation=options;

    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
	return result;

    // Return result
    return r->result;
}

// Fsck a partition
STATUS Request::Fsck(const char *partition, int force, unsigned int *pflags)
{
    // Make request & send it
    FsckRequestPacket p;
    FsckResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_FSCK;
    p.header.packet_id=m_packet_id;
    p.header.datasize=sizeof(FsckRequestPacket)-sizeof(EmpegPacketHeader);
    strcpy(p.partition,partition);
    p.force=force;

    STATUS result = Flush();
    if (FAILED(result))
	return result;

    Send(&p.header);

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    // Timed out?
    if (FAILED(result))
	return(result);

    *pflags = r->flags;

    return r->result;
}

// Get drive free space
STATUS Request::FreeSpace(int *drive0size,int *drive0space,int *drive0blocksize,
			  int *drive1size,int *drive1space,int *drive1blocksize)
{
    // Make request & send it
    StatfsRequestPacket p;
    StatfsResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_STATFS;
    p.header.packet_id=m_packet_id;
    p.header.datasize=sizeof(StatfsRequestPacket)-sizeof(EmpegPacketHeader);

    Flush();

    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
	return(result);

    // Return result
    if (drive0size!=NULL) *drive0size=r->drive0size;
    if (drive0space!=NULL) *drive0space=r->drive0space;
    if (drive0blocksize!=NULL) *drive0blocksize=r->drive0blocksize;
    if (drive1size!=NULL) *drive1size=r->drive1size;
    if (drive1space!=NULL) *drive1space=r->drive1space;
    if (drive1blocksize!=NULL) *drive1blocksize=r->drive1blocksize;

    return S_OK;
}

// Send a generic command
STATUS Request::SendCommand(int command, int parameter0, int parameter1,
			    const char *parameter2)
{
    // Make request & send it
    CommandRequestPacket p;
    CommandResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_COMMAND;
    p.header.packet_id=m_packet_id;
    p.header.datasize=sizeof(CommandRequestPacket)-sizeof(EmpegPacketHeader);
    p.command=command;
    p.parameter0=parameter0;
    p.parameter1=parameter1;
    if (parameter2)
    {
        strcpy(p.parameter2,parameter2);
    }
    else
    {
        p.parameter2[0]=0;
        for(unsigned int i = 1; i < sizeof(p.parameter2); ++i)
            p.parameter2[i] = i;
    }

    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
	return result;

    // Return result
    return r->result;
}

// Send a generic command
STATUS Request::GrabScreen(int command, unsigned char *screen)
{
    // Make request & send it
    GrabScreenRequestPacket p;
    GrabScreenResponsePacket *r;

    p.header.type=OPTYPE_REQUEST;
    p.header.opcode=OP_GRABSCREEN;
    p.header.packet_id=m_packet_id;
    p.header.datasize=sizeof(GrabScreenRequestPacket)-sizeof(EmpegPacketHeader);
    p.command=command;

    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    // Wait for reply
    result=WaitForReply((EmpegPacketHeader**)&r,m_packet_id);

    // Timed out?
    if (FAILED(result))
	return(result);

    // If result is OK, copy screen buffer
    if (r->result==S_OK && screen!=NULL) {
      memcpy(screen,r->screen,sizeof(r->screen));
    }

    // Return result
    return r->result;
}

STATUS Request::InitiateSession(const char *host_description, const char *password, UINT32 *session_cookie, std::string *failure_reason)
{
    InitiateSessionRequestPacket p;
    InitiateSessionResponsePacket *r;

    *failure_reason = "";
    
    p.header.type = OPTYPE_REQUEST;
    p.header.opcode = OP_INITIATESESSION;
    p.header.packet_id = m_packet_id;
    p.header.datasize = sizeof(InitiateSessionRequestPacket)
	- sizeof(EmpegPacketHeader);

    empeg_safestrcpy(p.password, password, sizeof(p.password));
    empeg_safestrcpy(p.host_description, host_description, sizeof(p.host_description));
    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    result = WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    if (FAILED(result))
	return result;

    ASSERT(r->header.datasize == sizeof(InitiateSessionResponsePacket)
				    - sizeof(EmpegPacketHeader));

    if (r->result == S_OK)
    {
	*session_cookie = r->session_cookie;
    }
    else
    {
	*failure_reason = r->failure_reason;
    }

    return r->result;
}

STATUS Request::SessionHeartbeat(UINT32 session_cookie)
{
    SessionHeartbeatRequestPacket p;
    SessionHeartbeatResponsePacket *r;

    p.header.type = OPTYPE_REQUEST;
    p.header.opcode = OP_SESSIONHEARTBEAT;
    p.header.packet_id = m_packet_id;
    p.header.datasize = sizeof(SessionHeartbeatRequestPacket)
	- sizeof(EmpegPacketHeader);
    p.session_cookie = session_cookie;

    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    result = WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    if (FAILED(result))
	return result;

    return r->result;
}

STATUS Request::TerminateSession(UINT32 session_cookie)
{
    TerminateSessionRequestPacket p;
    TerminateSessionResponsePacket *r;

    p.header.type = OPTYPE_REQUEST;
    p.header.opcode = OP_TERMINATESESSION;
    p.header.packet_id = m_packet_id;
    p.header.datasize = sizeof(TerminateSessionRequestPacket)
	- sizeof(EmpegPacketHeader);
    p.session_cookie = session_cookie;

    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    result = WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    if (FAILED(result))
	return result;

    return r->result;
}

STATUS Request::NotifySyncComplete()
{
    NotifySyncCompleteRequestPacket p;
    NotifySyncCompleteResponsePacket *r;

    p.header.type = OPTYPE_REQUEST;
    p.header.opcode = OP_COMPLETE;
    p.header.packet_id = m_packet_id;
    p.header.datasize = sizeof(TerminateSessionRequestPacket)
	- sizeof(EmpegPacketHeader);
    p.unused = 0;

    Flush();
    STATUS result = Send(&p.header);
    if (FAILED(result))
	return result;

    result = WaitForReply((EmpegPacketHeader**)&r, m_packet_id);

    if (FAILED(result))
	return result;

    return r->result;
}

// Wait for reply, processing PROGRESS packets as they arrive
STATUS Request::WaitForReply(EmpegPacketHeader**header_return, unsigned int packet_id)
{
    STATUS result;
    int period=2;
    int wrong_packets = 0; // Count of packets with wrong ID.

    LOG_INFO("Request::WaitForReply(header_return=%p, packet_id=%d)\n", header_return, packet_id);

    EmpegPacketHeader *rx_header = NULL;
    *header_return = NULL;

    // The first timeout period is 2 seconds, as we should have a
    // PROGRESS or RESPONSE packet returned immediately. If we get
    // a PROGRESS reply then we use the timeout value enclosed to
    // set the timer for the next reply.
    while(true)
    {
	LOG_INFO("WaitForReply loop\n");

	LOG_INFO("Waiting for reply with id 0x%x and timeout %d\n",
		 packet_id, period);
	// Get a packet
	result = Receive(&rx_header,period);
	LOG_INFO("Receive result was 0x%x\n", PrintableStatus(result));

	if (FAILED(result))
	{
	    LOG_INFO("Receive() failed in WaitForReply, error is 0x%x\n",
		     PrintableStatus(result));
	    if (result == CONN_E_TIMEDOUT)
		LOG_INFO("...that's a timeout error (timeout was %d)\n", period);
	    else if (result != MakeErrnoStatus(ENAK)
		 && result != MakeErrnoStatus(ENAK_CRC)
		 && result != MakeErrnoStatus(ENAK_DROPOUT)
		 && result != MakeErrnoStatus(ETIMEDOUT)
		 && result != MakeErrnoStatus(EREMOTENAK)
		 && result != MakeErrnoStatus(EREMOTENAK_CRC)
		 && result != MakeErrnoStatus(EREMOTENAK_BADHEADER)
		 && result != MakeErrnoStatus(EREMOTENAK_DROPOUT) )
		LOG_INFO("...that's a hitherto unknown error\n");

	    LOG_INFO("Packet::WaitForReply(A) = 0x%x\n", PrintableStatus(result));
	    return result;
	}

	ASSERT(rx_header);

	// Check the packet ID
	if (rx_header->packet_id != packet_id)
	{
	    LOG_INFO("Packet ID didn't match: received (id=0x%x, type=%d, opcode=%d), expected (id=0x%x)",
		      rx_header->packet_id, rx_header->type, rx_header->opcode, packet_id);
	    LOG_INFO("Packet::WaitForReply(F) = 0x%x", PrintableStatus(result));
            if (++wrong_packets > MAX_WRONG_PACKETS)
            {
		LOG_ERROR("More than %d wrong packets in a row. What's going on?\n", MAX_WRONG_PACKETS);
	        result = MakeErrnoStatus(EWRONGPACKET);
	        return result;
            }
            else
            {
                LOG_INFO("Continuing to process buffer in the hope that the right packet is there.");
                continue;
            }
	}
        else
        {
            LOG_INFO("Got a response to packet %x", packet_id);
        }

	// Check the type
	if (rx_header->type==OPTYPE_PROGRESS)
	{
	    LOG_INFO("We have a progress reply.");
	    // We have a progress reply
	    ProgressResponsePacket *response=(ProgressResponsePacket*)rx_header;

	    // Use new supplied timeout
	    period=response->newtimeout;
	    LOG_INFO("Progress reply, new timeout is %d", period);

	    if(observer != NULL)
	    {
		char tmp[65];
		strncpy(tmp, response->string, 64);
		tmp[64] = 0;
		// Call observer to inform user of progress info
		observer->ProgressAction(this, response->stage, response->stage_maximum,
					 response->current, response->maximum, tmp);
	    }
	}
	else if (rx_header->type==OPTYPE_RESPONSE)
	{
	    LOG_INFO("Got a real reply.\n");
	    // Got a reply, return ACK
	    // result = ACK;
	    *header_return = rx_header;
	    LOG_INFO("Packet::WaitForReply(G) = 0x%x", PrintableStatus(result));
	    return result;
	}
	else
	{
	    WARN("Got unknown packet type in WaitForReply.\n");
	}
    }
}

#if DEBUG > 3
#define DEBUG_PACKET_CONTENTS
#endif

#ifdef DEBUG_PACKET_CONTENTS
class DataDumper
{
    bool dump;
    std::basic_string<BYTE> s;

public:
    DataDumper() : dump(true) {}
    void Append(BYTE *b, int count)
    {
	if (count > 0)
	    s.append(b, count);
    }
    void Cancel() { dump = false; }
    ~DataDumper()
    {
	TRACE_HEX("Complete packet.\n", s.data(), s.data() + s.length());
    }
};
#endif

// Get a packet
STATUS Request::Receive(EmpegPacketHeader **rx_header, int timeout)
{
    LOG_INFO("Request::Receive(h = %p, timeout=%d)", rx_header, timeout);

    // Work out our timeout
    Interval interval(timeout * 1000);

    // Initialise state machine
    state = 0;

    // Deal with up to 1024 bytes at a time
    BYTE tbuffer[1024];

    int total_bytes = 0;

#ifdef DEBUG_PACKET_CONTENTS
    DataDumper dumper;
#endif

    while(true)
    {
	LOG_INFO("Trying to receive bytes.\n");
#ifdef ARCH_PC
	// Make timeout longer on PC which may go off and swap or something
	const int packet_timeout = 5000;
#else
	// 250ms timeout on each byte
	const int packet_timeout = 2000;
#endif

	DWORD count;
	STATUS result = connection->Receive(tbuffer, sizeof(tbuffer), packet_timeout, count);

	LOG_INFO("Got %ld bytes from connection->Receive", count);

#ifdef DEBUG_PACKET_CONTENTS
	dumper.Append(tbuffer, count);
#endif

	// Got a timeout?
	if (result == CONN_E_TIMEDOUT)
        {
	    if (state > 0)
            {
		if(connection->GetDebugLevel()>=1)
		    fprintf(stderr, "timeout in state %d (substate=%d)\n",state,substate);
		LOG_INFO("Timeout in state %d (substate %d)\n", state, substate);

		state=0;
		LOG_INFO("Flush\n");
		Flush();

		// We've had a timeout *during* a packet - this shouldn't
		// happen.
		result = MakeErrnoStatus(ENAK_DROPOUT);
		LOG_INFO("Request::Receive(A) = 0x%x\n", PrintableStatus(result));
		return result;
	    }

	    // Otherwise, see if real timer has expired
	    if (interval.Expired())
	    {
		LOG_INFO("No data timeout in state 0");
		if(connection->GetDebugLevel()>=1)
		    fprintf(stderr, "no data timeout in state 0\n");

		LOG_INFO("Request::Receive(B) = 0x%x", PrintableStatus(result));
		return result;
	    }
	    else
	    {
		LOG_INFO("It timed out - trying again");
		continue;
	    }
	}

	if (FAILED(result))
	{
            LOG_INFO("Got result 0x%x from connection->Receive", PrintableStatus(result));
            LOG_INFO("Request::Receive(C) = 0x%x", PrintableStatus(result));
	    return result;
	}

	total_bytes += count;

	for(DWORD a = 0; a < count; a++)
        {
	    // All done?
	    STATUS b = ProcessByte(tbuffer[a]);

	    if (b != S_AGAIN)
	    {
		// b is either S_OK or a negative error.
		*rx_header = header;

		result = b;
		if (result == S_OK)
		{
		    LOG_INFO("We got a valid packet!");
		    // result = total_bytes;  //  FIXME
#ifdef DEBUG_PACKET_CONTENTS
		    dumper.Cancel();
#endif

		    // Push back everything we haven't used.
		    LOG_INFO("End of packet, pushing back %ld bytes", count - a - 1);
		    connection->PushBack(count - a - 1);
		}
		else
		{
		    LOG_INFO("We didn't get a valid packet - result is 0x%08x",
			     PrintableStatus(b));
                    LOG_DEBUG("Dump of dodgy packet:");
                    if ( bufptr && bufptr > buffer )
                    {
                        LOG_DEBUG_HEX(buffer, bufptr - buffer);
                    }

                    // Not an ACK?
		    // Some sort of error, flush the receiver
		    Flush();
		}

		LOG_INFO("Request::Receive(D) = 0x%x", PrintableStatus(result));
		return result;
	    }
	}
#if 0
        if (interval.Expired()) {
	    if(connection->GetDebugLevel()>=1)
		fprintf(stderr, "timeout with data in state 0");

	    result = MakeErrnoStatus(ETIMEDOUT);
	    LOG_INFO("Request::Receive(E) = 0x%x", PrintableStatus(result));
	    return result;
	}
#endif
    }
}

// Process data
STATUS Request::ProcessByte(BYTE b)
{
    switch(state)
    {
    case 0: // Looking for PSOH or NAK
	if (b==PSOH)
	{
	    LOG_INFO("Got a PSOH");
	    // Initialise packet buffer
	    memset(buffer,0,buffer_size);

	    // Next state
	    state++;
	}
	else if (b==NAK)
	{
	    return MakeErrnoStatus(EREMOTENAK);
	}
	else if (b==NAK_DROPOUT)
	{
	    return MakeErrnoStatus(EREMOTENAK_DROPOUT);
	}
	else if (b==NAK_CRC)
	{
	    return MakeErrnoStatus(EREMOTENAK_CRC);
	}
	else
	{
	    // Between packets, print the debug info
	    LOG_INFO("%c (\\x%02x)", (isprint(b) ? b : '.'), b);
	}
	break;

    case 1: // Collecting length (low byte)
	header->datasize=b;
	state++;
	break;

    case 2: // Collecting length (high byte)
	header->datasize|=(b<<8);
	if (header->datasize>=(unsigned)(buffer_size-16))
	{
	    LOG_INFO("Strange header size noticed: %d.\n", header->datasize);
	    state=0;
	    return MakeErrnoStatus(ENAK_BADHEADER);
	}
	else
	{
	    // Collect this many bytes
	    substate=(1+1+4+header->datasize);
	    bufptr=&buffer[2];
            LOG_INFO("Got packet length: %d bytes.\n", header->datasize);

	    state++;
	}
	break;

    case 3: // Collecting and storing data
	// Store it
	ASSERT_TPTR(BYTE, bufptr);
	ASSERT(bufptr >= buffer);
	ASSERT(bufptr < buffer + buffer_size);
	*bufptr++=b;

	// Finished?
	ASSERT(substate > 0);
	if (!--substate)
	    state++;
	break;

    case 4: // Collecting CRC (low byte)
        LOG_INFO("We've read all the data we were expecting, reading CRC.\n");
	sentcrc=b;
	state++;
	break;

    case 5: // Collecting CRC (high byte)
	sentcrc|=(b<<8);
	state=0;

	// Calculate CRC
	crc=CRC::CRC16(buffer+2,(header->datasize+1+1+4));

	// Do they match?
	if (sentcrc!=crc)
	{
	    // Bad request: nak it
	    LOG_INFO("NAK (bad CRC) got=0x%08x, expected=0x%08x\n", sentcrc, crc);
	    if(connection->GetDebugLevel()>=1)
		fprintf(stderr, "NAK (bad CRC)\n");
	    return MakeErrnoStatus(ENAK_CRC);
	}
	else
	{
            LOG_INFO("CRC is good.\n");

	    // Process request
	    return S_OK;
	}
	break;

    default:
	ERROR("Invalid state in %d Request::Process - resetting to default.\n", state);
        LOG_ERROR("Invalid state %d, in Request::Process - resetting to default.", state);
	state = 0;
	return ProcessByte(b);
    }

    // Not finished processing.
    return S_AGAIN;
}

STATUS Request::Flush()
{
    LOG_INFO("FLUSH\n");
    connection->FlushReceiveBuffer();
    return S_OK;
}

STATUS Request::GetProtocolVersion(short *major, short *minor)
{
    if (protocol_version_major >= 0)
    {
	*major = protocol_version_major;
	*minor = protocol_version_minor;
	return S_OK;
    }
    else
	return E_GET_PROTOCOL_VER;
}
