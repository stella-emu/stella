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
// $Id: TextButton3d.cxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "TextButton3d.hxx"

// button style should be VISIBLE / DISABLED / OWNERDRAW

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CTextButton3d::CTextButton3d()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
LRESULT CTextButton3d::WndProc( HWND hWnd, UINT msg, WPARAM wParam, 
                                LPARAM lParam, BOOL& rfHandled )
{
  switch (msg)
  {
#if 0
    // TODO: re do this now that I send this message here
    case WM_DRAWITEM:
      rfHandled = TRUE;
      pThis->OnDrawItem(hWnd, (UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
      return TRUE;
#endif

    case WM_PAINT:
      rfHandled = TRUE;
      OnPaint(hWnd, (HDC)wParam);
      return TRUE;

    case WM_ERASEBKGND:
      rfHandled = TRUE;
      return OnEraseBkgnd(hWnd, (HDC)wParam);
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CTextButton3d::OnPaint( HWND hwnd, HDC hdcPaint )
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);

  int cxClient, cyClient;

  RECT rc;
  GetClientRect(hwnd, &rc);

  RECT rcClient;
  CopyRect(&rcClient, &rc);

  // lpdis->rcItem, lpdis->itemState

  int l = GetWindowTextLength(hwnd);
  LPTSTR text = new TCHAR[l + 2];
  if (text == NULL)
  {
    EndPaint(hwnd, &ps);
    return;
  }
  GetWindowText(hwnd, text, l+1);

  HFONT hfont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
  LOGFONT lf;
  GetObject(hfont, sizeof(lf), &lf);
  if (lf.lfHeight == 0)
    lf.lfHeight = 20;
  lf.lfWidth = 0;
  lf.lfWeight = FW_BLACK;
  lf.lfEscapement = 0;
  lf.lfOrientation = 0;
  HFONT hfontTry = CreateFontIndirect(&lf);

  HFONT hfontOld = (HFONT)SelectObject(hdc, hfontTry);

  rcClient.left += 3;
  rcClient.top += 3;
  rcClient.right -= 3;
  rcClient.bottom -= 3;

  cxClient = rcClient.right - rcClient.left;
  cyClient = rcClient.bottom - rcClient.top;

  SIZE sizeTextClient;
  GetTextExtentPoint(hdc, text, lstrlen(text), &sizeTextClient);
  if (cxClient*sizeTextClient.cy > cyClient*sizeTextClient.cx)
    lf.lfHeight = MulDiv(lf.lfHeight, cyClient, sizeTextClient.cy);
  else
    lf.lfHeight = MulDiv(lf.lfHeight, cxClient, sizeTextClient.cx);

  lf.lfHeight--;

  rcClient.left -= 3;
  rcClient.top -= 3;
  rcClient.right += 3;
  rcClient.bottom += 3;

  cxClient = rcClient.right - rcClient.left;
  cyClient = rcClient.bottom - rcClient.top;

  hfont = CreateFontIndirect(&lf);
  SelectObject(hdc, hfont);
  GetTextExtentPoint(hdc, text, lstrlen(text), &sizeTextClient);

  int minx = rcClient.left + (cxClient - sizeTextClient.cx) / 2;
  int miny = rcClient.top + (cyClient - sizeTextClient.cy) / 2;

  int iOldBkMode = SetBkMode(hdc, TRANSPARENT);
  COLORREF crText = GetSysColor(COLOR_BTNTEXT);
  COLORREF crOldText = SetTextColor(hdc, crText);

  int cx = minx;
  int cy = miny;

  int s = 1;
  cx += 3;
  cy += 3;

  // draw 3D highlights
  SetTextColor(hdc, GetSysColor(COLOR_3DDKSHADOW));
  TextOut(hdc, cx-s*2, cy+s*2, text, lstrlen(text));
  TextOut(hdc, cx+s*2, cy-s*2, text, lstrlen(text));
  TextOut(hdc, cx+s*2, cy+s*2, text, lstrlen(text));
  SetTextColor(hdc, ::GetSysColor(COLOR_3DHILIGHT));
  TextOut(hdc, cx+s*1, cy-s*2, text, lstrlen(text));
  TextOut(hdc, cx-s*2, cy+s*1, text, lstrlen(text));
  TextOut(hdc, cx-s*2, cy-s*2, text, lstrlen(text));
  SetTextColor(hdc, GetSysColor(COLOR_3DSHADOW));
  TextOut(hdc, cx-s*1, cy+s*1, text, lstrlen(text));
  TextOut(hdc, cx+s*1, cy-s*1, text, lstrlen(text));
  TextOut(hdc, cx+s*1, cy+s*1, text, lstrlen(text));
  SetTextColor(hdc, GetSysColor(COLOR_3DLIGHT));
  TextOut(hdc, cx, cy-s*1, text, lstrlen(text));
  TextOut(hdc, cx-s*1, cy, text, lstrlen(text));
  TextOut(hdc, cx-s*1, cy-s*1, text, lstrlen(text));
  SetTextColor(hdc, crText);

  // draw the text
  TextOut(hdc, cx, cy, text, lstrlen(text));

  // restore dc
  SetTextColor(hdc, crOldText);
  SetBkMode(hdc, iOldBkMode);
  SelectObject(hdc, hfontOld);

  DeleteObject(hfont);
  DeleteObject(hfontTry);

  delete[] text;

  EndPaint(hwnd, &ps);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CTextButton3d::OnEraseBkgnd( HWND hwnd, HDC hdc )
{
  // dont do any erasing
  return TRUE;
}
