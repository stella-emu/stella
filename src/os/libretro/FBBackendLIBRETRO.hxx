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

#ifndef FB_BACKEND_LIBRETRO_HXX
#define FB_BACKEND_LIBRETRO_HXX

class OSystem;

#include "bspf.hxx"
#include "FBBackend.hxx"
#include "FBSurfaceLIBRETRO.hxx"

/**
  This class implements a standard LIBRETRO framebuffer backend.  Most of
  the functionality is not used, since libretro has its own rendering system.

  @author  Stephen Anthony
*/
class FBBackendLIBRETRO : public FBBackend
{
  public:
    explicit FBBackendLIBRETRO(OSystem&) { }
    ~FBBackendLIBRETRO() override = default;

  protected:
    /**
      This method is called to query and initialize the video hardware
      for desktop and fullscreen resolution information.  Since several
      monitors may be attached, we need the resolution for all of them.

      @param fullscreenRes  Maximum resolution supported in fullscreen mode
      @param windowedRes    Maximum resolution supported in windowed mode
      @param renderers      List of renderer names (internal name -> end-user name)
    */
    void queryHardware(std::unordered_map<uInt32, Common::Size>& fullscreenRes,
                       std::unordered_map<uInt32, Common::Size>& windowedRes,
                       VariantList& renderers) override
    {
      fullscreenRes.emplace(0, Common::Size{1920, 1080});
      windowedRes.emplace(0, Common::Size{1920, 1080});

      VarList::push_back(renderers, "software", "Software");
    }

    /**
      This method is called to create a surface with the given attributes.

      @param w     The requested width of the new surface.
      @param h     The requested height of the new surface.
    */
    unique_ptr<FBSurface>
      createSurface(uInt32 w, uInt32 h, ScalingInterpolation,
                    const uInt32*) const override
    {
      return std::make_unique<FBSurfaceLIBRETRO>(w, h);
    }

    /**
      This method is called to provide information about the backend.
    */
    string about() const override { return "Video system: libretro"; }


    //////////////////////////////////////////////////////////////////////
    // Most methods here aren't used at all.  See FBBacked class for
    // description, if needed.
    //////////////////////////////////////////////////////////////////////

    int scaleX(int x) const override { return x; }
    int scaleY(int y) const override { return y; }
    void setTitle(string_view) override { }
    void showCursor(bool) override { }
    bool fullScreen() const override { return true; }
    uInt32 rMask() const override { return 0x00FF0000; }
    uInt32 gMask() const override { return 0x0000FF00; }
    uInt32 bMask() const override { return 0x000000FF; }
    uInt32 aMask() const override { return 0xFF000000; }
    const FBSurface& compositedSurface() { static FBSurfaceLIBRETRO tmp(0, 0); return tmp; }
    bool isCurrentWindowPositioned() const override { return true; }
    Common::Point getCurrentWindowPos() const override { return Common::Point{}; }
    uInt32 getCurrentDisplayID() const override { return 0; }
    void clear() override { }
    void flush() override { }
    bool setVideoMode(const VideoModeHandler::Mode&,
                      uInt32, const Common::Point&) override { return true; }
    void grabMouse(bool) override { }
    void enableTextEvents(bool enable) override { }
    void renderToScreen() override { }
    int refreshRate() const override { return 0; }
    bool isLightTheme() const override { return false; }
    bool isDarkTheme() const override { return false; }

  private:
    // Following constructors and assignment operators not supported
    FBBackendLIBRETRO() = delete;
    FBBackendLIBRETRO(const FBBackendLIBRETRO&) = delete;
    FBBackendLIBRETRO(FBBackendLIBRETRO&&) = delete;
    FBBackendLIBRETRO& operator=(const FBBackendLIBRETRO&) = delete;
    FBBackendLIBRETRO& operator=(FBBackendLIBRETRO&&) = delete;
};

#endif  // FB_BACKEND_LIBRETRO_HXX
