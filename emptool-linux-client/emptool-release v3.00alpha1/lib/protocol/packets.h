/* packets.h
 *
 * Packet structures in the player protocol
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *   Hugo Fiennes <hugo@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.52 13-Mar-2003 18:15 rob:)
 */

#ifndef PACKETS_H
#define PACKETS_H

#ifdef _MSC_VER
// We don't care about zero length arrays being non-standard.
#pragma warning(disable : 4200)
#endif


#include "empeg_error.h"
#include "mutex.h"
#include "connection.h"
#include "types.h"

#define PROTOCOL_TCP_PORT	8300
#define PROTOCOL_TCP_FASTPORT	8301

#define PROTOCOL_VERSION_MAJOR 6
#define PROTOCOL_VERSION_MINOR 0

enum {
    ACK = 6,
    NAK = 21,
    NAK_DROPOUT = 22,
    NAK_CRC = 23,
    NAK_BADHEADER = 24
};

// When the empeg sends back one of the above it is translated
// into one of the following so we can differentiate it from
// the same problem occurring at this end.
#define REMOTE_NAK		11
#define REMOTE_NAK_DROPOUT	12
#define REMOTE_NAK_CRC		13
#define REMOTE_NAK_BADHEADER	14

// Error for NAK
#define ENAK                    2100
#define ENAK_DROPOUT		2101
#define ENAK_CRC		2102
#define ENAK_BADHEADER		2103
#define EWRONGPACKET		2200

// Matching versions of the remote versions above
#define EREMOTENAK		2300
#define EREMOTENAK_DROPOUT	2301
#define EREMOTENAK_CRC		2302
#define EREMOTENAK_BADHEADER	2303

#define LOCAL_NAK_ERROR(X) (((X) == ENAK) || ((X) == ENAK_DROPOUT) || ((X) == ENAK_CRC) || ((X) == ENAK_BADHEADER))
#define LOCAL_NAK_STATUS(X) (((X) == MakeErrnoStatus(ENAK)) \
			     || ((X) == MakeErrnoStatus(ENAK_DROPOUT)) \
			     || ((X) == MakeErrnoStatus(ENAK_CRC)) \
			     || ((X) == MakeErrnoStatus(ENAK_BADHEADER)))
#define REMOTE_NAK_ERROR(X) (((X) == EREMOTENAK)             \
			     || ((X) == EREMOTENAK_DROPOUT)  \
			     || ((X) == EREMOTENAK_CRC)      \
			     || ((X) == EREMOTENAK_BADHEADER))
#define REMOTE_NAK_STATUS(X) (((X) == MakeErrnoStatus(EREMOTENAK))  \
			      || ((X) == MakeErrnoStatus(EREMOTENAK_DROPOUT)) \
			      || ((X) == MakeErrnoStatus(EREMOTENAK_CRC)) \
			      || ((X) == MakeErrnoStatus(EREMOTENAK_BADHEADER)))

typedef struct tagEmpegPacketHeader
{
    unsigned datasize : 16;
    unsigned opcode : 8;
    unsigned type : 8;
    unsigned packet_id : 32;
//	unsigned char data[0];
} EmpegPacketHeader;

//
// EmpegPacketHeader::type
//
#define OPTYPE_REQUEST	0
#define OPTYPE_RESPONSE	1
#define OPTYPE_PROGRESS 2

//
// EmpegPacketHeader::opcode
//
#define OP_PING		    0	// Ping: check remote presence
#define OP_QUIT		    1	// Quit: forces remote player to quit
#define OP_MOUNT	    2	// Remount read-only or read-write
#define OP_WRITEFID 	    4	// Write to a FID
#define OP_READFID	    5	// Read from a FID
#define OP_PREPAREFID	    6	// Create an empty FID of a set length to
				// write to
#define OP_STATFID  	    7	// Find out the length of a FID.
#define OP_DELETEFID 	    8	// Delete a FID
#define OP_REBUILD	    9	// Rebuild databases (and save it if set r/w)
#define OP_FSCK		    10	// fsck drives
#define OP_STATFS	    11	// Stat a filing system
#define OP_COMMAND	    12	// Send a generic command
#define OP_GRABSCREEN	    13	// Grab a screen image
#define OP_INITIATESESSION  14  // Obtain modification lock on player
#define OP_SESSIONHEARTBEAT 15  // "Hello, still here"
#define OP_TERMINATESESSION 16  // Relinquish modification lock
#define OP_COMPLETE         17  // Notify sync complete

// sub-opcodes for OP_COMMAND commands
#define COM_RESTART	    0	// Exit player/restart player/reboot unit
#define COM_LOCKUI 	    1	// Lock the user interface during sync
				// operations
#define COM_SLUMBER	    2	// Switch the player into slumber mode.
#define COM_PLAYFID	    3	// Start playing a specific FID.
#define COM_PLAYSTATE	    4	// Play/pause the unit.
#define COM_BUILDMULTFIDS   5	// Start building a list of FIDs for playing.
#define COM_PLAYMULTFIDS    6	// Last list of FIDs for playing.

#define RESTART_EXIT            0x00
#define RESTART_PLAYER          0x01
#define RESTART_UNIT            0x02
#define RESTART_HALT	        0x03
#define RESTART_UPGRADE_CD      0x04 /* Attempt to run upgrader from a CD */
#define RESTART_UPGRADE_DL      0x05 /* Attempt to run upgrader from download */
#define RESTART_SHELL           0x06 /* Exit player, run a shell, then restart player */
#define RESTART_IN_SLUMBER      0xf00

// Progress packet return
typedef struct tagProgressResponsePacket
{
    EmpegPacketHeader header;
    int newtimeout;
    int stage;
    int stage_maximum;
    int current;
    int maximum;
    char string[64];
} ProgressResponsePacket;

// Ping packet
typedef struct tagPingRequestPacket
{
    EmpegPacketHeader header;
} PingRequestPacket;

typedef struct tagPingResponsePacket
{
    EmpegPacketHeader header;
    short version_minor;
    short version_major;
} PingResponsePacket;

// Structure for mounting
typedef struct tagMountRequestPacket
{
    EmpegPacketHeader header;
    int partition;
    int mode;
} MountRequestPacket;

typedef struct tagMountResponsePacket
{
    EmpegPacketHeader header;
    STATUS response;
} MountResponsePacket;

// Structure for reading/writing FIDs
typedef struct tagTransferRequestPacket
{
    EmpegPacketHeader header;
    UINT32 file_id;
    UINT32 chunk_offset;
    INT32 chunk_size;
    BYTE data[0];
} TransferRequestPacket;

typedef struct tagTransferResponsePacket
{
    EmpegPacketHeader header;
    STATUS result;
    FILEID file_id;
    UINT32 chunk_offset; // for information only.
    INT32  nbytes;    // how many bytes were actually read/written
    BYTE data[0];
} TransferResponsePacket;

typedef struct tagPrepareRequestPacket
{
    EmpegPacketHeader header;
    UINT32 file_id;
    UINT32 file_size;
} PrepareRequestPacket;

typedef struct tagDeleteRequestPacket
{
    EmpegPacketHeader header;
    FILEID	file_id;
    UINT32	id_mask;
} DeleteRequestPacket;

typedef struct tagDeleteResponsePacket
{
    EmpegPacketHeader header;
    STATUS	result;
} DeleteResponsePacket;

typedef struct tagStatRequestPacket
{
    EmpegPacketHeader header;
    FILEID	file_id;
} StatRequestPacket;

typedef struct tagStatResponsePacket
{
    EmpegPacketHeader header;
    STATUS      result;
    FILEID	file_id;
    INT32	file_size;
} StatResponsePacket;

typedef struct tagRebuildRequestPacket
{
    EmpegPacketHeader header;
    INT32	operation;
} RebuildRequestPacket;

typedef struct tagRebuildResponsePacket
{
    EmpegPacketHeader header;
    STATUS	result;
} RebuildResponsePacket;

typedef struct tagFsckRequestPacket
{
    EmpegPacketHeader header;
    char	partition[16];
    int	force;
} FsckRequestPacket;

typedef struct tagFsckResponsePacket
{
    EmpegPacketHeader header;
    STATUS      result;
    UINT32	flags;
} FsckResponsePacket;

typedef struct tagStatfsRequestPacket
{
    EmpegPacketHeader header;
} StatfsRequestPacket;

typedef struct tagStatfsResponsePacket
{
    EmpegPacketHeader header;
    UINT32 drive0size;		// Space is reported in blocks
    UINT32 drive0space;
    UINT32 drive0blocksize;
    UINT32 drive1size;
    UINT32 drive1space;
    UINT32 drive1blocksize;
} StatfsResponsePacket;

typedef struct tagCommandRequestPacket
{
    EmpegPacketHeader header;
    int	command;
    int	parameter0;
    int	parameter1;
    char parameter2[256];
} CommandRequestPacket;

typedef struct tagCommandResponsePacket
{
    EmpegPacketHeader header;
    STATUS	result;
} CommandResponsePacket;

typedef struct tagGrabScreenRequestPacket
{
    EmpegPacketHeader header;
    int	command;
} GrabScreenRequestPacket;

typedef struct tagGrabScreenResponsePacket
{
    EmpegPacketHeader header;
    STATUS result;
    unsigned char screen[2048];
} GrabScreenResponsePacket;

typedef struct tagInitiateSessionRequestPacket
{
    EmpegPacketHeader header;
    char password[64];
    char host_description[256];
} InitiateSessionRequestPacket;

typedef struct tagInitiateSessionResponsePacket
{
    EmpegPacketHeader header;
    STATUS result;
    UINT32 session_cookie;
    char failure_reason[256];
} InitiateSessionResponsePacket;

typedef struct tagSessionHeartbeatRequestPacket
{
    EmpegPacketHeader header;
    UINT32 session_cookie;
} SessionHeartbeatRequestPacket;

typedef struct tagSessionHeartbeatResponsePacket
{
    EmpegPacketHeader header;
    STATUS result;
} SessionHeartbeatResponsePacket;

typedef struct tagTerminateSessionRequestPacket
{
    EmpegPacketHeader header;
    UINT32 session_cookie;
} TerminateSessionRequestPacket;

typedef struct tagTerminateSessionResponsePacket
{
    EmpegPacketHeader header;
    STATUS result;
} TerminateSessionResponsePacket;

typedef struct tagNotifySyncCompleteRequestPacket
{
    EmpegPacketHeader header;
    UINT32 unused;
} NotifySyncCompleteRequestPacket;

typedef struct tagNotifySyncCompleteResponsePacket
{
    EmpegPacketHeader header;
    STATUS result;
} NotifySyncCompleteResponsePacket;

// File subtype Ids:
// (These only apply for tunes, other file types may be different)

#define FST_CONTENT	0x0 	// Actual content, for a tune this is the tune
				// file. For playlists this is a collection of
				// little-endian tune IDs
#define FST_META	0x1	// Tune metadata (plain text)

class Request;

// Observer class for request
class RequestObserver
{
public:
    virtual void ProgressAction(Request *req, int stage, int stage_maximum,
				int current, int maximum, char *string) = 0;
};

class RequestIdentifierFactory
{
    static Mutex m_lock_id;
    static UINT32 m_id;

public:
    static UINT32 Reserve();
};

// Request processing in/out
class Request
{
public:
    Request(Connection *c);		// Instantiated with connection to use
    ~Request();

    STATUS Ping();			// Check remote end is present
    STATUS Quit();			// Ask remote end to exit server
    STATUS Mount(int readwrite);	// Remount disks
    STATUS WriteFID(int fid, int offset, int size, const BYTE *buffer);
    // Send block of data
    STATUS ReadFID(int fid, int offset, int size, BYTE *buffer,
		   int *pBytesRead);
    // Read block of data
    STATUS PrepareFID(int fid, int size); // Prepare a file for download

    STATUS StatFID(int fid, int *psize);	// Find out how big a file is.

    STATUS DeleteFID(int fid, unsigned int mask);
    // Delete fids

    STATUS WaitForReply(EmpegPacketHeader **header, unsigned int packet_id);
    // Wait for a reply & process PROGRESS responses

    // Wait for a good packet
    STATUS Receive(EmpegPacketHeader **header, int timeout);

    /// Send packet and CRC
    STATUS Send(EmpegPacketHeader *header) const;

    /// Send current operation progress
    void SendProgress(ProgressResponsePacket* progress) const;

    /// Rebuild static tune database
    STATUS BuildAndSaveDatabase(int options);

    STATUS Fsck(const char *partition, int force,
		unsigned int *pfsck_flags);	/// Fsck a partition

    /// Find out how much free space is on drives
    STATUS FreeSpace(int *drive0size,int *drive0space,int *drive0blocksize,
		     int *drive1size,int *drive1space,int *drive1blocksize);

    /// Send a generic command
    STATUS SendCommand(int command, int parameter0 = 0, int parameter1 = 0,
		       const char *parameter2 = NULL);

    /// Grab screen image
    STATUS GrabScreen(int command, unsigned char *buffer);

    STATUS InitiateSession(const char *host_description, const char *password, UINT32 *session_cookie, std::string *failure_reason);
    STATUS SessionHeartbeat(UINT32 session_cookie);
    STATUS TerminateSession(UINT32 session_cookie);
    STATUS NotifySyncComplete();

    /// Flush everything.
    STATUS Flush();

    void NewRequest();

    // Not to be used except for debugging.
    void SetRequest(UINT32 i);

    STATUS GetProtocolVersion(short *major, short *minor);

    void SetObserver(RequestObserver *o) { this->observer = o; }

private:
    STATUS ProcessByte(BYTE b);		// Process it

    STATUS GenericWrite(EmpegPacketHeader* h, int *pnbytes);

private:
    Connection*	connection;		// Connection we're using
    UINT32	m_packet_id;		// ID of transaction

    int		state;			// State of receiver
    int		substate;		// Collect data
    UINT16	crc;			// Calculated CRC
    UINT16	sentcrc;		// CRC they sent

    BYTE	*bufptr;		// Buffer pointer

    BYTE	*buffer;		// Receive buffer
    int		buffer_size;            // Sizeof receive buffer
    EmpegPacketHeader *header;		// Packet header
    short	protocol_version_minor; // major version of protocol
    short	protocol_version_major; // minor version of protocol

    RequestObserver *observer;		// observer for OPTYPE_PROGRESS reports
};

// Streamlined protocol over TCP fast connection

struct TCPFastHeader
{
    UINT32 operation;  // 0 == write FID
    UINT32 fid;
    UINT32 offset;
    UINT32 size;
};

#endif
