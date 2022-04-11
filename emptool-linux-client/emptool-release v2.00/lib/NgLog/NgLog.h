/* NgLog.h
 *
 * (C) 1999-2000 Roger Lipscombe
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release  01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#ifndef NGLOG_H_
#define NGLOG_H_

#ifndef _INC_STDARG
#include <stdarg.h>
#include <stddef.h> // size_t
#endif

#ifdef WIN32
  #ifdef NGLOG_EXPORTS
    #define NGLOG_API __declspec(dllexport)
  #else
    #define NGLOG_API __declspec(dllimport)
  #endif
#else
  #define NGLOG_API
#endif

class NGLOG_API CNgLog
{
public:
    enum LogLevel {
	// other values are reserved
	logDebug    = 4,
	logInfo	    = 3,
	logWarning  = 2,
	logError    = 1,
	logFatal    = 0,
    };
    
public:
    CNgLog(const char *component, const char *sourceFile, int sourceLine, LogLevel logLevel);
    ~CNgLog();
    void operator() (const char *pszFormat, ...) const;
    static void SetFile(const char *fileName);
    static bool IsFile();
    static void SetDbWin(bool b);
    static bool IsDbWin();
    
protected:
    void Log(const char *messageFormat, ...) const;
    bool ShouldLog() const;
    
private:
    void LogV(const char *messageFormat, va_list list) const;
    void Format(const char *messageFormat, va_list list,
	char **ppMessageContents, size_t *pMessageContentsLength) const;
    
    const char *m_component;
    const char *m_sourceFile;
    int      m_sourceLine;
    LogLevel m_logLevel;
};

class NGLOG_API CNgLogHex : public CNgLog
{
public:
    CNgLogHex(const char *component, const char *sourceFile, int sourceLine, LogLevel logLevel)
	: CNgLog(component, sourceFile, sourceLine, logLevel) {  }
    
    void operator() (const char *pBuffer, unsigned cbBuffer);
    void operator() (const void *pBuffer, unsigned cbBuffer);
    
private:
    void OutputHexLine(const char *pBuffer, unsigned cbBuffer);
};

#define LOG_SETFILE(f)	    \
    CNgLog::SetFile(f)

#define LOG_ISFILE	    \
    CNgLog::IsFile

#define LOG_SETDBWIN(b)	    \
    CNgLog::SetDbWin(b)

#define LOG_ISDBWIN	    \
    CNgLog::IsDbWin

// If LOG_RESTRICT is defined, we output only info, warn, error and fatal messages...
// We're also a little more coy about component and file names.

// These all used to be 'const CNgLog' but that's non-standard (the type name
// in a constructor expression must be a simple-type-specifier not a
// type-specifier, and so can't have const). We could const_cast them but 
// life's too short.

#ifdef __GNUC__
    /* GCC isn't smart enough to magic away the strings in release builds 
     * unless we do something much less clever.
     */
    #ifndef TRACE_H
        #include "trace.h"
    #endif
    #define LOG_COMPONENT(x) static const int nglog_trace_tag = TRACE_ ## x;
    #define LOG_FATAL(A, B...) ERROR(A "\n" , ## B)
    #define LOG_ERROR(A, B...) ERROR(A "\n" , ## B)
    #define LOG_WARNING(A, B...) WARN(A "\n" , ## B)
    #define LOG_WARN(A, B...) WARN(A "\n" , ## B)
    #define LOG_INFO(A, B...) TRACEC(nglog_trace_tag, A "\n" , ## B)
    #define LOG_ENTER(A, B...) TRACEC(nglog_trace_tag, A "\n" , ## B)
    #define LOG_LEAVE(A, B...) TRACEC(nglog_trace_tag, A "\n" , ## B)
    #define LOG_DEBUG(A, B...) TRACEC(nglog_trace_tag, A "\n" , ## B)
    #define LOG_INFO_HEX(a,b) /* NYI */
    #define LOG_DEBUG_HEX(a,b) /* NYI */
#else /* !__GCC__ */
  #ifdef LOG_RESTRICT
    #define LOG_COMPONENT(c)

    #define LOG_FATAL	    \
	CNgLog(NULL, NULL, 0, CNgLog::logFatal)

    #define LOG_ERROR	    \
	CNgLog(NULL, NULL, 0, CNgLog::logError)

    #define LOG_WARNING	    \
	CNgLog(NULL, NULL, 0, CNgLog::logWarning)
    #define LOG_WARN LOG_WARNING

    #define LOG_INFO	    \
	CNgLog(NULL, NULL, 0, CNgLog::logInfo)

    #define LOG_ENTER   \
	CNgLog(NULL, NULL, 0, CNgLog::logInfo)

    #define LOG_LEAVE   \
	CNgLog(NULL, NULL, 0, CNgLog::logInfo)

    #define LOG_DEBUG	    \
	(void)

    #define LOG_INFO_HEX		    \
	CNgLogHex(NULL, NULL, 0, CNgLog::logInfo)

    #define LOG_DEBUG_HEX		    \
	(void)

  #else	// Output the full monty...
    #define LOG_COMPONENT(c)    \
	static char ngLog_Component[] = #c;

    #define LOG_FATAL	    \
	CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logFatal)

    #define LOG_ERROR	    \
	 CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logError)

    #define LOG_WARNING	    \
	CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logWarning)
    #define LOG_WARN LOG_WARNING

    #define LOG_INFO	    \
	CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logInfo)

    #define LOG_ENTER   \
	CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logInfo)

    #define LOG_LEAVE   \
	CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logInfo)

    #define LOG_DEBUG	    \
	CNgLog(ngLog_Component, __FILE__, __LINE__, CNgLog::logDebug)

    #define LOG_INFO_HEX		    \
	CNgLogHex(ngLog_Component, __FILE__, __LINE__, CNgLog::logInfo)

    #define LOG_DEBUG_HEX		    \
	CNgLogHex(ngLog_Component, __FILE__, __LINE__, CNgLog::logDebug)
  #endif
#endif /* !__GCC__ */

#endif /* NGLOG_H_ */

