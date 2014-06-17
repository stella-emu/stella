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
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
  {
    ostringstream buf;
    buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    return;
  }

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
void FrameBufferSDL2::queryHardware(uInt32& w, uInt32& h, VariantList& renderers)
{
  // First get the maximum windowed desktop resolution
  SDL_DisplayMode desktop;
  SDL_GetDesktopDisplayMode(0, &desktop);
  w = desktop.w;
  h = desktop.h;

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

  uInt32 pos = myOSystem.settings().getBool("center")
                 ? SDL_WINDOWPOS_CENTERED : SDL_WINDOWPOS_UNDEFINED;
    uInt32 flags = mode.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

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
FBSurface* FrameBufferSDL2::createSurface(uInt32 w, uInt32 h,
                                          const uInt32* data) const
{
  return new FBSurfaceSDL2((FrameBufferSDL2&)*this, w, h, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::readPixels(const GUI::Rect& rect,
                                 uInt8* pixels, uInt32 pitch) const
{
  SDL_Rect r;
  r.x = rect.x();  r.y = rect.y();
  r.w = rect.width();  r.h = rect.height();

  SDL_RenderReadPixels(myRenderer, &r, 0, pixels, pitch);
}
