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
// $Id: StellaXMain.cxx,v 1.5 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>
#include <shellapi.h>

#include "GlobalData.hxx"
#include "Settings.hxx"
#include "pch.hxx"
#include "StellaXMain.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CStellaXMain::CStellaXMain()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CStellaXMain::~CStellaXMain()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CStellaXMain::PlayROM( string& romfile, CGlobalData& globaldata )
{
  ostringstream buf;

  // Make sure the specfied ROM exists
  if(!globaldata.settings().fileExists(romfile))
  {
    buf << "\"" << romfile << "\" doesn't exist";

    MessageBox( NULL, buf.str().c_str(), "Unknown ROM", MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  // Assume that the ROM file does exist, attempt to run external Stella
  // Since all settings are saved to the stella.ini file, we don't need
  // to pass any arguments here ...
  buf << "\"" << romfile << "\"";
  ShellExecute(NULL, "open", "stella.exe", buf.str().c_str(), NULL, 0);
}
