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
// $Id: HeaderCtrl.hxx,v 1.3 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef HEADER_CTRL_HXX
#define HEADER_CTRL_HXX

#include "Wnd.hxx"

class CHeaderCtrl : public CWnd
{
  public:
    CHeaderCtrl();

    void SetSortCol(int col, BOOL fAscending);
    int GetSortCol(void) const;
    BOOL GetSortAsc(void) const;

  protected:
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL& rfHandled);

  private:
    void OnDrawItem(HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT lpdis);

    UINT m_nSortCol;
    BOOL m_fSortAsc;

    CHeaderCtrl( const CHeaderCtrl& );     // no implementation
    void operator=( const CHeaderCtrl& );  // no implementation
};

inline int CHeaderCtrl::GetSortCol( void ) const
{
  return m_nSortCol;
}

inline BOOL CHeaderCtrl::GetSortAsc( void ) const
{
  return m_fSortAsc;
}

#endif
