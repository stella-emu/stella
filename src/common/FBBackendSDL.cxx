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

#include <cmath>
#include <climits>

#include "bspf.hxx"
#include "Logger.hxx"

#include "Console.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"

#include "ThreadDebugging.hxx"
#include "TIASurface.hxx"
#include "FBSurfaceSDL.hxx"
#include "FBBackendSDL.hxx"

#ifndef BSPF_MACOS
  #include "stella_icon.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBBackendSDL::FBBackendSDL(OSystem& osystem)
  : myOSystem{osystem}
{
  ASSERT_MAIN_THREAD;

  // Initialize SDL context
  SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "unaware");
  if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
  {
    throw std::runtime_error(
      std::format("ERROR: Couldn't initialize SDL: {}", SDL_GetError()));
  }
  Logger::debug("FBBackendSDL::FBBackendSDL SDL_Init()");

  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceSDL object)
  // since the structure may be needed before any FBSurface's have
  // been created
  myPixelFormat = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_ARGB8888);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBBackendSDL::~FBBackendSDL()
{
  ASSERT_MAIN_THREAD;

  if(myRenderer)
    SDL_DestroyRenderer(myRenderer);
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, false); // on some systems, a crash occurs
                                              // when destroying fullscreen window
    SDL_DestroyWindow(myWindow);
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::queryHardware(std::unordered_map<uInt32, Common::Size>& fullscreenRes,
                                 std::unordered_map<uInt32, Common::Size>& windowedRes,
                                 VariantList& renderers)
{
  ASSERT_MAIN_THREAD;

  // Get number of displays (for most systems, this will be '1')
  int count = 0;
  SDL_DisplayID* displays = SDL_GetDisplays(&count);
  if(!displays || count == 0)
    return;
  myNumDisplays = static_cast<uInt32>(count);

  // Get the maximum fullscreen and windowed desktop resolutions
  for(uInt32 i = 0; i < myNumDisplays; ++i)
  {
SDL_DisplayID instance_id = displays[i];
cerr << std::format("Display {} -> {}\n", instance_id, SDL_GetDisplayName(instance_id));

    // Fullscreen mode
    const SDL_DisplayMode* display = SDL_GetDesktopDisplayMode(displays[i]);
    SDL_Rect bounds;
    SDL_GetDisplayBounds(displays[i], &bounds);
    fullscreenRes.try_emplace(displays[i], bounds.w, bounds.h);

    // Windowed mode
    SDL_Rect r{};
    if(SDL_GetDisplayUsableBounds(displays[i], &r))
      windowedRes.try_emplace(displays[i], r.w, r.h);

    int numModes = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displays[i], &numModes);

    string log = std::format("Supported video modes ({}) for display {} ({}):",
                             numModes, i, SDL_GetDisplayName(displays[i]));
    string lastRes;

    for(const auto* mode: std::span(modes, numModes))
    {
      string res = std::format("{:4}x{:4}", mode->w, mode->h);

      if(lastRes != res)
      {
        Logger::debug(log);
        lastRes = res;
        log = std::format("  {}: ", res);
      }

      const bool isDesktopMode =
        //mode->w == display->w &&
        //mode->h == display->h &&
        mode->w == bounds.w &&
        mode->h == bounds.h &&
        std::equal_to()(mode->refresh_rate, display->refresh_rate);
      log += std::format("{:>7}{}", std::format("{}Hz", mode->refresh_rate),
                         isDesktopMode ? "* " : "  ");
    }
    Logger::debug(log);
  }
  SDL_free(displays);

  struct RenderName
  {
    string_view sdlName;
    string_view stellaName;
  };
  // Create name map for all currently known SDL renderers
  static constexpr std::array<RenderName, 8> RENDERER_NAMES = {{
    { "direct3d",   "Direct3D"    },
    { "direct3d11", "Direct3D 11" },
    { "direct3d12", "Direct3D 12" },
    { "metal",      "Metal"       },
    { "opengl",     "OpenGL"      },
    { "opengles",   "OpenGL ES"   },
    { "opengles2",  "OpenGL ES 2" },
    { "software",   "Software"    }
  }};

  const int numDrivers = SDL_GetNumRenderDrivers();
  for(int i = 0; i < numDrivers; ++i)
  {
    const char* const rendername = SDL_GetRenderDriver(i);
    if(rendername)
    {
      // Map SDL names into nicer Stella names (if available)
      bool found = false;
      for(const auto& render: RENDERER_NAMES)
      {
        if(render.sdlName == rendername)
        {
          VarList::push_back(renderers, render.stellaName, rendername);
          found = true;
          break;
        }
      }
      if(!found)
        VarList::push_back(renderers, rendername, rendername);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::isCurrentWindowPositioned() const
{
  ASSERT_MAIN_THREAD;

  return myWindow && !fullScreen() && !myCenter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Point FBBackendSDL::getCurrentWindowPos() const
{
  ASSERT_MAIN_THREAD;

  Common::Point pos;
  SDL_GetWindowPosition(myWindow, &pos.x, &pos.y);

  return pos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FBBackendSDL::getCurrentDisplayID() const
{
  ASSERT_MAIN_THREAD;

  return SDL_GetDisplayForWindow(myWindow);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::setVideoMode(const VideoModeHandler::Mode& mode,
                                uInt32 winIdx, const Common::Point& winPos)
{
  ASSERT_MAIN_THREAD;

  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

  SDL_DisplayID* displayIds = SDL_GetDisplays(nullptr);
  const SDL_DisplayID displayId = winIdx;
  int posX = 0, posY = 0;

  myCenter = myOSystem.settings().getBool("center");
  if(myCenter)
    posX = posY = SDL_WINDOWPOS_CENTERED_DISPLAY(displayId);
  else
  {
    posX = winPos.x;
    posY = winPos.y;

    // Make sure the window is at least partially visibile
    int x0 = INT_MAX, y0 = INT_MAX, x1 = 0, y1 = 0;

    for(int i = myNumDisplays - 1; i >= 0; --i)
    {
      SDL_Rect rect;

      if(SDL_GetDisplayUsableBounds(displayIds[i], &rect))
      {
        x0 = std::min(x0, rect.x);
        y0 = std::min(y0, rect.y);
        x1 = std::max(x1, rect.x + rect.w);
        y1 = std::max(y1, rect.y + rect.h);
      }
    }
    posX = BSPF::clamp(posX, x0 - static_cast<Int32>(mode.screenS.w) + 50, x1 - 50);
    posY = BSPF::clamp(posY, y0 + 50, y1 - 50);
  }

#ifdef ADAPTABLE_REFRESH_SUPPORT
  SDL_DisplayMode adaptedSdlMode{};
  const int gameRefreshRate =
      myOSystem.hasConsole() ? myOSystem.console().gameRefreshRate() : 0;
  const bool shouldAdapt = mode.fullscreen
    && myOSystem.settings().getBool("tia.fs_refresh")
    && gameRefreshRate
    // take care of 59.94 Hz
    && refreshRate() % gameRefreshRate != 0
    && refreshRate() % (gameRefreshRate - 1) != 0;
  const bool adaptRefresh = shouldAdapt &&
      adaptRefreshRate(displayId, adaptedSdlMode);
#else
  const bool adaptRefresh = false;
#endif

  // Don't re-create the window if its display and size hasn't changed,
  // as it's not necessary, and causes flashing in fullscreen mode
  if(myWindow)
  {
    const uInt32 d = getCurrentDisplayID();
    int w{0}, h{0};

    SDL_GetWindowSize(myWindow, &w, &h);
    if(d != displayId ||
       std::cmp_not_equal(w, mode.screenS.w) ||
       std::cmp_not_equal(h, mode.screenS.h) || adaptRefresh)
    {
      // Renderer has to be destroyed *before* the window gets destroyed to avoid memory leaks
      SDL_DestroyRenderer(myRenderer);
      myRenderer = nullptr;
      SDL_DestroyWindow(myWindow);
      myWindow = nullptr;
    }
  }

  if(myWindow)
  {
    // Even though window size stayed the same, the title may have changed
    SDL_SetWindowTitle(myWindow, myScreenTitle.c_str());
    SDL_SetWindowPosition(myWindow, posX, posY);
  }
  else
  {
    // Re-create with new properties
    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          myScreenTitle.c_str());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, posX);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, posY);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                          mode.screenS.w);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                          mode.screenS.h);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN,
                           true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN,
                           mode.fullscreen);
    myWindow = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if(myWindow == nullptr)
    {
      Logger::error(std::format("ERROR: Unable to open SDL window: {}",
                                SDL_GetError()));
      SDL_free(displayIds);
      return false;
    }

    enableTextEvents(myTextEventsEnabled);
    setWindowIcon();
  }

#ifdef ADAPTABLE_REFRESH_SUPPORT
  if(adaptRefresh)
  {
    // Switch to mode for adapted refresh rate
    if(!SDL_SetWindowFullscreenMode(myWindow, &adaptedSdlMode))
    {
      Logger::error("ERROR: Display refresh rate change failed");
    }
    else
    {
      Logger::info(std::format("Display refresh rate changed to {} Hz ({}x{})",
        adaptedSdlMode.refresh_rate, adaptedSdlMode.w, adaptedSdlMode.h));

//       const SDL_DisplayMode* setSdlMode = SDL_GetWindowFullscreenMode(myWindow);
//       cerr << setSdlMode->refresh_rate << "Hz\n";
    }
  }
  else
#endif
  if(mode.fullscreen)
  {
    SDL_Rect r;
    SDL_GetDisplayBounds(displayId, &r);

    // Ensure the window fits entirely inside the target monitor.
    // This prevents SDL from assigning fullscreen to the wrong display.
    SDL_SetWindowSize(myWindow, r.w, r.h);
    // Place the window exactly at the top-left corner of the target monitor.
    // This guarantees the window lies fully inside that monitor.
    SDL_SetWindowPosition(myWindow, r.x, r.y);
    // Activate fullscreen on that monitor.
    // In SDL3 this is borderless fullscreen unless you set a specific mode.
    SDL_SetWindowFullscreenMode(myWindow, nullptr);
    SDL_SetWindowFullscreen(myWindow, true);
  }

  const bool result = createRenderer();  // NOLINT(readability-misleading-indentation)
  if(result)
  {
    // TODO: Checking for fullscreen status later returns invalid results,
    //       so we check and cache it here
    myIsFullscreen = SDL_GetWindowFlags(myWindow) & SDL_WINDOW_FULLSCREEN;
    SDL_ShowWindow(myWindow);
  }

  SDL_free(displayIds);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::adaptRefreshRate(SDL_DisplayID displayId,
                                    SDL_DisplayMode& adaptedSdlMode)
{
  ASSERT_MAIN_THREAD;

  const SDL_DisplayMode* sdlMode = SDL_GetCurrentDisplayMode(displayId);

  if(sdlMode == nullptr)
  {
    Logger::error("ERROR: Display mode could not be retrieved");
    return false;
  }

  const int currentRefreshRate = sdlMode->refresh_rate;
  const int wantedRefreshRate =
      myOSystem.hasConsole() ? myOSystem.console().gameRefreshRate() : 0;
  // Take care of rounded refresh rates (e.g. 59.94 Hz)
  const float factor = std::min(
      static_cast<float>(currentRefreshRate) / wantedRefreshRate,
      static_cast<float>(currentRefreshRate) / (wantedRefreshRate - 1));
  // Calculate difference taking care of integer factors (e.g. 100/120)
  float bestDiff = std::abs(factor - std::round(factor)) / factor;
  int numModes = 0;
  bool adapt = false;

  // Display refresh rate should be an integer factor of the game's refresh rate
  // Now find the display mode whose refresh rate is closest to an integer
  // multiple of the target refresh rate. If two modes have the same error, the
  // one matching the current resolution is preferred.
  const float epsilon = 0.001F;  // floating point rounding tolerance
  bool bestSameRes = false;
  SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayId, &numModes);
  // Wrap raw pointer into a span so we can use range-based for
  const std::span<SDL_DisplayMode*> span(modes, numModes);

  for(const auto* m: span)
  {
    // Determine nearest integer multiple of desired wantedRefreshRate
    const float nearestInt = std::round(m->refresh_rate / wantedRefreshRate);
    // Compute ideal rate for multiple
    const float ideal = nearestInt * wantedRefreshRate;
    // diff = delta betweem actual rate and ideal multiple
    const float diff = std::fabs(m->refresh_rate - ideal) / ideal;
    // Check if mode is same as current resolution
    const bool sameRes = (m->w == sdlMode->w && m->h == sdlMode->h);
    // 1. Prefer smaller error
    // 2. If error is equal (within epsilon), prefer same resolution
    if (diff < bestDiff - epsilon ||
        (std::fabs(diff - bestDiff) < epsilon &&
        sameRes && !bestSameRes))
    {
      bestDiff = diff;
      bestSameRes = sameRes;
      adaptedSdlMode = *m;
      adapt = true;
    }
  }
  //cerr << "refresh rate adapt ";
  //if(adapt)
  //  cerr << "(current " << currentRefreshRate << " Hz, required (" << wantedRefreshRate << " Hz -> set to " << adaptedSdlMode.refresh_rate << " Hz)";
  //else
  //  cerr << "not required/possible";
  //cerr << '\n';

  // Only change if the display supports a better refresh rate
  return adapt;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::createRenderer()
{
  ASSERT_MAIN_THREAD;

  // A new renderer is only created when necessary:
  // - no renderer existing
  // - different renderer name
  // - different renderer vsync
  const bool enableVSync = myOSystem.settings().getBool("vsync") &&
                          !myOSystem.settings().getBool("turbo");
  const string& video = myOSystem.settings().getString("video");

  bool recreate = myRenderer == nullptr;
  if(myRenderer)
  {
    recreate = recreate || video != SDL_GetRendererName(myRenderer);

    const SDL_PropertiesID props = SDL_GetRendererProperties(myRenderer);
    const bool currentVSync = SDL_GetNumberProperty(props,
        SDL_PROP_RENDERER_VSYNC_NUMBER, 0) != 0;
    recreate = recreate || currentVSync != enableVSync;
  }

  if(recreate)
  {
    if(myRenderer)
      SDL_DestroyRenderer(myRenderer);

    // Re-create with new properties
    const SDL_PropertiesID props = SDL_CreateProperties();
    if(!video.empty())
      SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING,
                            video.c_str());
    SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER,
                          enableVSync ? 1 : 0);
    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER,
                           myWindow);

    myRenderer = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);

    detectFeatures();
    determineDimensions();

    if(myRenderer == nullptr)
    {
      Logger::error(std::format("ERROR: Unable to create SDL renderer: {}",
                                SDL_GetError()));
      return false;
    }
  }
  clear();

  const char* const detectedvideo = SDL_GetRendererName(myRenderer);
  if(detectedvideo)
    myOSystem.settings().setValue("video", detectedvideo);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::setTitle(string_view title)
{
  ASSERT_MAIN_THREAD;

  myScreenTitle = title;

  if(myWindow)
    SDL_SetWindowTitle(myWindow, myScreenTitle.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FBBackendSDL::about() const
{
  ASSERT_MAIN_THREAD;

  string out = std::format("Video system: {}\n", SDL_GetCurrentVideoDriver());

  const SDL_PropertiesID props = SDL_GetRendererProperties(myRenderer);
  if(props != 0)
  {
    out += std::format("  Renderer: {}\n",
      SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, ""));

    const uInt64 maxTexSize =
      SDL_GetNumberProperty(props, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
    if(maxTexSize > 0)
      out += std::format("  Max texture: {}x{}\n", maxTexSize, maxTexSize);

    const bool usingVSync =
      SDL_GetNumberProperty(props, SDL_PROP_RENDERER_VSYNC_NUMBER, 0) != 0;
    out += std::format("  VSync: {}\n", usingVSync ? "enabled" : "disabled");
  }

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::showCursor(bool show)
{
  ASSERT_MAIN_THREAD;

  if(show) SDL_ShowCursor();
  else     SDL_HideCursor();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::grabMouse(bool grab)
{
  ASSERT_MAIN_THREAD;

  SDL_SetWindowMouseGrab(myWindow, grab);
  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, grab ? "1" : "0");
  SDL_SetWindowRelativeMouseMode(myWindow, grab);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::enableTextEvents(bool enable)
{
  ASSERT_MAIN_THREAD;

  if(enable)
    SDL_StartTextInput(myWindow);
  else
    SDL_StopTextInput(myWindow);
  // myWindows can still be null, so we remember the state and set again when
  // the window is created
  myTextEventsEnabled = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::fullScreen() const
{
  ASSERT_MAIN_THREAD;

#ifdef WINDOWED_SUPPORT
  return myIsFullscreen;  // TODO: this should query SDL directly (BUG?)
#else
  return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FBBackendSDL::refreshRate() const
{
  ASSERT_MAIN_THREAD;

  const SDL_DisplayID displayID = getCurrentDisplayID();
  const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);

  if(mode)
    return mode->refresh_rate;

  if(myWindow != nullptr)
    Logger::error("Could not retrieve current display mode");

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::renderToScreen()
{
  ASSERT_MAIN_THREAD;

  // Show all changes made to the renderer
  SDL_RenderPresent(myRenderer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::setWindowIcon()
{
#ifndef BSPF_MACOS
  ASSERT_MAIN_THREAD;

  SDL_Surface* surface =
    SDL_CreateSurfaceFrom(32, 32, pixelFormat().format, stella_icon, 32 * 4);
  SDL_SetWindowIcon(myWindow, surface);
  SDL_DestroySurface(surface);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<FBSurface> FBBackendSDL::createSurface(
  uInt32 w,
  uInt32 h,
  ScalingInterpolation inter,
  const uInt32* data
) const
{
  unique_ptr<FBSurface> s = std::make_unique<FBSurfaceSDL>
    (const_cast<FBBackendSDL&>(*this), w, h, inter, data);
  s->setBlendLevel(100);  // by default, disable shading (use full alpha)

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FBSurface& FBBackendSDL::compositedSurface()
{
  ASSERT_MAIN_THREAD;

  const FrameBuffer& fb = myOSystem.frameBuffer();
  const Common::Rect& rectUnscaled = fb.imageRect();
  const Common::Rect rect(
    Common::Point(fb.scaleX(rectUnscaled.x()), fb.scaleY(rectUnscaled.y())),
    fb.scaleX(rectUnscaled.w()), fb.scaleY(rectUnscaled.h())
  );

  const SDL_Rect surfaceRect = ToSDLRect(rect);
  SDL_Surface* tmp = SDL_RenderReadPixels(myRenderer, &surfaceRect);
  if(!tmp)
    throw(std::runtime_error(std::format("Snapshot failed: {}", SDL_GetError())));

  SDL_Surface* sdlSurface = SDL_ConvertSurface(tmp, myPixelFormat->format);
  SDL_DestroySurface(tmp);

  if(!sdlSurface)
    throw(std::runtime_error(std::format("Snapshot failed: {}", SDL_GetError())));

  // SDL3's OpenGL renderer converts sRGB textures to linear space for blending,
  // and SDL_RenderReadPixels returns these linear values. For phosphor mode,
  // the averaged pixel values go through this conversion and come back darker.
  // Apply sRGB gamma correction to restore correct brightness.
  // NOTE: this correction may need revisiting for other renderers (e.g. Metal).
  if(fb.tiaSurface().phosphorEnabled())
  {
    static const std::array<uInt8, 256> gammaLUT = [] {
      std::array<uInt8, 256> lut{};
      for(int i = 0; i < 256; ++i)
        lut[i] = static_cast<uInt8>(
          std::lround(std::pow(i / 255.0, 1.0 / 1.35) * 255.0));
      return lut;
    }();

    const uInt32 rShift = std::countr_zero(rMask());
    const uInt32 gShift = std::countr_zero(gMask());
    const uInt32 bShift = std::countr_zero(bMask());
    const uInt32 aMask_ = aMask();

    const auto w = static_cast<uInt32>(surfaceRect.w);
    const auto h = static_cast<uInt32>(surfaceRect.h);
    const uInt32 pitch = sdlSurface->pitch / sizeof(uInt32);
    auto* pixels = static_cast<uInt32*>(sdlSurface->pixels);

    const auto applyGamma = [rShift, gShift, bShift, aMask_](uInt32 px) -> uInt32 {
      const uInt8 r = gammaLUT[(px >> rShift) & 0xFF];
      const uInt8 g = gammaLUT[(px >> gShift) & 0xFF];
      const uInt8 b = gammaLUT[(px >> bShift) & 0xFF];
      return aMask_ | (r << rShift) | (g << gShift) | (b << bShift);
    };

    auto* row = pixels;
    for(uInt32 y = 0; y < h; ++y, row += pitch)
      std::transform(row, row + w, row, applyGamma);
  }
  myCompositedSurface = std::make_unique<FBSurfaceSDL>
    (const_cast<FBBackendSDL&>(*this), sdlSurface, ScalingInterpolation::none);

  return *myCompositedSurface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::clear()
{
  ASSERT_MAIN_THREAD;

  SDL_RenderClear(myRenderer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::detectFeatures()
{
  myRenderTargetSupport = detectRenderTargetSupport();

  if(myRenderer && !myRenderTargetSupport)
    Logger::info("Render targets are not supported --- QIS not available");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::detectRenderTargetSupport()
{
  ASSERT_MAIN_THREAD;

  if(myRenderer == nullptr)
    return false;

  // All texture modes except software support render targets
  const char* const detectedvideo = SDL_GetRendererName(myRenderer);
  return detectedvideo && !BSPF::equalsIgnoreCase(detectedvideo, "software");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::determineDimensions()
{
  ASSERT_MAIN_THREAD;

  SDL_GetWindowSize(myWindow, &myWindowW, &myWindowH);

  if(myRenderer == nullptr)
  {
    myRenderW = myWindowW;
    myRenderH = myWindowH;
  }
  else
    SDL_GetCurrentRenderOutputSize(myRenderer, &myRenderW, &myRenderH);
}
