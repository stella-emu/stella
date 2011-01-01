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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "LauncherDialog.hxx"
#include "Version.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "FSNode.hxx"
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
FBInitStatus Launcher::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION;
  return myOSystem->frameBuffer().initialize(title, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Launcher::selectedRomMD5()
{
  return ((LauncherDialog*)myBaseDialog)->selectedRomMD5();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& Launcher::currentNode() const
{
  return ((LauncherDialog*)myBaseDialog)->currentNode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::reload()
{
  ((LauncherDialog*)myBaseDialog)->reload();
}
