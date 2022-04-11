/* utf8.h
 *
 * Constants for Unicode characters encoded as UTF-8.
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 */

#ifndef UTF8_CHARS_H
#define UTF8_CHARS_H 1

// What would be _really_ cool here is a macro that manages to convert
// Latin-1 to UTF-8 at compile time. However, such magic is probably
// beyond the preprocessor I suspect. :(

#define UTF8_COPYRIGHT "\xc2\xa9"

#endif // UTF8_CHARS_H
