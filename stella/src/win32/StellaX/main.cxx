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
// $Id: main.cxx,v 1.3 2004-07-15 03:03:27 stephena Exp $
//============================================================================

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
      if ( m_hMutex != INVALID_HANDLE_VALUE && m_dwError != ERROR_ALREADY_EXISTS )
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
LPCTSTR g_ctszDebugLog = _T("stella.log");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
int WINAPI _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine, int nCmdShow )
{
  UNUSED_ALWAYS( hPrevInstance );
  UNUSED_ALWAYS( lpCmdLine );
  UNUSED_ALWAYS( nCmdShow );

  (void)::DeleteFile(g_ctszDebugLog);

  CSingleInstance mutex( _T("StellaXMutex") );
  if ( mutex.AlreadyExists() )
  {
    MessageBox( hInstance, NULL, IDS_ALREADYRUNNING );
    return 1;
  }

  HRESULT hrCoInit = ::CoInitialize( NULL );
  if ( FAILED(hrCoInit) )
    MessageBox( hInstance, NULL, IDS_COINIT_FAILED );

  ::InitCommonControls();

  CGlobalData globaldata( hInstance );

  // show the ui
  MainDlg dlg( globaldata, hInstance );
  dlg.DoModal( NULL );

  if ( hrCoInit == S_OK )
    ::CoUninitialize();

  return 0;
}
