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
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: main.cxx,v 1.1 2004-06-28 23:13:54 stephena Exp $
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

// see debug.cpp

LPCTSTR g_ctszDebugLog = _T("stella.log");

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
  CMainDlg dlg( globaldata, hInstance );
  dlg.DoModal( NULL );

  if ( hrCoInit == S_OK )
    ::CoUninitialize();

  return 0;
}
