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
// $Id: HeaderCtrl.cxx,v 1.3 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "HeaderCtrl.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CHeaderCtrl::CHeaderCtrl()
            : m_nSortCol(0),
              m_fSortAsc(TRUE)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
LRESULT CHeaderCtrl::WndProc( HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam, BOOL& rfHandled )
{  
  switch ( msg )
  {
    case WM_DRAWITEM:
      rfHandled = TRUE;
      OnDrawItem(hWnd, (UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
      return TRUE;
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CHeaderCtrl::SetSortCol( int nCol, BOOL bAsc )
{
  m_nSortCol = nCol;
  m_fSortAsc = bAsc;

  // change this item to owner draw
  HWND hwndHeader = ::GetDlgItem( *this, 0 );
  HDITEM hdi;
  hdi.mask = HDI_FORMAT;
  Header_GetItem(hwndHeader, nCol, &hdi);
  hdi.fmt |= HDF_OWNERDRAW;
  Header_SetItem(hwndHeader, nCol, &hdi);
	
  // repaint the header
  InvalidateRect(hwndHeader, NULL, TRUE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CHeaderCtrl::OnDrawItem( HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT lpdis )
{
  UNUSED_ALWAYS( idCtl );
  HDC hdc = lpdis->hDC;
  RECT rcLabel;

  CopyRect( &rcLabel, &(lpdis->rcItem) );
	
  /* save the DC */
  int nSavedDC = ::SaveDC( hdc );
	
  /* set clip region to column */
  HRGN hrgn = ::CreateRectRgnIndirect( &rcLabel );
  SelectObject( hdc, hrgn );
  DeleteObject( hrgn );
	
  /* draw the background */
  FillRect( hdc, &rcLabel, ::GetSysColorBrush(COLOR_3DFACE) );
	
  /* offset the label */
  SIZE size;
  GetTextExtentPoint32( hdc, _T(" "), 1, &size );
  int nOffset = size.cx * 2;
	
  /* get the column text and format */
  TCHAR tszText[255 + 1];
  HDITEM hdi;
  hdi.mask = HDI_TEXT | HDI_FORMAT;
  hdi.pszText = tszText;
  hdi.cchTextMax = 255;
  Header_GetItem( GetDlgItem(hwnd, 0), lpdis->itemID, &hdi );
	
  /* determine format for drawing label */
  UINT uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS;
	
  /* determine justification */
  if (hdi.fmt & HDF_CENTER)
    uFormat |= DT_CENTER;
  else if (hdi.fmt & HDF_RIGHT)
    uFormat |= DT_RIGHT;
  else
    uFormat |= DT_LEFT;
	
  /* adjust the rect if selected */
  if (lpdis->itemState & ODS_SELECTED)
  {
    rcLabel.left++;
    rcLabel.top += 2;
    rcLabel.right++;
  }
	
  /* adjust rect for sort arrow */
  if ( lpdis->itemID == m_nSortCol )
    rcLabel.right -= (3 * nOffset);

  rcLabel.left += nOffset;
  rcLabel.right -= nOffset;
	
  /* draw label */
  if ( rcLabel.left < rcLabel.right )
    DrawText(hdc, tszText, -1, &rcLabel, uFormat );
	
  /* draw the arrow */
  if ( lpdis->itemID == m_nSortCol )
  {
    RECT rcIcon;
    HPEN hpenLight, hpenShadow, hpenOld;
		
    CopyRect( &rcIcon, &(lpdis->rcItem) );
		
    hpenLight = ::CreatePen( PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT) );
    hpenShadow = ::CreatePen( PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW) );
    hpenOld = (HPEN)::SelectObject( hdc, hpenLight );
		
    if (m_fSortAsc)
    {
      /* draw triangle pointing up */
      MoveToEx( hdc, rcIcon.right - 2 * nOffset, nOffset - 1, NULL );
      LineTo( hdc, rcIcon.right - 3 * nOffset / 2, rcIcon.bottom - nOffset );
      LineTo( hdc, rcIcon.right - 5 * nOffset / 2 - 2, rcIcon.bottom - nOffset );
      MoveToEx( hdc, rcIcon.right - 5 * nOffset / 2 - 1, rcIcon.bottom - nOffset, NULL );

      SelectObject( hdc, hpenShadow );
      LineTo( hdc, rcIcon.right - 2 * nOffset, nOffset - 2 );
    }
    else
    {
      /* draw triangle pointing down */
      MoveToEx( hdc, rcIcon.right - 3 * nOffset / 2, nOffset - 1, NULL );
      LineTo( hdc, rcIcon.right - 2 * nOffset - 1, rcIcon.bottom - nOffset );
      LineTo( hdc, rcIcon.right - 2 * nOffset - 1, rcIcon.bottom - nOffset );
      MoveToEx( hdc, rcIcon.right - 2 * nOffset - 1, rcIcon.bottom - nOffset, NULL );
			
      SelectObject( hdc, hpenShadow );
      LineTo( hdc, rcIcon.right - 5 * nOffset / 2 - 1, nOffset - 1 );
      LineTo( hdc, rcIcon.right - 3 * nOffset / 2, nOffset - 1 );
    }
		
    SelectObject( hdc, hpenOld );
    DeleteObject( hpenShadow );
    DeleteObject( hpenLight );
  }

  RestoreDC( hdc, nSavedDC );
}
