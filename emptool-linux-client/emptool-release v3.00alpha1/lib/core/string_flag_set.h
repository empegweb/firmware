/* string_flag_set.h
 *
 * Access to a semicolon separated set of string flags.
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.1 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Mike Crowe <mac@empeg.com>
 *
 */

#ifndef STRING_FLAG_SET_H
#define STRING_FLAG_SET_H 1

#include "utf8.h"

class StringFlagSet
{
    utf8_string m_flag_set;

public:
    StringFlagSet(const utf8_string &flag_set);

    bool IsPresent(const utf8_string &flag_name) const;

    void Set(const utf8_string &flag_name);
    void Clear(const utf8_string &flag_name);

    utf8_string GetFlagSet() const;
};

#endif // STRING_FLAG_SET