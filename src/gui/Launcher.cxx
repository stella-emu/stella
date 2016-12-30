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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
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
  const GUI::Size& s = myOSystem.settings().getSize("launcherres");
  const GUI::Size& d = myOSystem.frameBuffer().desktopSize();
  myWidth = s.w;  myHeight = s.h;

  // The launcher dialog is resizable, within certain bounds
  // We check those bounds now
  myWidth  = std::max(myWidth,  uInt32(FrameBuffer::kFBMinW));
  myHeight = std::max(myHeight, uInt32(FrameBuffer::kFBMinH));
  myWidth  = std::min(myWidth,  uInt32(d.w));
  myHeight = std::min(myHeight, uInt32(d.h));

  myOSystem.settings().setValue("launcherres",
                                GUI::Size(myWidth, myHeight));

  myBaseDialog = new LauncherDialog(myOSystem, *this, 0, 0, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Launcher::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION;
  return myOSystem.frameBuffer().createDisplay(title, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Launcher::selectedRomMD5()
{
  return (static_cast<LauncherDialog*>(myBaseDialog))->selectedRomMD5();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& Launcher::currentNode() const
{
  return (static_cast<LauncherDialog*>(myBaseDialog))->currentNode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::reload()
{
  (static_cast<LauncherDialog*>(myBaseDialog))->reload();
}
