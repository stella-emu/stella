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
// $Id: debug.cxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "debug.hxx"

#ifdef _DEBUG   // entire file

// log file name (or NULL for no log) -- must be defined by client

extern LPCTSTR g_ctszDebugLog;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void AFX_CDECL AfxTrace( LPCTSTR lpszFormat, ... )
{
  va_list args;
  va_start(args, lpszFormat);

  int nBuf;
  TCHAR szBuffer[512];

  nBuf = _vsntprintf(szBuffer, _countof(szBuffer), lpszFormat, args);

  // was there an error? was the expanded string too long?
  ASSERT(nBuf >= 0);

  if (nBuf < 511)
    lstrcat(szBuffer, _T("\r\n"));

  _RPT0(_CRT_WARN, szBuffer);

  if (g_ctszDebugLog)
  {
    HANDLE hfile = CreateFile(g_ctszDebugLog, GENERIC_WRITE, 0, NULL,
      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile != INVALID_HANDLE_VALUE)
    {
      SetFilePointer(hfile, 0, NULL, FILE_END);

      DWORD dw;
      WriteFile(hfile, szBuffer, (lstrlen(szBuffer)+1)*sizeof(TCHAR), 
        &dw, NULL);
			
      CloseHandle(hfile);
    }
  }

  va_end(args);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL AFXAPI AfxAssertFailedLine( LPCSTR lpszFileName, int nLine, 
                                 LPCTSTR lpszCondition )
{
  // we remove WM_QUIT because if it is in the queue then the message box
  // won't display
  MSG msg;
  BOOL bQuit = PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
  BOOL bResult = _CrtDbgReport(_CRT_ASSERT, lpszFileName, nLine, NULL, lpszCondition);
  if (bQuit)
    PostQuitMessage(msg.wParam);
  return bResult;
}

#endif
