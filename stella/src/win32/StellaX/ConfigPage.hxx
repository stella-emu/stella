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
// $Id: ConfigPage.hxx,v 1.1 2004-06-28 23:13:54 stephena Exp $
//============================================================================ 

#ifndef CONFIGPG_H
#define CONFIGPG_H

#include "PropertySheet.hxx"
#include "GlobalData.hxx"

class CConfigPage : public CPropertyPage
{
  public:
    CConfigPage( CGlobalData& rGlobalData );

  protected:
    virtual BOOL OnInitDialog( HWND hwnd );
    virtual void OnDestroy();
    virtual LONG OnApply( LPPSHNOTIFY lppsn );

    virtual BOOL OnCommand( WORD /* wNotifyCode */, WORD /* wID */, HWND /* hwndCtl */ );

  private:
    CGlobalData& myGlobalData;
    HWND m_hwnd;

    CConfigPage( const CConfigPage& );     // no implementation
    void operator=( const CConfigPage& );  // no implementation
};

#endif
