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
// $Id: pch.hxx,v 1.3 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef PCH_HXX
#define PCH_HXX

#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 5

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <mmsystem.h>
#include <commdlg.h>
#include <commctrl.h>

#include <dinput.h>

#include "debug.hxx"

// ---------------------------------------------------------------------------
// Conditional defines

// Macros
//

#ifdef _DEBUG
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif
#define UNUSED_ALWAYS(x) x

// Utility methods
//

void MessageBox( HINSTANCE hInstance, HWND hwndParent, UINT uIDText );

void MessageBoxFromWinError( DWORD dwError, LPCTSTR pszCaption /* = NULL */ );

void MessageBoxFromGetLastError( LPCTSTR pszCaption /* = NULL */ );

#endif
