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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: GameList.h,v 1.5 2003-11-24 23:56:10 stephena Exp $
//============================================================================

#ifndef GAME_LIST_H
#define GAME_LIST_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropsSet.hxx"
#include "SettingsWin32.hxx"

/////////////////////////////////////////////////////////////////////////////
// GameList window
class GameList : public CListCtrl
{
  public:
    GameList();
    virtual ~GameList();


    void init(PropertiesSet* newPropertiesSet,
              SettingsWin32* settings, CWnd* newParent);

    CString getCurrentNote();
    CString getPath() { return myRomPath; }
    uInt32 getRomCount() { return myRomCount; }

    virtual BOOL PreTranslateMessage(MSG* pMsg);

    void insertColumns();
    void populateRomList();
    void deleteItemsAndProperties();
    CString getCurrentFile();
    CString getCurrentName();

  private:
    CWnd* myParent;
    PropertiesSet* myPropertiesSet;
    SettingsWin32* mySettings;
    CString myRomPath;
    uInt32 myRomCount;

  private:
    void displayDrives();
    void displayPath();
    Properties* readRomData(CString binFile);

  protected:
    afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemActivate(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
};

#endif
