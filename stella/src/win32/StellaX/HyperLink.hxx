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
// $Id: HyperLink.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef HYPER_LINK_HXX
#define HYPER_LINK_HXX

#define MAX_HYPERLINK_TEXT_LEN 256
#define MAX_HYPERLINK_URL 256

#include "Wnd.hxx"

class CHyperLink : public CWnd
{
  public:
    CHyperLink();
    ~CHyperLink();

    void SetURL(LPCTSTR ctszURL);

  protected:
    virtual void PreSubclassWindow(HWND hwnd);
    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                            BOOL& rfHandled);

  private:
    void SetDefaultCursor();
    LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
    void ReportError(int nError);
    HINSTANCE GotoURL(LPCTSTR url, int showcmd);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, 
                                    WPARAM wParam, LPARAM lParam);

    HBRUSH OnCtlColorStatic(HWND hwnd, HDC hdcStatic, HWND hwndStatic);
    BOOL OnSetCursor(HWND hwnd, WORD nHittest, WORD wMouseMsg);
    LRESULT OnPaint(HWND hwnd);
    LRESULT OnLButtonUp(HWND, WPARAM, WORD, WORD);
    LRESULT OnMouseMove(HWND, WPARAM, WORD, WORD);

    // Stuff from the control
    TCHAR m_tszText[MAX_HYPERLINK_TEXT_LEN + 1];
    HFONT m_hFont;

    // link specific
    COLORREF m_crLinkColor;
    COLORREF m_crVisitedColor;
    COLORREF m_crHoverColor;

    BOOL m_bOverControl;
    BOOL m_bVisited;

    HCURSOR m_hLinkCursor;

    TCHAR m_tszURL[MAX_HYPERLINK_URL + 1];

    CHyperLink( const CHyperLink& );      // no implementation
    void operator=( const CHyperLink& );  // no implementation
};

#endif
