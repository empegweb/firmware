/* upgrader.cpp
*
* empeg upgrader class
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
* (:Empeg Source Release 1.38 01-Apr-2003 18:52 rob:)
*
* version 0.07 20000629 HBF
*/

/** \class Upgrader 
 * This class deals with the processing of an upgrade package file, which
 * happens in several stages:
 * - Length sanity check
 * - CRC check of entire file
 * - Load text/release note portions into memory and tell the observer about them
 *   (so user can inspect release notes if necessary)
 * - Process each upgrade block in file, which fall into two categories:
 *   a. Flash upgrade - erases/reprograms a section of flash ROM
 *   b. Disk pump - sends a .gz image directly to a disk or disk partition
 * - Reboot
 *
 * (a reboot also occurs between upgrading flash and pumping the disk).
 *
 * Note that this code isn't pretty, but it does the job. There are different
 * protocols for downloading the flash as this talks to the original flash
 * downloader in the write-protected portion of the flash rom - and this has
 * never been tidied on the "it ain't broke" principle. This uses simple
 * checksums on blocks and a very strange command interface. Two extra
 * commands, 'l' and 'u' are supported on mk2 empegs which use the Intel
 * TE28F800C3B flash which has individually lockable blocks.
 *
 * The disk pumping code runs from the flash-based ramdisk on the empeg at
 * every boot of the system (and steps out of the way if it's not needed
 * fairly smartish) and streams a gzipped file direct to a block device -
 * be it the actual physical drive (used to partition drives) or a
 * partition on the drive. As the player partition is a read-only partition
 * with no user data on it, this allows easy upgrades to the player.
 *
 * To get status information to update dialogs, etc, during an upgrade,
 * you need to register an UpgradeObserver with the Upgrader class. This
 * gets called with status as the upgrade progresses.
 *
 * Version 0.05 - now supports hda5/6/7/8 and hdc5/6/7/8
 * Version 0.06 - flasher portion now deals with C3 flash chips (id 88c1)
 * Version 0.07 - deals with pumping hdb if necessary
 */

#include "config.h"
#include "trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef WIN32
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "connection.h"
#include "crc.h"
#include "upgrader.h"
#include "protocolclient.h"
#include "protocol_errors.h"
#include "timeout.h"

// Some definitions
#define SOH 1
#define ACK 6
#define NAK 21
#define CAN 24

// Pump packet size
#define PACKETSIZE 4096

// Timeouts
#define FINDUNIT_TIMEOUT		180000	// 180 seconds
#define FINDUNIT_INTERVAL		100	// 10Hz
#define FINDUNIT_STATUS_INTERVAL	1000	// 1Hz

/** Status reporting
 */
void Upgrader::Status(int state, int current, int max)
{
    // ...this allows some shortcuts in the calling code...
    if (current>max) current=max;

    // Call observer if present
    if (m_observer) m_observer->ShowStatus((int)m_section,state,current,max);
}

/** Status reporting for when we don't have much to say
 */
void Upgrader::Status(int state)
{
    Status(state,0,0);
}

/** Status reporting for errors
 */
void Upgrader::Error(int state)
{
    if (m_observer)
	m_observer->ShowError((int)m_section,state);
}

/** Wait for a string
 * WaitFor times out only when there's no data for n seconds.
 * The whole string is sent in one, so will never span across calls.
 */
bool Upgrader::WaitFor(const char *s, int timeout)
{
    const char *p=s;
    int ch;
    int to = time(NULL) + timeout;
    while(time(NULL) < to)
    {
	STATUS result = m_connection->Receive(1000, ch);
        if (SUCCEEDED(result))
	{
	    if (ch == *p)
	    {
		if (!*++p)
		    return true;

	    }
	    else
		p = (ch == s[0]) ? (s+1) : s;
	}
    }

    return false;
}

/** Read a hex value
 */
STATUS Upgrader::ReadHex(int timeout, int digits, int *hex)
{
    char buf[16];
    int a, d, result;
    STATUS status;

    // Read hex data
    for(a = 0; a < digits; a++)
    {
	if (FAILED(status = m_connection->Receive(timeout * 1000, d)))
	    return status;

	buf[a] = d;
    }

    buf[digits] = 0;

    // Parse it
    if (sscanf(buf, "%x", &result) != 1)
	return UPG_E_PARSEHEX;

    *hex = result;
    return S_OK;
}

/** Read a decimal value
 */
STATUS Upgrader::ReadDec(int timeout, int *dec)
{
    int d;

    // Process data until we get a digit
    do {

	STATUS status;
	if (FAILED(status = m_connection->Receive(timeout * 1000, d)))
	    return status;

    } while (!isdigit(d));

    int result = (d - '0');

    // Process whole number
    do {
	// It went bang - give up.
	if (FAILED(m_connection->Receive(timeout * 1000, d)))
	{
	    *dec = result;
	    return S_OK;
	}

	if (isdigit(d))
	    result = (result * 10) + (d - '0');

    } while (isdigit(d));

    // Return it
    *dec = result;
    return S_OK;
}

/** Flush input buffer
 */
void Upgrader::FlushRX()
{
    m_connection->FlushReceiveBuffer();
}

/** Generic command
 * @todo Find a better way to handle the specific errors from the flash chip.
 */
STATUS Upgrader::CommandAddress(int command,int address)
{
    STATUS status;

    // Empty buffer first
    FlushRX();

    // Send command
    m_connection->Send(command);
    int ch;
    if (FAILED(status = m_connection->Receive(1000, ch)) || ch != command)
	return status;

    // Send address
    m_connection->Send((address>> 0)&0xff);
    m_connection->Send((address>> 8)&0xff);
    m_connection->Send((address>>16)&0xff);
    m_connection->Send((address>>24)&0xff);

    // Get 8-byte hex address reply and see if it matches
    int a;
    if (FAILED(status = ReadHex(1,8,&a)))
	return status;

    if (a != address)
	return UPG_E_ADDRESSMISMATCH;

    // Should now have a '-' showing it's working on the erase
    if (FAILED(m_connection->Receive(1000, ch)) || ch != '-')
	return UPG_E_NOHYPHEN;

    // Read result (leave 2 seconds)
    int res;
    if (FAILED(status = ReadHex(2,2,&res)))
	return status;

    if (res != 0x80) // It's a chip-specific error - see the todo comment, above.
	return UPG_E_CHIPERROR;

    return S_OK;
}

/** Erase page of flash
 */
STATUS Upgrader::ErasePage(int address)
{
    return(CommandAddress('e',address));
}

/** Lock page of flash (C3 flash only)
 */
STATUS Upgrader::LockPage(int address)
{
    return(CommandAddress('l',address));
}

/** Unlock page of flash (C3 flash only)
 */
STATUS Upgrader::UnlockPage(int address)
{
    return(CommandAddress('u',address));
}

/** Enter disk pumper
 */
STATUS Upgrader::StartPump()
{
    // Waiting for unit to be powered on
    Status(UPGRADE_FINDPUMP);

    // Wait for pump start note
    int timeout=0;
    while(timeout<=60)
    {
	// This works as waitfor times out only when there's no data for
	// n seconds - the whole string is sent in one, so will never span
	// waitfor calls
	Status(UPGRADE_FINDPUMP,timeout,60);
	if (WaitFor("Ctrl-A",3))
	{
	    // Found it!
	    break;
	}
	else
	    timeout++;
    }

    if (timeout>60)
    {
	// Can't find it
	Error(ERROR_NOPUMP);
	return UPG_E_NOPUMP;
    }

    // Send ctrl-A to drop into pump mode
    m_connection->Send(SOH);

    return S_OK;
}

/** Get the size of a partition.
 */
STATUS Upgrader::SizePartition(const char *partitionname, int *partition_size)
{
    // Waiting for unit to be powered on
    Status(UPGRADE_PUMPWAIT);

    // Clear buffer
    FlushRX();

    // Send a CR to elicit a pump response
    m_connection->Send('\r');

    // Wait for pump prompt
    if (!WaitFor("pump> ",5)) {
	// Pump doesn't appear to be there
	Error(ERROR_NOPUMP);
	return UPG_E_NOPUMP;
    }

    // Send partition select
    Status(UPGRADE_PUMPSELECT,0,1);

    int l=sprintf((char*)m_buffer,"device %s",partitionname);
    m_connection->Send(m_buffer,l);
    m_connection->Send('\r');
    if (!WaitFor((char*)m_buffer,5)) {
	// Didn't select the partition correctly
	Error(ERROR_BADPUMPSELECT);
	return UPG_E_BADPUMPSELECT;
    }

    // Get partition size
    m_connection->Send((const unsigned char*)"size\r",5);

    return ReadDec(2, partition_size);
}

/** Size partition
 */
int Upgrader::Partition(const char *partitionname)
{
    int l;

    // Waiting for unit to be powered on
    Status(UPGRADE_PUMPWAIT);

    // Clear buffer
    FlushRX();

    // Send a CR to elicit a pump response
    m_connection->Send('\r');

    // Wait for pump prompt
    if (!WaitFor("pump> ",5)) {
	// Pump doesn't appear to be there
	Error(ERROR_NOPUMP);
	return(2);
    }

    // Send partition select
    Status(UPGRADE_PUMPSELECT,0,1);

    l=sprintf((char*)m_buffer,"partition %s",partitionname);
    m_connection->Send(m_buffer,l);
    m_connection->Send('\r');
    if (!WaitFor((char*)m_buffer,5)) {
	// Didn't select the partition correctly
	Error(ERROR_BADPUMPSELECT);
	return(3);
    }

    return(0);
}

/** Pump a partition with gzipped data
 */
int Upgrader::PumpPartition(const char *partitionname, int length)
{
    long offset = 0;
    int errors = 0;
    int l;
    unsigned int chunk;

    // Waiting for unit to be powered on
    Status(UPGRADE_PUMPWAIT);

    // Clear buffer
    FlushRX();

    // Send a CR to elicit a pump response
    m_connection->Send('\r');

    // Wait for pump prompt
    if (!WaitFor("pump> ", 5))
    {
	// Pump doesn't appear to be there
	Error(ERROR_NOPUMP);
	return 2;
    }

    // Send partition select
    Status(UPGRADE_PUMPSELECT,0,1);

    l = sprintf((char*)m_buffer,"device %s", partitionname);
    m_connection->Send(m_buffer, l);
    m_connection->Send('\r');
    if (!WaitFor((char*)m_buffer, 5))
    {
	// Didn't select the partition correctly
	Error(ERROR_BADPUMPSELECT);
	return 3;
    }

    // Tell unit what to expect
    l = sprintf((char*)m_buffer,"pump %d", length);
    m_connection->Send(m_buffer,l);
    m_connection->Send('\r');
    if (!WaitFor((char*)m_buffer, 5))
    {
	// Incorrect pump response
	//TRACE("Incorrect pump response\n");
	Error(ERROR_BADPUMP);
	return 4;
    }

    // Load first bufferful
    chunk = (length < PACKETSIZE) ? length : PACKETSIZE;
    size_t result = fread(m_buffer, 1, chunk, m_file);
    if (result!=chunk)
    {
	// Problem reading the file
	Error(ERROR_BADPUMPFILE);
	return 5;
    }

    // Write file PACKETSIZE at a time
    while(offset<length && errors<16)
    {
	// Send chunk
	if (FAILED(m_connection->Send(SOH)))
	    break;

	if (FAILED(m_connection->Send(m_buffer,PACKETSIZE)))
	    break;

	// Calculate CRC
	int crc = CRC::CRC32(m_buffer, PACKETSIZE);

	// Send CRC, LSB first
	// Flush buffer
	FlushRX();

	if (FAILED(m_connection->Send(static_cast<unsigned char>(crc)    ))) break;
	if (FAILED(m_connection->Send(static_cast<unsigned char>(crc>> 8)))) break;
	if (FAILED(m_connection->Send(static_cast<unsigned char>(crc>>16)))) break;
	if (FAILED(m_connection->Send(static_cast<unsigned char>(crc>>24)))) break;

	// Wait for ACK or NAK
	do {
	    if(FAILED(m_connection->Receive(20000, l)) || l < 0)
		l = NAK;

	} while (l != ACK && l != NAK);

	// Resend packet on NAK
	if (l == NAK)
	{
	    errors++;
	    continue;
	}

	// Otherwise, move onto next buffer
	offset += PACKETSIZE;

	// Reset error count (it's max errors on each block)
	errors = 0;

	// Update status
	Status(UPGRADE_PUMP,(offset > length) ? length : offset, length);

	// If we're not finished, read the next bit
	if (offset < length)
	{
	    if ((length - offset) < PACKETSIZE)
		chunk = length - offset;
	    else
		chunk = PACKETSIZE;

	    result = fread(m_buffer, 1, chunk, m_file);
	    if (result != chunk)
	    {
		// Problem reading file
		Error(ERROR_BADPUMPFILE);
		return 6;
	    }
	}
    }

    // Finished?
    if (offset<length)
    {
	// Sleep then send a CAN: the sleep will reset the remote state
	// machine, the CAN will abort the download
	// This seems to happen sometimes MAC 1999/08/17
	//TRACE("offset < length. offset=%d, length=%d\n", offset, length);
#ifdef WIN32
        Sleep(4000);
#else
	sleep(4);
#endif
	m_connection->Send(CAN);

	// Errors during the download. Oops.
	Error(ERROR_BADPUMP);
	return 7;
    }

    // All sent OK!
    Status(UPGRADE_PUMPDONE, 1, 1);

    return 0;
}

/** Program flash rom from ram buffer
 */
int Upgrader::ProgramBlock(int address,int length)
{
    int checksum, a;

    // Empty buffer first
    FlushRX();

    // Send command
    m_connection->Send('p');

    int ch;
    if (FAILED(m_connection->Receive(1000, ch)) || ch != 'p')
	return 1;

    // Send address
    m_connection->Send((address>> 0)&0xff);
    m_connection->Send((address>> 8)&0xff);
    m_connection->Send((address>>16)&0xff);
    m_connection->Send((address>>24)&0xff);

    // Get 8-char hex address reply and see if it matches
    int reported_address;
    if (FAILED(ReadHex(1, 8, &reported_address)) || reported_address != address)
	return 2;

    // Should now have a '-' showing it's waiting for length
    if (FAILED(m_connection->Receive(1000, ch)) || ch !='-')
	return 3;

    // Send length
    m_connection->Send((length>> 0)&0xff);
    m_connection->Send((length>> 8)&0xff);

    // Get 4-char hex length reply and see if it matches
    int reported_length;
    if (FAILED(ReadHex(1, 4, &reported_length)) || reported_length !=length)
	return 4;

    // Should now have a '-' showing it's waiting for data
    if (FAILED(m_connection->Receive(1000, ch)) || ch !='-')
	return 5;

    // Send data
    for(a = checksum = 0; a < length; a++)
    {
	checksum += m_buffer[a];
	m_connection->Send(m_buffer[a]);
    }

    // Send checksum
    m_connection->Send(checksum&0xff);

    // Should get an 'ok' indicating good download
    if (FAILED(m_connection->Receive(1000, ch)) || ch != 'o')
	return 6;

    if (FAILED(m_connection->Receive(1000, ch)) || ch != 'k')
	return 7;

    if (FAILED(m_connection->Receive(1000, ch)) || ch != '\r')
	return 8;

    if (FAILED(m_connection->Receive(1000, ch)) || ch != '\n')
	return 9;

    // Should have program reply within a second
    if (FAILED(m_connection->Receive(1000, ch)) || ch != 'o')
	return 10;

    if (FAILED(m_connection->Receive(1000, ch)) || ch != 'k')
	return 11;

    // All ok
    return 0;
}

/** Enter flash ROM download
 */
STATUS Upgrader::StartFlash()
{
    // Waiting for unit to be powered on
    Status(UPGRADE_FINDUNIT);

    // Wait for powerup message
    static const char *magic = "Flash";
    const char *m = magic;
    Timeout giveup(FINDUNIT_TIMEOUT);
    Timeout status_interval(FINDUNIT_STATUS_INTERVAL);
    Timeout send_interval(FINDUNIT_INTERVAL);
    giveup.Reset();
    status_interval.Reset();
    send_interval.Reset();		// send ctrl-A at on first loop
    do {
	if(!*m)
	    break;		// got string

	if(status_interval.TimedOut()) {
	    // update status
	    Status(UPGRADE_FINDUNIT,
		FINDUNIT_TIMEOUT - giveup.GetRemainingMilliseconds(),
		FINDUNIT_TIMEOUT);
	    status_interval.Reset();
	}

	if(send_interval.TimedOut()) {
	    // send ctrl-A
	    m_connection->Send(SOH);
	    send_interval.Reset();
	}

	// wait the least amount of time required
	int remaining_send = send_interval.GetRemainingMilliseconds();
	int remaining_giveup = giveup.GetRemainingMilliseconds();
	int wait_time = remaining_send;
	if(remaining_giveup < wait_time) wait_time = remaining_giveup;
	if(wait_time < 0) wait_time = 0;

	int c;
        if(SUCCEEDED(m_connection->Receive(wait_time, c)) && c >= 0)
	{
	    if(c == *m)
		m++;		    // gotcha!
	    else
		m = magic;		    // not gotcha!
	}

    } while(!giveup.TimedOut());

    // Found it?
    if(giveup.TimedOut())
    {
	Error(ERROR_NOUNIT);
	return UPG_E_NOUNIT;
    }

    // Manufacturer ID: Look for '#' then 4 hex digits
    int d;
    STATUS result;
    while(SUCCEEDED(result = m_connection->Receive(1000, d)) && d != '#')
	;
    if (FAILED(result))
    {
	// Couldn't find manufacturer ID
	Error(ERROR_BADID);
	return UPG_E_BADID;
    }
    ReadHex(1, 4, &m_manufacturer);

    // Product ID: Look for '#' then 4 hex digits
    while(SUCCEEDED(result = m_connection->Receive(1000, d)) && d != '#')
	;
    if (FAILED(result))
    {
    	// Couldn't find product ID
	Error(ERROR_BADID);
	return UPG_E_BADID;
    }
    ReadHex(1, 4, &m_product);

    // Wait for prompt
    while(SUCCEEDED(result = m_connection->Receive(1000, d)) && d != '?')
	;
    if (FAILED(result))
    {
   	// Couldn't find prompt
	Error(ERROR_BADPROMPT);
	return UPG_E_BADPROMPT;
    }

    // In case more than one ^A got through we'd better wait until everything goes quiet

    while (SUCCEEDED(result = m_connection->Receive(1000, d)))
        ;

    // All OK
    return S_OK;
}

/** Flash an area in the unit's flash ROM: erase as necessary
 */
STATUS Upgrader::FlashArea(int address, int length)
{
    // Start with nothing...
    Status(UPGRADE_ERASE,0,length);

    // Erase area in question
    int a=address,d=0;
    do {
	// Unlock page if necessary
	if (m_product==PRODUCT_C3_8M) {
	    // Unlock the page
	    UnlockPage(a);
	}

	// Erase this page
	if (FAILED(ErasePage(a))) {
	    // Problem with erasing
	    Error(ERROR_BADERASE);
	    return UPG_E_BADERASE;
	}

	// Move to next page (pages below 64k boundary are 8k, above are 64k)
	if (a<0x10000) {
	    a+=8192; d+=8192;
	} else {
	    a+=65536; d+=65536;
	}

	// Update status
	Status(UPGRADE_ERASE,d,length);
    } while(d<length);

    // Program this area
    int done=0;
    unsigned int chunk;
    Status(UPGRADE_PROGRAM,0,length);
    int p=address;
    while(done<length) {
	// Get chunk size: we can program a max of 16k at a time
	chunk=((length-done)>16384)?16384:(length-done);

	// Read it from file
	if (fread(m_buffer,1,chunk,m_file)!=chunk) {
	    // Problem reading from file
	    Error(ERROR_BADFLASHFILE);
	    return UPG_E_PREMATUREEOF;
	}

	// Must be an even number of bytes: doesn't matter if the
	// last one is junk
	if (chunk&1) chunk++;

	// Program it
	if ((d=ProgramBlock(p,chunk))!=0) {
	    // Problem programming
	    Error(ERROR_BADPROGRAM);
	    return UPG_E_BADPROGRAM;
	}

	// Move on to next block
	done+=chunk;
	p+=chunk;

	// Show status
	Status(UPGRADE_PROGRAM,done,length);
    }

    // Lock pages if we have a C3 (the prom code will do lock-down)
    if (m_product==PRODUCT_C3_8M) {
	a=address; d=0;
	do {
	    // Lock the page
	    LockPage(a);

	    // Move to next page (pages below 64k boundary are 8k, above are 64k)
	    if (a<0x10000) {
		a+=8192;
		d+=8192;
	    } else {
		a+=65536;
		d+=65536;
	    }
	} while(d<length);
    }

    return S_OK;
}

/** Read a 32 bit little-endian unsigned long from the file.
 */
STATUS Upgrader::ReadUL(unsigned long *ul)
{
    unsigned long b = 0;

    for(int a = 0; a < 4; a++)
	b = (b >> 8) | (fgetc(m_file) << 24);

    *ul = b;
    return S_OK;
}

STATUS Upgrader::ReadL(long *l)
{
    return ReadUL(reinterpret_cast<unsigned long *>(l));
}

/** Verify the upgrade file to make sure it's kosher.
 * Tell the observer about any text we find in it.
 */
STATUS Upgrader::CheckUpgrade(const char *fn)
{
    // Open file
    if ((m_file=fopen(fn,"rb"))==NULL) {
	Error(ERROR_NOFILE);
	return E_NOTFOUND;
    }

    // First, do a complete scan of the file building the CRC
    fseek(m_file,0L,SEEK_END);
    long filelength = ftell(m_file);
    fseek(m_file,0L,SEEK_SET);

    // Read claimed length
    long claimedlength;
    ReadL(&claimedlength);

    // Short file?
    if (filelength<(claimedlength+8)) {
	Error(ERROR_SHORTFILE);
	return UPG_E_SHORTFILE;
    }

    // Tell user what we're doing
    Status(UPGRADE_CHECKFILE,0,claimedlength);

    // Calculate CRC of entire file
    unsigned long filecrc=0;
    unsigned int chunk;
    long a=0;
    while(a<claimedlength) {
	// Deal with it a chunk at a time
	chunk=((claimedlength-a)>16384)?16384:(claimedlength-a);
	if (fread(m_buffer,1,chunk,m_file)!=chunk) {
	    return UPG_E_PREMATUREEOF;
	}

	// CRC this block
	filecrc=CRC::CRC32(filecrc,m_buffer,chunk);

	// Next block
	a+=chunk;
	Status(UPGRADE_CHECKFILE,a,claimedlength);
    }

    // See if CRC matches
    unsigned long theircrc;
    ReadUL(&theircrc);
    if (filecrc!=theircrc) {
	Error(ERROR_BADFILE);
	return UPG_E_BADCRC;
    }

    // Dump any previously loaded text
    DumpText();

    // File is good: deal with chunks in upgrade file
    long currentpos=4;
    unsigned long blocktype,blocklength;
    char **text;
    long upgraderversion=-1;
    while(currentpos<claimedlength && currentpos!=0) {
	// Seek to correct position in case last op overrun
	fseek(m_file,currentpos,SEEK_SET);

	// Read blocktype & length
	ReadUL(&blocktype);
	ReadUL(&blocklength);

	text=NULL;
	switch(m_section=blocktype) {
	case CHUNK_UPGRADERVERSION:
	    ReadL(&upgraderversion);
	    break;

	case CHUNK_INFO:
	    text=&m_info;
	    // fall through
	case CHUNK_WHAT:
	    if (!text) text=&m_what;
	    // fall through
	case CHUNK_RELEASE:
	    if (!text) text=&m_release;
	    // fall through
	case CHUNK_VERSION:
	    if (!text) text=&m_version;

	    // Load this text
	    if (((*text)=(char*)malloc(blocklength+1))!=NULL) {
		if (fread((*text),1,blocklength,m_file)==blocklength) {
		    (*text)[blocklength] = 0;
		} else {
		    free(*text);
		    (*text)=NULL;
		}
	    }
	    break;

	default:
	    break;
	}
	currentpos+=8+blocklength;
    }

    // Right version?
    if (upgraderversion > UPGRADERVERSION) {
	Error(ERROR_OLDUPGRADER);
	return UPG_E_OLDUPGRADER;
    }

    // Tell observer about the text we've found
    if (m_observer)
	m_observer->LoadedText(m_info,m_what,m_release,m_version);

    // Done file
    Status(UPGRADE_CHECKEDFILE,1,1);

    // Keep track of filename for later
    m_filename=strdup(fn);

    // Close file
    fclose(m_file);
    m_file=NULL;

    return S_OK;
}

/** @todo Unify this loop, the one in CheckUpgrade, and the one in
 *        pump/upgrade-player.cpp
 */
STATUS Upgrader::DoUpgrade(Connection *p)
{
    m_section = 0;

    m_connection = p;

    // Open file
    if (!m_filename || (m_file=fopen(m_filename,"rb"))==NULL) {
	Error(ERROR_NOFILE);
	return E_NOTFOUND;
    }

    // Read claimed length
    long claimedlength;
    ReadL(&claimedlength);

    // Open connection to unit
    if (FAILED(m_connection->Open())) {
	Error(ERROR_NOUNIT);
	fclose(m_file);
	return UPG_E_NOUNIT;
    }

    // Now restart the player if it is running
    {
	ProtocolClient client(m_connection);
        // We sometimes don't get through the first time on serial so we try twice
	if (client.IsUnitConnected() || client.IsUnitConnected())
	    client.RestartUnit(false, false); // We don't want it to wait for completion 'cos we might miss the bootloader.
    }

    if (FAILED(StartFlash()))
    {
	Status(UPGRADE_ERRORS);
	m_connection->Close();
	m_connection = NULL;
	fclose(m_file);
	return UPG_E_STARTFLASH;
    }

    m_connection->Send('i');

    // If this player groks 'i' you'll get i\nXY ...
    // otherwise                           eh\n?

    int hwrev;
    int ch;
    m_connection->Receive(1000, ch);
    if (ch == 'i')
    {
	char idbuf[5];

	for ( int i=0; i<4; i++ )
	{
	    m_connection->Receive(500, ch);
	    idbuf[i] = ch;
	}
	idbuf[4] = '\0';  // CR NL x y \0

	sscanf( idbuf+2, "%x", &hwrev );
    }
    else
	hwrev = 1; // Actually covers 1-6, i.e. all Mark 1s (plus Marvin, the Mark 2 prototype)

    // File is good: deal with chunks in upgrade file
    long currentpos=4;
    unsigned long blocktype,blocklength,start;
    int firstflash=0,firstpump=1;
    char partition[16];
    while(currentpos<claimedlength && currentpos!=0)
    {
	// Seek to correct position in case last operation overran
	fseek(m_file,currentpos,SEEK_SET);
	// Read blocktype & length
	ReadUL(&blocktype);
	ReadUL(&blocklength);

	m_section = blocktype;
	switch(blocktype)
	{
	case CHUNK_UPGRADERVERSION:
	case CHUNK_INFO:
	case CHUNK_WHAT:
	case CHUNK_RELEASE:
	    break;

	case CHUNK_HWREVS:
	    {
		bool ok = false; // Assume not ours unless we explicitly match
		char *text = NEW char[blocklength+1];
		if ( fread( text, 1, blocklength, m_file ) == blocklength )
		{
		    text[blocklength] = 0;

		    char *token = strtok(text, "\n"); // NOT THREAD SAFE, notice...

		    while ( token )
		    {
			int this_rev;
			sscanf( token, "%x", &this_rev );
			if ( this_rev == hwrev )
			{
			    ok = true;
			    break; // the while not the switch
			}
			token = strtok(NULL, "\n");
		    }

		    if ( !ok )
		    {
			Error(ERROR_HWREV);
			currentpos = 0; // abort! abort!
		    }
		}
		else
		{
		    Error(ERROR_BADFILE);
		    currentpos = 0;
		}
		delete[] text;
	    }
	    break;

	case CHUNK_FLASHLOADER:
	case CHUNK_FLASHKERNEL:
	case CHUNK_FLASHRAMDISK:
	case CHUNK_FLASHRANDOM:
	    // Deal with flash programming block: read start address
	    ReadUL(&start);

	    // First one?
	    if (firstflash) {
		firstflash=0;
		if (FAILED(StartFlash())) {
		    currentpos=0;
		    break;
		}
	    }

	    // Do the program
	    if (FAILED(FlashArea(start,blocklength-4))) {
		// Problems: abort!
		currentpos=0;
	    }

	    break;

	case CHUNK_PUMPHDA:
	case CHUNK_PUMPHDB:
	case CHUNK_PUMPHDC:
	    // First one?
	    if (firstpump)
	    {
		firstpump=0;

		// Have we flashed? If so, we'll need to send an 'r' to force a restart
		if (!firstflash)
		    m_connection->Send('r');

		// Wait for pump to come up
		if (FAILED(StartPump()))
		{
		    currentpos=0;
		    break;
		}
	    }

	    // Build partition name
	    strcpy(partition,(blocktype==CHUNK_PUMPHDA)?"/dev/hda":
	    ((blocktype==CHUNK_PUMPHDB)?"/dev/hdb":"/dev/hdc"));

	    // Do the pump
	    if (Partition(partition)) {
		currentpos=0;
	    }

	    break;

	case CHUNK_PUMPHDA1:
	case CHUNK_PUMPHDA2:
	case CHUNK_PUMPHDA3:
	case CHUNK_PUMPHDA4:
	case CHUNK_PUMPHDA5:
	case CHUNK_PUMPHDA6:
	case CHUNK_PUMPHDA7:
	case CHUNK_PUMPHDA8:
	case CHUNK_PUMPHDB1:
	case CHUNK_PUMPHDB2:
	case CHUNK_PUMPHDB3:
	case CHUNK_PUMPHDB4:
	case CHUNK_PUMPHDB5:
	case CHUNK_PUMPHDB6:
	case CHUNK_PUMPHDB7:
	case CHUNK_PUMPHDB8:
	case CHUNK_PUMPHDC1:
	case CHUNK_PUMPHDC2:
	case CHUNK_PUMPHDC3:
	case CHUNK_PUMPHDC4:
	case CHUNK_PUMPHDC5:
	case CHUNK_PUMPHDC6:
	case CHUNK_PUMPHDC7:
	case CHUNK_PUMPHDC8:
	    // First one?
	    if (firstpump)
	    {
		firstpump=0;

		// Have we flashed? If so, we'll need to send an 'r' to force a restart
		if (!firstflash) m_connection->Send('r');

		// Wait for pump to come up
		if (FAILED(StartPump()))
		{
		    currentpos=0;
		    break;
		}

	    }

	    // Form filename
	    if (blocktype>=CHUNK_PUMPHDA1 && blocktype<=CHUNK_PUMPHDA8) {
		sprintf(partition,"/dev/hda%ld",(blocktype-CHUNK_PUMPHDA));
	    } else if (blocktype>=CHUNK_PUMPHDB1 && blocktype<=CHUNK_PUMPHDB8) {
		sprintf(partition,"/dev/hdb%ld",(blocktype-CHUNK_PUMPHDB));
	    } else {
		sprintf(partition,"/dev/hdc%ld",(blocktype-CHUNK_PUMPHDC));
	    }

	    // Do the pump
	    int result = PumpPartition(partition, blocklength);
	    if (result) {
		currentpos=0;
	    }

	    break;


		}

		if (currentpos) {
		    // Move current position on
		    currentpos+=8+blocklength;
		}
	}

	fclose(m_file);
	m_file=NULL;

	// Reboot as necessary
	if (!firstpump) {
	    // We've pumped, so we're still in pumper: force a reboot
	    m_connection->Send((unsigned char*)"reboot\r",7);
	} else if (!firstflash) {
	    // We've not pumped, but we have flashed: force a reboot
	    m_connection->Send('r');
	}

	// All done (currentpos==0 indicates upgrade process was aborted)
	if (!currentpos) {
	    // Something bad happened...
	    Status(UPGRADE_ERRORS);
	    m_connection->Close();
	    m_connection = NULL;
	    return UPG_E_SOMETHINGBAD;
	}

	// All ok
	Status(UPGRADE_DONE);

	// It won't be valid after we return anyway.
	m_connection->Close();
	m_connection = NULL;
	
	return S_OK;
}

void Upgrader::SetObserver(UpgradeObserver *o)
{
    // Save this observer: we call it when we have status to show
    m_observer=o;
}

void Upgrader::DumpText()
{
    // Dispose of the 4 text segments
    if (m_info) {
	free(m_info);
	m_info=NULL;
    }
    if (m_what) {
	free(m_what);
	m_what=NULL;
    }
    if (m_release) {
	free(m_release);
	m_release=NULL;
    }
    if (m_version) {
	free(m_version);
	m_version=NULL;
    }

    // Dump filename
    if (m_filename) {
	free(m_filename);
	m_filename=NULL;
    }
}
