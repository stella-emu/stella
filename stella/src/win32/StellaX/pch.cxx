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
// $Id: pch.cxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "pch.hxx"
#include <stdio.h>
#include <stdarg.h>

#include "resource.h"

// Bring in the common control library
#pragma comment( lib, "comctl32" )

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MessageBox( HINSTANCE hInstance, HWND hwndParent, UINT uIDText )
{
  const int nMaxStrLen = 1024;
  TCHAR tszCaption[nMaxStrLen + 1] = { 0 };
  TCHAR tszText[nMaxStrLen + 1] = { 0 };

  // Caption is always "StellaX"
  LoadString(hInstance, IDS_STELLA, tszCaption, nMaxStrLen);

  LoadString(hInstance, uIDText, tszText, nMaxStrLen);
  if (hwndParent == NULL)
    hwndParent = ::GetForegroundWindow();

  ::MessageBox(hwndParent, tszText, tszCaption, MB_ICONWARNING | MB_OK);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MessageBoxFromWinError( DWORD dwError, LPCTSTR pszCaption /* = NULL */ )
{
  const int nMaxStrLen = 1024;
  TCHAR pszCaptionStellaX[nMaxStrLen + 1];

  if ( pszCaption == NULL )
  {
    // LoadString(hInstance, IDS_STELLA, tszCaption, nMaxStrLen);
    lstrcpy( pszCaptionStellaX, _T("StellaX") );
  }

  LPTSTR pszText = NULL;

  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPTSTR)&pszText, 0, NULL);

  ::MessageBox(::GetForegroundWindow(), pszText, 
               pszCaption ? pszCaption : pszCaptionStellaX,
               MB_ICONWARNING | MB_OK);

  LocalFree( pszText );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MessageBoxFromGetLastError( LPCTSTR pszCaption /* = NULL */ )
{
  MessageBoxFromWinError( GetLastError(), pszCaption );
}
