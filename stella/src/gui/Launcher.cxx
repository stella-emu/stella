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
// $Id: Launcher.cxx,v 1.11 2007-01-01 18:04:53 stephena Exp $
//============================================================================

#include "Version.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "FrameBuffer.hxx"
#include "LauncherDialog.hxx"
#include "bspf.hxx"
#include "Launcher.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::Launcher(OSystem* osystem)
  : DialogContainer(osystem),
    myWidth(320),
    myHeight(240)
{
  int size = myOSystem->settings().getInt("launchersize");
  switch(size)
  {
    case 1:
      myWidth  = 320;
      myHeight = 240;
      break;
    case 2:
      myWidth  = 400;
      myHeight = 300;
      break;
    case 3:
      myWidth  = 512;
      myHeight = 384;
      break;
  }
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
  myOSystem->frameBuffer().initialize(title, myWidth, myHeight, false);
}
