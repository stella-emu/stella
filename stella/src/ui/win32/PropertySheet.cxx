//
// StellaX
// Jeff Miller 05/02/2000
//
#include "pch.hxx"
#include "PropertySheet.hxx"

#pragma comment(lib, "comctl32")

#include <pshpack1.h>
typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>

CPropertyPage::CPropertyPage(
	UINT nIDTemplate,
	UINT nIDCaption /* = 0 */
    )
{
    UNUSED_ALWAYS( nIDCaption );

	ZeroMemory(&m_psp, sizeof(m_psp));
	m_psp.dwSize = sizeof(m_psp);
	m_psp.dwFlags = PSP_USECALLBACK;
	// m_psp.hInstance = hInstance;
	m_psp.pszTemplate = MAKEINTRESOURCE(nIDTemplate);
	m_psp.pfnDlgProc = StaticDlgProc;
	m_psp.lParam = (LPARAM)this;
	m_psp.pfnCallback = StaticCallback;
}

UINT CALLBACK CPropertyPage::StaticCallback(
	HWND hwnd, 
	UINT uMsg, 
	LPPROPSHEETPAGE ppsp
    )
{
    UNUSED_ALWAYS( hwnd );
    UNUSED_ALWAYS( ppsp );

	switch (uMsg)
	{
	case PSPCB_CREATE:
		// ppsp->lParam holds create lParam
		return TRUE;

	case PSPCB_RELEASE:
		break;
	}

	return 0;
}

BOOL CALLBACK CPropertyPage::StaticDlgProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam
    )
{
	CPropertyPage* pPage = NULL;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        pPage = reinterpret_cast<CPropertyPage*>( 
            reinterpret_cast<PROPSHEETPAGE*>( lParam )->lParam );
        (void)::SetWindowLong( hwnd, 
                               DWL_USER,
                               reinterpret_cast<LONG>( pPage ) );
        break;

    default:
        pPage = reinterpret_cast<CPropertyPage*>( 
            ::GetWindowLong( hwnd, DWL_USER ) );
        if ( pPage == NULL )
        {
            return FALSE;
        }
        break;
    }

    return pPage->DlgProc( hwnd, uMsg, wParam, lParam );
}

BOOL CALLBACK CPropertyPage::DlgProc(
	HWND hwnd, 
	UINT msg, 
	WPARAM wParam, 
	LPARAM lParam
    )
{
	switch (msg)
	{
    case WM_ACTIVATE:
        OnActivate( (UINT)LOWORD(wParam), (HWND)lParam, (BOOL)HIWORD(wParam) );
        return TRUE;

	case WM_INITDIALOG:
		return OnInitDialog( hwnd );

	case WM_DESTROY:
		OnDestroy( );
		return TRUE;

	case WM_COMMAND:
		return OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND)lParam );

	case WM_NOTIFY:
		// Handle PSN_QUERYCANCEL?
		// Handle PSN_KILLACTIVE?

		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_SETACTIVE:
			::SetWindowLong(hwnd, DWL_MSGRESULT, OnSetActive( (LPPSHNOTIFY)lParam ) );
			return TRUE;

		case PSN_KILLACTIVE:
			::SetWindowLong(hwnd, DWL_MSGRESULT, OnKillActive( (LPPSHNOTIFY)lParam ) );
			return TRUE;

		case PSN_APPLY:
            ::SetWindowLong( hwnd, DWL_MSGRESULT, OnApply( (LPPSHNOTIFY)lParam) );
			return TRUE;

		case PSN_RESET:
			OnReset( (LPPSHNOTIFY)lParam );
			return TRUE;
		}

		return OnNotify( (int)wParam, (LPNMHDR)lParam );
	}

	return FALSE;
}


//
// CPropertySheet
//

CPropertySheet::CPropertySheet
(
	LPCTSTR pszCaption,
	HWND hwndParent,
	UINT nStartPage /* = 0 */
) :\
	m_strCaption( pszCaption )
{
	ZeroMemory(&m_psh, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
	m_psh.hwndParent = hwndParent;
	m_psh.hInstance = (HINSTANCE)GetWindowLong(hwndParent, GWL_HINSTANCE);
	m_psh.pszCaption = m_strCaption.c_str();
	m_psh.nStartPage = nStartPage;
	m_psh.pfnCallback = StaticCallback;
}

CPropertySheet::~CPropertySheet(
    )
{
    // BUGBUG: This is static!

    /*
    if ( m_hfontLogo )
    {
        ::DeleteObject( m_hfontLogo );
        m_hfontLogo = NULL;
    }
    */
}

void CPropertySheet::AddPage(
	CPropertyPage* pPage
    )
{
	if (pPage)
	{
		m_pages.push_back(pPage);
	}
}

int CPropertySheet::DoModal(
	void
    )
{
	int nSize = m_pages.size();

	if (nSize == 0)
		return IDCANCEL;

	PROPSHEETPAGE* pspage = new PROPSHEETPAGE[nSize];
	for (int i = 0; i < nSize; ++i)
	{
		CopyMemory(&(pspage[i]), m_pages[i]->GetPropSheetPage(), 
			sizeof(PROPSHEETPAGE));
		pspage[i].hInstance = m_psh.hInstance;
	}

	m_psh.nPages = nSize;
	m_psh.ppsp = pspage;

	int nRet = ::PropertySheet( &m_psh );

	delete[] pspage;

	return nRet;
}

int CALLBACK CPropertySheet::StaticCallback(
	HWND hwnd, 
	UINT uMsg, 
	LPARAM lParam
    )
{
    UNUSED_ALWAYS( hwnd );

	switch (uMsg)
	{
	case PSCB_INITIALIZED:
		// Property sheet is being initialized

        return TRUE;

	case PSCB_PRECREATE:
		// Property sheet is about to be created


        //
        // Remove the DS_CONTEXTHELP style from the dialog template
        // (This will remove the "?" in the top right corner)
        //

        if ( ( (LPDLGTEMPLATEEX)lParam )->signature == 0xFFFF)
        {
            ( (LPDLGTEMPLATEEX)lParam )->style &= ~DS_CONTEXTHELP;
        }
        else 
        {
            ( (LPDLGTEMPLATE)lParam )->style &= ~DS_CONTEXTHELP;
        }
        return TRUE;

	}
	
	return 0;
}
