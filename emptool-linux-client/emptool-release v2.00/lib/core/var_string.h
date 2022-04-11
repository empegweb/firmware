/* var_string.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#ifndef VAR_STRING_H
#define VAR_STRING_H

#include <stdarg.h>	// for va_list
#include <string>	// for std::string
#include "config.h"

namespace VarString
{
    std::string vPrintf(const char *fmt, va_list ap);
    std::string Printf(const char *fmt, ...) ATTRIBUTE((__format__(printf, 1, 2)));
};

#endif
