//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferWin32.cxx,v 1.6 2003-11-24 01:14:38 stephena Exp $
//============================================================================

#include <sstream>

#include "pch.hxx"
#include "resource.h"

#include "bspf.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "StellaEvent.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferWin32.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"

LPCTSTR FrameBufferWin32::pszClassName = _T("StellaXClass");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferWin32::FrameBufferWin32()
   :  myDD(NULL),
      myPrimarySurface(NULL),
      myBackSurface(NULL),
      myDDPalette(NULL),
      theZoomLevel(1),
      theMaxZoomLevel(1),
      isFullscreen(false),
      isWindowActive(true)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferWin32::~FrameBufferWin32()
{
  cleanup();

  if(myHWND)
  {
    DestroyWindow( myHWND );

    // Remove the WM_QUIT which will be in the message queue
    // so that the main window doesn't exit
    MSG msg;
    PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);

    myHWND = NULL;
  }

  UnregisterClass(pszClassName, GetModuleHandle(NULL));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::cleanup()
{
  if(myDDPalette)
  {
    myDDPalette->Release();
    myDDPalette = NULL;
  }

  if(myBackSurface)
  {
    myBackSurface->Release();
    myBackSurface = NULL;
  }

  if(myDD)
  {
    if(myPrimarySurface)
    {
      myPrimarySurface->Release();
      myPrimarySurface = NULL;
    }

    myDD->Release();
    myDD = NULL;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferWin32::init()
{
  HRESULT hrCoInit = ::CoInitialize( NULL );

  // Get the game's width and height
  myGameSize.cx = myWidth  = myMediaSource->width() << 1;
  myGameSize.cy = myHeight = myMediaSource->height();

  // Initialize the pixel data table
  for(uInt32 i = 0; i < 256; ++i)
    myPalette[i] = i | (i << 8);

  HRESULT hr = S_OK;
  const unsigned int* pPalette = myMediaSource->palette();

	WNDCLASSEX wcex;
	ZeroMemory( &wcex, sizeof(wcex) );
	wcex.cbSize        = sizeof( wcex );
	wcex.hInstance     = GetModuleHandle(NULL);
	wcex.lpszClassName = pszClassName;
	wcex.lpfnWndProc   = StaticWindowProc;
	wcex.style         = CS_OWNDC;
	wcex.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
	wcex.hIconSm       = LoadIcon( NULL, IDI_WINLOGO );
	wcex.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );

  if(!RegisterClassEx(&wcex))
  {
    OutputDebugString("Error:  RegisterClassEX FAILED");
    hr = HRESULT_FROM_WIN32( ::GetLastError() );
    cleanup();
    return false;
  }

  myHWND = CreateWindowEx( WS_EX_TOPMOST, 
                           pszClassName, 
                           _T("StellaX"), 
		                       WS_VISIBLE | WS_POPUP, 
		                       0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		                       NULL, 
                           NULL, 
                           GetModuleHandle(NULL), 
                           this );
  if(myHWND == NULL )
  {
    OutputDebugString("Error:  CreateWindowEx FAILED");
    hr = HRESULT_FROM_WIN32( GetLastError() );
    cleanup();
    return false;
  }

  ::SetFocus(myHWND);
  ::ShowWindow(myHWND, SW_SHOW);
  ::UpdateWindow(myHWND);
	
  ::ShowCursor( FALSE );

  // Initialize DirectDraw 
  //
  hr = ::CoCreateInstance( CLSID_DirectDraw, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_IDirectDraw, 
                           (void**)&myDD );
  if(FAILED(hr))
  {
    OutputDebugString("Error:  CoCreateInstance FAILED");
    cleanup();
    return false;
  }

  // Initialize it
  // This method takes the driver GUID parameter that the DirectDrawCreate 
  // function typically uses (NULL is active display driver)
  hr = myDD->Initialize(NULL);
  if(FAILED(hr))
  {
    OutputDebugString("Error:  DirectDraw Initialize FAILED");
    cleanup();
    return false;
  }

  // Get the best video mode for game width
  //int cx = 640; int cy = 480;
  int cx = 320; int cy = 240;
  SetRect(&myScreenRect, 0, 0, cx, cy);

  if(cx == 0 || cy == 0)
  {
    hr = myDD->EnumDisplayModes(0, NULL, this, EnumModesCallback);
    if(FAILED(hr))
    {
      OutputDebugString("Error:  Displaymode Enumeration FAILED");
      cleanup();
      return false;
    }
  }

  if(myScreenRect.right == 0 || myScreenRect.bottom == 0)
  {
    OutputDebugString("Error:  ScreenRect INVALID");
    cleanup();
    return false;
  }

  // compute blit offset to center image
  myBlitOffset.x = ((myScreenRect.right - myGameSize.cx) / 2);
  myBlitOffset.y = ((myScreenRect.bottom - myGameSize.cy) / 2);

  // Set cooperative level
  hr = myDD->SetCooperativeLevel(myHWND, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
  if(FAILED(hr))
  {
    OutputDebugString("Error:  SetCooperativeLevel FAILED");
    cleanup();
    return false;
  }

  hr = myDD->SetDisplayMode(myScreenRect.right, myScreenRect.bottom, 8);
  if(FAILED(hr))
  {
    OutputDebugString("Error:  SetDisplayMode FAILED");
    cleanup();
    return false;
  }

  // Create the primary surface
  DDSURFACEDESC ddsd;
  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize         = sizeof(ddsd);
  ddsd.dwFlags        = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

  hr = myDD->CreateSurface(&ddsd, &myPrimarySurface, NULL);
  if(FAILED(hr))
  {
    OutputDebugString("Error:  Create primary surface FAILED");
    cleanup();
    return false;
  }

  // Create the offscreen surface
  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize         = sizeof(ddsd);
  ddsd.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  ddsd.dwWidth        = myGameSize.cx;
  ddsd.dwHeight       = myGameSize.cy;

  hr = myDD->CreateSurface(&ddsd, &myBackSurface, NULL);
  if(FAILED(hr))
  {
    OutputDebugString("Error:  Create back surface FAILED");
    cleanup();
    return false;
  }

  // Erase the surface
  HDC hdc;
  hr = myBackSurface->GetDC(&hdc);
  if(hr == DD_OK)
  {
    SetBkColor(hdc, RGB(0, 0, 0));
    RECT rc;
    SetRect(&rc, 0, 0, myGameSize.cx, myGameSize.cy);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    (void)myBackSurface->ReleaseDC(hdc);
  }

  // Create Palette
  PALETTEENTRY pe[256];
  for(uInt32 i = 0; i < 256; ++i)
  {
    pe[i].peRed   = (BYTE)( (pPalette[i] & 0x00FF0000) >> 16 );
    pe[i].peGreen = (BYTE)( (pPalette[i] & 0x0000FF00) >> 8 );
    pe[i].peBlue  = (BYTE)( (pPalette[i] & 0x000000FF) );
    pe[i].peFlags = 0;
  }

  hr = myDD->CreatePalette(DDPCAPS_8BIT, pe, &myDDPalette, NULL );
  if(FAILED(hr))
  {
    OutputDebugString("Error:  CreatePalette FAILED");
    cleanup();
    return false;
  }

  hr = myPrimarySurface->SetPalette(myDDPalette);
  if(FAILED(hr))
  {
    OutputDebugString("Error:  SetPalette FAILED");
    cleanup();
    return false;
  }

  // If we get this far, then assume that there were no problems
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawMediaSource()
{
  if(myPrimarySurface == NULL || !isWindowActive)
    return;

  const BYTE* current  = myMediaSource->currentFrameBuffer();
  const BYTE* previous = myMediaSource->previousFrameBuffer();
  BYTE* pbBackBytes = (BYTE*)myDDSurface.lpSurface;

  register int y;
  for(y = 0; y < myGameSize.cy; ++y)
  {
    const WORD bufofsY = (WORD) ( y * myGameSize.cx >> 1 );
    const DWORD screenofsY = ( y * myDDSurface.lPitch );

    register int x;
    for(x = 0; x < myGameSize.cx >> 1; ++x )
    {
      const WORD bufofs = bufofsY + x;
      BYTE v = current[ bufofs ];
      if(v == previous[ bufofs ] && !theRedrawEntireFrameIndicator)
        continue;

      // x << 1 is times 2 ( doubling width ) WIDTH_FACTOR
      const DWORD pos = screenofsY + ( x << 1 );
      pbBackBytes[ pos + 0 ] = pbBackBytes[ pos + 1 ] = myPalette[v];
    }
  }

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameIndicator = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::preFrameUpdate()
{
  // Acquire pointer to linear video ram
  ZeroMemory(&myDDSurface, sizeof(myDDSurface));
  myDDSurface.dwSize = sizeof(myDDSurface);

  HRESULT hr = myBackSurface->Lock(NULL, &myDDSurface, DDLOCK_WAIT, NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::postFrameUpdate()
{
  // We only send any changes to the screen once per frame
  (void)myBackSurface->Unlock(myDDSurface.lpSurface);

  // Blit offscreen to onscreen
  RECT rc = { 0, 0, myGameSize.cx, myGameSize.cy };

  HRESULT hr = myPrimarySurface->BltFast( myBlitOffset.x, myBlitOffset.y,
                                myBackSurface, 
                                &rc, 
                                DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::pauseEvent(bool status)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::toggleFullscreen()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  BYTE* pbBackBytes = (BYTE*)myDDSurface.lpSurface;

  // First draw the background
  for(uInt32 row = 0; row < h; row++)
  {
    for(uInt32 col = 0; col < w; col++)
    {
      BYTE* ptr = pbBackBytes + ((row + y) * myDDSurface.lPitch) + col + x;
      *ptr = myPalette[myBGColor];
    }
  }

  // Now draw the surrounding lines
  for(uInt32 col = 0; col < w+1; col++)  // Top line
  {
    BYTE* ptr = pbBackBytes + y * myDDSurface.lPitch + col + x;
    *ptr = myPalette[myFGColor];
  }

  for(uInt32 col = 0; col < w+1; col++)  // Bottom line
  {
    BYTE* ptr = pbBackBytes + (y+h) * myDDSurface.lPitch + col + x;
    *ptr = myPalette[myFGColor];
  }

  for(uInt32 row = 0; row < h; row++)  //  Left line
  {
    BYTE* ptr = pbBackBytes + (row + y) * myDDSurface.lPitch + x;
    *ptr = myPalette[myFGColor];
  }

  for(uInt32 row = 0; row < h; row++)  //  Right line
  {
    BYTE* ptr = pbBackBytes + (row + y) * myDDSurface.lPitch + x + w;
    *ptr = myPalette[myFGColor];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawText(uInt32 xorig, uInt32 yorig, const string& message)
{
  BYTE* pbBackBytes = (BYTE*)myDDSurface.lpSurface;
  uInt8 length = message.length();
  for(uInt32 z = 0; z < length; z++)
  {
    for(uInt32 y = 0; y < 8; y++)
    {
      for(uInt32 x = 0; x < 8; x++)
      {
        char letter = message[z];
        if((ourFontData[(letter << 3) + y] >> x) & 1)
          pbBackBytes[((z<<3) + x + xorig) + (y + yorig) * myDDSurface.lPitch] =
            myPalette[myFGColor];
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawChar(uInt32 xorig, uInt32 yorig, uInt32 c)
{
  BYTE* pbBackBytes = (BYTE*)myDDSurface.lpSurface;
  for(uInt32 y = 0; y < 8; y++)
  {
    for(uInt32 x = 0; x < 8; x++)
    {
      if((ourFontData[(c << 3) + y] >> x) & 1)
        pbBackBytes[x + xorig + (y + yorig) * myDDSurface.lPitch] =
          myPalette[myFGColor];
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LRESULT CALLBACK FrameBufferWin32::StaticWindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam)
{
  FrameBufferWin32* pThis;

  if(uMsg == WM_CREATE)
  {
    pThis = reinterpret_cast<FrameBufferWin32*>( 
            reinterpret_cast<CREATESTRUCT*>( lParam )->lpCreateParams );

    SetWindowLong(hwnd, GWL_USERDATA, reinterpret_cast<LONG>(pThis));
  }
  else
  {
    pThis = reinterpret_cast<FrameBufferWin32*>(GetWindowLong(hwnd, GWL_USERDATA));
  }

  if(pThis)
  {
    if(pThis->WndProc(uMsg, wParam, lParam))
    {
      // Handled message
      return 0L;
    }
  }

  // Unhandled message
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOL FrameBufferWin32::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_ACTIVATE:
      isWindowActive = (wParam != WA_INACTIVE);
      break;

    case WM_DESTROY:
      ::PostQuitMessage(0);
      break;

    case WM_PAINT:
      PAINTSTRUCT ps;
      ::BeginPaint( myHWND, &ps );
      ::EndPaint( myHWND, &ps );
      break;

    default:
      return FALSE;
  }

  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HRESULT WINAPI FrameBufferWin32::EnumModesCallback(
    LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext)
{
  FrameBufferWin32* pThis = (FrameBufferWin32*)lpContext;

  DWORD dwWidthReq  = pThis->myGameSize.cx;
  DWORD dwHeightReq = pThis->myGameSize.cy;

  DWORD dwWidth = lpDDSurfaceDesc->dwWidth;
  DWORD dwHeight = lpDDSurfaceDesc->dwHeight;
  DWORD dwRGBBitCount = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

  // Must be 8 bit mode
  if(dwRGBBitCount != 8)
    return DDENUMRET_OK;

  // Must be larger then required screen size
  if(dwWidth < dwWidthReq || dwHeight < dwHeightReq)
    return DDENUMRET_OK;

  if(pThis->myScreenRect.right != 0 && pThis->myScreenRect.bottom != 0)
  {
    // check to see if this is better than the previous choice
    if((dwWidth - dwWidthReq) > (pThis->myScreenRect.right - dwWidthReq))
      return DDENUMRET_OK;

    if((dwHeight - dwHeightReq) > (pThis->myScreenRect.bottom - dwHeightReq))
      return DDENUMRET_OK;
  }

  // use it!
  pThis->myScreenRect.right = dwWidth;
  pThis->myScreenRect.bottom = dwHeight;

  return DDENUMRET_OK;
}
