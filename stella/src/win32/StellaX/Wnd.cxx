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
// $Id: Wnd.cxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "Wnd.hxx"

// REVIEW: Need to reset old wnd proc?

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CWnd::CWnd()
    : m_pOldWindowProc(NULL),
      m_hwnd(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CWnd::SubclassDlgItem( HWND hwndParent, UINT nID )
{
  if ( nID == 0 || hwndParent == NULL )
    return FALSE;

  // can't subclass twice!
  if ( m_pOldWindowProc != NULL )
    return FALSE;

  m_hwnd = GetDlgItem( hwndParent, nID );
  if (m_hwnd == NULL)
  {
    ASSERT( FALSE );
    return FALSE;
  }

  PreSubclassWindow( m_hwnd );

  m_pOldWindowProc = (WNDPROC)GetWindowLong( m_hwnd, GWL_WNDPROC );
  SetWindowLong( m_hwnd, GWL_USERDATA, (LONG)this ); 
  SetWindowLong( m_hwnd, GWL_WNDPROC, (LONG)StaticWndProc );

  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CWnd::PreSubclassWindow( HWND hwnd )
{
  // no default behavior
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
LRESULT CALLBACK CWnd::StaticWndProc( HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam )
{
  CWnd* pThis = (CWnd*)GetWindowLong(hWnd, GWL_USERDATA);

  BOOL fHandled = FALSE;
  LRESULT lRes = pThis->WndProc( hWnd, msg, wParam, lParam, fHandled );

  if (fHandled)
    return lRes;

  return CallWindowProc( pThis->m_pOldWindowProc, hWnd, msg, wParam, lParam );
}
