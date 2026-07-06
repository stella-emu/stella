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
#include "TIAConstants.hxx"
#include "TiaWindowDialog.hxx"
#include "TiaWindow.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWindow::TiaWindow(OSystem& osystem)
  : DialogContainer(osystem)
{
  // Size the window so the ENTIRE TIA output buffer fits at 2x zoom with no
  // downscaling, for the highest possible frame (hence any NTSC/PAL/custom
  // layout).  The buffer is frameBufferWidth x frameBufferHeight 'narrow'
  // pixels (160 x 320); horizontally the narrow pixels are always doubled for
  // the correct aspect (-> 320 x 320), and the 2x zoom doubles both axes
  // (-> 640 x 640).  Border 'chrome' is added so the image is never squeezed;
  // room for on-screen controls will be added here later.  TiaDisplayWidget
  // scales the current frame to fit, so it renders at 2x by default.  This is
  // also the intended minimum size once the window becomes resizable.
  static constexpr uInt32 zoom = 2;
  static constexpr uInt32 chrome = 2 * (2 + 1);  // dialog (2px) + widget (1px) borders
  mySize = Common::Size(
    TIAConstants::frameBufferWidth  * 2 * zoom + chrome,   // 160*2*2 + 6 = 646
    TIAConstants::frameBufferHeight     * zoom + chrome);  // 320*2   + 6 = 646
  myBaseDialog = std::make_unique<TiaWindowDialog>(myOSystem, *this, 0, 0,
                                                   static_cast<int>(mySize.w),
                                                   static_cast<int>(mySize.h));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWindow::~TiaWindow() = default;
