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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
Launcher::Launcher(OSystem& osystem)
  : DialogContainer(osystem)
{
  const Common::Size& s = myOSystem.settings().getSize("launcherres");
  const Common::Size& d = myOSystem.frameBuffer().desktopSize();
  double overscan = 1 - myOSystem.settings().getInt("tia.fs_overscan") / 100.0;
  myWidth = s.w;  myHeight = s.h;

  // The launcher dialog is resizable, within certain bounds
  // We check those bounds now
  myWidth  = std::max(myWidth,  FBMinimum::Width);
  myHeight = std::max(myHeight, FBMinimum::Height);
  myWidth  = std::min(myWidth,  d.w);
  myHeight = std::min(myHeight, d.h);
  // do not include overscan when launcher saving size
  myOSystem.settings().setValue("launcherres", Common::Size(myWidth, myHeight));
  // now make overscan effective
  myWidth  = std::min(myWidth,  uInt32(d.w * overscan));
  myHeight = std::min(myHeight, uInt32(d.h * overscan));

  myBaseDialog = new LauncherDialog(myOSystem, *this, 0, 0, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::~Launcher()
{
  delete myBaseDialog;  myBaseDialog = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Launcher::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION;
  return myOSystem.frameBuffer().createDisplay(title, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Launcher::selectedRom()
{
  return (static_cast<LauncherDialog*>(myBaseDialog))->selectedRom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Launcher::selectedRomMD5()
{
  return (static_cast<LauncherDialog*>(myBaseDialog))->selectedRomMD5();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& Launcher::currentDir() const
{
  return (static_cast<LauncherDialog*>(myBaseDialog))->currentDir();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::reload()
{
  (static_cast<LauncherDialog*>(myBaseDialog))->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog* Launcher::baseDialog()
{
  return myBaseDialog;
}
