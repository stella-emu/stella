//
// StellaX
// Jeff Miller 05/13/2000
//
#include "pch.hxx"
#include "StellaXMain.hxx"

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"

#include "resource.h"
#include "GlobalData.hxx"
#include "SoundWin32.hxx"

//
// Undefining USE_FS will use the (untested) windowed mode
//

//#define USE_FS

#ifdef USE_FS
#include "DirectXFullScreen.hxx"
#else
#include "DirectXWindow.hxx"
#endif

#include "DirectInput.hxx"
#include "DirectDraw.hxx"

//
// CStellaXMain
//
// equivalent to main() in the DOS version of stella
//

#define FORCED_VIDEO_CX 640
#define FORCED_VIDEO_CY 480

#ifdef USE_MY_STELLAPRO
#include "misc\stellapro.h"
#endif

CStellaXMain::CStellaXMain(
    ) : \
    m_pPropertiesSet( NULL )
{
    TRACE( "CStellaXMain::CStellaXMain" );
}

CStellaXMain::~CStellaXMain(
    )
{
    TRACE( "CStellaXMain::~CStellaXMain" );

    delete m_pPropertiesSet;
    m_pPropertiesSet = NULL;
}

DWORD CStellaXMain::Initialize(
    void
    )
{
    TRACE( "CStellaXMain::SetupProperties" );

    // Create a properties set for us to use

    if ( m_pPropertiesSet )
    {
        return ERROR_SUCCESS;
    }

    m_pPropertiesSet = new PropertiesSet(); 
    if ( m_pPropertiesSet == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    // Try to load the file stella.pro file

    string filename( "stella.pro" );
    
    // See if we can open the file and load properties from it
    ifstream stream(filename.c_str()); 
    if(stream)
    {
        // File was opened so load properties from it
        stream.close();
        m_pPropertiesSet->load(filename, &Console::defaultProperties());
    }
    else
    {
#ifdef USE_MY_STELLAPRO
        int cPropSet = sizeof( g_propset ) / sizeof( g_propset[0] );
        int iPropSet;
        for ( iPropSet = 0; iPropSet < cPropSet; ++iPropSet )
        {
            Properties properties( &Console::defaultProperties() );

            PROPSET::PROPS* pProps = g_propset[iPropSet].props;

            while ( pProps->key != NULL )
            {
                properties.set( pProps->key, pProps->value );
                ++pProps;
            }

            m_pPropertiesSet->insert( properties );
        }
#endif
    }


    return ERROR_SUCCESS;
}

HRESULT CStellaXMain::PlayROM(
    HWND hwnd,
    LPCTSTR pszPathName,
    LPCTSTR pszFriendlyName,
    CGlobalData* rGlobalData
    )
{
    UNUSED_ALWAYS( hwnd );

    HRESULT hr = S_OK;

    TRACE("CStellaXMain::PlayROM");

    //
    // show wait cursor while loading
    //

    HCURSOR hcur = ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

    //
    // setup objects used here
    //

    BYTE* pImage = NULL;
    LPCTSTR pszFileName = NULL;

#ifdef USE_FS
    CDirectXFullScreen* pwnd = NULL;
#else
    CDirectXWindow* pwnd = NULL;
#endif
    Console* pConsole = NULL;
    Sound* pSound = NULL;
    Event rEvent;

    //
    // Load the rom file
    //

    HANDLE hFile;
    DWORD dwImageSize;

    hFile = ::CreateFile( pszPathName, 
                          GENERIC_READ, 
                          FILE_SHARE_READ, 
                          NULL, 
                          OPEN_EXISTING, 
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        HINSTANCE hInstance = (HINSTANCE)::GetWindowLong( hwnd, GWL_HINSTANCE );

        DWORD dwLastError = ::GetLastError();

        TCHAR pszCurrentDirectory[ MAX_PATH + 1 ];
        ::GetCurrentDirectory( MAX_PATH, pszCurrentDirectory );

        // ::MessageBoxFromGetLastError( pszPathName );
        TCHAR pszFormat[ 1024 ];
        LoadString( hInstance,
                    IDS_ROM_LOAD_FAILED,
                    pszFormat, 1023 );

        LPTSTR pszLastError = NULL;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, 
            dwLastError, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&pszLastError, 
            0, 
            NULL);

        TCHAR pszError[ 1024 ];
        wsprintf( pszError, 
                  pszFormat, 
                  pszCurrentDirectory,
                  pszPathName, 
                  dwLastError,
                  pszLastError );

        ::MessageBox( hwnd, 
                      pszError, 
                      _T("Error"),
                      MB_OK | MB_ICONEXCLAMATION );

        ::LocalFree( pszLastError );

        hr = HRESULT_FROM_WIN32( ::GetLastError() ); 
        goto exit;
    }

    dwImageSize = ::GetFileSize( hFile, NULL );

    pImage = new BYTE[dwImageSize + 1];
    if ( pImage == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    DWORD dwActualSize;
    if ( ! ::ReadFile( hFile, pImage, dwImageSize, &dwActualSize, NULL ) )
    {
        VERIFY( ::CloseHandle( hFile ) );

        MessageBoxFromGetLastError( pszPathName );

        hr = HRESULT_FROM_WIN32( ::GetLastError() );
        goto exit;
    }

    VERIFY( ::CloseHandle(hFile) );

    //
    // Create Sound driver object
    // (Will be initialized once we have a window handle below)
    //

    if (rGlobalData->bNoSound)
    {
        TRACE("Creating Sound driver");
        pSound = new Sound;
    }
    else
    {
        TRACE("Creating SoundWin32 driver");
        pSound = new SoundWin32;
    }
    if ( pSound == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    //
    // get just the filename
    //

    pszFileName = _tcsrchr( pszPathName, _T('\\') );
    if ( pszFileName )
    {
        ++pszFileName;
    }
    else
    {
        pszFileName = pszPathName;
    }

    try
    {
        // If this throws an exception, then it's probably a bad cartridge

        pConsole = new Console( pImage, 
                                dwActualSize,
                                pszFileName, 
                                rEvent, 
                                *m_pPropertiesSet, 
                                *pSound );
        if ( pConsole == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }
    catch (...)
    {
        MessageBox(rGlobalData->instance,
            NULL, IDS_CANTSTARTCONSOLE);

        goto exit;
    }

#ifdef USE_FS
    pwnd = new CDirectXFullScreen( rGlobalData,
                                   pConsole, 
                                   rEvent );
#else
    pwnd = new CDirectXWindow( rGlobalData,
                               pConsole,
                               rEvent );
#endif
    if ( pwnd == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

#ifdef USE_FS
    if (rGlobalData->bAutoSelectVideoMode)
    {
        hr = pwnd->Initialize( );
    }
    else
    {
        //
        // Initialize with 640 x 480
        //

        hr = pwnd->Initialize( FORCED_VIDEO_CX, FORCED_VIDEO_CY );
    }
#else
    hr = pwnd->Initialize( hwnd, pszFriendlyName );
#endif
    if ( FAILED(hr) )
    {
        TRACE( "CWindow::Initialize failed, err = %X", hr );
        goto exit;
    }

    if (!rGlobalData->bNoSound)
    {
        //
        // 060499: Pass pwnd->GetHWND() in instead of hwnd as some systems
        // will not play sound if this isn't set to the active window
        //

        SoundWin32* pSoundWin32 = static_cast<SoundWin32*>( pSound );

        hr = pSoundWin32->Initialize( *pwnd );
        if ( FAILED(hr) )
        {
            TRACE( "Sndwin32 Initialize failed, err = %X", hr );
            goto exit;
        }
    }

    // restore cursor

    ::SetCursor( hcur );
    hcur = NULL;

    ::ShowWindow( hwnd, SW_HIDE );

    (void)pwnd->Run();

    ::ShowWindow( hwnd, SW_SHOW );

exit:

    if ( hcur )
    {
        ::SetCursor( hcur );
    }

    delete pwnd;

    delete pConsole;
    delete pSound;
    delete pImage;

    return hr;
}
