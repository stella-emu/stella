//============================================================================
//
//   SSSS    tt          lll  lll          XX     XX
//  SS  SS   tt           ll   ll           XX   XX
//  SS     tttttt  eeee   ll   ll   aaaa     XX XX
//   SSSS    tt   ee  ee  ll   ll      aa     XXX
//      SS   tt   eeeeee  ll   ll   aaaaa    XX XX
//  SS  SS   tt   ee      ll   ll  aa  aa   XX   XX
//   SSSS     ttt  eeeee llll llll  aaaaa  XX     XX
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: debug.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef _AFX
#define _AFX

#include <windows.h>
#include <tchar.h>

// ---------------------------------------------------------------------------
// This file defines:
// _countof
// TRACE, TRACE0, TRACE1, TRACE2, TRACE3
// ASSERT
// VERIFY
// DEBUG_ONLY


// determine number of elements in an array (not bytes)
#define _countof(array) (sizeof(array)/sizeof(array[0]))

// AFXAPI is used on global public functions
#ifndef AFXAPI
	#define AFXAPI __stdcall
#endif

// AFX_CDECL is used for rare functions taking variable arguments
#ifndef AFX_CDECL
	#define AFX_CDECL __cdecl
#endif


#ifdef _DEBUG

// ---------------------------------------------------------------------------
// DEBUG DEFINED

#include <tchar.h>

#include <stdio.h>
#include <stdarg.h>

#include <crtdbg.h>

#ifndef AfxDebugBreak
#define AfxDebugBreak() _CrtDbgBreak()
#endif

BOOL AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine, LPCTSTR lpszCondition);

void AFX_CDECL AfxTrace(LPCTSTR lpszFormat, ...);

#define TRACE              ::AfxTrace
#define THIS_FILE          __FILE__
#define ASSERT(f) \
	do \
	{ \
	if (!(f) && AfxAssertFailedLine(THIS_FILE, __LINE__, #f)) \
		AfxDebugBreak(); \
	} while (0) \

#define VERIFY(f)          ASSERT(f)
#define DEBUG_ONLY(f)      (f)

// The following trace macros are provided for backward compatiblity
//  (they also take a fixed number of parameters which provides
//   some amount of extra error checking)
#define TRACE0(sz)              ::AfxTrace(_T("%s"), _T(sz))
#define TRACE1(sz, p1)          ::AfxTrace(_T(sz), p1)
#define TRACE2(sz, p1, p2)      ::AfxTrace(_T(sz), p1, p2)
#define TRACE3(sz, p1, p2, p3)  ::AfxTrace(_T(sz), p1, p2, p3)

#else   // _DEBUG

// ---------------------------------------------------------------------------
// DEBUG NOT DEFINED

#ifdef AfxDebugBreak
#undef AfxDebugBreak
#endif
#define AfxDebugBreak()

#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)(f))
#define DEBUG_ONLY(f)      ((void)0)
inline void AFX_CDECL AfxTrace(LPCTSTR, ...) { }
#define TRACE              1 ? (void)0 : ::AfxTrace
#define TRACE0(sz)
#define TRACE1(sz, p1)
#define TRACE2(sz, p1, p2)
#define TRACE3(sz, p1, p2, p3)

#endif // !_DEBUG

#endif
