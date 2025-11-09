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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "bspf.hxx"
#include "Logger.hxx"

#include "Console.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"

#include "ThreadDebugging.hxx"
#include "FBSurfaceSDL.hxx"
#include "FBBackendSDL.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBBackendSDL::FBBackendSDL(OSystem& osystem)
  : myOSystem{osystem}
{
  ASSERT_MAIN_THREAD;

  // Initialize SDL context
  if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
  {
    ostringstream buf;
    buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError();
    throw runtime_error(buf.str());
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
  {
    SDL_DestroyRenderer(myRenderer);
    myRenderer = nullptr;
  }
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, false); // on some systems, a crash occurs
                                              // when destroying fullscreen window
    SDL_DestroyWindow(myWindow);
    myWindow = nullptr;
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::queryHardware(vector<Common::Size>& fullscreenRes,
                                 vector<Common::Size>& windowedRes,
                                 VariantList& renderers)
{
  ASSERT_MAIN_THREAD;

  // Get number of displays (for most systems, this will be '1')
  int count = 0;
  SDL_DisplayID* displays = SDL_GetDisplays(&count);
  if(displays && count > 0)
    myNumDisplays = static_cast<uInt32>(count);
  else
    return;

  // Get the maximum fullscreen and windowed desktop resolutions
  for(uInt32 i = 0; i < myNumDisplays; ++i)
  {
    // Fullscreen mode
    const SDL_DisplayMode* display = SDL_GetDesktopDisplayMode(displays[i]);
    fullscreenRes.emplace_back(display->w, display->h);

    // Windowed mode
    SDL_Rect r{};
    if(SDL_GetDisplayUsableBounds(displays[i], &r))
      windowedRes.emplace_back(r.w, r.h);

    int numModes = 0;
    ostringstream s;
    string lastRes;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displays[i], &numModes);  // NOLINT

    s << "Supported video modes (" << numModes << ") for display " << i
      << " (" << SDL_GetDisplayName(displays[i]) << "):";

    for(int m = 0; modes != nullptr && modes[m] != nullptr; m++)
    {
      const SDL_DisplayMode* mode = modes[m];
      ostringstream res, ref;

      res << std::setw(4) << mode->w << "x" << std::setw(4) << mode->h;

      if(lastRes != res.view())
      {
        Logger::debug(s.view());
        s.str("");
        lastRes = res.view();
        s << "  " << lastRes << ": ";
      }

      ref << mode->refresh_rate << "Hz";
      s << std::setw(7) << std::left << ref.str();
      if(mode->w == display->w && mode->h == display->h &&
         mode->refresh_rate == display->refresh_rate)
        s << "* ";
      else
        s << "  ";
    }
    Logger::debug(s.view());
  }
  SDL_free(displays);

  struct RenderName
  {
    string_view sdlName;
    string_view stellaName;
  };
  // Create name map for all currently known SDL renderers
  static const std::array<RenderName, 8> RENDERER_NAMES = {{
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
                                int winIdx, const Common::Point& winPos)
{
  ASSERT_MAIN_THREAD;

// cerr << mode << '\n';

  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

  const uInt32 displayIndex = std::min<uInt32>(myNumDisplays, winIdx);
  int posX = 0, posY = 0;

  myCenter = myOSystem.settings().getBool("center");
  if(myCenter)
    posX = posY = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
  else
  {
    posX = winPos.x;
    posY = winPos.y;

    // Make sure the window is at least partially visibile
    int x0 = INT_MAX, y0 = INT_MAX, x1 = 0, y1 = 0;

    for(int display = myNumDisplays - 1; display >= 0; --display)
    {
      SDL_Rect rect;

      if (!SDL_GetDisplayUsableBounds(display, &rect))
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
      adaptRefreshRate(displayIndex, adaptedSdlMode);
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
    if(d != displayIndex ||
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
    SDL_SetBooleanProperty(props,
                           SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN,
                           true);
    myWindow = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if(myWindow == nullptr)
    {
      const string msg = "ERROR: Unable to open SDL window: " + string(SDL_GetError());
      Logger::error(msg);
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
      ostringstream msg;

      msg << "Display refresh rate changed to "
          << adaptedSdlMode.refresh_rate << " Hz " << "(" << adaptedSdlMode.w << "x" << adaptedSdlMode.h << ")";
      Logger::info(msg.view());

      const SDL_DisplayMode* setSdlMode = SDL_GetWindowFullscreenMode(myWindow);
      cerr << setSdlMode->refresh_rate << "Hz\n";
    }
  }
#endif

  const bool result = createRenderer();
  if(result)
  {
    // TODO: Checking for fullscreen status later returns invalid results,
    //       so we check and cache it here
    myIsFullscreen = SDL_GetWindowFlags(myWindow) & SDL_WINDOW_FULLSCREEN;
    SDL_ShowWindow(myWindow);
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBBackendSDL::adaptRefreshRate(Int32 displayIndex,
                                    SDL_DisplayMode& adaptedSdlMode)
{
  ASSERT_MAIN_THREAD;

  const SDL_DisplayMode* sdlMode = SDL_GetCurrentDisplayMode(displayIndex);

  if(sdlMode == nullptr)
  {
    Logger::error("ERROR: Display mode could not be retrieved");
    return false;
  }

  const int currentRefreshRate = sdlMode->refresh_rate;
  const int wantedRefreshRate =
      myOSystem.hasConsole() ? myOSystem.console().gameRefreshRate() : 0;
  // Take care of rounded refresh rates (e.g. 59.94 Hz)
  float factor = std::min(
      static_cast<float>(currentRefreshRate) / wantedRefreshRate, static_cast<float>(currentRefreshRate) / (wantedRefreshRate - 1));
  // Calculate difference taking care of integer factors (e.g. 100/120)
  float bestDiff = std::abs(factor - std::round(factor)) / factor;
  bool adapt = false;

  // Display refresh rate should be an integer factor of the game's refresh rate
  // Note: Modes are scanned with size being first priority,
  //       therefore the size will never change.
  // Check for integer factors 1 (60/50 Hz) and 2 (120/100 Hz)
  for(int m = 1; m <= 2; ++m)
  {
    SDL_DisplayMode closestSdlMode{};
    const float refresh_rate = wantedRefreshRate * m;

    if(!SDL_GetClosestFullscreenDisplayMode(displayIndex, sdlMode->w, sdlMode->h, refresh_rate, true, &closestSdlMode))
    {
      Logger::error("ERROR: Closest display mode could not be retrieved");
      return adapt;
    }
    factor = std::min(
        static_cast<float>(refresh_rate) / refresh_rate,
        static_cast<float>(refresh_rate) / (refresh_rate - 1));
    const float diff = std::abs(factor - std::round(factor)) / factor;

    if(diff < bestDiff)
    {
      bestDiff = diff;
      adaptedSdlMode = closestSdlMode;
      adapt = true;
    }
  }
  //cerr << "refresh rate adapt ";
  //if(adapt)
  //  cerr << "required (" << currentRefreshRate << " Hz -> " << adaptedSdlMode.refresh_rate << " Hz)";
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
      Logger::error("ERROR: Unable to create SDL renderer: " +
                    string{SDL_GetError()});
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

  ostringstream out;
  out << "Video system: " << SDL_GetCurrentVideoDriver() << '\n';

  const SDL_PropertiesID props = SDL_GetRendererProperties(myRenderer);
  if(props != 0)
  {
    out << "  Renderer: "
        << SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "")
        << '\n';
    const uInt64 maxTexSize =
      SDL_GetNumberProperty(props, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
    if(maxTexSize > 0)
      out << "  Max texture: " << maxTexSize << "x" << maxTexSize << '\n';
    const bool usingVSync = SDL_GetNumberProperty(props,
      SDL_PROP_RENDERER_VSYNC_NUMBER, 0) != 0;
    out << "  Flags: "
        << (usingVSync ? "+" : "-") << "vsync"
        << '\n';
  }
  return out.str();
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
  return myIsFullscreen;  // TODO: should query SDL directly
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
  #include "stella_icon.hxx"
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
  unique_ptr<FBSurface> s = make_unique<FBSurfaceSDL>
    (const_cast<FBBackendSDL&>(*this), w, h, inter, data);
  s->setBlendLevel(100);  // by default, disable shading (use full alpha)

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBBackendSDL::readPixels(uInt8* buffer, size_t pitch,
                              const Common::Rect& rect) const
{
// FIXME: this method needs to be refactored to return an FBSurface,
//        since SDL_RenderReadPixels now returns an SDL_Surface
//        PNGLibrary is the only user of this; that will need to be refactored too
#if 0
  ASSERT_MAIN_THREAD;

  SDL_Rect r;
  r.x = rect.x();  r.y = rect.y();
  r.w = rect.w();  r.h = rect.h();

  SDL_RenderReadPixels(myRenderer, &r, 0, buffer, static_cast<int>(pitch));
#endif
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
