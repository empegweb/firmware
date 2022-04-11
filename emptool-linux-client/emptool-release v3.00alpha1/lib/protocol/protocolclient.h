/* protocolclient.h
 *
 * High-level interface to the empeg protocol (transport-agnostic)
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.54 13-Mar-2003 18:15 rob:)
 */

#ifndef PROTOCOLCLIENT_H
#define PROTOCOLCLIENT_H 1

#include "empeg_status.h"
#include "connection.h"
#include "memory_stream.h"
#include "allocator.h"
#include "dyndata_format.h"

#ifndef WIN32
#include <errno.h>
#endif
#include <vector>

#define LINUX_EPERM 1
#define LINUX_ENOENT 2
#define LINUX_EIO 5
#define LINUX_ENXIO 6
#define LINUX_ENOMEM 12
#define LINUX_EACCES 13
#define LINUX_ENOSPC 28

class ProtocolObserver;
class TuneDatabaseObserver;
typedef std::vector<FILEID> vecFids;

class ProtocolClient
{
public:
    enum Activity
    {
	Read = 1,
	Write = 2,
	Stat = 3,
	Prepare = 4,
	DiskInfo = 5,
	Delete = 6,
	Check = 7,
	Remount = 8,
	Rebuild = 9,
	Waiting = 10
    };

    enum Warning
    {
	LOCAL_TIMEOUT = 1,
	LOCAL_ACKFAIL = 2,
	LOCAL_WRONGPACKET = 3,
	LOCAL_CRCFAIL = 4,
	LOCAL_DROPOUT = 5,
	
	REMOTE_TIMEOUT = 11,
	REMOTE_ACKFAIL = 12,
	REMOTE_WRONGPACKET = 13,
	REMOTE_CRCFAIL = 14,
	REMOTE_DROPOUT = 15
    };
    static const char *DescribeWarning(Warning w);

    enum PlayState
    {
	PAUSE = 0,
	PLAY = 1,
	TOGGLE = -1
    };

    struct DiskSpaceInfo
    {
	int size;	// in blocks
	int space;	// in blocks
	int block_size; // in bytes
    };

    struct PlayerVersionInfo
    {
	char version[32];
	char beta[32];
	int protocol;
    };

    struct PlayerIdentity
    {
	int hwrev;
	int serial;
	int build;
	unsigned long id[4];
	int ram;
	int flash;
    };

    ProtocolClient(Connection *p);
    ~ProtocolClient();

    ProtocolObserver *GetObserver() const { return observer; }
    ProtocolObserver *SetObserver(ProtocolObserver *o)
    {
	ProtocolObserver *old = observer;
	observer = o;
	return old;
    }
    
    class FidInMemory
    {
	Allocator *m_pAlloc;
	MemoryStream m_memStm;
	BYTE *m_buffer;

    public:
	FidInMemory(Allocator *a)
	    : m_pAlloc(a), m_memStm(a, 1024 * 1024), m_buffer(NULL)
	{
	}

	~FidInMemory();

	void Append(const BYTE *p, int cb)
	{
	    m_memStm.Append(p, cb);
	}

	const BYTE *GetBuffer() { return m_buffer = m_memStm.CompactAndExtractPointer(); }
	int GetSize() { return m_memStm.GetSize(); }
    };

    STATUS CheckProtocolVersion();
    STATUS RetrieveTagIndex(TuneDatabaseObserver *);
    STATUS RetrieveDatabases(TuneDatabaseObserver *);
    STATUS DeleteDatabases();
    STATUS StatFid(FILEID fid, int *psize);
    STATUS ReadFidToMemory(FILEID fid, BYTE **ppdest, int *psize);
    STATUS ReadFidToMemory2(FILEID fid, FidInMemory **ppfm);
    STATUS WriteFidFromMemory(FILEID fid, const BYTE *psource, int size);
    STATUS WriteFidFromFile(FILEID fid, const char *filename);
    STATUS PrepareFid(FILEID fid, int size);
    STATUS WriteFidPartial(FILEID fid, const BYTE *psource, int offset, int size);
    STATUS ReadFidPartial(FILEID fid, BYTE *pdest, int offset, int required_size, int *actual_size);
    void FreeFidMemory(BYTE **ppdest);
    STATUS DeleteFid(FILEID fid, int mask = 0xFFFF);
    STATUS CheckDatabaseAvailability();
    STATUS RebuildPlayerDatabase(int options);
    STATUS EnableWrite(bool b);
    STATUS GetDiskInfo(DiskSpaceInfo *, int count);
    STATUS CheckMedia( unsigned int *pfsck_flags );
    bool IsUnitConnected();
    STATUS WaitForUnitConnected(int timeout = 90);

    /// Restart just the player application. Used by synchronisation.
    STATUS RestartPlayer(bool auto_slumber = false, bool wait_for_completion = true);

    // Restart the entire unit, back to the bootloader. Used by the upgrader.
    STATUS RestartUnit(bool auto_slumber = false, bool wait_for_completion = true);
    STATUS LockUI(bool lock);
    STATUS SetSlumber(bool slumber);
    STATUS PlayFid(FILEID fid, int mode = 2);
    STATUS PlayFids(const vecFids &fids, int mode = 2);
    STATUS SetPlayState(int);
    STATUS GrabScreen(unsigned char *screen);

    STATUS InitiateSession(const char *host_description, const char *password, UINT32 *session_cookie, std::string *failure_reason);
    STATUS SessionHeartbeat(UINT32 session_cookie);
    STATUS TerminateSession(UINT32 session_cookie);
    STATUS NotifySyncComplete();

    STATUS GetPlayerType(std::string *type);
    STATUS GetVersionInfo(PlayerVersionInfo *);
    STATUS GetProtocolVersion(short *major, short *minor);
    STATUS GetPlayerConfiguration(std::string *s);
    STATUS SetPlayerConfiguration(const std::string &s);
    STATUS GetPlayerIdentity(PlayerIdentity *);

    bool FileNotFoundStatus(STATUS result);
    int GetMaximumRetryCount() const
    {
	return max_retries;
    }

    void SetMaximumRetryCount(int i)
    {
	if (i >= 0)
	    max_retries = i;
	else
	    max_retries = DEFAULT_MAX_RETRIES;
    }
    int GetPacketSize() const
    {
	return connection->PacketSize();
    }

private:
    static const int DEFAULT_MAX_RETRIES;
    int max_retries;

    Connection *connection;
    ProtocolObserver *observer;

    short m_protocol_version_major; // -1 for don't know yet
    short m_protocol_version_minor;

    void DoReportProgress(Activity a, int current, int maximum);
    void DoReportWarning(Warning w);
    STATUS SendCommand(int command, int parameter0 = 0, int parameter1 = 0,
		       const char *parameter2 = NULL);
    STATUS PlayFidList(const vecFids &fids, int mode);
    void DoReportResult(STATUS result);
    void ErrorAction(STATUS result, int retries);
    STATUS Fsck(const char *drive, unsigned int *pfsck_flags);

    STATUS InternalRestart(int param, bool wait_for_complete);

    class RequestProgress;

};

class ProtocolObserver
{
public:
    virtual void ReportProgress(ProtocolClient *client, ProtocolClient::Activity a, int current, int maximum) = 0;
    virtual void ReportWarning(ProtocolClient *client, ProtocolClient::Warning w) = 0;
    virtual ~ProtocolObserver() {};
};

class TuneDatabaseObserver
{
public:
    virtual STATUS TagAddAction(const char *name) = 0;
    virtual STATUS TagIndexAction(int index, const char *name) = 0;
    virtual STATUS DatabaseEntryAction(FILEID fid,
				       const BYTE *static_entry,
				       const DynamicData *dynamic_entry) = 0;
    virtual ~TuneDatabaseObserver() {};
};

inline void ProtocolClient::DoReportProgress(ProtocolClient::Activity a, int current, int maximum)
{
    if (observer)
	observer->ReportProgress(this, a, current, maximum);
}

inline void ProtocolClient::DoReportWarning(ProtocolClient::Warning w)
{
    if (observer)
	observer->ReportWarning(this, w);
}

#endif
