//
// StellaX
// Jeff Miller 05/15/2000
//
#include "pch.hxx"
#include "DirectXWindow.hxx"
#include "resource.h"

#include "DirectInput.hxx"

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"

/*

CDirectXWindow

Handles Windowed DirectX
It's responsible for updating all DirectInput devices as well

The message loop (See the Run method) will do the processing in
idle time

*/

// BUGBUG: Colors are off in 8bit mode
// BUGBUG: 16-bit mode not supported
// BUGBUG: Should detect invalid video bpp modes
// BUGBUG: Re-add 640x480 forced mode
// BUGBUG: F11 switch doesn't work (note: should be Alt-Enter)
// BUGBUG: Maximize, minimize, restore -- width will be off
// TODO: Add cool caption
// BUGBUG: Error in Initialize causes AV &  not showing error dialog

#define WIDTH_FACTOR 2

LPCTSTR CDirectXWindow::pszClassName = _T("StellaXClass");

static inline void ClientToScreen(
    HWND hWnd,
    LPRECT lpRect
    )
{
	ASSERT( ::IsWindow( hWnd ) );
	::ClientToScreen( hWnd, (LPPOINT)lpRect );
	::ClientToScreen( hWnd, ((LPPOINT)lpRect)+1 );
}

CDirectXWindow::CDirectXWindow(
    const CGlobalData* rGlobalData,
    const Console* pConsole,
    Event& rEvent 
    ) : \
    m_rGlobalData( rGlobalData ),
    m_pConsole( pConsole ),
    m_rEvent( rEvent ),

    m_fReady( FALSE ),
    m_fWindowed( TRUE ),
    m_fActive( FALSE ),

    m_piDD( NULL ),
    m_piDDSPrimary( NULL ),
    m_piDDSBack( NULL ),
    m_piDDPalette( NULL ),

    m_hwnd( NULL ),

    m_pDirectMouse( NULL ),
    m_pDirectJoystick( NULL ),
    m_pDirectKeyboard( NULL )
{
    TRACE("CDirectXWindow::CDirectXWindow");

    m_sizeGame.cx = m_pConsole->mediaSource().width();
    m_sizeGame.cy = m_pConsole->mediaSource().height();
    TRACE("m_sizeGame cx=%ld, cy=%ld", m_sizeGame.cx, m_sizeGame.cy);

    //
    // Initialize the pixel data table
    //

    for (int j = 0; j < 256; ++j)
    {
        m_rgbPixelDataTable[j] = j | (j << 8);
    }

}

CDirectXWindow::~CDirectXWindow
(
)
{
    TRACE("CDirectXWindow::~CDirectXWindow");

    ReleaseAllObjects();

    delete m_pDirectMouse;
    m_pDirectMouse = NULL;

    delete m_pDirectJoystick;
    m_pDirectJoystick = NULL;

    delete m_pDirectKeyboard;
    m_pDirectKeyboard = NULL;

    if (m_piDDPalette)
    {
        m_piDDPalette->Release();
        m_piDDPalette = NULL;
    }

    if ( m_piDD )
    {
        m_piDD->Release( );
        m_piDD = NULL;
    }

    ::UnregisterClass( pszClassName, m_rGlobalData->ModuleInstance() );
}

HRESULT CDirectXWindow::Initialize(
    HWND hwndParent,
    LPCTSTR pszTitle
    )
{
    TRACE( "CDirectXWindow::Initialize" );

    if ( m_hwnd )
    {
        return S_OK;
    }

    HRESULT hr;
    WNDCLASS wc;
    RECT rc;
    const unsigned int* pPalette = m_pConsole->mediaSource().palette();
    TCHAR pszStellaTitle[ MAX_PATH ];

    wc.lpszClassName = pszClassName;
    wc.lpfnWndProc = StaticWindowProc;
    wc.style = CS_HREDRAW | CS_VREDRAW; // CS_OWNDC
    wc.hInstance = m_rGlobalData->ModuleInstance();
    wc.hIcon = ::LoadIcon( m_rGlobalData->ModuleInstance(), 
                           MAKEINTRESOURCE( IDI_APP ) );
    wc.hCursor = ::LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)::GetStockObject( NULL_BRUSH );
    wc.lpszMenuName = NULL; // TODO
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    if ( ! ::RegisterClass( &wc ) )
    {
        TRACE( "RegisterClass failed" );

        hr = HRESULT_FROM_WIN32( ::GetLastError() );
        goto cleanup;
    }

    //
    // Create and show the main window
    //

    lstrcpy( pszStellaTitle, pszTitle );
    lstrcat( pszStellaTitle, _T(" - StellaX") );

    m_hwnd = ::CreateWindowEx( 0,
                               pszClassName, 
                               pszStellaTitle, 
                               // WS_OVERLAPPEDWINDOW,
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                               WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                               WS_BORDER,
                               0, 0, 0, 0,
                               NULL, 
                               NULL, 
                               m_rGlobalData->ModuleInstance(), 
                               this );
    if ( m_hwnd == NULL )
    {
        TRACE( "CreateWindow failed" );
        hr = HRESULT_FROM_WIN32( GetLastError() );
        MessageBox( m_rGlobalData->ModuleInstance(), NULL, IDS_CW_FAILED );
        goto cleanup;
    }

    //
    // Move the window into position
    //

    ::SetRect( &rc, 
               0, 0, 
               m_sizeGame.cx * WIDTH_FACTOR,
               m_sizeGame.cy );
    ::AdjustWindowRectEx( &rc, 
                          GetWindowStyle( m_hwnd ), 
                          ::GetMenu( m_hwnd ) != NULL,
                          GetWindowExStyle( m_hwnd ) );

    ::MoveWindow( m_hwnd,
                  CW_USEDEFAULT, CW_USEDEFAULT,
                  rc.right - rc.left,
                  rc.bottom - rc.top,
                  FALSE );

    ::SetFocus( m_hwnd );
    ::ShowWindow( m_hwnd, SW_SHOW );
    ::UpdateWindow( m_hwnd );
    
    //
    // m_rectWin will hold the window size/pos for switching modes
    //

    ::GetWindowRect( m_hwnd, &m_rcWindow );

    // TODO: Load accelerators

    //
    // Initialize DirectDraw 
    //

    hr = ::CoCreateInstance( CLSID_DirectDraw, 
                             NULL, 
                             CLSCTX_SERVER, 
                             IID_IDirectDraw, 
                             (void**)&m_piDD );
    if ( FAILED(hr) )
    {
        TRACE( "CCI DirectDraw failed" );
        goto cleanup;
    }

    //
    // Initialize it
    // This method takes the driver GUID parameter that the DirectDrawCreate 
    // function typically uses (NULL is active display driver)
    //

    hr = m_piDD->Initialize( NULL );
    if ( FAILED(hr) )
    {
        TRACE( "DDraw::Initialize failed, hr=%x", hr );
        goto cleanup;
    }

    //
	// Get the best video mode for game width
    //

    m_sizeFS.cx = 0;
    m_sizeFS.cy = 0;

    hr = m_piDD->EnumDisplayModes( 0, NULL, this, EnumModesCallback );
    if ( FAILED(hr) )
    {
        TRACE( "EnumDisplayModes failed" );
        goto cleanup;
    }

	if ( m_sizeFS.cx == 0 || m_sizeFS.cy == 0 )
	{
		TRACE("No good video mode found");
        hr = E_INVALIDARG;
        goto cleanup;
	}

    // m_sizeFS.cx = 640; m_sizeFS.cy = 480;
	TRACE( "Video Mode Selected: %d x %d", m_sizeFS.cx, m_sizeFS.cy );

    //
    // Create palette
    //
    
    int i;
    for ( i = 0; i < 256; ++i )
    {
        m_rgpe[i].peRed   = (BYTE)( (pPalette[i] & 0x00FF0000) >> 16 );
        m_rgpe[i].peGreen = (BYTE)( (pPalette[i] & 0x0000FF00) >> 8 );
        m_rgpe[i].peBlue  = (BYTE)( (pPalette[i] & 0x000000FF) );
        m_rgpe[i].peFlags = 0;
    }

    //
    // Initialize surfaces
    //

    hr = InitSurfaces( );
    if ( FAILED(hr) )
    {
        TRACE( "InitSurfaces failed, hr=%x", hr );
        goto cleanup;
    }

    //
    // Initialize DirectInput
    //

    m_pDirectMouse = new CDirectMouse( m_hwnd );
    if (m_pDirectMouse == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = m_pDirectMouse->Initialize();
    if ( FAILED(hr) )
    {
        TRACE( "Directmouse initialzie failed, hr = %X\n", hr );
        goto cleanup;
    }

    if ( m_rGlobalData->DisableJoystick() )
    {
        m_pDirectJoystick = new CDisabledJoystick( m_hwnd );
    }
    else
    {
        m_pDirectJoystick = new CDirectJoystick( m_hwnd );
    }
    if (m_pDirectJoystick == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = m_pDirectJoystick->Initialize( );
    if ( FAILED(hr) )
    {
        TRACE( "joystick initialzie failed, hr=%X", hr );
        goto cleanup;
    }

    m_pDirectKeyboard = new CDirectKeyboard( m_hwnd );
    if (m_pDirectKeyboard == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = m_pDirectKeyboard->Initialize();
    if ( FAILED(hr) )
    {
        TRACE( "keyboard init failed. hr=%X", hr );
        goto cleanup;
    }

cleanup:

    if ( FAILED(hr) )
    {
        ReleaseAllObjects();
        SendMessage( m_hwnd, WM_CLOSE, 0, 0 );
    }

    return hr;
}

HRESULT WINAPI CDirectXWindow::EnumModesCallback(
	LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
	CDirectXWindow* pThis = (CDirectXWindow*)lpContext;

    DWORD dwWidthReq = pThis->m_sizeGame.cx * WIDTH_FACTOR;
    DWORD dwHeightReq = pThis->m_sizeGame.cy;

	DWORD dwWidth = lpDDSurfaceDesc->dwWidth;
	DWORD dwHeight = lpDDSurfaceDesc->dwHeight;
	DWORD dwRGBBitCount = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

    //
    // must be 8 bit mode
    //

	if ( dwRGBBitCount != 8 )
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

    if ( pThis->m_sizeFS.cx != 0 && 
         pThis->m_sizeFS.cy != 0 )
    {
        //
        // check to see if this is better than the previous choice
        //

        if ( ( dwWidth - dwWidthReq ) > ( pThis->m_sizeFS.cx - dwWidthReq ) )
        {
            return DDENUMRET_OK;
        }

        if ( ( dwHeight - dwHeightReq ) > ( pThis->m_sizeFS.cy - dwHeightReq ) )
        {
            return DDENUMRET_OK;
        }
    }

    //
    // use it!
    //

    pThis->m_sizeFS.cx = dwWidth;
    pThis->m_sizeFS.cy = dwHeight;

	return DDENUMRET_OK;
}

HRESULT CDirectXWindow::InitSurfaces(
    void
    )
{
    if ( m_hwnd == NULL || m_piDD == NULL )
    {
        return E_UNEXPECTED;
    }

    HRESULT hr;
    DDSURFACEDESC ddsd;
    DDPIXELFORMAT ddpf;
    const unsigned int* pPalette = m_pConsole->mediaSource().palette();

    if ( m_fWindowed )
    {
        //
        // Get normal windowed mode
        //

        hr = m_piDD->SetCooperativeLevel( m_hwnd, DDSCL_NORMAL );
        if ( hr != DD_OK )
        {
            return hr;
        }

        //
        // Get the dimensions of the viewport and the screen bounds
        //

        ::GetClientRect( m_hwnd, &m_rcScreen );
        ::ClientToScreen( m_hwnd, &m_rcScreen );
        TRACE( "m_rcScreen = { %d,%d,%d,%d }", 
            m_rcScreen.left, m_rcScreen.right, m_rcScreen.top, m_rcScreen.bottom );
    }
    else
    {
        //
        // Get exclusive (full screen) mode
        //

        hr = m_piDD->SetCooperativeLevel( m_hwnd, 
                                          DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
        if ( hr != DD_OK )
        {
            return hr;
        }

        hr = m_piDD->SetDisplayMode( m_sizeFS.cx, m_sizeFS.cy, 8 );
        if ( hr != DD_OK )
        {
            return hr;
        }

        //
        // Store the dimensions of the viewport and screen bounds
        //

        ::SetRect( &m_rcScreen, 0, 0, m_sizeFS.cx, m_sizeFS.cy );
        TRACE( "m_rcScreen = { %d,%d,%d,%d }", 
            m_rcScreen.left, m_rcScreen.right, m_rcScreen.top, m_rcScreen.bottom );
    }

    //
    // Create the primary surface
    //
    
    ZeroMemory( &ddsd, sizeof(ddsd) );
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = m_piDD->CreateSurface( &ddsd, &m_piDDSPrimary, NULL );
    if ( hr != DD_OK )
    {
        return hr;
    }

    if ( m_fWindowed )
    {
        //
        // Create a clipper object
        //

        IDirectDrawClipper* piDDC;

        hr = m_piDD->CreateClipper( 0, &piDDC, NULL );
        if ( hr != DD_OK )
        {
            return hr;
        }

        //
        // Associate the clippper with the window
        //

        (void)piDDC->SetHWnd( 0, m_hwnd );
        (void)m_piDDSPrimary->SetClipper( piDDC );
        (void)piDDC->Release();
        piDDC = NULL;
    }

    //
    // Set palette (if necessary)
    //

    ddpf.dwSize = sizeof( ddpf );
    if ( ( m_piDDSPrimary->GetPixelFormat( &ddpf ) == DD_OK ) &&
        ( ddpf.dwFlags & DDPF_PALETTEINDEXED8 ) )
    {
        
        hr = m_piDD->CreatePalette( DDPCAPS_8BIT, 
            m_rgpe, 
            &m_piDDPalette, 
            NULL );
        if ( FAILED(hr) )
        {
            TRACE( "IDD::CreatePalette failed, hr=%X", hr);
            return hr;
        }
        

        hr = m_piDDSPrimary->SetPalette( m_piDDPalette );
        if ( FAILED(hr) )
        {
            TRACE( "SetPalette failed, hr=%x", hr );
            return hr;
        }
    }
    
    //
    // Get the back buffer
    //
    
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = m_sizeGame.cx;
    ddsd.dwHeight = m_sizeGame.cy;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = m_piDD->CreateSurface( &ddsd, &m_piDDSBack, NULL );
    if ( hr != DD_OK )
    {
        return hr;
    }

    //
    // Erase all surfaces
    //

    DDBLTFX ddbltfx;
    ZeroMemory( &ddbltfx, sizeof(ddbltfx) );
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = 0;

    (void)m_piDDSBack->Blt( NULL, NULL, NULL, 
                            DDBLT_COLORFILL | DDBLT_WAIT,
                            &ddbltfx );

    return S_OK;
}

void CDirectXWindow::ReleaseAllObjects(
    void
    )
{
    TRACE( "CDirectXWindow::ReleaseAllObjects" );

    if ( m_piDD )
    {
        (void)m_piDD->SetCooperativeLevel( m_hwnd, DDSCL_NORMAL );
    }

    if ( m_piDDSBack )
    {
        (void)m_piDDSBack->Release();
        m_piDDSBack = NULL;
    }

    if ( m_piDDSPrimary )
    {
        (void)m_piDDSPrimary->SetPalette( NULL );

        (void)m_piDDSPrimary->Release();
        m_piDDSPrimary = NULL;
    }

}



// BUGBUG: This class should derive from CWnd
// (fix up CWnd using this code)

LRESULT CALLBACK CDirectXWindow::StaticWindowProc(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    CDirectXWindow* pThis = NULL;

    if ( uMsg == WM_CREATE )
    {
        pThis = reinterpret_cast<CDirectXWindow*>( 
            reinterpret_cast<CREATESTRUCT*>( lParam )->lpCreateParams );

        ::SetWindowLong( hwnd, 
                         GWL_USERDATA, 
                         reinterpret_cast<LONG>( pThis ) );
    }
    else
    {
        pThis = reinterpret_cast<CDirectXWindow*>( 
            ::GetWindowLong( hwnd, GWL_USERDATA ) );
    }

    if ( pThis )
    {
        //
        // Most handled messages return 0, but some return a different
        // value
        //

        LRESULT lResult = 0;

        if ( pThis->WndProc( uMsg, wParam, lParam, &lResult ) )
        {
            //
            // Handled message
            //

            return lResult;
        }
    }

    //
    // Unhandled message
    //

    return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
}

BOOL CDirectXWindow::WndProc(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult // *plResult == 0 by default
    )
{
    switch ( uMsg )
    {
    case WM_ACTIVATE:
        //
        // Pause if minimized
        //
        m_fActive = !( HIWORD(wParam) );
        TRACE( "WM_ACTIVATE: m_fActive = %d", m_fActive );
        break;

        // case WM_COMMAND:
        // (menu commands)

    case WM_DESTROY:
        ReleaseAllObjects( );
        ::PostQuitMessage( 0 );
        break;

    case WM_GETMINMAXINFO:
        if ( m_fWindowed )
        {
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;

            RECT rcWin;
            ::GetWindowRect( m_hwnd, &rcWin );
            RECT rcCli;
            ::GetClientRect( m_hwnd, &rcCli );

            pmmi->ptMinTrackSize.x = ( rcWin.right - rcWin.left - rcCli.right ) + m_sizeGame.cx * WIDTH_FACTOR;
            pmmi->ptMinTrackSize.y = ( rcWin.bottom - rcWin.top - rcCli.bottom ) + m_sizeGame.cy;
            
            pmmi->ptMaxTrackSize.x = pmmi->ptMinTrackSize.x * 2;
            pmmi->ptMaxTrackSize.y = pmmi->ptMinTrackSize.y * 2;
        }
        break;


    case WM_KEYDOWN:
        switch ( (int)wParam )
        {
        case VK_ESCAPE:
            //
            // Escape = Exit
            //
            ::PostMessage( m_hwnd, WM_CLOSE, 0, 0 );
            return TRUE;

        case VK_F3:
            //
            // F11 = toggle fullscreen/windowed
            //
            if (m_fActive && m_fReady)
            {
                if (m_fWindowed)
                {
                    MessageBox(NULL, "Hallo!", "Hallo!", MB_OK);
                }
            }
            break;


        case VK_F11:
            //
            // F11 = toggle fullscreen/windowed
            //
            if ( m_fActive && m_fReady )
            {
                if ( m_fWindowed )
                {
                    ::GetWindowRect( m_hwnd, &m_rcWindow );
                }

                ChangeCoopLevel( );
            }
            break;


        case VK_F12:
            {
                //
                // F12 = write screenshot
                //

                TCHAR pszFileName[ MAX_PATH ];
                int i;

                for ( i = 0; i < 99 ; ++i )
                {
                    wsprintf( pszFileName, _T("stella%02d.bmp"), i );
                    if ( ::GetFileAttributes( pszFileName ) == 0xFFFFFFFF )
                    {
                        break;
                    }
                }

// TODO: Get the BMP writing code added to the core  
//                m_pConsole->mediaSource().writeBmp( pszFileName );
            }

        default:
            return FALSE;
        }
        break;

    case WM_MOVE:
        //
        // Retrieve the window position after a move
        //

        if ( m_fActive && m_fReady && m_fWindowed )
        {
            if ( ! ::IsIconic( m_hwnd ) )
            {
                ::GetWindowRect( m_hwnd, &m_rcWindow );

                ::GetClientRect( m_hwnd, &m_rcScreen );
                ClientToScreen( m_hwnd, &m_rcScreen );
                TRACE( "m_rcScreen = { %d,%d,%d,%d }", 
                    m_rcScreen.left, m_rcScreen.right, m_rcScreen.top, m_rcScreen.bottom );

            }
        }
        return FALSE;

    case WM_PAINT:
        //
        // Update the screen if we need to refresh
        //

        if ( m_fWindowed && m_fReady )
        {
            //
            // If we are in windowed mode, perform a blit
            //

            HRESULT hr;

            for ( ; ; )
            {
                hr = m_piDDSPrimary->Blt( &m_rcScreen, // destrect
                                          m_piDDSBack,
                                          NULL,
                                          DDBLT_WAIT,
                                          NULL );
                if ( hr == DD_OK )
                {
                    break;
                }

                if ( hr == DDERR_SURFACELOST )
                {
                    hr = m_piDDSPrimary->Restore();
                    if ( hr != DD_OK )
                    {
                        break;
                    }
                }
                if ( hr != DDERR_WASSTILLDRAWING )
                {
                    break;
                }
            }
        }
        return FALSE;

    case WM_PALETTECHANGED:
        TRACE( "WM_PALETTECHANGED from dxwin" );
        //
        // Check to see if we caused this
        //

        if ( (HWND)wParam == m_hwnd )
        {
            if ( m_piDDSPrimary && m_piDDPalette )
            {
                // (void)m_piDDSPrimary->SetPalette( m_piDDPalette );
            }
        }
        break;

    case WM_QUERYNEWPALETTE:
        TRACE( "WM_QUERYNEWPALETTE from dxwin" );
        if ( m_piDDSPrimary && m_piDDPalette )
        {
            // (void)m_piDDSPrimary->SetPalette( m_piDDPalette );

            *plResult = TRUE;
        }
        break;

    case WM_SETCURSOR:
        if ( m_fActive && m_fReady && ! m_fWindowed )
        {
            ::SetCursor( NULL );

            *plResult = TRUE;
        }
        else
        {
            return FALSE;
        }
        break;

    case WM_SIZE:
        m_fActive = (  wParam != SIZE_MAXHIDE &&
                       wParam != SIZE_MINIMIZED );
        if ( ! ::IsIconic( m_hwnd ) )
        {
            // ::GetWindowRect( m_hwnd, &m_rcWindow );

            ::GetClientRect( m_hwnd, &m_rcScreen );
            ClientToScreen( m_hwnd, &m_rcScreen );
            TRACE( "m_rcScreen = { %d,%d,%d,%d }", 
                m_rcScreen.left, m_rcScreen.right, m_rcScreen.top, m_rcScreen.bottom );

        }

        return FALSE;

    default:
        //
        // Unhandled message
        //

        return FALSE;
    }

    //
    // Handled message
    //

    return TRUE;
}

DWORD CDirectXWindow::Run(
    void
    )
{
    TRACE("CDirectXWindow::Run");

    //
    // Get the initial tick count
    //

    UINT uFrameCount = 0;

    unsigned __int64 uiStartRun;
    ::QueryPerformanceCounter( (LARGE_INTEGER*)&uiStartRun );

    //
    // Find out how many ticks occur per second
    //

    unsigned __int64 uiCountsPerSecond;
    ::QueryPerformanceFrequency( (LARGE_INTEGER*)&uiCountsPerSecond );

    const unsigned __int64 uiCountsPerFrame = 
        ( uiCountsPerSecond / m_rGlobalData->DesiredFrameRate() );

    TRACE( "uiCountsPerSecond=%ld", uiCountsPerSecond );
    TRACE( "uiCountsPerFrame = %ld", uiCountsPerFrame );

    unsigned __int64 uiFrameStart;
    unsigned __int64 uiFrameCurrent;

    //
    // Get Stella's media source
    //

    MediaSource& rMediaSource = m_pConsole->mediaSource();

    //
    // Main message loop
    //

    m_fReady = TRUE;

    MSG msg;

    for ( ; ; )
    {
        if ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
        {
            if ( ::GetMessage( &msg, NULL, 0, 0 ) <= 0 )
            {
                break;
            }

            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );

            continue;
        }

        if ( m_fActive && m_fReady )
        {
            HRESULT hr;
            
            //
            // idle time -- do stella updates
            //
            
            ++uFrameCount;
            
            ::QueryPerformanceCounter( (LARGE_INTEGER*)&uiFrameStart );
            
            //
            // ask media source to prepare the next frame
            //
            
            rMediaSource.update();
            
            //
            // update the display
            //
            
            if ( ! UpdateDisplay( rMediaSource ) )
            {
                // fatal error!
                
                ::MessageBox(NULL, "Fatal error updating display", "Error", MB_OK);
                ::PostQuitMessage( 0 );
                break;
            }
            
            for ( ; ; )
            {
                hr = m_piDDSPrimary->Blt( &m_rcScreen, // destrect
                                          m_piDDSBack,
                                          NULL,
                                          DDBLT_WAIT,
                                          NULL );
                if ( hr == DD_OK )
                {
                    break;
                }
                if ( hr == DDERR_SURFACELOST )
                {
                    hr = m_piDDSPrimary->Restore();
                    if ( hr != DD_OK )
                    {
                        break;
                    }
                }
                if ( hr != DDERR_WASSTILLDRAWING )
                {
                    break;
                }
            }
            
            //
            // Check for keyboard, mouse, joystick input
            //
            
            UpdateEvents();
            
            //
            // waste time to to meet desired frame rate
            //
            
            for ( ; ; )
            {
                ::QueryPerformanceCounter( (LARGE_INTEGER*)&uiFrameCurrent );
                
                if ( ( uiFrameCurrent - uiFrameStart ) >= uiCountsPerFrame )
                {
                    break;
                }
            }

            continue;
        }

        //
        // Sleep if we don't have anything to do
        //
        
        ::WaitMessage();

    } // for ( ; ; )

    // Main message loop done

    if ( m_rGlobalData->ShowFPS() )
    {
        // get number of scanlines in last frame

        uInt32 uScanLines = rMediaSource.scanlines();

        // Get the final tick count

        unsigned __int64 uiEndRun;
        ::QueryPerformanceCounter( (LARGE_INTEGER*)&uiEndRun );

        // Get number of ticks

        DWORD secs = (DWORD)( ( uiEndRun - uiStartRun ) / uiCountsPerSecond );

        DWORD fps = (secs == 0) ? 0 : (uFrameCount / secs);

        TCHAR pszBuf[1024];
        wsprintf( pszBuf, _T("Frames drawn: %ld\nFPS: %ld\nScanlines in last frame: %ld\n"),
                  uFrameCount,
                  fps, 
                  uScanLines );
        MessageBox( NULL, pszBuf, _T("Statistics"), MB_OK );
    }

    return msg.wParam;
}

// ---------------------------------------------------------------------------
// Event Handling
// VK_* is defined in winuser.h

struct KeyEventStruct
{
    int nVirtKey;
    int nAsyncVirtKey;
    Event::Type eventCode;
};


static KeyEventStruct KeyEventMap[] = 
{
    /* -------------------------------------------------------------------- */
    /* left keypad */

    { DIK_1,        '1',        Event::KeyboardZero1 },
    { DIK_2,        '2',        Event::KeyboardZero2 },
    { DIK_3,        '3',        Event::KeyboardZero3 },
    { DIK_Q,        'Q',        Event::KeyboardZero4 },
    { DIK_W,        'W',        Event::KeyboardZero5 },
    { DIK_E,        'E',        Event::KeyboardZero6 },
    { DIK_A,        'A',        Event::KeyboardZero7 },
    { DIK_S,        'S',        Event::KeyboardZero8 },
    { DIK_D,        'D',        Event::KeyboardZero9 },
    { DIK_Z,        'Z',        Event::KeyboardZeroStar },
    { DIK_X,        'X',        Event::KeyboardZero0 },
    { DIK_C,        'C',        Event::KeyboardZeroPound },

    /* -------------------------------------------------------------------- */
    /* right keypad */

    { DIK_8,        '8',        Event::KeyboardOne1 },
    { DIK_9,        '9',        Event::KeyboardOne2 },
    { DIK_0,        '0',        Event::KeyboardOne3 },
    { DIK_I,        'I',        Event::KeyboardOne4 },
    { DIK_O,        'O',        Event::KeyboardOne5 },
    { DIK_P,        'P',        Event::KeyboardOne6 },
    { DIK_K,        'K',        Event::KeyboardOne7 },
    { DIK_L,        'L',        Event::KeyboardOne8 },
    { DIK_SEMICOLON,';',        Event::KeyboardOne9 },
    { DIK_COMMA,    ',',        Event::KeyboardOneStar },
    { DIK_PERIOD,   '.',        Event::KeyboardOne0 },
    { DIK_SLASH,    '/',        Event::KeyboardOnePound },

    /* -------------------------------------------------------------------- */
    /* left joystick */

    { DIK_DOWN,     VK_DOWN,    Event::JoystickZeroDown },
    { DIK_UP,       VK_UP,      Event::JoystickZeroUp },
    { DIK_LEFT,     VK_LEFT,    Event::JoystickZeroLeft },
    { DIK_RIGHT,    VK_RIGHT,   Event::JoystickZeroRight },
    { DIK_SPACE,    VK_SPACE,   Event::JoystickZeroFire },

    /* left joystick (alt.) */

    { DIK_W,        'W',        Event::JoystickZeroUp },
    { DIK_S,        'S',        Event::JoystickZeroDown },
    { DIK_A,        'A',        Event::JoystickZeroLeft },
    { DIK_D,        'D',        Event::JoystickZeroRight },
    { DIK_TAB,      VK_TAB,     Event::JoystickZeroFire },

    /* I added this one (for my powerramp joystick) */

    { DIK_LCONTROL, VK_CONTROL, Event::JoystickZeroFire },

    /* left joystick booster-grip */

    { DIK_Z,        'Z',        Event::BoosterGripZeroTrigger },
    { DIK_X,        'X',        Event::BoosterGripZeroBooster },

    /* left joystick booster-grip (alt.) */

    { DIK_1,        '1',        Event::BoosterGripZeroTrigger },
    { DIK_2,        '2',        Event::BoosterGripZeroBooster },

    /* -------------------------------------------------------------------- */
    /* right joystick */

    { DIK_L,        'L',        Event::JoystickOneDown },
    { DIK_O,        'O',        Event::JoystickOneUp },
    { DIK_K,        'K',        Event::JoystickOneLeft },
    { DIK_SEMICOLON,';',        Event::JoystickOneRight },
    { DIK_J,        'J',        Event::JoystickOneFire },

    /* right joystick booster-grip */

    { DIK_N,        'N',        Event::BoosterGripOneTrigger },
    { DIK_M,        'M',        Event::BoosterGripOneBooster },

    /* -------------------------------------------------------------------- */
    /* console controls */

    { DIK_F1,       VK_F1,      Event::ConsoleSelect },
    { DIK_F2,       VK_F2,      Event::ConsoleReset },
    { DIK_F3,       VK_F3,      Event::ConsoleColor },
    { DIK_F4,       VK_F4,      Event::ConsoleBlackWhite },
    { DIK_F5,       VK_F5,      Event::ConsoleLeftDifficultyA },
    { DIK_F6,       VK_F6,      Event::ConsoleLeftDifficultyB },
    { DIK_F7,       VK_F7,      Event::ConsoleRightDifficultyA },
    { DIK_F8,       VK_F8,      Event::ConsoleRightDifficultyB }
};

void CDirectXWindow::UpdateEvents(
    void
    )
{
    if ( ! ( m_fReady && m_fActive ) )
    {
        return;
    }

    //
    // I do this because an event may appear multiple times in the map
    // and I don't want to undo a set i may have done earlier in the loop
    //

    const int nEventCount = Event::LastType;
    long rgEventState[ nEventCount ];
    ZeroMemory( rgEventState, nEventCount * sizeof(long) );

    int i;

    //
    // Update keyboard
    //

    if (m_pDirectKeyboard->Update() == S_OK)
    {
        int nSize = _countof(KeyEventMap);

        for (i = 0; i < nSize; ++i)
        {
            rgEventState[KeyEventMap[i].eventCode] |= 
                m_pDirectKeyboard->IsButtonPressed(KeyEventMap[i].nVirtKey);
        }
    }
    else
    {
        // Fallback -- if Keyboard update failed (most likely due to 
        // DirectInput not being available), then use the old async

        int nSize = _countof(KeyEventMap);

        for (i = 0; i < nSize; ++i)
        {
            rgEventState[KeyEventMap[i].eventCode] |=
                (::GetAsyncKeyState(KeyEventMap[i].nAsyncVirtKey) ? 1: 0);
        }
    }

    //
    // Update joystick
    //

    if (m_pDirectJoystick->Update() == S_OK)
    {
        rgEventState[Event::JoystickZeroFire] |=
            m_pDirectJoystick->IsButtonPressed(0);

        LONG x;
        LONG y;
        m_pDirectJoystick->GetPos( &x, &y );

        if (x < 0)
        {
            rgEventState[Event::JoystickZeroLeft] = 1;
        }
        else if (x > 0)
        {
            rgEventState[Event::JoystickZeroRight] = 1;
        }
        if (y < 0)
        {
            rgEventState[Event::JoystickZeroUp] = 1;
        }
        else if (y > 0)
        {
            rgEventState[Event::JoystickZeroDown] = 1;
        }
    }

    //
    // Update mouse
    //

    if (m_pDirectMouse->Update() == S_OK)
    {
        // NOTE: Mouse::GetPos returns a value from 0..999

        LONG x;
        m_pDirectMouse->GetPos( &x, NULL );

        // Mouse resistance is measured between 0...1000000

        rgEventState[ m_rGlobalData->PaddleResistanceEvent() ] = (999-x)*1000;
        
        rgEventState[ m_rGlobalData->PaddleFireEvent() ] |= m_pDirectMouse->IsButtonPressed(0);
    }

    //
    // Write new event state
    //

    for (i = 0; i < nEventCount; ++i)
    {
        m_rEvent.set( (Event::Type)i, rgEventState[i] );
    }
}

// ---------------------------------------------------------------------------
//  Display Update

BOOL CDirectXWindow::UpdateDisplay(
    MediaSource& rMediaSource
    )
{
    HRESULT hr;

    if ( ! ( m_fActive && m_fReady ) )
    {
        return TRUE;
    }

    if ( m_piDDSBack == NULL )
    {
        return FALSE;
    }


    const BYTE* current = rMediaSource.currentFrameBuffer();
    const BYTE* previous = rMediaSource.previousFrameBuffer();

    //
    // acquire pointer to linear video ram
    //

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
    for ( y = 0; y < m_sizeGame.cy; ++y )
    {
        DWORD oy = y * m_sizeGame.cx;
        DWORD pitchY = y * ddsd.lPitch;

        register int x;
        for ( x = 0; x < m_sizeGame.cx; ++x )
        {
            DWORD o = oy + x;
            BYTE v = current[ o ];
            if ( v == previous[ o ] )
            {
                continue;
            }

            BYTE c = m_rgbPixelDataTable[ v ];
            DWORD pos;

            switch ( ddsd.ddpfPixelFormat.dwRGBBitCount )
            {
            case 32:
                // x << 2 is times 4 ( 32 bit is 4 bytes per pixel )

                pos = pitchY + ( x << 2 );

                pbBackBytes[ pos + 0 ] = m_rgpe[ c ].peBlue;
                pbBackBytes[ pos + 1 ] = m_rgpe[ c ].peGreen;
                pbBackBytes[ pos + 2 ] = m_rgpe[ c ].peRed;
                pbBackBytes[ pos + 3 ] = 0; //alpha
                break;

            case 24:
                // 3 bytes per pixel

                pos = pitchY + ( x * 3 );

                pbBackBytes[ pos + 0 ] = m_rgpe[ c ].peBlue;
                pbBackBytes[ pos + 1 ] = m_rgpe[ c ].peGreen;
                pbBackBytes[ pos + 2 ] = m_rgpe[ c ].peRed;
                break;

                // case 16: pos = pitchY + ( x << 1 )

            case 8:
                pos = pitchY + x;

                pbBackBytes[ pos ] = m_rgbPixelDataTable[ current[ y * m_sizeGame.cx + x ] ];
                break;
            }
        }
    }

    (void)m_piDDSBack->Unlock( ddsd.lpSurface );

#ifdef _DEBUG
    HDC hdc;
	if ( m_piDDSBack->GetDC( &hdc ) == DD_OK )
	{
		::SetBkColor( hdc, RGB( 255, 255, 0 ) );
        ::TextOut( hdc, 5, 5, _T("DEBUG MODE"), 10 );
        m_piDDSBack->ReleaseDC( hdc );
	}
#endif

    return TRUE;
}

HRESULT CDirectXWindow::ChangeCoopLevel(
    void
    )
{
    TRACE( "ChangeCoopLevel m_fWindowed was %d", m_fWindowed );

    HRESULT hr;

    if ( m_hwnd == NULL )
    {
        return S_OK;
    }

    m_fReady = FALSE;

    m_fWindowed = !m_fWindowed;

    ReleaseAllObjects();

    if ( m_fWindowed )
    {
        ::SetWindowPos( m_hwnd, HWND_NOTOPMOST,
                        m_rcWindow.left, m_rcWindow.top,
                        m_rcWindow.right-m_rcWindow.left,
                        m_rcWindow.bottom-m_rcWindow.top,
                        SWP_SHOWWINDOW );
    }

    hr = InitSurfaces( );

    m_fReady = TRUE;

    return hr;
}
