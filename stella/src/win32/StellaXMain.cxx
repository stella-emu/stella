//
// StellaX
// Jeff Miller 05/13/2000
//

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>

#include "resource.h"
#include "GlobalData.hxx"
#include "pch.hxx"
#include "StellaXMain.hxx"

//
// CStellaXMain
//
// equivalent to main() in the DOS version of stella
//


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
}

DWORD CStellaXMain::Initialize(
    void
    )
{
    return ERROR_SUCCESS;
}

HRESULT CStellaXMain::PlayROM(
    HWND hwnd,
    LPCTSTR pszPathName,
    LPCTSTR pszFriendlyName,
    CGlobalData& rGlobalData
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

    // restore cursor

    ::SetCursor( hcur );
    hcur = NULL;

    ::ShowWindow( hwnd, SW_HIDE );

// launch game here

    ::ShowWindow( hwnd, SW_SHOW );

exit:

    if ( hcur )
    {
        ::SetCursor( hcur );
    }

	return hr;
}
