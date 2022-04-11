/* wildcard.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 */

#ifndef WILDCARD_H
#define WILDCARD_H

bool strwild(const char *text, const char *pattern, bool caseless = false);
inline bool strcasewild(const char *text, const char *pattern) {
    return strwild(text, pattern, true);
}

#endif
