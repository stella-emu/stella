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
// $Id: MainDlg.hxx,v 1.3 2004-07-10 22:25:58 stephena Exp $
//============================================================================ 

#ifndef __MAINDLG_H_
#define __MAINDLG_H_

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

    virtual int DoModal( HWND hwndParent );

    operator HWND( void ) const { return myHwnd; }

  private:
    HWND myHwnd;

    CCoolCaption  m_CoolCaption;
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
//    void OnColumnClick( LPNMLISTVIEW pnmv );

    // cool caption handlers
    void OnDestroy( void );
    void OnNcPaint( HRGN hrgn );
    void OnNcActivate( BOOL fActive );
    BOOL OnNcLButtonDown( INT nHitTest, POINTS pts );

    // callback methods
    BOOL CALLBACK DialogFunc( UINT uMsg, WPARAM wParam, LPARAM lParam );
    static BOOL CALLBACK StaticDialogFunc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

    // internal data
    void UpdateRomList();
    bool PopulateRomList();
    bool LoadRomListFromDisk();
    bool LoadRomListFromCache();

    HINSTANCE m_hInstance;

    // stuff in list
    HWND myHwndList;
    LPARAM ListView_GetItemData( HWND hwndList, int iItem );
    void ListView_Clear();

    HFONT m_hfontRomNote;

    // Stella stuff
    CGlobalData&    myGlobalData;
    CStellaXMain    m_stella;
};

#endif
