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
// $Id: ConfigPage.cxx,v 1.5 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "ConfigPage.hxx"
#include "resource.h"
#include "BrowseForFolder.hxx"

#include "bspf.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CConfigPage::CConfigPage( CGlobalData& rGlobalData )
           : myGlobalData( rGlobalData ),
             CPropertyPage( IDD_CONFIG_PAGE )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CConfigPage::OnInitDialog( HWND hwnd )
{
  m_hwnd = hwnd;
  HWND hwndCtrl;
    
  // Reload settings just in case the emulation changed them
  myGlobalData.settings().loadConfig();

  // Get romdir
  hwndCtrl = GetDlgItem( hwnd, IDC_ROMPATH );
  SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
  SetWindowText( hwndCtrl, myGlobalData.settings().getString("romdir").c_str() );

  // Get ssname
  int ssname = 0;
  hwndCtrl = GetDlgItem( hwnd, IDC_SNAPSHOT_TYPE );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"ROM Name" );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"ROM MD5" );
  if(myGlobalData.settings().getString("ssname") == "romname")
    ssname = 0;
  else if(myGlobalData.settings().getString("ssname") == "md5sum")
    ssname = 1;
  SendMessage( hwndCtrl, CB_SETCURSEL, ssname, 0 );

  // Get sssingle
  hwndCtrl = GetDlgItem( hwnd, IDC_SNAPSHOT_MULTIPLE );
  SendMessage( hwndCtrl, BM_SETCHECK,
               myGlobalData.settings().getBool("sssingle")
               ? BST_UNCHECKED : BST_CHECKED, 0 );

  // Get paddle
  hwndCtrl = GetDlgItem( hwnd, IDC_PADDLE );
  TCHAR psz[4] = _T("0");
  TCHAR i;
  for ( i = 0; i < 4; ++i )
  {
    psz[0] = _T('0') + i;
    SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)psz );
  }
  SendMessage( hwndCtrl, CB_SETCURSEL, myGlobalData.settings().getInt("paddle"), 0 );

  // Get video
  int videomode = 0;
  hwndCtrl = GetDlgItem( hwnd, IDC_VIDEO );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"Software" );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"OpenGL" );
  if(myGlobalData.settings().getString("video") == "soft")
    videomode = 0;
  else if(myGlobalData.settings().getString("video") == "gl")
    videomode = 1;
  SendMessage( hwndCtrl, CB_SETCURSEL, videomode, 0 );

  // Get gl_aspect
  hwndCtrl = GetDlgItem( hwnd, IDC_GL_ASPECT );
  SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
  SetWindowText( hwndCtrl, myGlobalData.settings().getString("gl_aspect").c_str() );

  // Get gl_fsmax
  hwndCtrl = GetDlgItem( hwnd, IDC_GL_FSMAX );
  SendMessage( hwndCtrl, BM_SETCHECK,
               myGlobalData.settings().getBool("gl_fsmax")
               ? BST_CHECKED : BST_UNCHECKED, 0 );

  // Get volume
//  hwndCtrl = GetDlgItem( hwnd, IDC_SOUND_VOLUME_SPIN );
//  CSpinButtonCtrl spin = (CSpinButtonCtrl) hwndCtrl;
  hwndCtrl = GetDlgItem( hwnd, IDC_SOUND_VOLUME );
  SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
  SetWindowText( hwndCtrl, myGlobalData.settings().getString("volume").c_str() );

  // Get fragsize
  int fragindex = 2, fragsize = 2048;
  hwndCtrl = GetDlgItem( hwnd, IDC_SOUND_FRAGSIZE );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"512" );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"1024" );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"2048" );
  SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)"4096" );
  fragsize = myGlobalData.settings().getInt("fragsize");
  if(fragsize == 512)
    fragindex = 0;
  else if(fragsize == 1024)
    fragindex = 1;
  else if(fragsize == 2048)
    fragindex = 2;
  else if(fragsize == 4096)
    fragindex = 3;
  SendMessage( hwndCtrl, CB_SETCURSEL, fragindex, 0 );

  // Get sound
  hwndCtrl = GetDlgItem( hwnd, IDC_SOUND_ENABLE );
  SendMessage( hwndCtrl, BM_SETCHECK,
               myGlobalData.settings().getBool("sound")
               ? BST_UNCHECKED : BST_CHECKED, 0 );

  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CConfigPage::OnDestroy( void )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
LONG CConfigPage::OnApply( LPPSHNOTIFY lppsn )
{
  UNUSED_ALWAYS( lppsn );

  HWND hwndCtrl;
  TCHAR buffer[ MAX_PATH ];
  string str;
  int i;
  bool b;

  // Set romdir
  hwndCtrl = GetDlgItem( m_hwnd, IDC_ROMPATH );
  ASSERT( hwndCtrl );
  GetWindowText( hwndCtrl, buffer, MAX_PATH );
  myGlobalData.settings().setString( "romdir", buffer );

  // Set ssname
  hwndCtrl = GetDlgItem( m_hwnd, IDC_SNAPSHOT_TYPE );
  ASSERT( hwndCtrl );
  i = SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 );
  if( i == 0 )
    str = "romname";
  else if( i == 1 )
    str = "md5sum";
  else
    str = "romname";
  myGlobalData.settings().setString( "ssname", str );

  // Set sssingle
  hwndCtrl = GetDlgItem( m_hwnd, IDC_SNAPSHOT_MULTIPLE );
  ASSERT( hwndCtrl );
  if( SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
    b = false;
  else
    b = true;
  myGlobalData.settings().setBool( "sssingle", b );

  // Set paddle
  hwndCtrl = GetDlgItem( m_hwnd, IDC_PADDLE );
  ASSERT( hwndCtrl );
  myGlobalData.settings().setInt( "paddle",
    SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 ) );

  // Set video
  hwndCtrl = GetDlgItem( m_hwnd, IDC_VIDEO );
  ASSERT( hwndCtrl );
  i = ::SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 );
  if( i == 0 )
    str = "soft";
  else if( i == 1 )
    str = "gl";
  else
    str = "soft";
  myGlobalData.settings().setString( "video", str );

  // Set gl_aspect
  hwndCtrl = GetDlgItem( m_hwnd, IDC_GL_ASPECT );
  ASSERT( hwndCtrl );
  GetWindowText( hwndCtrl, buffer, MAX_PATH );
  myGlobalData.settings().setString( "gl_aspect", buffer );

  // Set gl_fsmax
  hwndCtrl = GetDlgItem( m_hwnd, IDC_GL_FSMAX );
  ASSERT( hwndCtrl );
  if( SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
    b = true;
  else
    b = false;
  myGlobalData.settings().setBool( "gl_fsmax", b );

  // Set volume
  hwndCtrl = GetDlgItem( m_hwnd, IDC_SOUND_VOLUME );
  ASSERT( hwndCtrl );
  GetWindowText( hwndCtrl, buffer, MAX_PATH );
  myGlobalData.settings().setString( "volume", buffer );

  // Set fragsize
  hwndCtrl = GetDlgItem( m_hwnd, IDC_SOUND_FRAGSIZE );
  ASSERT( hwndCtrl );
  i = SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 );
  if( i == 0 )
    str = "512";
  else if( i == 1 )
    str = "1024";
  else if( i == 2 )
    str = "2048";
  else if( i == 3 )
    str = "4096";
  else
    str = "2048";
  myGlobalData.settings().setString( "fragsize", str );

  // Set sound
  hwndCtrl = GetDlgItem( m_hwnd, IDC_SOUND_ENABLE );
  ASSERT( hwndCtrl );
  if( SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
    b = false;
  else
    b = true;
  myGlobalData.settings().setBool( "sound", b );

  // Finally, save the config file
  myGlobalData.settings().saveConfig();

  return PSNRET_NOERROR;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
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
