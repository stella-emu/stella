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

#ifndef MACOSUTILS_HXX
#define MACOSUTILS_HXX

#include <string>

namespace MacOSUtils
{
  // Returns ~/Library/Application Support, or empty string on failure
  std::string applicationSupportPath();

  // Returns ~/Desktop, or empty string on failure
  std::string desktopPath();

  /**
    Stop AppKit rescaling the last drawn frame while a window is dragged,
    which is seen as the contents jittering.

    A layer-backed view's default 'layerContentsPlacement' scales its content
    to the view's new bounds, so between presented frames the compositor shows
    a resampled copy of the old one.  Pinning it to the corner opposite the
    dragged edge leaves it unscaled against the edges that are holding still.
    Same fix and same corner choice as sokol (floooh/sokol#963), taken from the
    window's frame delta rather than the pointer.  SDL3 does not do this as of
    3.4.14 (libsdl-org/SDL#12081).

    @param nsWindow      NSWindow, from SDL_PROP_WINDOW_COCOA_WINDOW_POINTER
    @param metalViewTag  Metal view's tag, from
                         SDL_PROP_WINDOW_COCOA_METAL_VIEW_TAG_NUMBER;
                         windows without such a view are left alone
  */
  void enableMetalLiveResizeAnchoring(void* nsWindow, long metalViewTag);
}

#endif  // MACOSUTILS_HXX
