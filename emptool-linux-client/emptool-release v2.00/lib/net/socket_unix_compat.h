/* socket_unix_compat.h
 *
 * All the declarations to make Unix socket handling look like Win32 (pardon?)
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#ifndef SOCKET_UNIX_COMPAT_H
#define SOCKET_UNIX_COMPAT_H

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include "empeg_error.h"
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int ERROR_TYPE;
inline int closesocket(int s) { return close(s); }
inline ERROR_TYPE WSAGetLastError() { return errno; }
inline STATUS MakeWin32Status(int e) { return MakeErrnoStatus(e); }
#define ioctlsocket(A, X...) ioctl(A,X)
#define _alloca(X) alloca(X)

#endif
