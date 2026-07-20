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
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "TIAConstants.hxx"
#include "TiaWindowDialog.hxx"
#include "TiaWindow.hxx"

// Border 'chrome' around the image: dialog (2px) + widget (1px) borders, so it
// is never squeezed; room for on-screen controls will be added here later
static constexpr uInt32 CHROME = 2 * (2 + 1);

// The window size that shows the ENTIRE TIA output buffer at the given zoom,
// for the highest possible frame (hence any NTSC/PAL/custom layout).  The buffer
// is frameBufferWidth x frameBufferHeight 'narrow' pixels (160 x 320);
// horizontally the narrow pixels are always doubled for the correct aspect
static constexpr Common::Size zoomedSize(uInt32 zoom)
{
  return Common::Size(TIAConstants::frameBufferWidth  * 2 * zoom + CHROME,
                      TIAConstants::frameBufferHeight     * zoom + CHROME);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size TiaWindow::minSize()
{
  // At 1x; TiaDisplayWidget scales its image to fit whatever area it is given,
  // so there is nothing to clip below this
  return zoomedSize(1);  // 326 x 326
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size TiaWindow::defaultSize()
{
  // At 2x, with no downscaling; TiaDisplayWidget renders at 2x here
  return zoomedSize(2);  // 646 x 646
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWindow::TiaWindow(OSystem& osystem)
  : DialogContainer(osystem)
{
  // The window opens at whatever size it was last dragged to, within bounds
  const Common::Size& d = myOSystem.frameBuffer().desktopSize(BufferType::TiaWindow);
  const Common::Size& m = minSize();

  mySize = myOSystem.settings().getSize("tiawindow.res");
  mySize.clamp(m.w, d.w, m.h, d.h);

  myBaseDialog = std::make_unique<TiaWindowDialog>(myOSystem, *this,
                                                   static_cast<int>(mySize.w),
                                                   static_cast<int>(mySize.h));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWindow::~TiaWindow() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaWindow::applyResize()
{
  FrameBuffer& fb = myOSystem.frameBuffer();

  // Nothing to do unless a new size is pending
  if(!fb.applyLiveResize())
    return false;

  // Derive the new (logical) size from the updated window.  Unlike the debugger
  // this is not throttled: the window holds a single scale-to-fit image, so a
  // re-flow is cheap, and applying every event means the drag always ends on
  // the size the user released at
  const uInt32 scale = fb.hidpiScaleFactor();
  const Common::Rect& r = fb.imageRect();
  const Common::Size& d = fb.desktopSize(BufferType::TiaWindow);
  const Common::Size& m = minSize();

  mySize = Common::Size(r.w() / scale, r.h() / scale);
  mySize.clamp(m.w, d.w, m.h, d.h);

  myOSystem.settings().setValue("tiawindow.res", mySize);
  relayout();
  return true;
}
