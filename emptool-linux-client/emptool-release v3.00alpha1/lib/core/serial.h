/* serial.h
 *
 * Lock files for serial ports (on Unix)
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#ifndef included_serial_h
#define included_serial_h

#include <string>

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

#ifndef ECOS

class PortLock
{
 public:
    PortLock( const char *port );
    ~PortLock();

    STATUS Lock();
    void Unlock();

 protected:
    bool gotLock;
    std::string lockFileName;
};

#else

class PortLock
{
 public:
    PortLock( const char * ) { }
    ~PortLock() { }

    STATUS Lock() { return S_OK; }
    void Unlock() { }
};

#endif // ndef ECOS

#endif
