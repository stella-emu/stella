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
// $Id: Wnd.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef WND_HXX
#define WND_HXX

class CWnd
{
  public:
    CWnd();

    BOOL SubclassDlgItem( HWND hwndParent, UINT nID );

    operator HWND() const { return m_hwnd; }

  protected:
    virtual void PreSubclassWindow( HWND hwnd );
    virtual LRESULT WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL& rfHandled ) = 0;

  private:
    static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

    WNDPROC m_pOldWindowProc;
    HWND m_hwnd;

    CWnd( const CWnd& );            // no implementation
    void operator=( const CWnd& );  // no implementation
};

#endif
