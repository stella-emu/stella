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
// $Id: Launcher.cxx,v 1.27 2009-01-14 20:31:07 stephena Exp $
//============================================================================

#include "LauncherDialog.hxx"
#include "Version.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "FrameBuffer.hxx"
#include "bspf.hxx"

#include "Launcher.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::Launcher(OSystem* osystem)
  : DialogContainer(osystem)
{
  myOSystem->settings().getSize("launcherres", (int&)myWidth, (int&)myHeight);

  // The launcher dialog is resizable, within certain bounds
  // We check those bounds now
  myWidth  = BSPF_max(myWidth, osystem->desktopWidth() >= 640 ? 640u : 320u);
  myHeight = BSPF_max(myHeight, osystem->desktopHeight() >= 480 ? 480u : 240u);
  myWidth  = BSPF_min(myWidth, osystem->desktopWidth());
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
