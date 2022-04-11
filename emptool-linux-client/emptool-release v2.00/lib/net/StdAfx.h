/* stdafx.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.8 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 */

#if !defined(AFX_STDAFX_H__F6AF3553_2769_11D4_9C1D_0050DA687855__INCLUDED_)
#define AFX_STDAFX_H__F6AF3553_2769_11D4_9C1D_0050DA687855__INCLUDED_

#ifdef WIN32

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include "config.h"
#include "trace.h"
#endif

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_STDAFX_H__F6AF3553_2769_11D4_9C1D_0050DA687855__INCLUDED_)
