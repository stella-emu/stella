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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifdef DISPLAY_OPENGL

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>
#include <time.h>
#include <fstream>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FrameBufferGL.hxx"

// OpenGL functions we'll be using in Stella
#define OGL_DECLARE(RET,FUNC,PARAMS) static RET (APIENTRY* p_ ## FUNC) PARAMS

OGL_DECLARE(void,glClear,(GLbitfield));
OGL_DECLARE(void,glEnable,(GLenum));
OGL_DECLARE(void,glDisable,(GLenum));
OGL_DECLARE(void,glPushAttrib,(GLbitfield));
OGL_DECLARE(const GLubyte*,glGetString,(GLenum));
OGL_DECLARE(void,glHint,(GLenum, GLenum));
OGL_DECLARE(void,glShadeModel,(GLenum));
OGL_DECLARE(void,glMatrixMode,(GLenum));
OGL_DECLARE(void,glOrtho,(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble));
OGL_DECLARE(void,glViewport,(GLint, GLint, GLsizei, GLsizei));
OGL_DECLARE(void,glLoadIdentity,(void));
OGL_DECLARE(void,glEnableClientState,(GLenum));
OGL_DECLARE(void,glDisableClientState,(GLenum));
OGL_DECLARE(void,glVertexPointer,(GLint,GLenum,GLsizei,const GLvoid*));
OGL_DECLARE(void,glTexCoordPointer,(GLint,GLenum,GLsizei,const GLvoid*));
OGL_DECLARE(void,glDrawArrays,(GLenum,GLint,GLsizei));
OGL_DECLARE(void,glReadPixels,(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
OGL_DECLARE(void,glPixelStorei,(GLenum, GLint));
OGL_DECLARE(void,glTexEnvf,(GLenum, GLenum, GLfloat));
OGL_DECLARE(void,glGenTextures,(GLsizei, GLuint*));
OGL_DECLARE(void,glDeleteTextures,(GLsizei, const GLuint*));
OGL_DECLARE(void,glActiveTexture,(GLenum));
OGL_DECLARE(void,glBindTexture,(GLenum, GLuint));
OGL_DECLARE(void,glTexImage2D,(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
OGL_DECLARE(void,glTexSubImage2D,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*));
OGL_DECLARE(void,glTexParameteri,(GLenum, GLenum, GLint));
OGL_DECLARE(GLenum,glGetError,(void));
OGL_DECLARE(void,glGenBuffers,(GLsizei,GLuint*));
OGL_DECLARE(void,glBindBuffer,(GLenum,GLuint));
OGL_DECLARE(void,glBufferData,(GLenum,GLsizei,const void*,GLenum));
OGL_DECLARE(void,glDeleteBuffers,(GLsizei, const GLuint*));

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL(OSystem* osystem)
  : FrameBuffer(osystem),
    myTiaSurface(NULL),
    myFilterParamName("GL_NEAREST"),
    myDirtyFlag(true)
{
  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceGL object)
  // since the structure may be needed before any FBSurface's have
  // been created
  SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 16,
#if defined(GL_BGRA) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV)
                     0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
#else
                     0x0000f800, 0x000007c0, 0x0000003e, 0x00000000);
#endif
  myPixelFormat = *(s->format);
  SDL_FreeSurface(s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::~FrameBufferGL()
{
  // We're taking responsibility for this surface
  delete myTiaSurface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::loadLibrary(const string& library)
{
  if(myLibraryLoaded)
    return true;

  // Try both the specified library and auto-detection
  bool libLoaded = (library != "" && SDL_GL_LoadLibrary(library.c_str()) >= 0);
  bool autoLoaded = false;
  if(!libLoaded) autoLoaded = (SDL_GL_LoadLibrary(0) >= 0);
  if(!libLoaded && !autoLoaded)
    return false;

  return myLibraryLoaded = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::loadFuncs(GLFunctionality functionality)
{
#define OGL_INIT(RET,FUNC,PARAMS) \
  p_ ## FUNC = (RET(APIENTRY*)PARAMS) SDL_GL_GetProcAddress(#FUNC); if(!p_ ## FUNC) return false

  if(myLibraryLoaded)
  {
    // Fill the function pointers for GL functions
    // If anything fails, we'll know it immediately, and return false
    switch(functionality)
    {
      case kGL_BASIC:
        OGL_INIT(void,glClear,(GLbitfield));
        OGL_INIT(void,glEnable,(GLenum));
        OGL_INIT(void,glDisable,(GLenum));
        OGL_INIT(void,glPushAttrib,(GLbitfield));
        OGL_INIT(const GLubyte*,glGetString,(GLenum));
        OGL_INIT(void,glHint,(GLenum, GLenum));
        OGL_INIT(void,glShadeModel,(GLenum));

        OGL_INIT(void,glMatrixMode,(GLenum));
        OGL_INIT(void,glOrtho,(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble));
        OGL_INIT(void,glViewport,(GLint, GLint, GLsizei, GLsizei));
        OGL_INIT(void,glLoadIdentity,(void));

        OGL_INIT(void,glEnableClientState,(GLenum));
        OGL_INIT(void,glDisableClientState,(GLenum));
        OGL_INIT(void,glVertexPointer,(GLint,GLenum,GLsizei,const GLvoid*));
        OGL_INIT(void,glTexCoordPointer,(GLint,GLenum,GLsizei,const GLvoid*));
        OGL_INIT(void,glDrawArrays,(GLenum,GLint,GLsizei));
        OGL_INIT(GLenum,glGetError,(void));

        OGL_INIT(void,glReadPixels,(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
        OGL_INIT(void,glPixelStorei,(GLenum, GLint));

        OGL_INIT(void,glTexEnvf,(GLenum, GLenum, GLfloat));
        OGL_INIT(void,glGenTextures,(GLsizei, GLuint*));
        OGL_INIT(void,glDeleteTextures,(GLsizei, const GLuint*));
        OGL_INIT(void,glActiveTexture,(GLenum));
        OGL_INIT(void,glBindTexture,(GLenum, GLuint));
        OGL_INIT(void,glTexImage2D,(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
        OGL_INIT(void,glTexSubImage2D,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*));
        OGL_INIT(void,glTexParameteri,(GLenum, GLenum, GLint));
        break; // kGL_Full

      case kGL_VBO:
        OGL_INIT(void,glGenBuffers,(GLsizei,GLuint*));
        OGL_INIT(void,glBindBuffer,(GLenum,GLuint));
        OGL_INIT(void,glBufferData,(GLenum,GLsizei,const void*,GLenum));
        OGL_INIT(void,glDeleteBuffers,(GLsizei, const GLuint*));
        break;  // kGL_VBO

      case kGL_FBO:
        return false;  // TODO - implement this
        break;  // kGL_FBO
    }
  }
  else
    return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::initSubsystem(VideoMode& mode)
{
  mySDLFlags |= SDL_OPENGL;

  // Set up the OpenGL attributes
  myDepth = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
  switch(myDepth)
  {
    case 15:
    case 16:
      myRGB[0] = 5; myRGB[1] = 5; myRGB[2] = 5; myRGB[3] = 0;
      break;
    case 24:
    case 32:
      myRGB[0] = 8; myRGB[1] = 8; myRGB[2] = 8; myRGB[3] = 0;
      break;
    default:  // This should never happen
      return false;
      break;
  }

  // Create the screen
  if(!setVidMode(mode))
    return false;

  // Now check to see what color components were actually created
  SDL_GL_GetAttribute( SDL_GL_RED_SIZE, (int*)&myRGB[0] );
  SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, (int*)&myRGB[1] );
  SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, (int*)&myRGB[2] );
  SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, (int*)&myRGB[3] );

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FrameBufferGL::about() const
{
  ostringstream out;
  out << "Video rendering: OpenGL mode" << endl
      << "  Vendor:     " << p_glGetString(GL_VENDOR) << endl
      << "  Renderer:   " << p_glGetString(GL_RENDERER) << endl
      << "  Version:    " << p_glGetString(GL_VERSION) << endl
      << "  Color:      " << myDepth << " bit, " << myRGB[0] << "-"
      << myRGB[1] << "-"  << myRGB[2] << "-" << myRGB[3] << ", "
#if defined(GL_BGRA) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV)
      << "GL_BGRA" << endl
#else
      << "GL_RGBA" << endl
#endif
      << "  Filter:     " << myFilterParamName << endl
      << "  Extensions: ";
  if(myVBOAvailable) out << "VBO ";
  if(myFBOAvailable) out << "FBO ";
  out << endl;
  return out.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::setVidMode(VideoMode& mode)
{
  bool inUIMode =
    myOSystem->eventHandler().state() == EventHandler::S_LAUNCHER ||
    myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER;

  // Grab the initial width and height before it's updated below
  uInt32 baseWidth = mode.image_w / mode.gfxmode.zoom;
  uInt32 baseHeight = mode.image_h / mode.gfxmode.zoom;

  // Set the zoom level
  myZoomLevel = mode.gfxmode.zoom;

  // Aspect ratio and fullscreen stretching only applies to the TIA
  if(!inUIMode)
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
    if(fullScreen() && myOSystem->settings().getBool("gl_fsmax") &&
       (mode.image_w < mode.screen_w) && (mode.image_h < mode.screen_h))
    {
      float stretchFactor = 1.0;
      float scaleX = float(mode.image_w) / mode.screen_w;
      float scaleY = float(mode.image_h) / mode.screen_h;

      if(scaleX > scaleY)
        stretchFactor = float(mode.screen_w) / mode.image_w;
      else
        stretchFactor = float(mode.screen_h) / mode.image_h;

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

  // Create screen containing GL context
  myScreen = SDL_SetVideoMode(mode.screen_w, mode.screen_h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }
  // Make sure the flags represent the current screen state
  mySDLFlags = myScreen->flags;

  // Load OpenGL function pointers
  if(loadFuncs(kGL_BASIC))
  {
    // Grab OpenGL version number
    string version((const char *)p_glGetString(GL_VERSION));
    myGLVersion = atof(version.substr(0, 3).c_str());

    myVBOAvailable = myOSystem->settings().getBool("gl_vbo") && loadFuncs(kGL_VBO);
    myFBOAvailable = false;
  }
  else
    return false;

  // Optimization hints
  // TODO - more testing required for OpenGL ES
  p_glShadeModel(GL_FLAT);
  p_glDisable(GL_CULL_FACE);
  p_glDisable(GL_DEPTH_TEST);
  p_glDisable(GL_ALPHA_TEST);
  p_glDisable(GL_LIGHTING);
  p_glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

  // Initialize GL display
  p_glViewport(0, 0, mode.screen_w, mode.screen_h);
  p_glMatrixMode(GL_PROJECTION);
  p_glLoadIdentity();
  p_glOrtho(0.0, mode.screen_w, mode.screen_h, 0.0, -1.0, 1.0);
  p_glMatrixMode(GL_MODELVIEW);
  p_glLoadIdentity();

/*
cerr << "dimensions: " << (fullScreen() ? "(full)" : "") << endl
	<< "  screen w = " << mode.screen_w << endl
	<< "  screen h = " << mode.screen_h << endl
	<< "  image x  = " << mode.image_x << endl
	<< "  image y  = " << mode.image_y << endl
	<< "  image w  = " << mode.image_w << endl
	<< "  image h  = " << mode.image_h << endl
	<< "  base w   = " << baseWidth << endl
	<< "  base h   = " << baseHeight << endl
	<< endl;
*/

  ////////////////////////////////////////////////////////////////////
  // Note that the following must be done in the order given
  // Basically, all surfaces must first be free'd before being
  // recreated
  // So, we delete the TIA surface first, then reset all other surfaces
  // (which frees all surfaces and then reloads all surfaces), then
  // re-create the TIA surface (if necessary)
  // In this way, all free()'s come before all reload()'s
  ////////////////////////////////////////////////////////////////////

  // We try to re-use the TIA surface whenever possible
  if(!inUIMode && !(myTiaSurface &&
      myTiaSurface->getWidth() == mode.image_w &&
      myTiaSurface->getHeight() == mode.image_h))
  {
    delete myTiaSurface;  myTiaSurface = NULL;
  }

  // Any previously allocated textures currently in use by various UI items
  // need to be refreshed as well (only seems to be required for OSX)
  resetSurfaces(myTiaSurface);

  // The framebuffer only takes responsibility for TIA surfaces
  // Other surfaces (such as the ones used for dialogs) are allocated
  // in the Dialog class
  if(!inUIMode)
  {
    // The actual TIA image is only half of that specified by baseWidth
    // The stretching can be done in hardware now that the TIA surface
    // and other UI surfaces are no longer tied together
    // Note that this may change in the future, when we add more
    // complex filters/scalers, but for now it's fine
    //
    // Also note that TV filtering is always available since we'll always
    // have access to Blargg filtering
    if(!myTiaSurface)
      myTiaSurface = new FBSurfaceGL(*this, baseWidth>>1, baseHeight,
                                     mode.image_w, mode.image_h, true);

    myTiaSurface->setPos(mode.image_x, mode.image_y);
    myTiaSurface->setFilter(myOSystem->settings().getString("gl_filter"));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::invalidate()
{
  p_glClear(GL_COLOR_BUFFER_BIT);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawTIA(bool fullRedraw)
{
  const TIA& tia = myOSystem->console().tia();

  // Copy the mediasource framebuffer to the RGB texture
  uInt8* currentFrame  = tia.currentFrameBuffer();
  uInt8* previousFrame = tia.previousFrameBuffer();
  uInt32 width         = tia.width();
  uInt32 height        = tia.height();
  uInt32 pitch         = myTiaSurface->pitch();
  uInt16* buffer       = (uInt16*) myTiaSurface->pixels();

  // TODO - is this fast enough?
  if(!myUsePhosphor)
  {
    uInt32 bufofsY    = 0;
    uInt32 screenofsY = 0;
    for(uInt32 y = 0; y < height; ++y )
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = 0; x < width; ++x )
      {
        const uInt32 bufofs = bufofsY + x;
        uInt8 v = currentFrame[bufofs];
        uInt8 w = previousFrame[bufofs];

        if(v != w || fullRedraw)
        {
          // If we ever get to this point, we know the current and previous
          // buffers differ.  In that case, make sure the changes are
          // are drawn in postFrameUpdate()
          myDirtyFlag = true;

          buffer[pos] = (uInt16) myDefPalette[v];
        }
        pos++;
      }
      bufofsY    += width;
      screenofsY += pitch;
    }
  }
  else
  {
    // Phosphor mode always implies a dirty update,
    // so we don't care about fullRedraw
    myDirtyFlag = true;

    uInt32 bufofsY    = 0;
    uInt32 screenofsY = 0;
    for(uInt32 y = 0; y < height; ++y )
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = 0; x < width; ++x )
      {
        const uInt32 bufofs = bufofsY + x;
        uInt8 v = currentFrame[bufofs];
        uInt8 w = previousFrame[bufofs];

        buffer[pos++] = (uInt16) myAvgPalette[v][w];
      }
      bufofsY    += width;
      screenofsY += pitch;
    }
  }

  // And blit the surface
  myTiaSurface->addDirtyRect(0, 0, 0, 0);
  myTiaSurface->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    // Now show all changes made to the texture(s)
    SDL_GL_SwapBuffers();
    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;

  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferGL::createSurface(int w, int h, bool isBase) const
{
  // Ignore 'isBase' argument; all GL surfaces are separate
  // Also, since this method will only be called for use in external
  // dialogs which cannot be scaled, the base and scaled parameters
  // are equal
  return new FBSurfaceGL((FrameBufferGL&)*this, w, h, w, h, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::scanline(uInt32 row, uInt8* data) const
{
  // Invert the row, since OpenGL rows start at the bottom
  // of the framebuffer
  const GUI::Rect& image = imageRect();
  row = image.height() + image.y() - row - 1;

  p_glPixelStorei(GL_PACK_ALIGNMENT, 1);
  p_glReadPixels(image.x(), row, image.width(), 1, GL_RGB, GL_UNSIGNED_BYTE, data);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  FBSurfaceGL implementation follows ...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceGL::FBSurfaceGL(FrameBufferGL& buffer,
                         uInt32 baseWidth, uInt32 baseHeight,
                         uInt32 scaleWidth, uInt32 scaleHeight,
                         bool allowFiltering)
  : myFB(buffer),
    myTexture(NULL),
    myTexID(0),
    myVBOID(0),
    myXOrig(0),
    myYOrig(0),
    myWidth(scaleWidth),
    myHeight(scaleHeight)
{
  // Fill buffer struct with valid data
  myTexWidth  = power_of_two(baseWidth);
  myTexHeight = power_of_two(baseHeight);
  myTexCoordW = (GLfloat) baseWidth / myTexWidth;
  myTexCoordH = (GLfloat) baseHeight / myTexHeight;

  // Based on experimentation, the following are the fastest 16-bit
  // formats for OpenGL (on all platforms)
  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE,
                  myTexWidth, myTexHeight, 16,
#if defined(GL_BGRA) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV)
                  0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
#else
                  0x0000f800, 0x000007c0, 0x0000003e, 0x00000000);
#endif
  myPitch = myTexture->pitch >> 1;

  // Associate the SDL surface with a GL texture object
  updateCoords();
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceGL::~FBSurfaceGL()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + y * myPitch + x;
  while(x++ <= x2)
    *buffer++ = (uInt16) myFB.myDefPalette[color];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + y * myPitch + x;
  while(y++ <= y2)
  {
    *buffer = (uInt16) myFB.myDefPalette[color];
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color)
{
  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(myTexture, &tmp, myFB.myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawChar(const GUI::Font* font, uInt8 chr,
                           uInt32 tx, uInt32 ty, uInt32 color)
{
  const FontDesc& desc = font->desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ') return;
    chr = desc.defaultchar;
  }
  chr -= desc.firstchar;

  // Get the bounding box of the character
  int bbw, bbh, bbx, bby;
  if(!desc.bbx)
  {
    bbw = desc.fbbw;
    bbh = desc.fbbh;
    bbx = desc.fbbx;
    bby = desc.fbby;
  }
  else
  {
    bbw = desc.bbx[chr].w;
    bbh = desc.bbx[chr].h;
    bbx = desc.bbx[chr].x;
    bby = desc.bbx[chr].y;
  }

  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * desc.fbbh));
  uInt16* buffer = (uInt16*) myTexture->pixels +
                   (ty + desc.ascent - bby - bbh) * myPitch +
                   tx + bbx;

  for(int y = 0; y < bbh; y++)
  {
    const uInt16 ptr = *tmp++;
    uInt16 mask = 0x8000;

    for(int x = 0; x < bbw; x++, mask >>= 1)
      if(ptr & mask)
        buffer[x] = (uInt16) myFB.myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawBitmap(uInt32* bitmap, uInt32 tx, uInt32 ty,
                             uInt32 color, uInt32 h)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + ty * myPitch + tx;

  for(uInt32 y = 0; y < h; ++y)
  {
    uInt32 mask = 0xF0000000;
    for(uInt32 x = 0; x < 8; ++x, mask >>= 4)
      if(bitmap[y] & mask)
        buffer[x] = (uInt16) myFB.myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawPixels(uInt32* data, uInt32 tx, uInt32 ty, uInt32 numpixels)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + ty * myPitch + tx;

  for(uInt32 i = 0; i < numpixels; ++i)
    *buffer++ = (uInt16) data[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawSurface(const FBSurface* surface, uInt32 tx, uInt32 ty)
{
  const FBSurfaceGL* s = (const FBSurfaceGL*) surface;

  SDL_Rect dstrect;
  dstrect.x = tx;
  dstrect.y = ty;
  SDL_Rect srcrect;
  srcrect.x = 0;
  srcrect.y = 0;
  srcrect.w = s->myWidth;
  srcrect.h = s->myHeight;

  SDL_BlitSurface(s->myTexture, &srcrect, myTexture, &dstrect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // OpenGL mode doesn't make use of dirty rectangles
  // It's faster to just update the entire surface
  mySurfaceIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::getPos(uInt32& x, uInt32& y) const
{
  x = myXOrig;
  y = myYOrig;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setPos(uInt32 x, uInt32 y)
{
  if(myXOrig != x || myYOrig != y)
  {
    myXOrig = x;
    myYOrig = y;
    updateCoords();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setWidth(uInt32 w)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  if(myWidth != w)
  {
    myWidth = w;
    myTexCoordW = (GLfloat) myWidth / myTexWidth;
    updateCoords();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setHeight(uInt32 h)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  if(myHeight != h)
  {
    myHeight = h;
    myTexCoordH = (GLfloat) myHeight / myTexHeight;
    updateCoords();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::translateCoords(Int32& x, Int32& y) const
{
  x -= myXOrig;
  y -= myYOrig;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::update()
{
  if(mySurfaceIsDirty)
  {
    // Texturemap complete texture to surface so we have free scaling
    // and antialiasing
    p_glActiveTexture(GL_TEXTURE0);
    p_glBindTexture(GL_TEXTURE_2D, myTexID);
    p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexWidth, myTexHeight,
#if defined(GL_BGRA) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV)
                      GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
#else
                      GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
#endif
                      myTexture->pixels);

    p_glEnableClientState(GL_VERTEX_ARRAY);
    p_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if(myFB.myVBOAvailable)
    {
      p_glBindBuffer(GL_ARRAY_BUFFER, myVBOID);
      p_glVertexPointer(2, GL_FLOAT, 0, (const GLvoid*)0);
      p_glTexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*)(8*sizeof(GLfloat)));
    }
    else
    {
      p_glVertexPointer(2, GL_FLOAT, 0, myCoord);
      p_glTexCoordPointer(2, GL_FLOAT, 0, myCoord+8);
    }
    p_glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    p_glDisableClientState(GL_VERTEX_ARRAY);
    p_glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    myFB.myDirtyFlag = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::free()
{
  p_glDeleteTextures(1, &myTexID);
  if(myFB.myVBOAvailable)
    p_glDeleteBuffers(1, &myVBOID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::reload()
{
  // This does a 'soft' reset of the surface
  // It seems that on some system (notably, OSX), creating a new SDL window
  // destroys the GL context, requiring a reload of all textures
  // However, destroying the entire FBSurfaceGL object is wasteful, since
  // it will also regenerate SDL software surfaces (which are not required
  // to be regenerated)
  // Basically, all that needs to be done is to re-call glTexImage2D with a
  // new texture ID, so that's what we do here

  p_glActiveTexture(GL_TEXTURE0);
  p_glEnable(GL_TEXTURE_2D);

  p_glGenTextures(1, &myTexID);
  p_glBindTexture(GL_TEXTURE_2D, myTexID);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Finally, create the texture in the most optimal format
  p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 myTexWidth, myTexHeight, 0,
#if defined(GL_BGRA) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV)
                 GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
#else
                 GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
#endif
                 myTexture->pixels);

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    p_glGenBuffers(1, &myVBOID);
    p_glBindBuffer(GL_ARRAY_BUFFER, myVBOID);
    p_glBufferData(GL_ARRAY_BUFFER, 16*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setFilter(const string& name)
{
  // We only do GL_NEAREST or GL_LINEAR for now
  GLint filter = GL_NEAREST;
  if(name == "linear")
    filter = GL_LINEAR;

  p_glBindTexture(GL_TEXTURE_2D, myTexID);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

  // The filtering has changed, so redraw the entire screen
  mySurfaceIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::updateCoords()
{
  // Vertex coordinates
  // Upper left (x,y)
  myCoord[0] = (GLfloat)myXOrig;
  myCoord[1] = (GLfloat)myYOrig;
  // Upper right (x+w,y)
  myCoord[2] = (GLfloat)(myXOrig + myWidth);
  myCoord[3] = (GLfloat)myYOrig;
  // Lower left (x,y+h)
  myCoord[4] = (GLfloat)myXOrig;
  myCoord[5] = (GLfloat)(myYOrig + myHeight);
  // Lower right (x+w,y+h)
  myCoord[6] = (GLfloat)(myXOrig + myWidth);
  myCoord[7] = (GLfloat)(myYOrig + myHeight);

  // Texture coordinates
  // Upper left (x,y)
  myCoord[8] = 0.0f;
  myCoord[9] = 0.0f;
  // Upper right (x+w,y)
  myCoord[10] = myTexCoordW;
  myCoord[11] = 0.0f;
  // Lower left (x,y+h)
  myCoord[12] = 0.0f;
  myCoord[13] = myTexCoordH;
  // Lower right (x+w,y+h)
  myCoord[14] = myTexCoordW;
  myCoord[15] = myTexCoordH;

  // Cache vertex and texture coordinates using vertex buffer object
  if(myFB.myVBOAvailable)
  {
    p_glBindBuffer(GL_ARRAY_BUFFER, myVBOID);
    p_glBufferData(GL_ARRAY_BUFFER, 16*sizeof(GLfloat), myCoord, GL_STATIC_DRAW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myLibraryLoaded = false;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FrameBufferGL::myGLVersion = 0.0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myVBOAvailable = false;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myFBOAvailable = false;

#endif  // DISPLAY_OPENGL
