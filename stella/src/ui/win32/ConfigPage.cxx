//
// StellaX
// Jeff Miller 05/07/2000
//

#include "pch.hxx"
#include "ConfigPage.hxx"
#include "resource.h"

#include "BrowseForFolder.hxx"

CConfigPage::CConfigPage(
    CGlobalData* rGlobalData
    ) : \
    m_rGlobalData( rGlobalData ),
	CPropertyPage( IDD_CONFIG_PAGE )
{
}

BOOL CConfigPage::OnInitDialog(
	HWND hwnd
    )
{
	// return FALSE if SetFocus is called

    m_hwnd = hwnd;

    HWND hwndCtrl;
    
    //
    // Set up ROMPATH
    //

    hwndCtrl = ::GetDlgItem( hwnd, IDC_ROMPATH );
    ::SendMessage( hwndCtrl, EM_LIMITTEXT, MAX_PATH, 0 );
    ::SetWindowText( hwndCtrl, m_rGlobalData->RomDir() );

    //
    // Set up PADDLE
    //

    hwndCtrl = ::GetDlgItem( hwnd, IDC_PADDLE );

    LPTSTR psz = _T("0");
    TCHAR i;
    for ( i = 0; i < 4; ++i )
    {
        psz[0] = _T('0') + i;
        ::SendMessage( hwndCtrl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)psz );
    }

    ::SendMessage( hwndCtrl, CB_SETCURSEL, 
                   m_rGlobalData->PaddleMode(), 0 );

    //
    // Set up SOUND
    //

    hwndCtrl = ::GetDlgItem( hwnd, IDC_SOUND );
    ::SendMessage( hwndCtrl, BM_SETCHECK, 
                   m_rGlobalData->NoSound() ? BST_CHECKED : BST_UNCHECKED,
                   0 );

    //
    // Set up AutoSelectVideoMode
    //


    hwndCtrl = ::GetDlgItem( hwnd, IDC_AUTO_SELECT_VIDEOMODE );
    ::SendMessage( hwndCtrl, BM_SETCHECK, 
                   m_rGlobalData->AutoSelectVideoMode() ? BST_CHECKED : BST_UNCHECKED,
                   0 );

    //
    // Set up JOYSTICK
    //

    hwndCtrl = ::GetDlgItem( hwnd, IDC_JOYSTICK );
    ::SendMessage( hwndCtrl, BM_SETCHECK, 
                   m_rGlobalData->DisableJoystick() ? BST_CHECKED : BST_UNCHECKED,
                   0 );

	return TRUE;
}

void CConfigPage::OnDestroy(
	void
    )
{
}

LONG CConfigPage::OnApply(
    LPPSHNOTIFY lppsn 
    )
{
    UNUSED_ALWAYS( lppsn );

    //
    // Apply the changes
    //

    // HWND hwnd = lppsn->hdr.hwndFrom; <<-- points to the sheet!

    HWND hwndCtrl;

    hwndCtrl = ::GetDlgItem( m_hwnd, IDC_ROMPATH );
    ASSERT( hwndCtrl );
    ::GetWindowText( hwndCtrl, m_rGlobalData->m_pszRomDir, MAX_PATH );

    hwndCtrl = ::GetDlgItem( m_hwnd, IDC_PADDLE );
    ASSERT( hwndCtrl );
    m_rGlobalData->m_nPaddleMode = ::SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0 );

    hwndCtrl = ::GetDlgItem( m_hwnd, IDC_SOUND );
    ASSERT( hwndCtrl );
    m_rGlobalData->m_fNoSound = ( ::SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 )
        == BST_CHECKED );

    hwndCtrl = ::GetDlgItem( m_hwnd, IDC_AUTO_SELECT_VIDEOMODE );
    ASSERT( hwndCtrl );
    m_rGlobalData->m_fAutoSelectVideoMode = ( ::SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 )
        == BST_CHECKED );

    hwndCtrl = ::GetDlgItem( m_hwnd, IDC_JOYSTICK );
    ASSERT( hwndCtrl );
    m_rGlobalData->m_fDisableJoystick= ( ::SendMessage( hwndCtrl, BM_GETCHECK, 0, 0 )
        == BST_CHECKED );

    m_rGlobalData->SetModified();

    return PSNRET_NOERROR;
}

BOOL CConfigPage::OnCommand( 
    WORD wNotifyCode, 
    WORD wID, 
    HWND hwndCtl
    )
{
    UNUSED_ALWAYS( wNotifyCode );
    UNUSED_ALWAYS( hwndCtl );

    if ( wID == IDC_BROWSE )
    {
        CBrowseForFolder bff( m_hwnd );
        bff.SetFlags( BIF_RETURNONLYFSDIRS );
        if ( bff.SelectFolder() )
        {
            ::SetDlgItemText( m_hwnd, IDC_ROMPATH, bff.GetSelectedFolder() );
        }
    }

    return FALSE;
}
