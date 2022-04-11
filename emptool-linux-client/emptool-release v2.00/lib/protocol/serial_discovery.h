/* serial_discovery.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * Authors:
 *   Peter Hartley <peter@empeg.com>
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#include "discovery.h"

#if defined(WIN32)
class Win32SerialEmpegDiscoverer : public EmpegDiscoverer
{
    int portNum;

public:
    Win32SerialEmpegDiscoverer(int portNum);
    virtual HRESULT Discover(unsigned timeout_ms, bool *pbContinue = NULL, bool *pbIgnoreFailure = NULL);
};
#endif /* defined(WIN32) */
