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
// $Id: CoolCaption.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef COOL_CAPTION_HXX
#define COOL_CAPTION_HXX

class CCoolCaption
{
  public:
    CCoolCaption();

    void OnInitDialog(HWND hDlg);
    void OnDestroy();

    void OnNcPaint(HRGN);
    void OnNcActivate(BOOL);
    BOOL OnNCLButtonDown(INT, POINTS);

  protected:
    HWND m_hDlg;

  private:
    void CalculateNCArea(void);
    void FillSolidRect(HDC hdc, int x, int y, int cx, int cy, COLORREF clr);
    void Draw3dRect(HDC hdc, int x, int y, int cx, int cy, 
	                  COLORREF clrTopLeft, COLORREF clrBottomRight);

    BOOL m_fIsActive;
    RECT m_rcWindow;
    int m_cxWindow;
    int m_cyWindow;
    int m_cxFrame;
    int m_cyFrame;
    int m_cxButtonSize;
    RECT m_rcClose;
    RECT m_rcMin;
    RECT m_rcCaption;
    RECT m_rcTextArea;
    HFONT m_hfont;
    LPTSTR m_tszCaption;

    CCoolCaption( const CCoolCaption& );    // no implementation
    void operator=( const CCoolCaption& );  // no implementation
};

#endif
