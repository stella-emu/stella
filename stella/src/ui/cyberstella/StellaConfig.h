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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StellaConfig.h,v 1.2 2003-11-24 01:14:38 stephena Exp $
//============================================================================

#ifndef STELLA_CONFIG_H
#define STELLA_CONFIG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SettingsWin32.hxx"


class StellaConfig : public CDialog
{
  public:
    StellaConfig(SettingsWin32* settings, CWnd* pParent = NULL);

  // Dialog Data
  enum { IDD = IDD_CONFIG_PAGE };


  // Overrides
  public:
    virtual BOOL DestroyWindow();

  protected:
    // DDX/DDV support
    virtual void DoDataExchange(CDataExchange* pDX);

    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnBrowse();
    afx_msg void OnContinue();
    DECLARE_MESSAGE_MAP()

  private:
    // The settings for this session
    SettingsWin32* mySettings;

    // method
    void retrieveData();
};

#endif
