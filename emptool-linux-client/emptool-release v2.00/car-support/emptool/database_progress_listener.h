/* database_progress_listener.h
 *
 * Object that logs and shows progress during database operations.
 *
 * (C) 1999-2000 empeg ltd.
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 01-Apr-2003 18:52 rob:)
 */

#ifndef DATABASE_PROGRESS_LISTENER_H
#define DATABASE_PROGRESS_LISTENER_H
    
class DatabaseProgressListener
{
 public:
    // Major operations (download, synchronise, etc...)
    virtual void OperationStart(int code, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4))) = 0;	// 3 including 'this'
    virtual void OperationUpdate(int progress, int total) throw() = 0;
    
    // Sub-operations/tasks (send file, receive tags, etc...)
    virtual void TaskStart(int code, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4))) = 0;	// 3 including 'this'
    virtual void TaskUpdate(int progress, int total) throw() = 0;

    // Logging of debug, information and errors during operations
    virtual void Debug(int level, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4))) = 0;	// 3 including 'this'
    virtual void Log(int code, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4))) = 0;	// 3 including 'this'
    virtual void Error(int code, const char *fmt, ...) throw()
	__attribute__ ((format (printf, 3, 4))) = 0;	// 3 including 'this'

    virtual ~DatabaseProgressListener() { }
};

#endif
