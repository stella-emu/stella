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
// $Id: ConfigPage.cxx,v 1.2 2004-05-27 22:02:35 stephena Exp $
//============================================================================ 

#include "pch.hxx"
#include "ConfigPage.hxx"
#include "resource.h"
#include "BrowseForFolder.hxx"

CConfigPage::CConfigPage( CGlobalData& rGlobalData )
           : m_rGlobalData( rGlobalData ),
             CPropertyPage( IDD_CONFIG_PAGE )
{
}

BOOL CConfigPage::OnInitDialog( HWND hwnd )
{
  // return FALSE if SetFocus is called
  m_hwnd = hwnd;
  HWND hwndCtrl;
    
  // Set up ROMPATH
  hwndCtrl = ::GetDlgItem( hwnd, IDC_ROMPATH );
  ::SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
  ::SetWindowText( hwndCtrl, m_rGlobalData.RomDir() );

  // Set up PADDLE
  hwndCtrl = ::GetDlgItem( hwnd, IDC_PADDLE );

  TCHAR psz[4] = _T("0");
  TCHAR i;
  for ( i = 0; i < 4; ++i )
  {
    psz[0] = _T('0') + i;
    ::SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)psz );
  }

  ::SendMessage( hwndCtrl, CB_SETCURSEL, m_rGlobalData.PaddleMode(), 0 );

  // Set up SOUND
  hwndCtrl = ::GetDlgItem( hwnd, IDC_SOUND );
  ::SendMessage( hwndCtrl, BM_SETCHECK, m_rGlobalData.NoSound() ? BST_CHECKED : BST_UNCHECKED, 0 );

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

  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_ROMPATH );
  ASSERT( hwndCtrl );
  ::GetWindowText( hwndCtrl, m_rGlobalData.m_pszRomDir, MAX_PATH );

  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_PADDLE );
  ASSERT( hwndCtrl );
  m_rGlobalData.m_nPaddleMode = ::SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 );

  hwndCtrl = ::GetDlgItem( m_hwnd, IDC_SOUND );
  ASSERT( hwndCtrl );
  m_rGlobalData.m_fNoSound = ( ::SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 ) == BST_CHECKED );

  m_rGlobalData.SetModified();

  return PSNRET_NOERROR;
}

BOOL CConfigPage::OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCtl )
{
  UNUSED_ALWAYS( wNotifyCode );
  UNUSED_ALWAYS( hwndCtl );

  if ( wID == IDC_BROWSE )
  {
    CBrowseForFolder bff( m_hwnd );
    bff.SetFlags( BIF_RETURNONLYFSDIRS );
    if ( bff.SelectFolder() )
      ::SetDlgItemText( m_hwnd, IDC_ROMPATH, bff.GetSelectedFolder() );
  }

  return FALSE;
}
