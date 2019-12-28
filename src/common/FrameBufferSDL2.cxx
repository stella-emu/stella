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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "SDL_lib.hxx"
#include "bspf.hxx"
#include "Logger.hxx"

#include "Console.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"

#include "ThreadDebugging.hxx"
#include "FBSurfaceSDL2.hxx"
#include "FrameBufferSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL2::FrameBufferSDL2(OSystem& osystem)
  : FrameBuffer(osystem),
    myWindow(nullptr),
    myRenderer(nullptr),
    myCenter(false),
    myRenderTargetSupport(false),
    myWindowW(0),
    myWindowH(0),
    myRenderW(0),
    myRenderH(0)
{
  ASSERT_MAIN_THREAD;

  // Initialize SDL2 context
  if(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
  {
    ostringstream buf;
    buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
    Logger::error(buf.str());
    throw runtime_error("FATAL ERROR");
  }
  Logger::debug("FrameBufferSDL2::FrameBufferSDL2 SDL_Init()");

  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceSDL2 object)
  // since the structure may be needed before any FBSurface's have
  // been created
  myPixelFormat = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);

  myWindowedPos = myOSystem.settings().getPoint("windowedpos");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL2::~FrameBufferSDL2()
{
  ASSERT_MAIN_THREAD;

  SDL_FreeFormat(myPixelFormat);

  if(myRenderer)
  {
    // Make sure to free surfaces/textures before destroying the renderer itself
    // Most platforms are fine with doing this in either order, but it seems
    // that OpenBSD in particular crashes when attempting to destroy textures
    // *after* the renderer is already destroyed
    freeSurfaces();

    SDL_DestroyRenderer(myRenderer);
    myRenderer = nullptr;
  }
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, 0); // on some systems, a crash occurs
                                          // when destroying fullscreen window
    SDL_DestroyWindow(myWindow);
    myWindow = nullptr;
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::queryHardware(vector<Common::Size>& fullscreenRes,
                                    vector<Common::Size>& windowedRes,
                                    VariantList& renderers)
{
  ASSERT_MAIN_THREAD;

  // Get number of displays (for most systems, this will be '1')
  myNumDisplays = SDL_GetNumVideoDisplays();

  // First get the maximum fullscreen desktop resolution
  SDL_DisplayMode display;
  for(int i = 0; i < myNumDisplays; ++i)
  {
    SDL_GetDesktopDisplayMode(i, &display);
    fullscreenRes.emplace_back(display.w, display.h);

    // evaluate fullscreen display modes (debug only for now)
    int numModes = SDL_GetNumDisplayModes(i);
    ostringstream s;

    s << "Supported video modes for display " << i << ":";
    Logger::debug(s.str());
    for (int m = 0; m < numModes; m++)
    {
      SDL_DisplayMode mode;

      SDL_GetDisplayMode(i, m, &mode);
      s.str("");
      s << "  " << m << ": " << mode.w << "x" << mode.h << "@" << mode.refresh_rate << "Hz";
      if (mode.w == display.w && mode.h == display.h && mode.refresh_rate == display.refresh_rate)
        s << " (active)";
      Logger::debug(s.str());
    }
  }

  // Now get the maximum windowed desktop resolution
  // Try to take into account taskbars, etc, if available
#if SDL_VERSION_ATLEAST(2,0,5)
  SDL_Rect r;
  for(int i = 0; i < myNumDisplays; ++i)
  {
    // Display bounds minus dock
    SDL_GetDisplayUsableBounds(i, &r);  // Requires SDL-2.0.5 or higher
    windowedRes.emplace_back(r.w, r.h);
  }
#else
  for(int i = 0; i < myNumDisplays; ++i)
  {
    SDL_GetDesktopDisplayMode(i, &display);
    windowedRes.emplace_back(display.w, display.h);
  }
#endif

  struct RenderName
  {
    string sdlName;
    string stellaName;
  };
  // Create name map for all currently known SDL renderers
  static const std::array<RenderName, 6> RENDERER_NAMES = {{
    { "direct3d",  "Direct3D"  },
    { "metal",     "Metal"     },
    { "opengl",    "OpenGL"    },
    { "opengles",  "OpenGLES"  },
    { "opengles2", "OpenGLES2" },
    { "software",  "Software"  }
  }};

  int numDrivers = SDL_GetNumRenderDrivers();
  for(int i = 0; i < numDrivers; ++i)
  {
    SDL_RendererInfo info;
    if(SDL_GetRenderDriverInfo(i, &info) == 0)
    {
      // Map SDL names into nicer Stella names (if available)
      bool found = false;
      for(size_t j = 0; j < RENDERER_NAMES.size(); ++j)
      {
        if(RENDERER_NAMES[j].sdlName == info.name)
        {
          VarList::push_back(renderers, RENDERER_NAMES[j].stellaName, info.name);
          found = true;
          break;
        }
      }
      if(!found)
        VarList::push_back(renderers, info.name, info.name);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameBufferSDL2::getCurrentDisplayIndex()
{
  ASSERT_MAIN_THREAD;

  return SDL_GetWindowDisplayIndex(myWindow);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::updateWindowedPos()
{
  ASSERT_MAIN_THREAD;

  // only save if the window is not centered and not in full screen mode
  if (!myCenter && myWindow && !(SDL_GetWindowFlags(myWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP))
  {
    // save current windowed position
    SDL_GetWindowPosition(myWindow, &myWindowedPos.x, &myWindowedPos.y);
    myOSystem.settings().setValue("windowedpos", myWindowedPos);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::setVideoMode(const string& title, const VideoMode& mode)
{
  ASSERT_MAIN_THREAD;

  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

  Int32 displayIndex = mode.fsIndex;
  if (displayIndex == -1)
  {
    // windowed mode
    if (myWindow)
    {
      // Show it on same screen as the previous window
      displayIndex = SDL_GetWindowDisplayIndex(myWindow);
    }
    if (displayIndex < 0)
    {
      // fallback to the last used screen if still existing
      displayIndex = std::min(myNumDisplays, myOSystem.settings().getInt("display"));
    }
  }

  // save and get last windowed window's position
  updateWindowedPos();

  // Always recreate renderer (some systems need this)
  if(myRenderer)
  {
    SDL_DestroyRenderer(myRenderer);
    myRenderer = nullptr;
  }

  int posX, posY;

  myCenter = myOSystem.settings().getBool("center");
  if (myCenter)
    posX = posY = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
  else
  {
    posX = myWindowedPos.x;
    posY = myWindowedPos.y;

    // make sure the window is at least partially visibile
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;

    for (int display = SDL_GetNumVideoDisplays() - 1; display >= 0; display--)
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
    posX = BSPF::clamp(posX, x0 - Int32(mode.screen.w) + 50, x1 - 50);
    posY = BSPF::clamp(posY, y0 + 50, y1 - 50);
  }
  uInt32 flags = mode.fsIndex != -1 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;

  // macOS seems to have issues with destroying the window, and wants to
  // keep the same handle
  // Problem is, doing so on other platforms results in flickering when
  // toggling fullscreen windowed mode
  // So we have a special case for macOS
#ifndef BSPF_MACOS
  // Don't re-create the window if its size hasn't changed, as it's not
  // necessary, and causes flashing in fullscreen mode
  if(myWindow)
  {
    int w, h;
    SDL_GetWindowSize(myWindow, &w, &h);
    if(uInt32(w) != mode.screen.w || uInt32(h) != mode.screen.h)
    {
      SDL_DestroyWindow(myWindow);
      myWindow = nullptr;
    }
  }
  if(myWindow)
  {
    // Even though window size stayed the same, the title may have changed
    SDL_SetWindowTitle(myWindow, title.c_str());
    SDL_SetWindowPosition(myWindow, posX, posY);
  }
#else
  // macOS wants to *never* re-create the window
  // This sometimes results in the window being resized *after* it's displayed,
  // but at least the code works and doesn't crash
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, flags);
    SDL_SetWindowSize(myWindow, mode.screen.w, mode.screen.h);
    SDL_SetWindowPosition(myWindow, posX, posY);
    SDL_SetWindowTitle(myWindow, title.c_str());
  }
#endif
  else
  {
    myWindow = SDL_CreateWindow(title.c_str(), posX, posY,
                                mode.screen.w, mode.screen.h, flags);
    if(myWindow == nullptr)
    {
      string msg = "ERROR: Unable to open SDL window: " + string(SDL_GetError());
      Logger::error(msg);
      return false;
    }
    setWindowIcon();
  }

  uInt32 renderFlags = SDL_RENDERER_ACCELERATED;
  if(myOSystem.settings().getBool("vsync"))  // V'synced blits option
    renderFlags |= SDL_RENDERER_PRESENTVSYNC;
  const string& video = myOSystem.settings().getString("video");  // Render hint
  if(video != "")
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, video.c_str());

  myRenderer = SDL_CreateRenderer(myWindow, -1, renderFlags);

  detectFeatures();
  determineDimensions();

  if(myRenderer == nullptr)
  {
    string msg = "ERROR: Unable to create SDL renderer: " + string(SDL_GetError());
    Logger::error(msg);
    return false;
  }
  clear();

  SDL_RendererInfo renderinfo;
  if(SDL_GetRendererInfo(myRenderer, &renderinfo) >= 0)
    myOSystem.settings().setValue("video", renderinfo.name);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::setTitle(const string& title)
{
  ASSERT_MAIN_THREAD;

  myScreenTitle = title;

  if(myWindow)
    SDL_SetWindowTitle(myWindow, title.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferSDL2::about() const
{
  ASSERT_MAIN_THREAD;

  ostringstream out;
  out << "Video system: " << SDL_GetCurrentVideoDriver() << endl;
  SDL_RendererInfo info;
  if(SDL_GetRendererInfo(myRenderer, &info) >= 0)
  {
    out << "  Renderer: " << info.name << endl;
    if(info.max_texture_width > 0 && info.max_texture_height > 0)
      out << "  Max texture: " << info.max_texture_width << "x"
                               << info.max_texture_height << endl;
    out << "  Flags: "
        << ((info.flags & SDL_RENDERER_PRESENTVSYNC) ? "+" : "-") << "vsync, "
        << ((info.flags & SDL_RENDERER_ACCELERATED) ? "+" : "-") << "accel"
        << endl;
  }
  return out.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::showCursor(bool show)
{
  ASSERT_MAIN_THREAD;

  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::grabMouse(bool grab)
{
  ASSERT_MAIN_THREAD;

  SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::fullScreen() const
{
  ASSERT_MAIN_THREAD;

#ifdef WINDOWED_SUPPORT
  return SDL_GetWindowFlags(myWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
  return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::renderToScreen()
{
  ASSERT_MAIN_THREAD;

  // Show all changes made to the renderer
  SDL_RenderPresent(myRenderer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::setWindowIcon()
{
  ASSERT_MAIN_THREAD;

#if !defined(BSPF_MACOS) && !defined(RETRON77)
#include "stella_icon.hxx"

  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(stella_icon, 32, 32, 32,
                         32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
  SDL_SetWindowIcon(myWindow, surface);
  SDL_FreeSurface(surface);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<FBSurface> FrameBufferSDL2::createSurface(
  uInt32 w,
  uInt32 h,
  FrameBuffer::ScalingInterpolation interpolation,
  const uInt32* data
) const
{
  return make_unique<FBSurfaceSDL2>(const_cast<FrameBufferSDL2&>(*this), w, h, interpolation, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::readPixels(uInt8* pixels, uInt32 pitch,
                                 const Common::Rect& rect) const
{
  ASSERT_MAIN_THREAD;

  SDL_Rect r;
  r.x = rect.x();  r.y = rect.y();
  r.w = rect.w();  r.h = rect.h();

  SDL_RenderReadPixels(myRenderer, &r, 0, pixels, pitch);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::clear()
{
  ASSERT_MAIN_THREAD;

  SDL_RenderClear(myRenderer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SDL_Renderer* FrameBufferSDL2::renderer()
{
  return myRenderer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::isInitialized() const
{
  return myRenderer != nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SDL_PixelFormat& FrameBufferSDL2::pixelFormat() const
{
  return *myPixelFormat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::detectFeatures()
{
  myRenderTargetSupport = detectRenderTargetSupport();

  if (myRenderer) {
    if (!myRenderTargetSupport) {
      Logger::info("Render targets are not supported --- QIS not available");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::detectRenderTargetSupport()
{
  if (myRenderer == nullptr) return false;

  SDL_RendererInfo info;

  SDL_GetRendererInfo(myRenderer, &info);

  if (!(info.flags & SDL_RENDERER_TARGETTEXTURE)) return false;

  SDL_Texture* tex = SDL_CreateTexture(myRenderer, myPixelFormat->format, SDL_TEXTUREACCESS_TARGET, 16, 16);

  if (!tex) return false;

  int sdlError = SDL_SetRenderTarget(myRenderer, tex);
  SDL_SetRenderTarget(myRenderer, nullptr);

  SDL_DestroyTexture(tex);

  return sdlError == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::hasRenderTargetSupport() const
{
  return myRenderTargetSupport;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::determineDimensions()
{
  SDL_GetWindowSize(myWindow, &myWindowW, &myWindowH);

  if (myRenderer == nullptr) {
    myRenderW = myWindowW;
    myRenderH = myWindowH;
  } else {
    SDL_GetRendererOutputSize(myRenderer, &myRenderW, &myRenderH);
  }
}
