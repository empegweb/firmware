/* logfile.h
 *
 * Logging for Win32
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.10 01-Apr-2003 18:52 rob:)
 */

#if !defined(AFX_LOGFILE_H__5209F143_20C6_11D3_BC0F_00104B62B7D4__INCLUDED_)
#define AFX_LOGFILE_H__5209F143_20C6_11D3_BC0F_00104B62B7D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>

class CLogFile  
{
    void Open();
    void Close();

 public:
    CLogFile();
    ~CLogFile();
    void Write(const char *fmt, ...);
    void EnterFunction(const char *fmt, ...);
    void LeaveFunction(const char *fmt, ...);
    void Enable(bool b = true);
    bool IsEnabled() const;
#ifdef _AFX
    CString GetFilename() const;
#endif
    void DumpHex(const char *title, const void *p, int count);

 private:
    bool enabled;
    FILE *fp;
    int depth;
    void DoWrite(const char *title, const char *fmt, va_list va);
    void InternalDumpHex(const void *p, int count);
};

inline void CLogFile::Enable(bool b)
{
    enabled = b; 
    if (!enabled) 
	Close(); 
}

inline void CLogFile::EnterFunction(const char *fmt, ...)
{
    if (enabled)
    {
	va_list va;
	va_start(va, fmt);
	DoWrite("enter ", fmt, va);
	depth++;
	va_end(va);
    }
}

inline void CLogFile::LeaveFunction(const char *fmt, ...)
{
    if (enabled)
    {
	va_list va;
	va_start(va, fmt);
	depth--;
	DoWrite("leave ", fmt, va);
	va_end(va);
    }
}

inline void CLogFile::Write(const char *fmt, ...)
{
    if (enabled)
    {
	va_list va;
	va_start(va, fmt);
	DoWrite("* ", fmt, va);
	va_end(va);
    }
}

inline void CLogFile::DumpHex(const char *title, const void *p, int count)
{
    if (enabled)
    {
	Write("---- Hex dump (%d bytes) start: %s", count, title);
	InternalDumpHex(p, count);
	Write("---- Hex dump (%d bytes) end: %s", count, title);
    }
}

inline bool CLogFile::IsEnabled() const
{
    return enabled;
}

extern CLogFile log;

#endif // !defined(AFX_LOGFILE_H__5209F143_20C6_11D3_BC0F_00104B62B7D4__INCLUDED_)
