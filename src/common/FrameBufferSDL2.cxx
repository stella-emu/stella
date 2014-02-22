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
    myScreen(0),
    myWindow(NULL),
    myRenderer(NULL),
    myWindowFlags(0),
    myTiaSurface(NULL),
    myDirtyFlag(true)
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
  // Note: alpha disabled for now, since it's not used
  SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32,
                       0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

  myPixelFormat = *(s->format);
  SDL_FreeSurface(s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSDL2::~FrameBufferSDL2()
{
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
bool FrameBufferSDL2::queryHardware(uInt32& w, uInt32& h, ResolutionList& res)
{
  // First get the maximum windowed desktop resolution
  SDL_DisplayMode desktop;
  SDL_GetDesktopDisplayMode(0, &desktop);
  w = desktop.w;
  h = desktop.h;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSDL2::setVideoMode(const string& title, VideoMode& mode, bool full)
{
  // If not initialized by this point, then immediately fail
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    return false;

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

  myWindow = SDL_CreateWindow(title.c_str(),
                 SDL_WINDOWPOS_CENTERED,
                 SDL_WINDOWPOS_CENTERED,
                 mode.image_w, mode.image_h,
                 0);
  if(myWindow == NULL)
  {
    string msg = "ERROR: Unable to open SDL window: " + string(SDL_GetError());
    myOSystem->logMessage(msg, 0);
    return false;
  }

  myRenderer = SDL_CreateRenderer(myWindow, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if(myWindow == NULL)
  {
    string msg = "ERROR: Unable to create SDL renderer: " + string(SDL_GetError());
    myOSystem->logMessage(msg, 0);
    return false;
  }

  bool inTIAMode =
    myOSystem->eventHandler().state() != EventHandler::S_LAUNCHER &&
    myOSystem->eventHandler().state() != EventHandler::S_DEBUGGER;

  // Grab the initial height before it's updated below
  // We need it for the creating the TIA surface
  uInt32 baseHeight = mode.image_h / mode.gfxmode.zoom;

#if 0
  // Aspect ratio and fullscreen stretching only applies to the TIA
  if(inTIAMode)
  {
    // Aspect ratio (depends on whether NTSC or PAL is detected)
    // Not available in 'small' resolutions
    if(myOSystem->desktopWidth() >= 640)
    {
      const string& frate = myOSystem->console().about().InitialFrameRate;
      int aspect =
        myOSystem->settings().getInt(frate == "60" ? "gl_aspectn" : "gl_aspectp");
      mode.image_w = (uInt16)(float(mode.image_w * aspect) / 100.0);
    }

    // Fullscreen mode stretching
    if(fullScreen() &&
       (mode.image_w < mode.screen_w) && (mode.image_h < mode.screen_h))
    {
      float stretchFactor = 1.0;
      float scaleX = float(mode.image_w) / mode.screen_w;
      float scaleY = float(mode.image_h) / mode.screen_h;

      // Scale to actual or integral factors
      if(myOSystem->settings().getBool("gl_fsscale"))
      {
        // Scale to full (non-integral) available space
        if(scaleX > scaleY)
          stretchFactor = float(mode.screen_w) / mode.image_w;
        else
          stretchFactor = float(mode.screen_h) / mode.image_h;
      }
      else
      {
        // Only scale to an integral amount
        if(scaleX > scaleY)
        {
          int bw = mode.image_w / mode.gfxmode.zoom;
          stretchFactor = float(int(mode.screen_w / bw) * bw) / mode.image_w;
        }
        else
        {
          int bh = mode.image_h / mode.gfxmode.zoom;
          stretchFactor = float(int(mode.screen_h / bh) * bh) / mode.image_h;
        }
      }
      mode.image_w = (Uint16) (stretchFactor * mode.image_w);
      mode.image_h = (Uint16) (stretchFactor * mode.image_h);
    }
  }

  // Now re-calculate the dimensions
  if(!fullScreen()) mode.screen_w = mode.image_w;
  mode.image_x = (mode.screen_w - mode.image_w) >> 1;
  mode.image_y = (mode.screen_h - mode.image_h) >> 1;

  SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   myRGB[0] );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, myRGB[1] );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  myRGB[2] );
  SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, myRGB[3] );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  // There's no guarantee this is supported on all hardware
  // We leave it to the user to test and decide
  int vsync = myOSystem->settings().getBool("gl_vsync") ? 1 : 0;
  SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, vsync );
#endif

  // Make sure the flags represent the current screen state
//  myWindowFlags = myScreen->flags;

#if 0
  // Optimization hints
  p_gl.ShadeModel(GL_FLAT);
  p_gl.Disable(GL_CULL_FACE);
  p_gl.Disable(GL_DEPTH_TEST);
  p_gl.Disable(GL_ALPHA_TEST);
  p_gl.Disable(GL_LIGHTING);
  p_gl.Hint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

  // Initialize GL display
  p_gl.Viewport(0, 0, mode.screen_w, mode.screen_h);
  p_gl.MatrixMode(GL_PROJECTION);
  p_gl.LoadIdentity();
  p_gl.Ortho(0.0, mode.screen_w, mode.screen_h, 0.0, -1.0, 1.0);
  p_gl.MatrixMode(GL_MODELVIEW);
  p_gl.LoadIdentity();
  p_gl.Translatef(0.375, 0.375, 0.0);  // fix scanline mis-draw issues
#endif
//cerr << "dimensions: " << (fullScreen() ? "(full)" : "") << endl << mode << endl;

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

    myTiaSurface->updateCoords(baseHeight, mode.image_x, mode.image_y,
                               mode.image_w, mode.image_h);

    myTiaSurface->enableScanlines(ntscEnabled());
    myTiaSurface->setScanIntensity(myOSystem->settings().getInt("tv_scanlines"));
    myTiaSurface->setTexInterpolation(myOSystem->settings().getBool("gl_inter"));
    myTiaSurface->setScanInterpolation(myOSystem->settings().getBool("tv_scaninter"));
    myTiaSurface->setTIA(myOSystem->console().tia());
  }

  // Any previously allocated textures currently in use by various UI items
  // need to be refreshed as well (only seems to be required for OSX)
  resetSurfaces(myTiaSurface);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferSDL2::about() const
{
#if 0
  ostringstream out;
  out << "Video rendering: OpenGL mode" << endl
      << "  Vendor:     " << p_gl.GetString(GL_VENDOR) << endl
      << "  Renderer:   " << p_gl.GetString(GL_RENDERER) << endl
      << "  Version:    " << p_gl.GetString(GL_VERSION) << endl
      << "  Color:      " << myDepth << " bit, " << myRGB[0] << "-"
      << myRGB[1] << "-"  << myRGB[2] << "-" << myRGB[3] << ", "
      << "GL_BGRA" << endl
      << "  Extensions: VBO " << (myVBOAvailable ? "enabled" : "disabled")
      << endl;
  return out.str();
#endif
return EmptyString;
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
bool FrameBufferSDL2::fullScreen() const
{
#if 0//FIXSDL
#ifdef WINDOWED_SUPPORT
  return myWindowFlags & SDL_FULLSCREEN;
#else
  return true;
#endif
#endif
return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferSDL2::setWindowIcon()
{
#if 0 //FIXSDL
#if !defined(BSPF_MAC_OSX) && !defined(BSPF_UNIX)
  #include "stella.xpm"   // The Stella icon

  // Set the window icon
  uInt32 w, h, ncols, nbytes;
  uInt32 rgba[256], icon[32 * 32];
  uInt8  mask[32][4];

  sscanf(stella_icon[0], "%u %u %u %u", &w, &h, &ncols, &nbytes);
  if((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1))
  {
    myOSystem->logMessage("ERROR: Couldn't load the application icon.", 0);
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
      myOSystem->logMessage("ERROR: Couldn't load the application icon.", 0);
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
  SDL_WM_SetIcon(surface, (unsigned char *) mask);
  SDL_FreeSurface(surface);
#endif
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
    myTiaSurface->setScanIntensity(myOSystem->settings().getInt("tv_scanlines"));
    myTiaSurface->setTexInterpolation(myOSystem->settings().getBool("gl_inter"));
    myTiaSurface->setScanInterpolation(myOSystem->settings().getBool("tv_scaninter"));

    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferSDL2::enableScanlines(int relative, int absolute)
{
  int intensity = myTiaSurface->myScanlineIntensity;
  if(myTiaSurface)
  {
    if(relative == 0)  intensity = absolute;
    else               intensity += relative;
    intensity = BSPF_max(0, intensity);
    intensity = BSPF_min(100, intensity);

    myTiaSurface->setScanIntensity(intensity);
    myRedrawEntireFrame = true;
  }
  return intensity;
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
FBSurface* FrameBufferSDL2::createSurface(int w, int h, bool isBase) const
{
  // Ignore 'isBase' argument; all GL surfaces are separate
  // Also, this method will only be called for use in external dialogs.
  // and never used for TIA surfaces
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
#if 0
          << (myTiaSurface->myTexFilter[1] == GL_LINEAR ? "inter" : "nointer");
#endif
 ;
      break;
    case kBlarggPhosphor:
      buf << myNTSCFilter.getPreset() << ", phosphor, scanlines="
          << myTiaSurface->myScanlineIntensity << "/"
#if 0
          << (myTiaSurface->myTexFilter[1] == GL_LINEAR ? "inter" : "nointer");
#endif
 ;
      break;
  }
  return buf.str();
}
