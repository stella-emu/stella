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
// $Id: RoundButton.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef ROUND_BUTTON_HXX
#define ROUND_BUTTON_HXX

#include "Wnd.hxx"

class CRoundButton : public CWnd
{
  public:
    CRoundButton();
    ~CRoundButton();

  protected:
    LRESULT WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    virtual void PreSubclassWindow(HWND hwnd);

  private:
    void OnDrawItem(HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT lpdis);

    HRGN m_hrgn;
    POINT m_ptCenter;
    POINT m_ptLeft;
    POINT m_ptRight;
    int m_nRadius;
    BOOL m_fDrawDashedFocusCircle;
    BOOL m_fStretch;

    CRoundButton( const CRoundButton& );    // no implementation
    void operator=( const CRoundButton& );  // no implementation
};

#endif
