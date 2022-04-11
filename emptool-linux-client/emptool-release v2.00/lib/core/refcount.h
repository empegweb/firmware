/* refcount.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 */

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

#if defined(WIN32)
#include "refcount_win32.h"
#else
#include "refcount_posix.h"
#endif
