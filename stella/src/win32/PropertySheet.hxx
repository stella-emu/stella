//
// StellaX
// Jeff Miller 05/02/2000
//
#ifndef PROPSHT_H
#define PROPSHT_H
#pragma once

#include <prsht.h>
#include <vector>
#include <string>

class CPropertyPage
{
public:

	CPropertyPage( UINT nIDTemplate, UINT nIDCaption = 0 );

	const PROPSHEETPAGE* GetPropSheetPage();

protected:

    //
    // Overridable callbacks
    //

    virtual void OnActivate( UINT state, HWND hwndActDeact, BOOL fMinimized )
        {
        }

	virtual BOOL OnInitDialog( HWND /* hwnd */ )
        {
            return FALSE;
        }

	virtual void OnDestroy( void )
        {
        }

    virtual BOOL OnSetActive( LPPSHNOTIFY /* lppsn */ )
        {
            return 0;
        }

	virtual BOOL OnKillActive( LPPSHNOTIFY /* lppsn */ )
        {
            return FALSE;
        }

	virtual LONG OnApply( LPPSHNOTIFY /* lppsn */ )
        {
            return PSNRET_NOERROR;
        }

	virtual void OnReset( LPPSHNOTIFY /* lppsn */ )
        {
        }

	virtual BOOL OnCommand( WORD /* wNotifyCode */, WORD /* wID */, HWND /* hwndCtl */ )
        {
            return FALSE;
        }

	virtual BOOL OnNotify( int /* idCtrl */, LPNMHDR /* pnmh */ )
        {
            return FALSE;
        }

private:

	static UINT CALLBACK StaticCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

	static BOOL CALLBACK StaticDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
    BOOL CALLBACK DlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

	PROPSHEETPAGE m_psp;

	CPropertyPage( const CPropertyPage& );  // no implementation
	void operator=( const CPropertyPage& );  // no implementation
};

inline const PROPSHEETPAGE* CPropertyPage::GetPropSheetPage(
	void
    )
{
	return &m_psp;
}

class CPropertySheet
{
public:

	CPropertySheet( LPCTSTR pszCaption, HWND hwndParent, UINT nStartPage = 0 );
	~CPropertySheet( );

	void AddPage( CPropertyPage* pPage );
	virtual int DoModal( void );

private:

	static int CALLBACK StaticCallback( HWND hwndDlg, UINT uMsg, LPARAM lParam );

	PROPSHEETHEADER m_psh;
	std::vector<CPropertyPage*> m_pages;
	std::string m_strCaption;

	CPropertySheet( const CPropertySheet& );  // no implementation
	void operator=( const CPropertySheet& );  // no implementation
};

#endif
