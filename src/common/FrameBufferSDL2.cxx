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

#include "FBSurfaceUI.hxx"
#include "FBSurfaceTIA.hxx"
#include "FrameBufferSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL2::FrameBufferSDL2(OSystem* osystem)
  : FrameBuffer(osystem),
    myFilterType(kNormal),
    myWindow(NULL),
    myRenderer(NULL),
    myWindowFlags(0),
    myTiaSurface(NULL),
    myDirtyFlag(true),
    myDblBufferedFlag(true)
{
  // Initialize SDL2 context
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
  {
    ostringstream buf;
    buf << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
    myOSystem->logMessage(buf.str(), 0);
    return;
  }

  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceUI object)
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
    SDL_DestroyWindow(myWindow);
    myWindow = NULL;
  }

  // We're taking responsibility for this surface
  delete myTiaSurface;
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
                                   bool full)
{
  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

  bool inTIAMode =
    myOSystem->eventHandler().state() != EventHandler::S_LAUNCHER &&
    myOSystem->eventHandler().state() != EventHandler::S_DEBUGGER;

  // Grab the initial height before it's updated below
  // We need it for the creating the TIA surface
  uInt32 baseHeight = mode.image.height() / mode.zoom;

  // (Re)create window and renderer
  if(myRenderer)
  {
    SDL_DestroyRenderer(myRenderer);
    myRenderer = NULL;
  }
  if(myWindow)
  {
    SDL_DestroyWindow(myWindow);
    myWindow = NULL;
  }

  // Window centering option
  int pos = myOSystem->settings().getBool("center")
              ? SDL_WINDOWPOS_CENTERED : SDL_WINDOWPOS_UNDEFINED;
  myWindow = SDL_CreateWindow(title.c_str(),
                 pos, pos, mode.image.width(), mode.image.height(),
                 0);
  if(myWindow == NULL)
  {
    string msg = "ERROR: Unable to open SDL window: " + string(SDL_GetError());
    myOSystem->logMessage(msg, 0);
    return false;
  }

  // V'synced blits option
  Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
  if(myOSystem->settings().getBool("vsync"))
    renderFlags |= SDL_RENDERER_PRESENTVSYNC;
  // Render hint
  const string& video = myOSystem->settings().getString("video");
  if(video != "")
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, video.c_str());
  myRenderer = SDL_CreateRenderer(myWindow, -1, renderFlags);
  if(myWindow == NULL)
  {
    string msg = "ERROR: Unable to create SDL renderer: " + string(SDL_GetError());
    myOSystem->logMessage(msg, 0);
    return false;
  }
  SDL_RendererInfo renderinfo;
  if(SDL_GetRendererInfo(myRenderer, &renderinfo) >= 0)
  {
    myOSystem->settings().setValue("video", renderinfo.name);
    // For now, accelerated renderers imply double buffering
    // Eventually, SDL may be updated to query this from the render backend
    myDblBufferedFlag = renderinfo.flags & SDL_RENDERER_ACCELERATED;
  }

  // The framebuffer only takes responsibility for TIA surfaces
  // Other surfaces (such as the ones used for dialogs) are allocated
  // in the Dialog class
  if(inTIAMode)
  {
    // Since we have free hardware stretching, the base TIA surface is created
    // only once, and its texture coordinates changed when we want to draw a
    // smaller or larger image
    if(!myTiaSurface)
      myTiaSurface = new FBSurfaceTIA(*this);

    myTiaSurface->updateCoords(baseHeight, mode.image.x(), mode.image.y(),
                               mode.image.width(), mode.image.height());

    myTiaSurface->enableScanlines(ntscEnabled());
    myTiaSurface->setTexInterpolation(myOSystem->settings().getBool("tia.inter"));
    myTiaSurface->setScanIntensity(myOSystem->settings().getInt("tv.scanlines"));
    myTiaSurface->setScanInterpolation(myOSystem->settings().getBool("tv.scaninter"));
    myTiaSurface->setTIA(myOSystem->console().tia());
  }

  // Any previously allocated textures currently in use by various UI items
  // need to be refreshed as well
  // This *must* come after the TIA settings have been updated
  resetSurfaces(myTiaSurface);

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
  SDL_RenderClear(myRenderer);
  if(myTiaSurface)
    myTiaSurface->invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::showCursor(bool show)
{
//FIXSDL  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::grabMouse(bool grab)
{
//FIXSDL  SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::enableFullscreen(bool enable)
{
  uInt32 flags = enable ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
  if(SDL_SetWindowFullscreen(myWindow, flags))
    myOSystem->settings().setValue("fullscreen", enable);
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
void FrameBufferSDL2::drawTIA(bool fullRedraw)
{
  // The TIA surface takes all responsibility for drawing
  myTiaSurface->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    // Now show all changes made to the texture(s)
    SDL_RenderPresent(myRenderer);
    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::enablePhosphor(bool enable, int blend)
{
  if(myTiaSurface)
  {
    myUsePhosphor   = enable;
    myPhosphorBlend = blend;
    myFilterType = FilterType(enable ? myFilterType | 0x01 : myFilterType & 0x10);
    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::enableNTSC(bool enable)
{
  if(myTiaSurface)
  {
    myFilterType = FilterType(enable ? myFilterType | 0x10 : myFilterType & 0x01);
    myTiaSurface->updateCoords();

    myTiaSurface->enableScanlines(ntscEnabled());
    myTiaSurface->setScanIntensity(myOSystem->settings().getInt("tv.scanlines"));
    myTiaSurface->setTexInterpolation(myOSystem->settings().getBool("tia.inter"));
    myTiaSurface->setScanInterpolation(myOSystem->settings().getBool("tv.scaninter"));

    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferSDL2::enableScanlines(int relative, int absolute)
{
  if(myTiaSurface)
  {
    int intensity = myTiaSurface->myScanlineIntensity;
    if(relative == 0)  intensity = absolute;
    else               intensity += relative;
    intensity = BSPF_max(0, intensity);
    intensity = BSPF_min(100, intensity);

    myTiaSurface->setScanIntensity(intensity);
    myRedrawEntireFrame = true;
    return intensity;
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::enableScanlineInterpolation(bool enable)
{
  if(myTiaSurface)
  {
    myTiaSurface->setScanInterpolation(enable);
    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::setTIAPalette(const uInt32* palette)
{
  FrameBuffer::setTIAPalette(palette);
  myTiaSurface->setTIAPalette(palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferSDL2::createSurface(int w, int h) const
{
  return new FBSurfaceUI((FrameBufferSDL2&)*this, w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::scanline(uInt32 row, uInt8* data) const
{
#if 0 //FIXSDL
  // Invert the row, since OpenGL rows start at the bottom
  // of the framebuffer
  const GUI::Rect& image = imageRect();
  row = image.height() + image.y() - row - 1;

  p_gl.PixelStorei(GL_PACK_ALIGNMENT, 1);
  p_gl.ReadPixels(image.x(), row, image.width(), 1, GL_RGB, GL_UNSIGNED_BYTE, data);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferSDL2::effectsInfo() const
{
  ostringstream buf;
  switch(myFilterType)
  {
    case kNormal:
      buf << "Disabled, normal mode";
      break;
    case kPhosphor:
      buf << "Disabled, phosphor mode";
      break;
    case kBlarggNormal:
      buf << myNTSCFilter.getPreset() << ", scanlines="
          << myTiaSurface->myScanlineIntensity << "/"
          << (myTiaSurface->myTexFilter[1] ? "inter" : "nointer");
      break;
    case kBlarggPhosphor:
      buf << myNTSCFilter.getPreset() << ", phosphor, scanlines="
          << myTiaSurface->myScanlineIntensity << "/"
          << (myTiaSurface->myTexFilter[1] ? "inter" : "nointer");
      break;
  }
  return buf.str();
}
