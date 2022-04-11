/* empeg_exception.cpp
 *
 * Important exception classes
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */

#include "empeg_exception.h"
#include "var_string.h"

string_exception::string_exception(const std::string & desc)
    : m_desc(desc)
{
}

const char *string_exception::what() const throw()
{
    return m_desc.c_str();
}

status_exception::status_exception(const std::string & desc, STATUS lastError)
    : m_desc(VarString::Printf( "%s: error 0x%x\n", desc.c_str(),
				PrintableStatus(lastError) )),
      m_lastError(lastError)
{
}

status_exception::status_exception(STATUS lastError)
    : m_desc(VarString::Printf( "Error 0x%x", PrintableStatus(lastError) )),
      m_lastError(lastError)
{
}

const char *status_exception::what() const throw()
{
    return m_desc.c_str();
}
