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
// $Id: MainDlg.hxx,v 1.6 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef MAIN_DLG_HXX
#define MAIN_DLG_HXX

#include "resource.h"

class CGlobalData;

#include "StellaXMain.hxx"
#include "CoolCaption.hxx"
#include "TextButton3d.hxx"
#include "HeaderCtrl.hxx"
#include "RoundButton.hxx"

class MainDlg
{
  public:
    enum { IDD = IDD_MAIN };

    MainDlg( CGlobalData& rGlobalData, HINSTANCE hInstance );
    virtual ~MainDlg( void );

    virtual int DoModal( HWND hwndParent );

    operator HWND( void ) const { return myHwnd; }

  private:
    HWND myHwnd;

    CCoolCaption  myCoolCaption;
    CTextButton3d myAppTitle;
    CHeaderCtrl   myHeader;
    CRoundButton  myPlayButton;
    CRoundButton  myHelpButton;
    CRoundButton  myReloadButton;
    CRoundButton  myConfigButton;
    CRoundButton  myExitButton;

    // Message handlers
    BOOL OnInitDialog( void );
    BOOL OnCommand( int id, HWND hwndCtl, UINT codeNotify );
    BOOL OnNotify( int idCtrl, LPNMHDR pnmh );
    BOOL OnEraseBkgnd( HDC hdc );
    HBRUSH OnCtlColorStatic( HDC hdcStatic, HWND hwndStatic );

    // wm_notify handlers
    void OnItemChanged( LPNMLISTVIEW pnmv );
    void OnColumnClick( LPNMLISTVIEW pnmv );

    // cool caption handlers
    void OnDestroy( void );
    void OnNcPaint( HRGN hrgn );
    void OnNcActivate( BOOL fActive );
    void Quit();
    BOOL OnNcLButtonDown( INT nHitTest, POINTS pts );

    // callback methods
    BOOL CALLBACK DialogFunc( UINT uMsg, WPARAM wParam, LPARAM lParam );
    static BOOL CALLBACK StaticDialogFunc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static int CALLBACK ListViewCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

    // internal data
    void UpdateRomList( bool forceReload = false );
    bool PopulateRomList( bool forceReload = false );
    bool LoadRomListFromDisk();
    bool LoadRomListFromCache();

    HINSTANCE myHInstance;

    HWND myHwndList;
    LPARAM ListView_GetItemData( HWND hwndList, int iItem );
    void ListView_SortByColumn( HWND hwndList, int col );
    int  ListView_GetColWidth( HWND hwndList, int col );
    void ListView_Clear();

    HFONT m_hfontRomNote;

    // Stella stuff
    CGlobalData&    myGlobalData;
    CStellaXMain    m_stella;
};

#endif
