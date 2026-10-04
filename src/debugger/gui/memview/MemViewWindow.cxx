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

#include "OSystem.hxx"
#include "Debugger.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "MemViewWindowDialog.hxx"
#include "MemViewWindow.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size MemViewWindow::minSize() const
{
  // Only a laid-out dialog knows how small the window may be (its layout
  // tree reports it), as in Debugger::dialogMinSize()
  if(myBaseDialog != nullptr)
    return static_cast<MemViewWindowDialog*>(myBaseDialog.get())->minSize();

  return Common::Size(1, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size MemViewWindow::defaultSize()
{
  // Empty, so the constructor clamps it up to the window's minimum
  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewWindow::MemViewWindow(OSystem& osystem)
  : DialogContainer(osystem)
{
  const Common::Size& d = myOSystem.frameBuffer().desktopSize(BufferType::MemViewWindow);

  // Build the dialog at a coarsely clamped size first, then clamp to its
  // real minimum, which only exists once the dialog does (as in
  // Debugger::initialize())
  mySize = myOSystem.settings().getSize("memview.res");
  mySize.clamp(1, d.w, 1, d.h);

  myBaseDialog = std::make_unique<MemViewWindowDialog>(myOSystem, *this,
                                                        I32(mySize.w),
                                                        I32(mySize.h));

  const Common::Size& m = minSize();
  mySize.clamp(m.w, d.w, m.h, d.h);
  myOSystem.settings().setValue("memview.res", mySize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewWindow::~MemViewWindow() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindow::updateMinSize()
{
  // minSize() is whatever the last layout settled on, so re-flow first
  relayout();

  myOSystem.frameBuffer().growWindowTo(window(), minSize());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWindow::applyResize()
{
  FrameBuffer& fb = myOSystem.frameBuffer();

  // Nothing to do unless a new size is pending
  if(!fb.applyLiveResize(window()))
    return false;

  const uInt32 scale = fb.hidpiScaleFactor(window());
  const Common::Rect& r = FrameBuffer::imageRect(window());
  const Common::Size& m = minSize();

  // Follow the window, even past the desktop it opened on (when dragged across
  // monitors); only the minimum applies
  mySize = Common::Size(std::max(r.w() / scale, m.w),
                        std::max(r.h() / scale, m.h));

  myOSystem.settings().setValue("memview.res", mySize);
  relayout();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindow::handleWindowResized(int width, int height)
{
  myOSystem.debugger().resizeMemViewWindow(width, height);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindow::handleWindowClose()
{
  myOSystem.debugger().closeMemViewWindow();
}
