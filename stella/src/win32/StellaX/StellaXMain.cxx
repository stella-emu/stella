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
// $Id: StellaXMain.cxx,v 1.2 2004-07-04 20:16:03 stephena Exp $
//============================================================================ 

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <windows.h>
#include <shellapi.h>

#include "resource.h"
#include "GlobalData.hxx"
#include "Settings.hxx"
#include "pch.hxx"
#include "StellaXMain.hxx"

// CStellaXMain
// equivalent to main() in the DOS version of stella

CStellaXMain::CStellaXMain()
{
}

CStellaXMain::~CStellaXMain()
{
}

void CStellaXMain::PlayROM( LPCTSTR filename, CGlobalData& globaldata )
{
  string rom = filename;

  // Make sure the specfied ROM exists
  if(!globaldata.settings().fileExists(filename))
  {
    ostringstream out;
    out << "\"" << rom << "\" doesn't exist";

    MessageBox( NULL, out.str().c_str(), "Unknown ROM", MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  // Assume that the ROM file does exist, attempt to run external Stella
  // Since all settings are saved to the stella.ini file, we don't need
  // to pass any arguments here ...
  ShellExecute(NULL, "open", "stella.exe", rom.c_str(), NULL, 0);
}
