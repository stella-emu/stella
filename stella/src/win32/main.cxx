/*

    StellaX
    Win32 DirectX port of Stella

    Written by Jeff Miller (contact Bradford for current email address)

    Stella core developed by Bradford W. Mott

    This software is Copyright (c) 1995-2000, Jeff Miller

REVISIONS:

    14-Mar-99   1.1.0   new code base

    19-Mar-99   1.1.1   took into account video width
                        fixed no sound card bug
                        -mute option
                        fixed mame32 www link

    11-Jun-99   1.1.2   removed registerfiletypes call
                        fixed directsound
                        added directinput support for keyboard, mouse, joystick
                        fixed minimize button bug
                        created virtual list view to speed load time
                        add screen capture (f12 key)
                        added romdir read from STELLA.INI file 
                            (defaults to ROMS directory)
                        I now use WM_SETICON so the stella icon shows on the task list
                        Works on NT4 (dont fail if DirectInput not available)
                        Added UI field for a cartridge.note
                        Changed stellax web site address
                        Added really cool help property sheet w/adobe doc

     9-Sep-99   1.1.3   Improved video detection logic
                        Added DisableJoystick and ListSort options to .ini
                        Rewrote the sound driver code
                        Added multiple screen shot support - first writes to 
                            stella00.bmp then stella01.bmp, etc.
                        Added cool round buttons on main screen
     18-Apr-00          Started removing all exceptions
                        Fixed sound code (now uses streaming properly)
                        Added configuration dialog
                        Fixed it so that Alt-F4 will close the dialog
                        Updated the master ROM list

     02-May-00  1.1.3a  Fixed joystick handling regression
                        Fixed repaint problem on doc page
                        Fixed problem where some machines would report path not found
                        Added force 640x480 video mode
                        Added browse button on config dialog

     05-Jan-02  n/a     Wow, it's been awhile...Released source code

*/

#include "pch.hxx"
#include "resource.h"

#include "GlobalData.hxx"
#include "MainDlg.hxx"

class CSingleInstance
{
public:

    CSingleInstance( LPCTSTR pszName )
        {
            ::SetLastError( ERROR_SUCCESS );

            m_hMutex = ::CreateMutex( NULL, TRUE, pszName );

            m_dwError = ::GetLastError();
        }

    ~CSingleInstance()
        {
            if ( m_hMutex != INVALID_HANDLE_VALUE &&
                 m_dwError != ERROR_ALREADY_EXISTS )
            {
                VERIFY( ::ReleaseMutex( m_hMutex ) );
                VERIFY( ::CloseHandle( m_hMutex ) );
            }
        }

    BOOL AlreadyExists( void ) const
        {
            return ( m_dwError == ERROR_ALREADY_EXISTS );
        }

private:

    HANDLE m_hMutex;
    DWORD m_dwError;

CSingleInstance( const CSingleInstance& );  // no implementation
void operator=( const CSingleInstance& );  // no implementation
};

// see debug.cpp

LPCTSTR g_ctszDebugLog = _T("stella.log");


int WINAPI _tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine,
    int nCmdShow
    )
{
    UNUSED_ALWAYS( hPrevInstance );
    UNUSED_ALWAYS( lpCmdLine );
    UNUSED_ALWAYS( nCmdShow );

    DWORD dwRet;

    (void)::DeleteFile(g_ctszDebugLog);

    CSingleInstance mutex( _T("StellaXMutex") );
    if ( mutex.AlreadyExists() )
    {
        MessageBox( hInstance, NULL, IDS_ALREADYRUNNING );
        return 1;
    }

    HRESULT hrCoInit = ::CoInitialize( NULL );
    if ( FAILED(hrCoInit) )
    {
        MessageBox( hInstance, NULL, IDS_COINIT_FAILED );
    }

    ::InitCommonControls();

    BOOL fOk = FALSE;

    CGlobalData globaldata( hInstance );

    fOk = globaldata.ParseCommandLine( __argc, __argv );
    if (!fOk)
    {
        MessageBox( hInstance, NULL, IDS_BADARGUMENT );
    }
    else
    {
        LPCTSTR ctszPathName = globaldata.PathName();
        if (ctszPathName != NULL)
        {
            //
            // a filename was given on the commandline, skip the UI
            //

            CStellaXMain    stellax;

            dwRet = stellax.Initialize();
            if ( dwRet != ERROR_SUCCESS )
            {
                MessageBoxFromWinError( dwRet, _T("CStellaX::Initialize") );
            }
            else
            {
                dwRet = stellax.PlayROM( GetDesktopWindow(), 
                                         ctszPathName,
                                         _T("StellaX"), // Dont knwo the friendly name
                                         globaldata );
            }
        }
        else
        {
            //
            // show the ui
            //

            CMainDlg dlg( globaldata, hInstance );
            dlg.DoModal( NULL );
        }
    }

    if ( hrCoInit == S_OK )
    {
        ::CoUninitialize();
    }

    return 0;
}
