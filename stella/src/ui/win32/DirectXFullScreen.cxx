//
// StellaX
// Jeff Miller 05/12/2000
//
#include "pch.hxx"
#include "DirectXFullScreen.hxx"
#include "resource.h"

#include "DirectInput.hxx"

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "debug.hxx"

/*

CDirectXFullScreen

Handles Full Screen DirectX
It's responsible for updating all DirectInput devices as well

The message loop (See the Run method) will do the processing in
idle time

*/

LPCTSTR CDirectXFullScreen::pszClassName = _T("StellaXClass");

CDirectXFullScreen::CDirectXFullScreen(
    const CGlobalData* rGlobalData,
    const Console* pConsole,
    Event& rEvent
    ) : \
    m_rGlobalData( rGlobalData ),
	m_pConsole( pConsole ),
	m_rEvent( rEvent ),

    m_fInitialized( FALSE ),
	m_hwnd( NULL ),

	m_piDD( NULL ),
	m_piDDSPrimary( NULL ),
	m_piDDSBack( NULL ),
	m_piDDPalette( NULL ),

    m_fActiveWindow( TRUE ),

	m_pDirectMouse( NULL ),
	m_pDirectJoystick( NULL ),
	m_pDirectKeyboard( NULL )
{
	TRACE("CDirectXFullScreen::CDirectXFullScreen");

	// Get the game's width and height

	m_sizeGame.cx = m_pConsole->mediaSource().width();
	m_sizeGame.cy = m_pConsole->mediaSource().height();
	TRACE("m_sizeGame cx=%ld, cy=%ld", m_sizeGame.cx, m_sizeGame.cy);

	// Initialize the pixel data table

	for (int j = 0; j < 256; ++j)
	{
		m_rgbPixelDataTable[j] = j | (j << 8);
	}

}

CDirectXFullScreen::~CDirectXFullScreen
(
)
{
	TRACE("CDirectXFullScreen::~CDirectXFullScreen");

	Cleanup();
}

HRESULT CDirectXFullScreen::Initialize(
    int cx /* = 0 */,
    int cy /* = 0 */
    )
{
    TRACE( "CDirectXFullScreen::Initialize" );

    if ( m_fInitialized )
    {
        return S_OK;
    }

    HRESULT hr = S_OK;
    const unsigned int* pPalette = m_pConsole->mediaSource().palette();

	// Create the windows class, see:
	// mk:@MSITStore:E:\MSDN\DXFound.chm::/devdoc/good/directx/ddoverv_0l2v.htm

	TRACE( "CDirectXFullScreen RegisterClassEx" );

	WNDCLASSEX wcex;
	ZeroMemory( &wcex, sizeof(wcex) );
	wcex.cbSize = sizeof( wcex );
	wcex.hInstance = m_rGlobalData->ModuleInstance();
	wcex.lpszClassName = pszClassName;
	wcex.lpfnWndProc = StaticWindowProc;
	wcex.style = CS_OWNDC;
	wcex.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wcex.hIconSm = LoadIcon( NULL, IDI_WINLOGO );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
    if ( ! ::RegisterClassEx( &wcex ) )
	{
        hr = HRESULT_FROM_WIN32( ::GetLastError() );
        goto cleanup;
	}


	TRACE("CDirectXFullScreen CreateWindowEx");

	// 060499: pass this into the createparams to make sure it's available
	// if command line param given

    m_hwnd = ::CreateWindowEx( WS_EX_TOPMOST, 
                               pszClassName, 
                               _T("StellaX"), 
		                       WS_VISIBLE | WS_POPUP, 
		                       0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
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

    ::SetFocus( m_hwnd );
    ::ShowWindow( m_hwnd, SW_SHOW );
    ::UpdateWindow( m_hwnd );
	
    ::ShowCursor( FALSE );

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

    ::SetRect( &m_rectScreen, 0, 0, cx, cy );

    if ( cx == 0 || cy == 0 )
    {
    	hr = m_piDD->EnumDisplayModes( 0, NULL, this, EnumModesCallback );
        if ( FAILED(hr) )
        {
            TRACE( "EnumDisplayModes failed" );
            goto cleanup;
        }
    }

	if ( m_rectScreen.right == 0 || m_rectScreen.bottom == 0 )
	{
		TRACE("No good video mode found");
        hr = E_INVALIDARG;
        goto cleanup;
	}

	TRACE( "Video Mode Selected: %d x %d", m_rectScreen.right, m_rectScreen.bottom );

	// compute blit offset to center image

	m_ptBlitOffset.x = ( ( m_rectScreen.right - m_sizeGame.cx * 2 ) / 2 ); // WIDTH_FACTOR
	m_ptBlitOffset.y = ( ( m_rectScreen.bottom - m_sizeGame.cy ) / 2 );

	TRACE("Game dimensions: %dx%d (blit offset = %d, %d)", 
		m_sizeGame.cx, m_sizeGame.cy, m_ptBlitOffset.x, m_ptBlitOffset.y);
	
	// Set cooperative level

	hr = m_piDD->SetCooperativeLevel( m_hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
	if ( FAILED(hr) )
	{
		TRACE( "IDD:SetCooperativeLevel failed, hr=%x", hr );
        goto cleanup;
	}

	hr = m_piDD->SetDisplayMode( m_rectScreen.right, m_rectScreen.bottom, 8 );
	if ( FAILED(hr) )
	{
		TRACE( "IDD:SetDisplayMode failed, hr=%x", hr );
        goto cleanup;
	}

    //
	// Create the primary surface
    //

	DDSURFACEDESC ddsd;

    ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = m_piDD->CreateSurface(&ddsd, &m_piDDSPrimary, NULL);
	if (FAILED(hr))
	{
        TRACE( "CreateSurface failed, hr=%X", hr );
		goto cleanup;
	}

    //
	// Create the offscreen surface
    //

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = m_sizeGame.cx * 2; // WIDTH_FACTOR
	ddsd.dwHeight = m_sizeGame.cy;
	hr = m_piDD->CreateSurface(&ddsd, &m_piDDSBack, NULL);
	if (FAILED(hr))
	{
        TRACE( "CreateSurface failed, hr=%x", hr );
		goto cleanup;
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
                   m_sizeGame.cx * 2, // WIDTH_FACTOR
                   m_sizeGame.cy );
		::ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );

		(void)m_piDDSBack->ReleaseDC( hdc );
	}

    //
    // Create Palette
    //

	PALETTEENTRY pe[256];

    int i;
	for ( i = 0; i < 256; ++i )
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
    if ( FAILED(hr) )
    {
    	TRACE( "IDD::CreatePalette failed, hr=%X", hr);
        goto cleanup;
    }

    hr = m_piDDSPrimary->SetPalette( m_piDDPalette );
    if ( FAILED(hr) )
    {
        TRACE( "SetPalette failed, hr=%x", hr );
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

    m_fInitialized = TRUE;

cleanup:

    if ( FAILED(hr) )
    {
        Cleanup();
    }

    return hr;
}

void CDirectXFullScreen::Cleanup(
	void
    )
{
    TRACE( "CDirectXFullScreen::Cleanup" );

    ShowCursor(TRUE);

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

        m_piDD->Release( );
        m_piDD = NULL;
    }

	if ( m_hwnd )
	{
        ::DestroyWindow( m_hwnd );

        //
        // Remove the WM_QUIT which will be in the message queue
        // so that the main window doesn't exit
        //

        MSG msg;
        ::PeekMessage( &msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE );

        m_hwnd = NULL;
	}

    ::UnregisterClass( pszClassName, m_rGlobalData->ModuleInstance() );

    m_fInitialized = FALSE;
}

LRESULT CALLBACK CDirectXFullScreen::StaticWindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam
    )
{
	CDirectXFullScreen* pThis;

    if ( uMsg == WM_CREATE )
    {
        pThis = reinterpret_cast<CDirectXFullScreen*>( 
            reinterpret_cast<CREATESTRUCT*>( lParam )->lpCreateParams );

        ::SetWindowLong( hwnd, 
                         GWL_USERDATA, 
                         reinterpret_cast<LONG>( pThis ) );
    }
    else
    {
        pThis = reinterpret_cast<CDirectXFullScreen*>( 
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


BOOL CDirectXFullScreen::WndProc(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg)
	{
	case WM_ACTIVATE:
        m_fActiveWindow = ( wParam != WA_INACTIVE );
		break;

	case WM_DESTROY:
        ::PostQuitMessage( 0 );
		break;

    case WM_KEYDOWN:
		switch ( (int)wParam )
		{
		case VK_ESCAPE:
            //
            // Escape = Exit
            //

            ::PostMessage( m_hwnd, WM_CLOSE, 0, 0 );
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
//				m_pConsole->mediaSource().writeBmp( pszFileName );
			}
			break;
		}
		break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            ::BeginPaint( m_hwnd, &ps );
            ::EndPaint( m_hwnd, &ps );
        }
        break;

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

DWORD CDirectXFullScreen::Run(
	void
    )
{
	TRACE("CDirectXFullScreen::Run");

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

	MSG msg;

	for ( ; ; )
	{
        if ( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				TRACE("WM_QUIT received");
				break;
			}

            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
		}
		else if (m_fActiveWindow)
		{
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
		}
		else
		{
			// if no new message, and we're not the active window, then sleep

            ::WaitMessage();
		}
	}

	TRACE("Message loop done");

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

	{ DIK_1,		'1',		Event::KeyboardZero1 },
	{ DIK_2,		'2',		Event::KeyboardZero2 },
	{ DIK_3,		'3',		Event::KeyboardZero3 },
	{ DIK_Q,		'Q',		Event::KeyboardZero4 },
	{ DIK_W,		'W',		Event::KeyboardZero5 },
	{ DIK_E,		'E',		Event::KeyboardZero6 },
	{ DIK_A,		'A',		Event::KeyboardZero7 },
	{ DIK_S,		'S',		Event::KeyboardZero8 },
	{ DIK_D,		'D',		Event::KeyboardZero9 },
	{ DIK_Z,		'Z',		Event::KeyboardZeroStar },
	{ DIK_X,		'X',		Event::KeyboardZero0 },
	{ DIK_C,		'C',		Event::KeyboardZeroPound },

	/* -------------------------------------------------------------------- */
	/* right keypad */

	{ DIK_8,		'8',		Event::KeyboardOne1 },
	{ DIK_9,		'9',		Event::KeyboardOne2 },
	{ DIK_0,		'0',		Event::KeyboardOne3 },
	{ DIK_I,		'I',		Event::KeyboardOne4 },
	{ DIK_O,		'O',		Event::KeyboardOne5 },
	{ DIK_P,		'P',		Event::KeyboardOne6 },
	{ DIK_K,		'K',		Event::KeyboardOne7 },
	{ DIK_L,		'L',		Event::KeyboardOne8 },
	{ DIK_SEMICOLON,';',		Event::KeyboardOne9 },
	{ DIK_COMMA,	',',		Event::KeyboardOneStar },
	{ DIK_PERIOD,	'.',		Event::KeyboardOne0 },
	{ DIK_SLASH,	'/',		Event::KeyboardOnePound },

	/* -------------------------------------------------------------------- */
	/* left joystick */

	{ DIK_DOWN,		VK_DOWN,	Event::JoystickZeroDown },
	{ DIK_UP,		VK_UP,		Event::JoystickZeroUp },
	{ DIK_LEFT,		VK_LEFT,	Event::JoystickZeroLeft },
	{ DIK_RIGHT,	VK_RIGHT,	Event::JoystickZeroRight },
	{ DIK_SPACE,	VK_SPACE,	Event::JoystickZeroFire },

	/* left joystick (alt.) */

	{ DIK_W,		'W',		Event::JoystickZeroUp },
	{ DIK_S,		'S',		Event::JoystickZeroDown },
	{ DIK_A,		'A',		Event::JoystickZeroLeft },
	{ DIK_D,		'D',		Event::JoystickZeroRight },
	{ DIK_TAB,		VK_TAB,		Event::JoystickZeroFire },

	/* I added this one (for my powerramp joystick) */

	{ DIK_LCONTROL,	VK_CONTROL,	Event::JoystickZeroFire },

	/* left joystick booster-grip */

	{ DIK_Z,		'Z',		Event::BoosterGripZeroTrigger },
	{ DIK_X,		'X',		Event::BoosterGripZeroBooster },

	/* left joystick booster-grip (alt.) */

	{ DIK_1,		'1',		Event::BoosterGripZeroTrigger },
	{ DIK_2,		'2',		Event::BoosterGripZeroBooster },

	/* -------------------------------------------------------------------- */
	/* right joystick */

	{ DIK_L,		'L',		Event::JoystickOneDown },
	{ DIK_O,		'O',		Event::JoystickOneUp },
	{ DIK_K,		'K',		Event::JoystickOneLeft },
	{ DIK_SEMICOLON,';',		Event::JoystickOneRight },
	{ DIK_J,		'J',		Event::JoystickOneFire },

	/* right joystick booster-grip */

	{ DIK_N,		'N',		Event::BoosterGripOneTrigger },
	{ DIK_M,		'M',		Event::BoosterGripOneBooster },

	/* -------------------------------------------------------------------- */
	/* console controls */

	{ DIK_F1,		VK_F1,		Event::ConsoleSelect },
	{ DIK_F2,		VK_F2,		Event::ConsoleReset },
	{ DIK_F3,		VK_F3,		Event::ConsoleColor },
	{ DIK_F4,		VK_F4,		Event::ConsoleBlackWhite },
	{ DIK_F5,		VK_F5,		Event::ConsoleLeftDifficultyA },
	{ DIK_F6,		VK_F6,		Event::ConsoleLeftDifficultyB },
	{ DIK_F7,		VK_F7,		Event::ConsoleRightDifficultyA },
	{ DIK_F8,		VK_F8,		Event::ConsoleRightDifficultyB }
};

void CDirectXFullScreen::UpdateEvents(
	void
    )
{
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
//	Display Update

BOOL CDirectXFullScreen::UpdateDisplay(
	MediaSource& rMediaSource
    )
{
    if ( m_piDDSPrimary == NULL )
    {
        return FALSE;
    }

    if ( ! m_fActiveWindow )
    {
        return TRUE;
    }

    HRESULT hr;

	// uInt32* current = (uInt32*)rMediaSource.currentFrameBuffer();
	// uInt32* previous = (uInt32*)rMediaSource.previousFrameBuffer();

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
        const WORD bufofsY = ( y * m_sizeGame.cx );
        const DWORD screenofsY = ( y * ddsd.lPitch );

        register int x;
        for ( x = 0; x < m_sizeGame.cx; ++x )
        {
            const WORD bufofs = bufofsY + x;
            BYTE v = current[ bufofs ];
            if ( v == previous[ bufofs ] )
            {
                continue;
            }

            // x << 1 is times 2 ( doubling width ) WIDTH_FACTOR

            const DWORD pos = screenofsY + ( x << 1 );

            pbBackBytes[ pos + 0 ] =
            pbBackBytes[ pos + 1 ] = m_rgbPixelDataTable[ v ];
        }
    }

    (void)m_piDDSBack->Unlock( ddsd.lpSurface );
	
	// Blit offscreen to onscreen

	RECT rc = { 0, 0, 
                m_sizeGame.cx*2, // WIDTH_FACTOR 
                m_sizeGame.cy };

    hr = m_piDDSPrimary->BltFast( m_ptBlitOffset.x, m_ptBlitOffset.y,
                                  m_piDDSBack, 
                                  &rc, 
                                  // DDBLTFAST_WAIT |DDBLTFAST_DESTCOLORKEY   );
                                  DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT  );

	return TRUE;
}

HRESULT WINAPI CDirectXFullScreen::EnumModesCallback(
	LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
	CDirectXFullScreen* pThis = (CDirectXFullScreen*)lpContext;

    DWORD dwWidthReq = pThis->m_sizeGame.cx * 2; // WIDTH_FACTOR
    DWORD dwHeightReq = pThis->m_sizeGame.cy;

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
