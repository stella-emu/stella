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

#include <cassert>
#include <cmath>
#include <format>

#include "Settings.hxx"
#include "Bezel.hxx"

#include "VideoModeHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoModeHandler::setImageSize(const Common::Size& image)
{
  myImage = image;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoModeHandler::setDisplaySize(const Common::Size& display, bool fullscreen)
{
  myDisplay = display;
  myFullscreen = fullscreen;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VideoModeHandler::Mode&
  VideoModeHandler::buildMode(const Settings& settings, bool inTIAMode,
                              const Bezel::Info& bezelInfo)
{
  const bool windowedRequested = !myFullscreen;

  // TIA mode allows zooming at non-integral factors in most cases
  if(inTIAMode)
  {
    if(windowedRequested)
    {
      const auto zoom = static_cast<double>(settings.getFloat("tia.zoom"));

      // Image and screen (aka window) dimensions are the same
      // Overscan is not applicable in this mode
      myMode = Mode(myImage.w, myImage.h,
                    Mode::Stretch::Fill, myFullscreen,
                    std::format("{:.4g}%", zoom * 100), zoom, bezelInfo);
    }
    else
    {
      const double overscan = 1 - settings.getInt("tia.fs_overscan") / 100.0;

      // First calculate maximum zoom that keeps aspect ratio
      const double scaleX = static_cast<double>(myImage.w) / (myDisplay.w / bezelInfo.ratioW()),
                   scaleY = static_cast<double>(myImage.h) / (myDisplay.h / bezelInfo.ratioH());
      double zoom = 1. / std::max(scaleX, scaleY);

      // When aspect ratio correction is off, we want pixel-exact images,
      // so we default to integer zooming
      if(!settings.getBool("tia.correct_aspect"))
        zoom = static_cast<uInt32>(zoom);

      if(!settings.getBool("tia.fs_stretch"))  // preserve aspect, use all space
      {
        myMode = Mode(myImage.w, myImage.h,
                      myDisplay.w, myDisplay.h,
                      Mode::Stretch::Preserve, myFullscreen,
                      "Fullscreen: Preserve aspect, no stretch",
                      zoom, overscan, bezelInfo);
      }
      else  // ignore aspect, use all space
      {
        myMode = Mode(myImage.w, myImage.h,
                      myDisplay.w, myDisplay.h,
                      Mode::Stretch::Fill, myFullscreen,
                      "Fullscreen: Ignore aspect, full stretch",
                      zoom, overscan, bezelInfo);
      }
    }
  }
  else  // UI mode (no zooming)
  {
    if(windowedRequested)
      myMode = Mode(myImage.w, myImage.h, Mode::Stretch::None);
    else
      myMode = Mode(myImage.w, myImage.h, myDisplay.w, myDisplay.h,
                    Mode::Stretch::None, myFullscreen);
  }
  return myMode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoModeHandler::Mode::Mode(uInt32 iw, uInt32 ih, Stretch smode,
                             bool fs, string_view desc,
                             double zoomLevel, const Bezel::Info& bezelInfo)
  : Mode(iw, ih, iw, ih, smode, fs, desc, zoomLevel, 1., bezelInfo)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoModeHandler::Mode::Mode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh,
                             Stretch smode, bool fs, string_view desc,
                             double zoomLevel, double overscan,
                             const Bezel::Info& bezelInfo)
  : screenS{sw, sh},
    stretch{smode},
    description{desc},
    zoom{zoomLevel},
    fullscreen{fs}
{
  // Now resize based on windowed/fullscreen mode and stretch factor
  if(fullscreen)
  {
    auto rounded = [](double v) noexcept -> uInt32 {
        return static_cast<uInt32>(std::round(v));
    };

    switch(stretch)
    {
      case Stretch::Preserve:
        iw = rounded(iw * overscan * zoomLevel);
        ih = rounded(ih * overscan * zoomLevel);
        break;

      case Stretch::Fill:
        // Scale to all available space
        iw = rounded(screenS.w * overscan / bezelInfo.ratioW());
        ih = rounded(screenS.h * overscan / bezelInfo.ratioH());
        break;

      case Stretch::None: // UI Mode
        // Don't do any scaling at all
        iw = rounded(std::min(static_cast<uInt32>
            (iw * zoomLevel), screenS.w) * overscan);
        ih = rounded(std::min(static_cast<uInt32>
            (ih * zoomLevel), screenS.h) * overscan);
        break;

      default:
        assert(false && "Unhandled Stretch enum value");
        break;
    }
  }
  else
  {
    // In windowed mode, currently the size is scaled to the screen
    // TODO - this may be updated if/when we allow variable-sized windows
    switch(stretch)
    {
      case Stretch::Preserve:
      case Stretch::Fill:
        iw = static_cast<uInt32>(iw * zoomLevel);
        ih = static_cast<uInt32>(ih * zoomLevel);
        screenS.w = static_cast<uInt32>(std::round(iw * bezelInfo.ratioW()));
        screenS.h = static_cast<uInt32>(std::round(ih * bezelInfo.ratioH()));
        break;

      case Stretch::None: // UI Mode
        break;  // Do not change image or screen rects whatsoever

      default:
        assert(false && "Unhandled Stretch enum value");
        break;
    }
  }

  // Now re-calculate the dimensions
  iw = std::min(iw, screenS.w);
  ih = std::min(ih, screenS.h);

  // Allow variable image positions in asymmetric bezels
  // (works in case of no bezel too)
  const uInt32 wx = bezelInfo.window().x() * iw / bezelInfo.window().w();
  const uInt32 wy = bezelInfo.window().y() * ih / bezelInfo.window().h();
  const uInt32 bezelW = std::min(screenS.w,
      static_cast<uInt32>(std::round(iw * bezelInfo.ratioW())));
  const uInt32 bezelH = std::min(screenS.h,
      static_cast<uInt32>(std::round(ih * bezelInfo.ratioH())));

  // Center image (no bezel) or move image relative to centered bezel
  imageR.moveTo((screenS.w - bezelW) / 2 + wx, (screenS.h - bezelH) / 2 + wy);

  imageR.setWidth(iw);
  imageR.setHeight(ih);

  screenR = Common::Rect(screenS);
}
