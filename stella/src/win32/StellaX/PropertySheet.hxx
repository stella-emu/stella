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
// $Id: PropertySheet.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef PROPERTY_SHEET_HXX
#define PROPERTY_SHEET_HXX

#include <prsht.h>
#include <vector>
#include <string>

class CPropertyPage
{
  public:
    CPropertyPage( UINT nIDTemplate, UINT nIDCaption = 0 );

    const PROPSHEETPAGE* GetPropSheetPage();

  protected:
    // Overridable callbacks
    virtual void OnActivate( UINT state, HWND hwndActDeact, BOOL fMinimized ) {}

    virtual BOOL OnInitDialog( HWND /* hwnd */ ) { return FALSE; }

    virtual void OnDestroy( void ) {}

    virtual BOOL OnSetActive( LPPSHNOTIFY /* lppsn */ ) { return 0; }

    virtual BOOL OnKillActive( LPPSHNOTIFY /* lppsn */ ) { return FALSE; }

    virtual LONG OnApply( LPPSHNOTIFY /* lppsn */ ) { return PSNRET_NOERROR; }

    virtual void OnReset( LPPSHNOTIFY /* lppsn */ ) {}

    virtual BOOL OnCommand( WORD /* wNotifyCode */, WORD /* wID */,
                            HWND /* hwndCtl */ ) { return FALSE; }

    virtual BOOL OnNotify( int /* idCtrl */, LPNMHDR /* pnmh */ ) { return FALSE; }

  private:
    static UINT CALLBACK StaticCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

    static BOOL CALLBACK StaticDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
    BOOL CALLBACK DlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

    PROPSHEETPAGE m_psp;

    CPropertyPage( const CPropertyPage& );   // no implementation
    void operator=( const CPropertyPage& );  // no implementation
};

inline const PROPSHEETPAGE* CPropertyPage::GetPropSheetPage()
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
