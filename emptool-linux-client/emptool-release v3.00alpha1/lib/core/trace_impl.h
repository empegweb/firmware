/* trace_impl.h
 *
 * Debug tracing implementation shared stuff
 *
 * (C) 2000-2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#ifndef TRACE_IMPL_H
#define TRACE_IMPL_H 1

#ifdef WIN32
// Make it match MSVC's regexp for click-to-go-to-source-file
#define DEBUG_TRACE_FORMAT "%s(%d) : [%ld] : "
#define RELEASE_TRACE_FORMAT "%s(%d) : "
#else
#define DEBUG_TRACE_FORMAT "%-18s:%4d (%ld): "
#define RELEASE_TRACE_FORMAT "%-18s:%4d:"
#endif
#define ERROR_PREFIX "! "
#define WARN_PREFIX "W "
#define ASSERT_PREFIX "A "
#define TRACE_PREFIX "  "

#endif // TRACE_IMPL_H
