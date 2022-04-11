/* empeg_wchar.h
 *
 * Wide character stuff for Ecos (which doesn't have wchar.h)
 *
 * (C) 2002 Sonicblue Inc
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef EMPEG_WCHAR_H
#define EMPEG_WCHAR_H

#define WCHAR_MIN (-2147483647l - 1l)
#define WCHAR_MAX 0x7FFFFFFF
#ifdef __cplusplus
#include <string>
namespace std {
    typedef basic_string<wchar_t> wstring;
};
#endif

#endif
