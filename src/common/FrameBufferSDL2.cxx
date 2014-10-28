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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <SDL.h>
#include <sstream>
#include <time.h>
#include <fstream>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FBSurfaceSDL2.hxx"
#include "FrameBufferSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL2::FrameBufferSDL2(OSystem& osystem)
  : FrameBuffer(osystem),
    myWindow(NULL),
    myRenderer(NULL),
    myDirtyFlag(true),
    myDblBufferedFlag(true)
{
  // Initialize SDL2 context
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0)
  {
    ostringstream buf;
    buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    return;
  }
  myOSystem.logMessage("FrameBufferSDL2::FrameBufferSDL2 SDL_Init()", 2);

  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceSDL2 object)
  // since the structure may be needed before any FBSurface's have
  // been created
  myPixelFormat = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL2::~FrameBufferSDL2()
{
  SDL_FreeFormat(myPixelFormat);

  if(myRenderer)
  {
    SDL_DestroyRenderer(myRenderer);
    myRenderer = NULL;
  }
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, 0); // on some systems, a crash occurs
                                          // when destroying fullscreen window
    SDL_DestroyWindow(myWindow);
    myWindow = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::queryHardware(Common::Array<GUI::Size>& displays,
                                    VariantList& renderers)
{
  // First get the maximum windowed desktop resolution
  SDL_DisplayMode display;
  int maxDisplays = SDL_GetNumVideoDisplays();
  for(int i = 0; i < maxDisplays; ++i)
  {
    SDL_GetDesktopDisplayMode(i, &display);
    displays.push_back(GUI::Size(display.w, display.h));
  }

  // For now, supported render types are hardcoded; eventually, SDL may
  // provide a method to query this
#if defined(BSPF_WINDOWS)
  renderers.push_back("Direct3D", "direct3d");
#endif
  renderers.push_back("OpenGL", "opengl");
  renderers.push_back("OpenGLES2", "opengles2");
  renderers.push_back("OpenGLES", "opengles");
  renderers.push_back("Software", "software");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameBufferSDL2::getCurrentDisplayIndex()
{
  return SDL_GetWindowDisplayIndex(myWindow);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::setVideoMode(const string& title, const VideoMode& mode,
                                   bool /*fullscreen_toggle*/)
{
  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

  // Always recreate renderer (some systems need this)
  if(myRenderer)
  {
    // Always clear the (double-buffered) renderer surface
    SDL_RenderClear(myRenderer);
    SDL_RenderPresent(myRenderer);
    SDL_RenderClear(myRenderer);
    SDL_DestroyRenderer(myRenderer);
    myRenderer = NULL;
  }

  Int32 displayIndex = mode.fsIndex;
  if(displayIndex == -1)
  {
    // windowed mode
    if (myWindow)
    {
      // Show it on same screen as the previous window
      displayIndex = SDL_GetWindowDisplayIndex(myWindow);
    }
    if(displayIndex < 0)
    {
      // fallback to the first screen
      displayIndex = 0;
    }
  }

  uInt32 pos = myOSystem.settings().getBool("center")
                 ? SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex)
                 : SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
  uInt32 flags = mode.fsIndex != -1 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

  // OSX seems to have issues with destroying the window, and wants to keep
  // the same handle
  // Problem is, doing so on other platforms results in flickering when
  // toggling fullscreen windowed mode
  // So we have a special case for OSX
#ifndef BSPF_MAC_OSX
  // Don't re-create the window if its size hasn't changed, as it's not
  // necessary, and causes flashing in fullscreen mode
  if(myWindow)
  {
    int w, h;
    SDL_GetWindowSize(myWindow, &w, &h);
    if((uInt32)w != mode.screen.w || (uInt32)h != mode.screen.h)
    {
      SDL_DestroyWindow(myWindow);
      myWindow = NULL;
    }
  }
  if(myWindow)
  {
    // Even though window size stayed the same, the title may have changed
    SDL_SetWindowTitle(myWindow, title.c_str());
  }
#else
  // OSX wants to *never* re-create the window
  // This sometimes results in the window being resized *after* it's displayed,
  // but at least the code works and doesn't crash
  if(myWindow)
  {
    SDL_SetWindowFullscreen(myWindow, flags);
    SDL_SetWindowSize(myWindow, mode.screen.w, mode.screen.h);
    SDL_SetWindowPosition(myWindow, pos, pos);
    SDL_SetWindowTitle(myWindow, title.c_str());
  }
#endif
  else
  {
    myWindow = SDL_CreateWindow(title.c_str(), pos, pos,
                                mode.screen.w, mode.screen.h, flags);
    if(myWindow == NULL)
    {
      string msg = "ERROR: Unable to open SDL window: " + string(SDL_GetError());
      myOSystem.logMessage(msg, 0);
      return false;
    }
    setWindowIcon();
  }

  Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
  if(myOSystem.settings().getBool("vsync"))  // V'synced blits option
    renderFlags |= SDL_RENDERER_PRESENTVSYNC;
  const string& video = myOSystem.settings().getString("video");  // Render hint
  if(video != "")
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, video.c_str());
  myRenderer = SDL_CreateRenderer(myWindow, -1, renderFlags);
  if(myRenderer == NULL)
  {
    string msg = "ERROR: Unable to create SDL renderer: " + string(SDL_GetError());
    myOSystem.logMessage(msg, 0);
    return false;
  }
  SDL_RendererInfo renderinfo;
  if(SDL_GetRendererInfo(myRenderer, &renderinfo) >= 0)
  {
    myOSystem.settings().setValue("video", renderinfo.name);
    // For now, accelerated renderers imply double buffering
    // Eventually, SDL may be updated to query this from the render backend
    myDblBufferedFlag = renderinfo.flags & SDL_RENDERER_ACCELERATED;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferSDL2::about() const
{
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
void FrameBufferSDL2::invalidate()
{
  myDirtyFlag = true;
  SDL_RenderClear(myRenderer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::showCursor(bool show)
{
  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::grabMouse(bool grab)
{
  SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::fullScreen() const
{
#ifdef WINDOWED_SUPPORT
  return SDL_GetWindowFlags(myWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
  return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    // Now show all changes made to the renderer
    SDL_RenderPresent(myRenderer);
    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::setWindowIcon()
{
#ifndef BSPF_MAC_OSX     // Currently not needed for OSX
  #include "stella.xpm"  // The Stella icon

  // Set the window icon
  uInt32 w, h, ncols, nbytes;
  uInt32 rgba[256], icon[32 * 32];
  uInt8  mask[32][4];

  sscanf(stella_icon[0], "%2u %2u %2u %2u", &w, &h, &ncols, &nbytes);
  if((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1))
  {
    myOSystem.logMessage("ERROR: Couldn't load the application icon.", 0);
    return;
  }

  for(uInt32 i = 0; i < ncols; i++)
  {
    unsigned char code;
    char color[32];
    uInt32 col;

    sscanf(stella_icon[1 + i], "%c c %s", &code, color);
    if(!strcmp(color, "None"))
      col = 0x00000000;
    else if(!strcmp(color, "black"))
      col = 0xFF000000;
    else if (color[0] == '#')
    {
      sscanf(color + 1, "%06x", &col);
      col |= 0xFF000000;
    }
    else
    {
      myOSystem.logMessage("ERROR: Couldn't load the application icon.", 0);
      return;
    }
    rgba[code] = col;
  }

  memset(mask, 0, sizeof(mask));
  for(h = 0; h < 32; h++)
  {
    const char* line = stella_icon[1 + ncols + h];
    for(w = 0; w < 32; w++)
    {
      icon[w + 32 * h] = rgba[(int)line[w]];
      if(rgba[(int)line[w]] & 0xFF000000)
        mask[h][w >> 3] |= 1 << (7 - (w & 0x07));
    }
  }

  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(icon, 32, 32, 32,
                         32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
  SDL_SetWindowIcon(myWindow, surface);
  SDL_FreeSurface(surface);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferSDL2::createSurface(uInt32 w, uInt32 h,
                                          const uInt32* data) const
{
  return new FBSurfaceSDL2((FrameBufferSDL2&)*this, w, h, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::readPixels(uInt8* pixels, uInt32 pitch,
                                 const GUI::Rect& rect) const
{
  SDL_Rect r;
  r.x = rect.x();  r.y = rect.y();
  r.w = rect.width();  r.h = rect.height();

  SDL_RenderReadPixels(myRenderer, &r, 0, pixels, pitch);
}
