/* empeg_exception.h
 *
 * Important exception classes
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 01-Apr-2003 18:52 rob:)
 */

#ifndef EMPEG_EXCEPTION_H
#define EMPEG_EXCEPTION_H

#include <string>
#include <exception>

#ifndef EMPEG_ERROR_H
#include "empeg_error.h"
#endif

class string_exception : public std::exception
{
    std::string m_desc;
public:
    string_exception(const std::string & desc);
    ~string_exception() throw() {}
    virtual const char *what() const throw();
};

class status_exception : public std::exception
{
    std::string m_desc;
    STATUS m_lastError;
public:
    status_exception(STATUS lastError);
    status_exception(const std::string &desc, STATUS lastError);
    ~status_exception() throw() {}
    virtual const char *what() const throw();
};

#endif // !defined(EMPEG_EXCEPTION_H)
