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
// $Id: ConfigPage.hxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef CONFIG_PAGE_HXX
#define CONFIG_PAGE_HXX

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
