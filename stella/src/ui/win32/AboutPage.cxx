//
// StellaX
// Jeff Miller 04/27/2000
//
#include "pch.hxx"
#include "AboutPage.hxx"
#include "resource.h"

CHelpPage::CHelpPage(
    ) : \
	CPropertyPage(IDD_ABOUT_PAGE)
{
}

BOOL CHelpPage::OnInitDialog(
	HWND hwnd
    )
{
	m_hlMail_JSM.SubclassDlgItem( hwnd, IDC_EMAIL_JEFFMILL );
	m_hlMail_JSM.SetURL( _T("mailto:miller@zipcon.net?Subject=StellaX") );

	m_hlWWW_JSM.SubclassDlgItem( hwnd, IDC_WEB_JEFFMILL );
	m_hlWWW_JSM.SetURL( _T("http://www.emuunlim.com/stellax/") );

	m_hlMail_Stella.SubclassDlgItem( hwnd, IDC_EMAIL_STELLA );
	m_hlMail_Stella.SetURL( _T("mailto:stella@csc.ncsu.edu") );

	m_hlWWW_Stella.SubclassDlgItem( hwnd, IDC_WEB_STELLA );
	m_hlWWW_Stella.SetURL( _T("http://stella.atari.org/") );

	m_hlWWW_Mame.SubclassDlgItem( hwnd, IDC_WWW_MAME );
	m_hlWWW_Mame.SetURL( _T("http://www.classicgaming.com/mame32qa/") );

	// return FALSE if SetFocus is called
	return TRUE;
}
