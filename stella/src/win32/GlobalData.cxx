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
// $Id: GlobalData.cxx,v 1.3 2004-05-28 23:16:26 stephena Exp $
//============================================================================ 

#include "pch.hxx"
#include "resource.h"

#include "Settings.hxx"
#include "SettingsWin32.hxx"
#include "GlobalData.hxx"


CGlobalData::CGlobalData( HINSTANCE hInstance )
           : myInstance(hInstance)
{
  myPathName[0] = _T('\0');
  mySettings = new SettingsWin32();
}

CGlobalData::~CGlobalData()
{
  if(mySettings)
    delete mySettings;
}
