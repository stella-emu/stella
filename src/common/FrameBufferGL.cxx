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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
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

#include "FBSurfaceGL.hxx"
#include "FBSurfaceTIA.hxx"
#include "FrameBufferGL.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL(OSystem* osystem)
  : FrameBuffer(osystem),
    myFilterType(kNone),
    myTiaSurface(NULL),
    myFilterParamName("GL_NEAREST"),
    myDirtyFlag(true)
{
  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceGL object)
  // since the structure may be needed before any FBSurface's have
  // been created
  SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 16,
#ifdef HAVE_GL_BGRA
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
#define OGL_INIT(NAME,RET,FUNC,PARAMS) \
  p_gl.NAME = (RET(APIENTRY*)PARAMS) SDL_GL_GetProcAddress(#FUNC); if(!p_gl.NAME) return false

  if(myLibraryLoaded)
  {
    // Fill the function pointers for GL functions
    // If anything fails, we'll know it immediately, and return false
    switch(functionality)
    {
      case kGL_BASIC:
        OGL_INIT(Clear,void,glClear,(GLbitfield));
        OGL_INIT(Enable,void,glEnable,(GLenum));
        OGL_INIT(Disable,void,glDisable,(GLenum));
        OGL_INIT(PushAttrib,void,glPushAttrib,(GLbitfield));
        OGL_INIT(GetString,const GLubyte*,glGetString,(GLenum));
        OGL_INIT(Hint,void,glHint,(GLenum, GLenum));
        OGL_INIT(ShadeModel,void,glShadeModel,(GLenum));
        OGL_INIT(MatrixMode,void,glMatrixMode,(GLenum));
        OGL_INIT(Ortho,void,glOrtho,(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble));
        OGL_INIT(Viewport,void,glViewport,(GLint, GLint, GLsizei, GLsizei));
        OGL_INIT(LoadIdentity,void,glLoadIdentity,(void));
        OGL_INIT(Translatef,void,glTranslatef,(GLfloat,GLfloat,GLfloat));
        OGL_INIT(EnableClientState,void,glEnableClientState,(GLenum));
        OGL_INIT(DisableClientState,void,glDisableClientState,(GLenum));
        OGL_INIT(VertexPointer,void,glVertexPointer,(GLint,GLenum,GLsizei,const GLvoid*));
        OGL_INIT(TexCoordPointer,void,glTexCoordPointer,(GLint,GLenum,GLsizei,const GLvoid*));
        OGL_INIT(DrawArrays,void,glDrawArrays,(GLenum,GLint,GLsizei));
        OGL_INIT(ReadPixels,void,glReadPixels,(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
        OGL_INIT(PixelStorei,void,glPixelStorei,(GLenum, GLint));
        OGL_INIT(TexEnvf,void,glTexEnvf,(GLenum, GLenum, GLfloat));
        OGL_INIT(GenTextures,void,glGenTextures,(GLsizei, GLuint*));
        OGL_INIT(DeleteTextures,void,glDeleteTextures,(GLsizei, const GLuint*));
        OGL_INIT(ActiveTexture,void,glActiveTexture,(GLenum));
        OGL_INIT(BindTexture,void,glBindTexture,(GLenum, GLuint));
        OGL_INIT(TexImage2D,void,glTexImage2D,(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
        OGL_INIT(TexSubImage2D,void,glTexSubImage2D,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*));
        OGL_INIT(TexParameteri,void,glTexParameteri,(GLenum, GLenum, GLint));
        OGL_INIT(GetError,GLenum,glGetError,(void));
        OGL_INIT(Color4f,void,glColor4f,(GLfloat,GLfloat,GLfloat,GLfloat));
        OGL_INIT(BlendFunc,void,glBlendFunc,(GLenum,GLenum));
        break; // kGL_Full

      case kGL_VBO:
        OGL_INIT(GenBuffers,void,glGenBuffers,(GLsizei,GLuint*));
        OGL_INIT(BindBuffer,void,glBindBuffer,(GLenum,GLuint));
        OGL_INIT(BufferData,void,glBufferData,(GLenum,GLsizei,const void*,GLenum));
        OGL_INIT(DeleteBuffers,void,glDeleteBuffers,(GLsizei, const GLuint*));
        break;  // kGL_VBO
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
      << "  Vendor:     " << p_gl.GetString(GL_VENDOR) << endl
      << "  Renderer:   " << p_gl.GetString(GL_RENDERER) << endl
      << "  Version:    " << p_gl.GetString(GL_VERSION) << endl
      << "  Color:      " << myDepth << " bit, " << myRGB[0] << "-"
      << myRGB[1] << "-"  << myRGB[2] << "-" << myRGB[3] << ", "
#ifdef HAVE_GL_BGRA
      << "GL_BGRA" << endl
#else
      << "GL_RGBA" << endl
#endif
      << "  Filter:     " << myFilterParamName << endl
      << "  Extensions: ";
  if(myVBOAvailable) out << "VBO ";
  out << endl;
  return out.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::setVidMode(VideoMode& mode)
{
  bool inTIAMode =
    myOSystem->eventHandler().state() != EventHandler::S_LAUNCHER &&
    myOSystem->eventHandler().state() != EventHandler::S_DEBUGGER;

  // Grab the initial height before it's updated below
  // We need it for the creating the TIA surface
  uInt32 baseHeight = mode.image_h / mode.gfxmode.zoom;

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
    string version((const char *)p_gl.GetString(GL_VERSION));
    myGLVersion = atof(version.substr(0, 3).c_str());

    myVBOAvailable = myOSystem->settings().getBool("gl_vbo") && loadFuncs(kGL_VBO);
  }
  else
    return false;

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
  p_gl.Translatef(0.375, 0.375, 0.0);

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

    myTiaSurface->enableScanlines(myFilterType == kBlarggNTSC);
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
void FrameBufferGL::invalidate()
{
  p_gl.Clear(GL_COLOR_BUFFER_BIT);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawTIA(bool fullRedraw)
{
  // The TIA surface takes all responsibility for drawing
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
  if(myTiaSurface)
  {
    myUsePhosphor   = enable;
    myPhosphorBlend = blend;
    myFilterType = enable ? kPhosphor : kNone;
    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::enableNTSC(bool enable)
{
  if(myTiaSurface)
  {
    myFilterType = enable ? kBlarggNTSC : myUsePhosphor ? kPhosphor : kNone;
    myTiaSurface->updateCoords();

    myTiaSurface->enableScanlines(myFilterType == kBlarggNTSC);
    myTiaSurface->setScanIntensity(myOSystem->settings().getInt("tv_scanlines"));
    myTiaSurface->setTexInterpolation(myOSystem->settings().getBool("gl_inter"));
    myTiaSurface->setScanInterpolation(myOSystem->settings().getBool("tv_scaninter"));

    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBufferGL::changeScanlines(int relative, int absolute)
{
  int intensity = myTiaSurface->myScanlineIntensityI;
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
void FrameBufferGL::enableScanlineInterpolation(bool enable)
{
  if(myTiaSurface)
  {
    myTiaSurface->setScanInterpolation(enable);
    myRedrawEntireFrame = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::setTIAPalette(const uInt32* palette)
{
  myTiaSurface->setTIAPalette(palette);
  FrameBuffer::setTIAPalette(palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferGL::createSurface(int w, int h, bool isBase) const
{
  // Ignore 'isBase' argument; all GL surfaces are separate
  // Also, this method will only be called for use in external dialogs.
  // and never used for TIA surfaces
  return new FBSurfaceGL((FrameBufferGL&)*this, w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::scanline(uInt32 row, uInt8* data) const
{
  // Invert the row, since OpenGL rows start at the bottom
  // of the framebuffer
  const GUI::Rect& image = imageRect();
  row = image.height() + image.y() - row - 1;

  p_gl.PixelStorei(GL_PACK_ALIGNMENT, 1);
  p_gl.ReadPixels(image.x(), row, image.width(), 1, GL_RGB, GL_UNSIGNED_BYTE, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myLibraryLoaded = false;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FrameBufferGL::myGLVersion = 0.0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myVBOAvailable = false;

#endif  // DISPLAY_OPENGL
