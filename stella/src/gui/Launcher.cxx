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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Launcher.cxx,v 1.5 2005-05-11 19:36:00 stephena Exp $
//============================================================================

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "LauncherDialog.hxx"
#include "bspf.hxx"
#include "Launcher.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::Launcher(OSystem* osystem)
    : DialogContainer(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::~Launcher()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::initialize()
{
  // We only create one instance of this dialog, since each time we do so,
  // the ROM listing is read from disk.  This can be very expensive.
  if(myBaseDialog == NULL)
    myBaseDialog = new LauncherDialog(myOSystem, this,
                                      0, 0, kLauncherWidth, kLauncherHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::initializeVideo()
{
  string title = string("Stella version ") + STELLA_VERSION;
  myOSystem->frameBuffer().initialize(title, kLauncherWidth, kLauncherHeight, false);
}
