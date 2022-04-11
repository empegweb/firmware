/* minmax.h
 *
 * Some versions of C++ STL don't contain these standard functions
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#ifndef MINMAX_H
#define MINMAX_H

#define EMPEG_NEED_MINMAX

#ifdef __GNUC__
//#if (__GNUC__ > 2) || ( (__GNUC__ == 2) && (__GNUC_MINOR__ > 94) )
#include <algorithm>
#undef EMPEG_NEED_MINMAX
//#endif
#endif

#ifdef EMPEG_NEED_MINMAX

template<class T>
const T &min(const T &a, const T &b)
{
    return (a < b) ? a : b;
}

template<class T>
const T &max(const T &a, const T &b)
{
    return (a < b) ? b : a;
}

#endif

#endif

