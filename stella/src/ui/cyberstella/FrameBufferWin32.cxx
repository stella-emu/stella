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
// $Id: FrameBufferWin32.cxx,v 1.4 2003-11-16 19:32:52 stephena Exp $
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
   :  m_piDD( NULL ),
      m_piDDSPrimary( NULL ),
      m_piDDSBack( NULL ),
      m_piDDPalette( NULL ),
      m_fActiveWindow( TRUE ),
      theZoomLevel(1),
      theMaxZoomLevel(1),
      isFullscreen(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferWin32::~FrameBufferWin32()
{
  cleanup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::cleanup()
{
  if(m_piDDPalette)
  {
    m_piDDPalette->Release();
    m_piDDPalette = NULL;
  }

  if ( m_piDDSBack )
  {
    m_piDDSBack->Release();
    m_piDDSBack = NULL;
  }

  if ( m_piDD )
  {
    if ( m_piDDSPrimary )
    {
      m_piDDSPrimary->Release();
      m_piDDSPrimary = NULL;
    }

    m_piDD->Release();
    m_piDD = NULL;
  }

	if(myHWND)
	{
    ::DestroyWindow( myHWND );

        //
        // Remove the WM_QUIT which will be in the message queue
        // so that the main window doesn't exit
        //

        MSG msg;
        ::PeekMessage( &msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE );

        myHWND = NULL;
	}

    ::UnregisterClass( pszClassName, GetModuleHandle(NULL));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferWin32::init()
{
  HRESULT hrCoInit = ::CoInitialize( NULL );

  // Get the game's width and height
  mySizeGame.cx = myMediaSource->width() << 1;
  mySizeGame.cy = myMediaSource->height();

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

  if( ! ::RegisterClassEx( &wcex ) )
  {
OutputDebugString("got here  failed 1");
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
	if( myHWND == NULL )
	{
OutputDebugString("got here  failed 2");
    hr = HRESULT_FROM_WIN32( GetLastError() );
    cleanup();
    return false;
	}

  ::SetFocus( myHWND );
  ::ShowWindow( myHWND, SW_SHOW );
  ::UpdateWindow( myHWND );
	
  ::ShowCursor( FALSE );

  //
  // Initialize DirectDraw 
  //
  hr = ::CoCreateInstance( CLSID_DirectDraw, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_IDirectDraw, 
                           (void**)&m_piDD );
  if( FAILED(hr) )
  {
OutputDebugString("got here  failed 3");
    cleanup();
  }

  //
  // Initialize it
  // This method takes the driver GUID parameter that the DirectDrawCreate 
  // function typically uses (NULL is active display driver)
  //
  hr = m_piDD->Initialize( NULL );
  if ( FAILED(hr) )
  {
OutputDebugString("got here  failed 4");
    cleanup();
    return false;
  }

  //
  // Get the best video mode for game width
  //
//int cx = 640; int cy = 480;
int cx = 320; int cy = 240;
    ::SetRect( &m_rectScreen, 0, 0, cx, cy );

    if ( cx == 0 || cy == 0 )
    {
    	hr = m_piDD->EnumDisplayModes( 0, NULL, this, EnumModesCallback );
        if ( FAILED(hr) )
        {
OutputDebugString("got here  failed 5");
cleanup();
return false;
        }
    }

	if ( m_rectScreen.right == 0 || m_rectScreen.bottom == 0 )
	{
OutputDebugString("got here  failed 6");
        hr = E_INVALIDARG;
cleanup();
return false;
	}

	TRACE( "Video Mode Selected: %d x %d", m_rectScreen.right, m_rectScreen.bottom );

	// compute blit offset to center image

	m_ptBlitOffset.x = ( ( m_rectScreen.right - mySizeGame.cx ) / 2 );
	m_ptBlitOffset.y = ( ( m_rectScreen.bottom - mySizeGame.cy ) / 2 );

	// Set cooperative level

	hr = m_piDD->SetCooperativeLevel( myHWND, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
	if ( FAILED(hr) )
	{
OutputDebugString("got here  failed 7");
cleanup();
return false;
	}

	hr = m_piDD->SetDisplayMode( m_rectScreen.right, m_rectScreen.bottom, 8 );
	if ( FAILED(hr) )
	{
OutputDebugString("got here  failed 8");
cleanup();
return false;
	}

    //
	// Create the primary surface
    //

  DDSURFACEDESC ddsd;
  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize         = sizeof(ddsd);
  ddsd.dwFlags        = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = m_piDD->CreateSurface(&ddsd, &m_piDDSPrimary, NULL);
	if (FAILED(hr))
	{
OutputDebugString("got here  failed 9");
cleanup();
return false;
	}

    //
	// Create the offscreen surface
    //

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth  = mySizeGame.cx;
	ddsd.dwHeight = mySizeGame.cy;
	hr = m_piDD->CreateSurface(&ddsd, &m_piDDSBack, NULL);
	if (FAILED(hr))
	{
OutputDebugString("got here  failed 10");
cleanup();
return false;
	}

    //
	// Erase the surface
    //

    HDC hdc;
	hr = m_piDDSBack->GetDC( &hdc );
	if ( hr == DD_OK )
	{
		::SetBkColor( hdc, RGB(0, 0, 0) );
        RECT rc;
        ::SetRect( &rc, 0, 0, 
                   mySizeGame.cx,
                   mySizeGame.cy );
		::ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );

		(void)m_piDDSBack->ReleaseDC( hdc );
	}

    //
    // Create Palette
    //

	PALETTEENTRY pe[256];

	for ( int i = 0; i < 256; ++i )
	{
		pe[i].peRed   = (BYTE)( (pPalette[i] & 0x00FF0000) >> 16 );
		pe[i].peGreen = (BYTE)( (pPalette[i] & 0x0000FF00) >> 8 );
		pe[i].peBlue  = (BYTE)( (pPalette[i] & 0x000000FF) );
		pe[i].peFlags = 0;
	}

    hr = m_piDD->CreatePalette( DDPCAPS_8BIT, 
                                pe, 
                                &m_piDDPalette, 
                                NULL );
  if( FAILED(hr) )
  {
OutputDebugString("got here  failed 11");
    cleanup();
    return false;
  }

  hr = m_piDDSPrimary->SetPalette( m_piDDPalette );
  if( FAILED(hr) )
  {
OutputDebugString("got here  failed 12");
    cleanup();
    return false;
  }

  // If we get this far, then assume that there were no problems
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawMediaSource()
{
  if(m_piDDSPrimary == NULL || !m_fActiveWindow)
    return;

  HRESULT hr;
  const BYTE* current  = myMediaSource->currentFrameBuffer();
  const BYTE* previous = myMediaSource->previousFrameBuffer();

  // acquire pointer to linear video ram
  DDSURFACEDESC ddsd;
  ZeroMemory( &ddsd, sizeof(ddsd) );
  ddsd.dwSize = sizeof(ddsd);

  hr = m_piDDSBack->Lock( NULL, 
                          &ddsd, 
                          /* DDLOCK_SURFACEMEMORYPTR | */ DDLOCK_WAIT, 
                          NULL );
  // BUGBUG: Check for error

  BYTE* pbBackBytes = (BYTE*)ddsd.lpSurface;

  register int y;
  for(y = 0; y < mySizeGame.cy; ++y)
  {
    const WORD bufofsY = (WORD) ( y * mySizeGame.cx >> 1 );
    const DWORD screenofsY = ( y * ddsd.lPitch );

    register int x;
    for(x = 0; x < mySizeGame.cx >> 1; ++x )
    {
      const WORD bufofs = bufofsY + x;
      BYTE v = current[ bufofs ];
      if(v == previous[ bufofs ])
        continue;

      // x << 1 is times 2 ( doubling width ) WIDTH_FACTOR
      const DWORD pos = screenofsY + ( x << 1 );
      pbBackBytes[ pos + 0 ] = pbBackBytes[ pos + 1 ] = myPalette[v];
    }
  }

  (void)m_piDDSBack->Unlock( ddsd.lpSurface );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::postFrameUpdate()
{
  // We only send any changes to the screen once per frame

  // Blit offscreen to onscreen
  RECT rc = { 0, 0, mySizeGame.cx, mySizeGame.cy };

  HRESULT hr = m_piDDSPrimary->BltFast( m_ptBlitOffset.x, m_ptBlitOffset.y,
                                m_piDDSBack, 
                                &rc, 
                                // DDBLTFAST_WAIT |DDBLTFAST_DESTCOLORKEY   );
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawText(uInt32 xorig, uInt32 yorig, const string& message)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferWin32::drawChar(uInt32 xorig, uInt32 yorig, uInt32 c)
{
}

LRESULT CALLBACK FrameBufferWin32::StaticWindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam
    )
{
	FrameBufferWin32* pThis;

    if ( uMsg == WM_CREATE )
    {
        pThis = reinterpret_cast<FrameBufferWin32*>( 
            reinterpret_cast<CREATESTRUCT*>( lParam )->lpCreateParams );

        ::SetWindowLong( hwnd, 
                         GWL_USERDATA, 
                         reinterpret_cast<LONG>( pThis ) );
    }
    else
    {
        pThis = reinterpret_cast<FrameBufferWin32*>( 
            ::GetWindowLong( hwnd, GWL_USERDATA ) );
    }

    if ( pThis )
    {
        if ( pThis->WndProc( uMsg, wParam, lParam ) )
        {
            //
            // Handled message
            //

            return 0L;
        }
    }

    //
    // Unhandled message
    //

    return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
}


BOOL FrameBufferWin32::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_ACTIVATE:
      m_fActiveWindow = (wParam != WA_INACTIVE);
      break;

    case WM_DESTROY:
      ::PostQuitMessage(0);
      break;

    case WM_KEYDOWN:
      switch((int)wParam)
      {
        case VK_ESCAPE:
          ::PostMessage(myHWND, WM_CLOSE, 0, 0);

          // For some braindead reason, the exit event must be handled
          // here.  Normally, an Escape event would be the same as any
          // other and the Console would handle it.
          // But since Windows insists on doing it this way, we have
          // to make sure that the Escape event is still received by
          // the core.
          myConsole->eventHandler().sendEvent(Event::Quit, 1);
          break;
      }
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

HRESULT WINAPI FrameBufferWin32::EnumModesCallback(
    LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext)
{
  FrameBufferWin32* pThis = (FrameBufferWin32*)lpContext;

  DWORD dwWidthReq  = pThis->mySizeGame.cx;
  DWORD dwHeightReq = pThis->mySizeGame.cy;

  DWORD dwWidth = lpDDSurfaceDesc->dwWidth;
  DWORD dwHeight = lpDDSurfaceDesc->dwHeight;
  DWORD dwRGBBitCount = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

    //
    // must be 8 bit mode
    //

	if (dwRGBBitCount != 8)
    {
        return DDENUMRET_OK;
    }

    //
    // must be larger then required screen size
    //

    if ( dwWidth < dwWidthReq || dwHeight < dwHeightReq )
    {
        return DDENUMRET_OK;
    }

    if ( pThis->m_rectScreen.right != 0 && pThis->m_rectScreen.bottom != 0 )
    {
        //
        // check to see if this is better than the previous choice
        //

        if ( (dwWidth - dwWidthReq) > (pThis->m_rectScreen.right - dwWidthReq) )
        {
            return DDENUMRET_OK;
        }

        if ( (dwHeight - dwHeightReq) > (pThis->m_rectScreen.bottom - dwHeightReq) )
        {
            return DDENUMRET_OK;
        }
    }

    //
    // use it!
    //

    pThis->m_rectScreen.right = dwWidth;
    pThis->m_rectScreen.bottom = dwHeight;

	return DDENUMRET_OK;
}
