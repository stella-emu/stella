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
// $Id: ConfigPage.cxx,v 1.3 2004-07-06 22:51:58 stephena Exp $
//============================================================================ 

#include "pch.hxx"
#include "ConfigPage.hxx"
#include "resource.h"
#include "BrowseForFolder.hxx"

#include "bspf.hxx"
#include "Settings.hxx"


CConfigPage::CConfigPage( CGlobalData& rGlobalData )
           : myGlobalData( rGlobalData ),
             CPropertyPage( IDD_CONFIG_PAGE )
{
}

BOOL CConfigPage::OnInitDialog( HWND hwnd )
{
  // return FALSE if SetFocus is called
  m_hwnd = hwnd;
  HWND hwndCtrl;
    
  // Reload settings just in case the emulation changed them
  myGlobalData.settings().loadConfig();

  // Set up ROMPATH
  hwndCtrl = ::GetDlgItem( hwnd, IDC_ROMPATH );
  ::SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
  ::SetWindowText( hwndCtrl,
                   myGlobalData.settings().getString("romdir").c_str() );

  // Set up PADDLE
  hwndCtrl = ::GetDlgItem( hwnd, IDC_PADDLE );
  TCHAR psz[4] = _T("0");
  TCHAR i;
  for ( i = 0; i < 4; ++i )
  {
    psz[0] = _T('0') + i;
    ::SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)psz );
  }
  ::SendMessage( hwndCtrl, CB_SETCURSEL,
                myGlobalData.settings().getInt("paddle"), 0 );

  // Set up Video mode
  int videomode = 0;
  hwndCtrl = ::GetDlgItem( hwnd, IDC_VIDEO );
  ::SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"Software" );
  ::SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"OpenGL" );
  if(myGlobalData.settings().getString("video") == "soft")
    videomode = 0;
  else if(myGlobalData.settings().getString("video") == "gl")
    videomode = 1;
  ::SendMessage( hwndCtrl, CB_SETCURSEL, videomode, 0 );

  // Set up Aspect Ratio
  hwndCtrl = ::GetDlgItem( hwnd, IDC_ASPECT );
  ::SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
  ::SetWindowText( hwndCtrl,
                   myGlobalData.settings().getString("gl_aspect").c_str() );

  // Set up SOUND
  hwndCtrl = ::GetDlgItem( hwnd, IDC_SOUND );
  ::SendMessage( hwndCtrl, BM_SETCHECK,
                 myGlobalData.settings().getBool("sound")
                 ? BST_UNCHECKED : BST_CHECKED, 0 );

  return TRUE;
}

void CConfigPage::OnDestroy( void )
{
}

LONG CConfigPage::OnApply( LPPSHNOTIFY lppsn )
{
  UNUSED_ALWAYS( lppsn );

  // Apply the changes
  // HWND hwnd = lppsn->hdr.hwndFrom; <<-- points to the sheet!

  HWND hwndCtrl;
  TCHAR buffer[ MAX_PATH ];
  string str;
  int i;
  bool b;

  // RomPath
  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_ROMPATH );
  ASSERT( hwndCtrl );
  ::GetWindowText( hwndCtrl, buffer, MAX_PATH );
  myGlobalData.settings().setString( "romdir", buffer );

  // Paddle
  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_PADDLE );
  ASSERT( hwndCtrl );
  myGlobalData.settings().setInt( "paddle",
    ::SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 ) );

  // VideoMode
  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_VIDEO );
  ASSERT( hwndCtrl );
  i = ::SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 );
  if( i == 0 )
    str = "soft";
  else if( i == 1 )
    str = "gl";
  else
    str = "soft";
  myGlobalData.settings().setString( "video", str );

  // AspectRatio
  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_ASPECT );
  ASSERT( hwndCtrl );
  ::GetWindowText( hwndCtrl, buffer, MAX_PATH );
  myGlobalData.settings().setString( "gl_aspect", buffer );

  // Sound
  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_SOUND );
  ASSERT( hwndCtrl );
  if( ::SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
    b = false;
  else
    b = true;
  myGlobalData.settings().setBool( "sound", b );

  // Finally, save the config file
  myGlobalData.settings().saveConfig();

  return PSNRET_NOERROR;
}

BOOL CConfigPage::OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCtl )
{
  UNUSED_ALWAYS( wNotifyCode );
  UNUSED_ALWAYS( hwndCtl );

  if ( wID == IDC_BROWSE )
  {
    CBrowseForFolder bff( m_hwnd, NULL, "Open ROM Folder " );
    if ( bff.SelectFolder() )
      SetDlgItemText( m_hwnd, IDC_ROMPATH, bff.GetSelectedFolder() );
  }

  return FALSE;
}
