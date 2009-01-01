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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Launcher.cxx,v 1.26 2009-01-01 18:13:38 stephena Exp $
//============================================================================

#include <sstream>

class Properties;

#include "LauncherDialog.hxx"
#include "Version.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "FrameBuffer.hxx"
#include "bspf.hxx"
#include "Launcher.hxx"

#ifdef SMALL_SCREEN
  #define MIN_WIDTH  320
  #define MIN_HEIGHT 240
#else
  #define MIN_WIDTH  640
  #define MIN_HEIGHT 480
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::Launcher(OSystem* osystem)
  : DialogContainer(osystem),
    myWidth(MIN_WIDTH),
    myHeight(MIN_HEIGHT)
{
  int w, h;
  myOSystem->settings().getSize("launcherres", w, h);
  myWidth = BSPF_max(w, 0);
  myHeight = BSPF_max(h, 0);

  // Error check the resolution
  myWidth = BSPF_max(myWidth, (uInt32)MIN_WIDTH);
  myWidth = BSPF_min(myWidth, osystem->desktopWidth());
  myHeight = BSPF_max(myHeight, (uInt32)MIN_HEIGHT);
  myHeight = BSPF_min(myHeight, osystem->desktopHeight());
  myOSystem->settings().setSize("launcherres", myWidth, myHeight);

  myBaseDialog = new LauncherDialog(myOSystem, this, 0, 0, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::~Launcher()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Launcher::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION;
  return myOSystem->frameBuffer().initialize(title, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Launcher::selectedRomMD5()
{
  return ((LauncherDialog*)myBaseDialog)->selectedRomMD5();
}
