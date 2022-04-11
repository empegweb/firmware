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
 * (:Empeg Source Release 1.97 13-Mar-2003 18:15 rob:)
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
#ifndef ECOS
#include <sys/time.h>
#include <sys/types.h>
#endif
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

#define TRACE_PROTOCOL 0

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
    TRACEC(TRACE_PROTOCOL, "NEW request, id=0x%x", m_packet_id);
}

// Not to be used except for debugging.
void Request::SetRequest(UINT32 i)
{
    TRACEC(TRACE_PROTOCOL, "Setting request to %x\n", i);
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

    TRACEC(TRACE_PROTOCOL, "Sending response.\n");
    BYTE buf[USB_MAXPAYLOAD+256],*b=buf;

    TRACEC(TRACE_PROTOCOL, "Sending packet of type=%d opcode=%d with id=%x\n", 
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
	TRACE_WARN("Send failed result = 0x%x\n", PrintableStatus(st));
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
    TRACEC(TRACE_PROTOCOL, "Request::Ping\n");
    char set_raw[2] = { 25, 13 };
    STATUS result;

    if(FAILED(result = connection->Send((BYTE *)set_raw, 2)))
    {
	TRACEC(TRACE_PROTOCOL, "Request::Ping(A) = 0x%x\n\n", PrintableStatus(result));
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
	TRACEC(TRACE_PROTOCOL, "Request::Ping(B) = 0x%x\n\n", PrintableStatus(result));
	return result;
    }

    PingResponsePacket *response;
    // Wait for reply
    result=WaitForReply((EmpegPacketHeader **)&response,m_packet_id);

    // Timed out?
    if (FAILED(result))
    {
	TRACEC(TRACE_PROTOCOL, "Request::Ping(C) = 0x%x\n\n", PrintableStatus(result));
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

    TRACEC(TRACE_PROTOCOL, "Request::Ping(D) = OK\n\n");
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

    TRACEC(TRACE_PROTOCOL, "StatFID: wfr returned %x remote returned %x\n\n",
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
    TRACEC(TRACE_PROTOCOL, "Request::WriteFid(fid=0x%x, offset=0x%x, size=0x%x, buf=%p)\n",
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

    TRACEC(TRACE_PROTOCOL, "Request::WriteFid() = %d (0x%x)\n", nbytes, PrintableStatus(result));
    if (FAILED(result))
	return result;

    if ( nbytes != size )
	return MakeErrnoStatus(EIO);

    return S_OK;
}

STATUS Request::GenericWrite(EmpegPacketHeader *h, int *pnbytes)
{
    TRACEC(TRACE_PROTOCOL, "Request::GenericWrite(h=%p)\n", h);

    // Fill in packet header
    h->type=OPTYPE_REQUEST;
    h->packet_id=m_packet_id;

    // Send the packet
    STATUS result = Send(h);
    if (FAILED(result))
    {
	TRACEC(TRACE_PROTOCOL, "Request::GenericWrite(A) = %d (0x%x)\n", PrintableStatus(result),
	       PrintableStatus(result));
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
	TRACEC(TRACE_PROTOCOL, "Request::GenericWrite(B) = %d (0x%x)\n", PrintableStatus(result),
	       PrintableStatus(result));
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
	TRACEC(TRACE_PROTOCOL, "Request::GenericWrite(D) = 0x%x\n",
		 PrintableStatus(result));
	return r->result;
    }

    *pnbytes = r->nbytes;
    TRACEC(TRACE_PROTOCOL, "Request::GenericWrite(C) = %d\n", r->nbytes);
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
	TRACE_ERROR("Chunk size response is bigger than the amount we asked for.\n");
	return MakeErrnoStatus(EIO);
    }

    if ( r->header.datasize
	 != r->nbytes + sizeof(TransferResponsePacket) - sizeof(EmpegPacketHeader) )
    {
	TRACE_ERROR("Packet header size (%d) disagrees with calculated packet size (%d)\n",
	      r->header.datasize,
	      (int) r->nbytes + (int) sizeof(TransferResponsePacket)
	                      - (int) sizeof(EmpegPacketHeader) );
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
	TRACEC(TRACE_PROTOCOL, "Prepare didn't prepare the expected number of bytes. expected=%d, actual=%d\n\n",
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

    TRACEC(TRACE_PROTOCOL, "Request::WaitForReply(header_return=%p, packet_id=%d)\n\n", header_return, packet_id);

    EmpegPacketHeader *rx_header = NULL;
    *header_return = NULL;

    // The first timeout period is 2 seconds, as we should have a
    // PROGRESS or RESPONSE packet returned immediately. If we get
    // a PROGRESS reply then we use the timeout value enclosed to
    // set the timer for the next reply.
    while(true)
    {
	TRACEC(TRACE_PROTOCOL, "WaitForReply loop\n\n");

	TRACEC(TRACE_PROTOCOL, "Waiting for reply with id %d and timeout %d\n\n",
		 packet_id, period);
	// Get a packet
	result = Receive(&rx_header,period);
	TRACEC(TRACE_PROTOCOL, "Receive result was 0x%x\n\n", PrintableStatus(result));

	if (FAILED(result))
	{
	    TRACEC(TRACE_PROTOCOL, "Receive() failed in WaitForReply, error is 0x%x\n\n",
		     PrintableStatus(result));
	    if (result == CONN_E_TIMEDOUT)
		TRACEC(TRACE_PROTOCOL, "...that's a timeout error (timeout was %d)\n\n", period);
	    else if (result != MakeErrnoStatus(ENAK)
		 && result != MakeErrnoStatus(ENAK_CRC)
		 && result != MakeErrnoStatus(ENAK_DROPOUT)
		 && result != MakeErrnoStatus(ETIMEDOUT)
		 && result != MakeErrnoStatus(EREMOTENAK)
		 && result != MakeErrnoStatus(EREMOTENAK_CRC)
		 && result != MakeErrnoStatus(EREMOTENAK_BADHEADER)
		 && result != MakeErrnoStatus(EREMOTENAK_DROPOUT) )
		TRACEC(TRACE_PROTOCOL, "...that's a hitherto unknown error\n\n");

	    TRACEC(TRACE_PROTOCOL, "Packet::WaitForReply(A) = 0x%x\n\n", PrintableStatus(result));
	    return result;
	}

	ASSERT(rx_header);

	// Check the packet ID
	if (rx_header->packet_id != packet_id)
	{
	    TRACEC(TRACE_PROTOCOL, "Packet ID didn't match: received (id=%d, type=%d, opcode=%d), expected (id=%d)\n",
		      rx_header->packet_id, rx_header->type, rx_header->opcode, packet_id);
	    TRACEC(TRACE_PROTOCOL, "Packet::WaitForReply(F) = 0x%x\n", PrintableStatus(result));
            if (++wrong_packets > MAX_WRONG_PACKETS)
            {
		TRACEC(TRACE_PROTOCOL, "More than %d wrong packets in a row. What's going on?\n\n", MAX_WRONG_PACKETS);
	        result = MakeErrnoStatus(EWRONGPACKET);
	        return result;
            }
            else
            {
                TRACEC(TRACE_PROTOCOL, "Continuing to process buffer in the hope that the right packet is there.\n");
                continue;
            }
	}
        else
        {
            TRACEC(TRACE_PROTOCOL, "Got a response to packet %x\n", packet_id);
        }

	// Check the type
	if (rx_header->type==OPTYPE_PROGRESS)
	{
	    TRACEC(TRACE_PROTOCOL, "We have a progress reply.\n");
	    // We have a progress reply
	    ProgressResponsePacket *response=(ProgressResponsePacket*)rx_header;

	    // Use new supplied timeout
	    period=response->newtimeout;
	    TRACEC(TRACE_PROTOCOL, "Progress reply, new timeout is %d\n", period);

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
	    TRACEC(TRACE_PROTOCOL, "Got a real reply.\n\n");
	    // Got a reply, return ACK
	    // result = ACK;
	    *header_return = rx_header;
	    TRACEC(TRACE_PROTOCOL, "Packet::WaitForReply(G) = 0x%x\n", PrintableStatus(result));
	    return result;
	}
	else
	{
	    TRACE_WARN("Got unknown packet type in WaitForReply.\n");
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
    TRACEC(TRACE_PROTOCOL, "Request::Receive(h = %p, timeout=%d)\n", rx_header, timeout);

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
	TRACEC(TRACE_PROTOCOL, "Trying to receive bytes.\n\n");
#ifdef ARCH_PC
	// Make timeout longer on PC which may go off and swap or something
	const int packet_timeout = 5000;
#else
	// 250ms timeout on each byte
	const int packet_timeout = 2000;
#endif

	DWORD count;
	STATUS result = connection->Receive(tbuffer, sizeof(tbuffer), packet_timeout, count);

	TRACEC(TRACE_PROTOCOL, "Got %ld bytes from connection->Receive\n", count);

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
		TRACEC(TRACE_PROTOCOL, "Timeout in state %d (substate %d)\n\n", state, substate);

		state=0;
		TRACEC(TRACE_PROTOCOL, "Flush\n\n");
		Flush();

		// We've had a timeout *during* a packet - this shouldn't
		// happen.
		result = MakeErrnoStatus(ENAK_DROPOUT);
		TRACEC(TRACE_PROTOCOL, "Request::Receive(A) = %d\n\n", PrintableStatus(result));
		return result;
	    }

	    // Otherwise, see if real timer has expired
	    if (interval.Expired())
	    {
		TRACEC(TRACE_PROTOCOL, "No data timeout in state 0\n");
		if(connection->GetDebugLevel()>=1)
		    fprintf(stderr, "no data timeout in state 0\n");

		TRACEC(TRACE_PROTOCOL, "Request::Receive(B) = 0x%x\n", PrintableStatus(result));
		return result;
	    }
	    else
	    {
		TRACEC(TRACE_PROTOCOL, "It timed out - trying again\n");
		continue;
	    }
	}

	if (FAILED(result))
	{
            TRACEC(TRACE_PROTOCOL, "Got result 0x%x from connection->Receive\n", PrintableStatus(result));
            TRACEC(TRACE_PROTOCOL, "Request::Receive(C) = 0x%x\n", PrintableStatus(result));
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
		    TRACEC(TRACE_PROTOCOL, "We got a valid packet!\n");
		    // result = total_bytes;  //  FIXME
#ifdef DEBUG_PACKET_CONTENTS
		    dumper.Cancel();
#endif

		    // Push back everything we haven't used.
		    TRACEC(TRACE_PROTOCOL, "End of packet, pushing back %ld bytes\n", count - a - 1);
		    connection->PushBack(count - a - 1);
		}
		else
		{
		    TRACEC(TRACE_PROTOCOL, "We didn't get a valid packet - result is %d\n",
			     PrintableStatus(b));
                    if ( bufptr && bufptr > buffer )
                    {
                        TRACE_HEX("Dump of dodgy packet:\n", buffer, bufptr);
                    }

                    // Not an ACK?
		    // Some sort of error, flush the receiver
		    Flush();
		}

		TRACEC(TRACE_PROTOCOL, "Processing complete: result is %d\n", PrintableStatus(result));
		TRACEC(TRACE_PROTOCOL, "Request::Receive(D) = 0x%x\n", PrintableStatus(result));
		return result;
	    }
	}
#if 0
        if (interval.Expired()) {
	    if(connection->GetDebugLevel()>=1)
		fprintf(stderr, "timeout with data in state 0");

	    result = MakeErrnoStatus(ETIMEDOUT);
	    TRACEC(TRACE_PROTOCOL, "Request::Receive(E) = 0x%x\n", PrintableStatus(result));
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
	    TRACEC(TRACE_PROTOCOL, "Got a PSOH\n");
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
	    TRACEC(TRACE_PROTOCOL, "%c (\\x%02x)\n", (isprint(b) ? b : '.'), b);
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
	    TRACEC(TRACE_PROTOCOL, "Strange header size noticed: %d.\n\n", header->datasize);
	    state=0;
	    return MakeErrnoStatus(ENAK_BADHEADER);
	}
	else
	{
	    // Collect this many bytes
	    substate=(1+1+4+header->datasize);
	    bufptr=&buffer[2];
            TRACEC(TRACE_PROTOCOL, "Got packet length: %d bytes.\n\n", header->datasize);

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
        TRACEC(TRACE_PROTOCOL, "We've read all the data we were expecting, reading CRC.\n\n");
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
	    TRACEC(TRACE_PROTOCOL, "NAK (bad CRC) got=0x%08x, expected=0x%08x\n\n", sentcrc, crc);
	    if(connection->GetDebugLevel()>=1)
		fprintf(stderr, "NAK (bad CRC)\n");
	    return MakeErrnoStatus(ENAK_CRC);
	}
	else
	{
            TRACEC(TRACE_PROTOCOL, "CRC is good.\n\n");

	    // Process request
	    return S_OK;
	}
	break;

    default:
	TRACE_ERROR("Invalid state in %d Request::Process - resetting to default.\n", state);
        TRACEC(TRACE_PROTOCOL, "Invalid state %d, in Request::Process - resetting to default.\n", state);
	state = 0;
	return ProcessByte(b);
    }

    // Not finished processing.
    return S_AGAIN;
}

STATUS Request::Flush()
{
    TRACEC(TRACE_PROTOCOL, "FLUSH\n\n");
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
