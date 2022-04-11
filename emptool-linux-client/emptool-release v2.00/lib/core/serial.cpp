/* serial.cpp
 *
 * Lock files for serial ports (on Unix)
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#include "config.h"
#include "trace.h"
#include "serial.h"

#ifndef WIN32

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

// This is correct for Linux; your unix may vary
#define LOCKDIR "/var/lock"

PortLock::PortLock( const char *port ): gotLock(false)
{
    if ( !strncmp( port, "/dev/", 5 ) )
	port += 5;
    lockFileName = std::string( LOCKDIR "/LCK..") + port;
}

PortLock::~PortLock()
{
    if ( gotLock )
	Unlock();
}

STATUS PortLock::Lock()
{
    if ( gotLock )
	return S_OK;

    struct stat st;

    if ( stat( LOCKDIR, &st ) == 0 )
    {
	char tempfile[] = LOCKDIR "/empeg-XXXXXX"; // not const!

	int fd = mkstemp( tempfile );

	if ( fd >= 0 )
	{
	    char contents[80];
	    sprintf( contents, "%10d empeg\n", getpid() );

	    write(fd, contents, strlen(contents));
	    close(fd);
	    
	    int rc = link( tempfile, lockFileName.c_str() );

	    if ( rc == 0 )
	    {
		// If the link succeeded, the lock file didn't previously
		// exist, so we must now have the lock

		gotLock = true;
	    }
	    else
	    {
		// Sometimes (on NFS) the link call works but returns failure.
		// In this case we did actually get the lock. We can test for
		// this by seeing whether the tempfile now has two links.

		stat( tempfile, &st );
		gotLock = ( st.st_nlink == 2 );
	    }

	    unlink( tempfile );

	    if ( gotLock )
	    {
		chown( lockFileName.c_str(), (uid_t)getuid(),
		       (gid_t)getgid() );
		chmod( lockFileName.c_str(), 0644 );
	    }
	    else
		return E_MUTEX_LOCKFAILED;
	}
	else
	{
	    // Couldn't create the lockfile; we probably don't have write
	    // access to LOCKDIR. We succeed or fail according to whether
	    // there's an existing lockfile, but we cannot ourselves lock
	    // the port against subsequent openers.

	    if ( stat( lockFileName.c_str(), &st ) < 0 )
	    {
		gotLock = true;
	    }
	    else
	    {
		gotLock = false;
		return E_MUTEX_LOCKFAILED;
	    }
	}
    }
    else
    {
	// No lock dir at all -- all we can do is assume we've got it
	gotLock = true;
    }

    return gotLock ? S_OK : E_MUTEX_LOCKFAILED;
}

void PortLock::Unlock()
{
    if ( gotLock )
	unlink( lockFileName.c_str() );
    gotLock = false;
}

#endif

#ifdef TEST

int main( int argc, char *argv[] )
{
    if ( argc != 2 )
    {
	printf( "usage: serial <device>\n" );
	return 1;
    }

    PortLock pl( argv[1] );

    printf( "Locking %s\n", argv[1] );

    if ( SUCCEEDED( pl.Lock() ) )
	printf( "Got lock\n" );
    else
	printf( "Didn't get lock, %s (%d)\n", strerror(errno), errno );

    return 0;
}

#endif

/* eof */
