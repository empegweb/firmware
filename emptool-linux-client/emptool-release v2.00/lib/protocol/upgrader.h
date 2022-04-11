/* upgrader.h
 *
 * Upgrading the player's built-in software
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Hugo Fiennes <hugo@empeg.com>
 *   Peter Hartley <peter@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.15 01-Apr-2003 18:52 rob:)
 */

#ifndef UPGRADER_H
#define UPGRADER_H

// Version number of upgrade file
#define UPGRADERVERSION         2

// Packetsize for disk pumping
#define PACKETSIZE       	4096

// Chunk types in upgrade file
#define CHUNK_INFO		0x00
#define CHUNK_WHAT		0x01
#define CHUNK_RELEASE		0x02
#define CHUNK_VERSION		0x03
#define CHUNK_HWREVS            0x04
#define CHUNK_UNTARDRIVE0       0x05

#define CHUNK_FLASHLOADER	0x10
#define CHUNK_FLASHKERNEL	0x11
#define CHUNK_FLASHRAMDISK	0x12
#define CHUNK_FLASHRANDOM	0x13

#define CHUNK_PUMPHDA		0x20
#define CHUNK_PUMPHDA1		0x21
#define CHUNK_PUMPHDA2		0x22
#define CHUNK_PUMPHDA3		0x23
#define CHUNK_PUMPHDA4		0x24
#define CHUNK_PUMPHDA5		0x25
#define CHUNK_PUMPHDA6		0x26
#define CHUNK_PUMPHDA7		0x27
#define CHUNK_PUMPHDA8		0x28

#define CHUNK_PUMPHDB		0x30
#define CHUNK_PUMPHDB1		0x31
#define CHUNK_PUMPHDB2		0x32
#define CHUNK_PUMPHDB3		0x33
#define CHUNK_PUMPHDB4		0x34
#define CHUNK_PUMPHDB5		0x35
#define CHUNK_PUMPHDB6		0x36
#define CHUNK_PUMPHDB7		0x37
#define CHUNK_PUMPHDB8		0x38

#define CHUNK_PUMPHDC		0x40
#define CHUNK_PUMPHDC1		0x41
#define CHUNK_PUMPHDC2		0x42
#define CHUNK_PUMPHDC3		0x43
#define CHUNK_PUMPHDC4		0x44
#define CHUNK_PUMPHDC5		0x45
#define CHUNK_PUMPHDC6		0x46
#define CHUNK_PUMPHDC7		0x47
#define CHUNK_PUMPHDC8		0x48

#define CHUNK_UPGRADERVERSION   0xff

#include "connection.h"

class UpgradeObserver
{
public:
    virtual void ShowStatus(int section, int operation, int current, int maximum)=0;
    virtual void ShowError(int section, int error)=0;
    virtual void LoadedText(const char *info, const char *what, 
	const char *release, const char *version)=0;
};

class Upgrader
{
    Connection	*m_connection;
    FILE		*m_file;
    unsigned char	m_buffer[16384];
    unsigned long	m_section;
    UpgradeObserver *m_observer;
    
    // Type of flash
    int 	m_manufacturer;
    int		m_product;
    
    // Filename of upgrade file
    char	*m_filename;
    
    // Text sections from upgrade file
    char	*m_info;
    char	*m_what;
    char	*m_release;
    char	*m_version;

public:
    Upgrader() 
	: m_file(NULL), m_section(0), m_observer(NULL), m_filename(NULL), 
	    m_info(NULL), m_what(NULL), m_release(NULL), m_version(NULL) 
    {
    }

    ~Upgrader()
    {
	DumpText();
	if (m_file!=NULL)
	    fclose(m_file);
    }
    
    // Check the upgrade file
    STATUS CheckUpgrade(const char *file);
    
    // Start the actual upgrade
    STATUS DoUpgrade(Connection *connection);
    
    // Tell the upgrader who 
    void SetObserver(UpgradeObserver *observer);
    
    // Status enums
    enum {
	UPGRADE_CHECKFILE=0, UPGRADE_CHECKEDFILE,
	    
	UPGRADE_FINDUNIT, UPGRADE_CHECKID, UPGRADE_ERASE,
	UPGRADE_PROGRAM, UPGRADE_RESTART,
	
	UPGRADE_FINDPUMP, UPGRADE_PUMPWAIT,
	UPGRADE_PUMPSELECT, UPGRADE_PUMP,
	UPGRADE_PUMPDONE,
	
	UPGRADE_DONE, UPGRADE_ERRORS,
    };
    
    /** @todo Unify these with the stuff in protocol_errors.mes?
     */
    enum {
	ERROR_NOERROR=0,
	ERROR_NOFILE, /* 1 */
	ERROR_SHORTFILE, /* 2 */
	ERROR_BADFILE, /* 3 */
	ERROR_NOCONNECTION, /* 4 */
	ERROR_NOUNIT, /* 5 */
	ERROR_BADID, /* 6 */
	ERROR_BADPROMPT, /* 7 */
	ERROR_BADERASE, /* 8 */
	ERROR_BADPROGRAM, /* 9 */
	ERROR_BADFLASHFILE, /* 10 */
	ERROR_NOPUMP, /* 11 */
	ERROR_BADPUMPSELECT, /* 12 */
	ERROR_BADPUMP, /* 13 */
	ERROR_BADPUMPFILE, /* 14 */
	ERROR_OLDUPGRADER, /* 15 */
	ERROR_HWREV, /* 16 */
    };
    
private:
    STATUS ReadUL(unsigned long *ul);
    STATUS ReadL(long *l);
    bool WaitFor(const char *string, int timeout);
    STATUS ReadHex(int timeout, int digits, int *hex);
    STATUS ReadDec(int timeout, int *hex);
    STATUS CommandAddress(int command, int address);
    STATUS ErasePage(int address);

    STATUS LockPage(int address);
    STATUS UnlockPage(int address);

    int ProgramBlock(int address, int length);
    void FlushRX();
    void Status(int stage, int percent);

    STATUS StartFlash();
    STATUS FlashArea(int address, int length);
    
    STATUS StartPump();
    int PumpPartition(const char *partitionname, int length);
    STATUS SizePartition(const char *partitionname, int *partition_size);
    int Partition(const char *partitionname);
    
    // Status reporting
    void Status(int state, int current, int max);
    void Status(int state);
    void Error(int state);
    
    // Dispose of text from upgrade file
    void DumpText();
};

#define PRODUCT_C3_8M	0x88c1

#endif
