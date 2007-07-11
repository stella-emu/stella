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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Launcher.cxx,v 1.16 2007-07-11 15:08:13 stephena Exp $
//============================================================================

#include <sstream>

#include "LauncherDialog.hxx"
#include "Version.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "FrameBuffer.hxx"
#include "bspf.hxx"
#include "Launcher.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::Launcher(OSystem* osystem)
  : DialogContainer(osystem),
    myWidth(320),
    myHeight(240)
{
  int w, h;
  myOSystem->settings().getSize("launcherres", w, h);
  myWidth = MAX(w, 0);
  myHeight = MAX(h, 0);

  // Error check the resolution
  myWidth = MAX(myWidth, 320u);
  myWidth = MIN(myWidth, osystem->desktopWidth());
  myHeight = MAX(myHeight, 240u);
  myHeight = MIN(myHeight, osystem->desktopHeight());
  myOSystem->settings().setSize("launcherres", myWidth, myHeight);

  myBaseDialog = new LauncherDialog(myOSystem, this, 0, 0, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::~Launcher()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION;
  myOSystem->frameBuffer().initialize(title, myWidth, myHeight);
}
