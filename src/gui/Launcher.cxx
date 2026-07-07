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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "LauncherDialog.hxx"
#include "TimerManager.hxx"
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
  loadSize();

  myBaseDialog = std::make_unique<LauncherDialog>(myOSystem, *this, 0, 0,
                                                  mySize.w, mySize.h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::loadSize()
{
  mySize = myOSystem.settings().getSize("launcherres");

  const Common::Size& d = myOSystem.frameBuffer().desktopSize(BufferType::Launcher);
  const double overscan = 1 - myOSystem.settings().getInt("tia.fs_overscan") / 100.0;

  // The launcher dialog is resizable, within certain bounds
  // We check those bounds now
  mySize.clamp(FBMinimum::Width, d.w, FBMinimum::Height, d.h);
  // Do not include overscan when launcher saving size
  myOSystem.settings().setValue("launcherres", mySize);
  // Now make overscan effective
  mySize.w = std::min(mySize.w, static_cast<uInt32>(d.w * overscan));
  mySize.h = std::min(mySize.h, static_cast<uInt32>(d.h * overscan));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Launcher::~Launcher() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Launcher::initializeVideo()
{
  // Pick up any size the user set by resizing the launcher window
  loadSize();

  return myOSystem.frameBuffer().createDisplay(STELLA_FULL_TITLE,
                                               BufferType::Launcher, mySize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Launcher::selectedRom()
{
  return myBaseDialog->selectedRom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Launcher::selectedRomMD5()
{
  return myBaseDialog->selectedRomMD5();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FSNode& Launcher::currentDir() const
{
  return myBaseDialog->currentDir();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::reload()
{
  myBaseDialog->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::quit()
{
  myBaseDialog->quit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog* Launcher::baseDialog()
{
  return myBaseDialog.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Launcher::applyResize()
{
  // Throttle to roughly the display rate: the modal-loop event-watch (and an
  // X11 event flood) can deliver resizes far faster than 60Hz, and a full
  // re-flow per event is wasteful
  static constexpr uInt64 INTERVAL = 1000000 / 60;  // microseconds
  const uInt64 now = TimerManager::getTicks();
  if(now - myLastResizeTime < INTERVAL)
    return false;

  // Nothing to do unless a new size is pending
  if(!myOSystem.frameBuffer().applyLiveResize())
    return false;

  myLastResizeTime = now;
  myDialogStack.applyAll([](Dialog*& d) { d->relayout(); });
  mySettleCountdown = 15;  // ~frames of idle before the resize is settled
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Launcher::updateTime(uInt64 time)
{
  DialogContainer::updateTime(time);

  // Live re-flow is normally applied straight from the event handler
  // (applyResize(), which also covers the Windows/macOS modal loop).  Here we
  // catch any size the handler's throttle skipped — notably the final one when
  // the drag stops — and, once idle, run one settle pass with
  // resizeInProgress() false so layout() performs the finalization it defers
  // during the drag (window minimum-size hint, persisting the final size).
  if(myOSystem.frameBuffer().applyLiveResize())
  {
    myDialogStack.applyAll([](Dialog*& d) { d->relayout(); });
    mySettleCountdown = 15;
  }
  else if(mySettleCountdown > 0 && --mySettleCountdown == 0)
    myDialogStack.applyAll([](Dialog*& d) { d->relayout(); });
}
