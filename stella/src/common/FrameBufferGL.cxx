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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferGL.cxx,v 1.113 2008-12-12 15:51:06 stephena Exp $
//============================================================================

#ifdef DISPLAY_OPENGL

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "MediaSrc.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "Surface.hxx"

#include "FrameBufferGL.hxx"

// Maybe this code could be cleaner ...
static void (APIENTRY* p_glClear)( GLbitfield );
static void (APIENTRY* p_glEnable)( GLenum );
static void (APIENTRY* p_glDisable)( GLenum );
static void (APIENTRY* p_glPushAttrib)( GLbitfield );
static const GLubyte* (APIENTRY* p_glGetString)( GLenum );
static void (APIENTRY* p_glHint)( GLenum, GLenum );
static void (APIENTRY* p_glShadeModel)( GLenum );

// Matrix
static void (APIENTRY* p_glMatrixMode)( GLenum );
static void (APIENTRY* p_glOrtho)( GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble );
static void (APIENTRY* p_glViewport)( GLint, GLint, GLsizei, GLsizei );
static void (APIENTRY* p_glPushMatrix)( void );
static void (APIENTRY* p_glLoadIdentity)( void );

// Drawing
static void (APIENTRY* p_glBegin)( GLenum );
static void (APIENTRY* p_glEnd)( void );
static void (APIENTRY* p_glVertex2i)( GLint, GLint );
static void (APIENTRY* p_glTexCoord2f)( GLfloat, GLfloat );

// Raster funcs
static void (APIENTRY* p_glReadPixels)( GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid* );
static void (APIENTRY* p_glPixelStorei)( GLenum, GLint );

// Texture mapping
static void (APIENTRY* p_glTexEnvf)( GLenum, GLenum, GLfloat );
static void (APIENTRY* p_glGenTextures)( GLsizei, GLuint* ); // 1.1
static void (APIENTRY* p_glDeleteTextures)( GLsizei, const GLuint* ); // 1.1
static void (APIENTRY* p_glBindTexture)( GLenum, GLuint );   // 1.1
static void (APIENTRY* p_glTexImage2D)( GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid* );
static void (APIENTRY* p_glTexSubImage2D)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid* ); // 1.1
static void (APIENTRY* p_glTexParameteri)( GLenum, GLenum, GLint );


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL(OSystem* osystem)
  : FrameBuffer(osystem),
    myBaseSurface(NULL),
    myFilterParamName("GL_NEAREST"),
    myWidthScaleFactor(1.0),
    myHeightScaleFactor(1.0),
    myHaveTexRectEXT(false),
    myDirtyFlag(true)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::~FrameBufferGL()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::loadLibrary(const string& library)
{
  if(myLibraryLoaded)
    return true;

  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    SDL_Init(SDL_INIT_VIDEO);

  // Try both the specified library and auto-detection
  bool libLoaded = (library != "" && SDL_GL_LoadLibrary(library.c_str()) >= 0);
  bool autoLoaded = false;
  if(!libLoaded) autoLoaded = (SDL_GL_LoadLibrary(0) >= 0);
  if(!libLoaded && !autoLoaded)
    return false;

  return myLibraryLoaded = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::loadFuncs()
{
  if(myLibraryLoaded)
  {
    // Fill the function pointers for GL functions
    // If anything fails, we'll know it immediately, and return false
    // Yes, this syntax is ugly, but I can type it out faster than the time
    // it takes to figure our macro magic to do it neatly
    p_glClear = (void(APIENTRY*)(GLbitfield))
      SDL_GL_GetProcAddress("glClear"); if(!p_glClear) return false;
    p_glEnable = (void(APIENTRY*)(GLenum))
      SDL_GL_GetProcAddress("glEnable"); if(!p_glEnable) return false;
    p_glDisable = (void(APIENTRY*)(GLenum))
      SDL_GL_GetProcAddress("glDisable"); if(!p_glDisable) return false;
    p_glPushAttrib = (void(APIENTRY*)(GLbitfield))
      SDL_GL_GetProcAddress("glPushAttrib"); if(!p_glPushAttrib) return false;
    p_glGetString = (const GLubyte*(APIENTRY*)(GLenum))
      SDL_GL_GetProcAddress("glGetString"); if(!p_glGetString) return false;
    p_glHint = (void(APIENTRY*)(GLenum, GLenum))
      SDL_GL_GetProcAddress("glHint"); if(!p_glHint) return false;
    p_glShadeModel = (void(APIENTRY*)(GLenum))
      SDL_GL_GetProcAddress("glShadeModel"); if(!p_glShadeModel) return false;

    p_glMatrixMode = (void(APIENTRY*)(GLenum))
      SDL_GL_GetProcAddress("glMatrixMode"); if(!p_glMatrixMode) return false;
    p_glOrtho = (void(APIENTRY*)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble))
      SDL_GL_GetProcAddress("glOrtho"); if(!p_glOrtho) return false;
    p_glViewport = (void(APIENTRY*)(GLint, GLint, GLsizei, GLsizei))
      SDL_GL_GetProcAddress("glViewport"); if(!p_glViewport) return false;
    p_glPushMatrix = (void(APIENTRY*)(void))
      SDL_GL_GetProcAddress("glPushMatrix"); if(!p_glPushMatrix) return false;
    p_glLoadIdentity = (void(APIENTRY*)(void))
      SDL_GL_GetProcAddress("glLoadIdentity"); if(!p_glLoadIdentity) return false;

    p_glBegin = (void(APIENTRY*)(GLenum))
      SDL_GL_GetProcAddress("glBegin"); if(!p_glBegin) return false;
    p_glEnd = (void(APIENTRY*)(void))
      SDL_GL_GetProcAddress("glEnd"); if(!p_glEnd) return false;
    p_glVertex2i = (void(APIENTRY*)(GLint, GLint))
      SDL_GL_GetProcAddress("glVertex2i"); if(!p_glVertex2i) return false;
    p_glTexCoord2f = (void(APIENTRY*)(GLfloat, GLfloat))
      SDL_GL_GetProcAddress("glTexCoord2f"); if(!p_glTexCoord2f) return false;

    p_glReadPixels = (void(APIENTRY*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*))
      SDL_GL_GetProcAddress("glReadPixels"); if(!p_glReadPixels) return false;
    p_glPixelStorei = (void(APIENTRY*)(GLenum, GLint))
      SDL_GL_GetProcAddress("glPixelStorei"); if(!p_glPixelStorei) return false;

    p_glTexEnvf = (void(APIENTRY*)(GLenum, GLenum, GLfloat))
      SDL_GL_GetProcAddress("glTexEnvf"); if(!p_glTexEnvf) return false;
    p_glGenTextures = (void(APIENTRY*)(GLsizei, GLuint*))
      SDL_GL_GetProcAddress("glGenTextures"); if(!p_glGenTextures) return false;
    p_glDeleteTextures = (void(APIENTRY*)(GLsizei, const GLuint*))
      SDL_GL_GetProcAddress("glDeleteTextures"); if(!p_glDeleteTextures) return false;
    p_glBindTexture = (void(APIENTRY*)(GLenum, GLuint))
      SDL_GL_GetProcAddress("glBindTexture"); if(!p_glBindTexture) return false;
    p_glTexImage2D = (void(APIENTRY*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*))
      SDL_GL_GetProcAddress("glTexImage2D"); if(!p_glTexImage2D) return false;
    p_glTexSubImage2D = (void(APIENTRY*)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*))
      SDL_GL_GetProcAddress("glTexSubImage2D"); if(!p_glTexSubImage2D) return false;
    p_glTexParameteri = (void(APIENTRY*)(GLenum, GLenum, GLint))
      SDL_GL_GetProcAddress("glTexParameteri"); if(!p_glTexParameteri) return false;
  }
  else
    return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::initSubsystem(VideoMode& mode)
{
cerr << "FrameBufferGL::initSubsystem\n";

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
  const string& extensions =
    myHaveTexRectEXT ? "GL_TEXTURE_RECTANGLE_ARB" : "None";

  ostringstream out;
  out << "Video rendering: OpenGL mode" << endl
      << "  Vendor:     " << p_glGetString(GL_VENDOR) << endl
      << "  Renderer:   " << p_glGetString(GL_RENDERER) << endl
      << "  Version:    " << p_glGetString(GL_VERSION) << endl
      << "  Color:      " << myDepth << " bit, " << myRGB[0] << "-"
      << myRGB[1] << "-"  << myRGB[2] << "-" << myRGB[3] << endl
      << "  Filter:     " << myFilterParamName << endl
      << "  Extensions: " << extensions << endl;

  return out.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::setVidMode(VideoMode& mode)
{
cerr << "setVidMode: w = " << mode.screen_w << ", h = " << mode.screen_h << endl;

  bool inUIMode =
    myOSystem->eventHandler().state() == EventHandler::S_LAUNCHER ||
    myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER;

  // Grab the initial width and height before it's updated below
  uInt32 baseWidth = mode.image_w / mode.gfxmode.zoom;
  uInt32 baseHeight = mode.image_h / mode.gfxmode.zoom;

  // FIXME - look at actual videomode type
  // Normally, we just scale to the given zoom level
  myWidthScaleFactor  = (float) mode.gfxmode.zoom;
  myHeightScaleFactor = (float) mode.gfxmode.zoom;

  // Activate aspect ratio correction in TIA mode
  int iaspect = myOSystem->settings().getInt("gl_aspect");
  if(!inUIMode && iaspect < 100)
  {
    float aspectFactor = float(iaspect) / 100.0;
    myWidthScaleFactor *= aspectFactor;
    mode.image_w = (uInt16)(float(mode.image_w) * aspectFactor);
  }

  // Activate stretching if its been requested in fullscreen mode
  float stretchFactor = 1.0;
  if(fullScreen() && (mode.image_w < mode.screen_w) &&
     (mode.image_h < mode.screen_h))
  {
    const string& gl_fsmax = myOSystem->settings().getString("gl_fsmax");

    // Only stretch in certain modes
    if((gl_fsmax == "always") || 
       (inUIMode && gl_fsmax == "ui") ||
       (!inUIMode && gl_fsmax == "tia"))
    {
      float scaleX = float(mode.image_w) / mode.screen_w;
      float scaleY = float(mode.image_h) / mode.screen_h;

      if(scaleX > scaleY)
        stretchFactor = float(mode.screen_w) / mode.image_w;
      else
        stretchFactor = float(mode.screen_h) / mode.image_h;
    }
  }
  myWidthScaleFactor  *= stretchFactor;
  myHeightScaleFactor *= stretchFactor;

  // Now re-calculate the dimensions
  mode.image_w = (Uint16) (stretchFactor * mode.image_w);
  mode.image_h = (Uint16) (stretchFactor * mode.image_h);
  if(!fullScreen()) mode.screen_w = mode.image_w;
  mode.image_x = (mode.screen_w - mode.image_w) / 2;
  mode.image_y = (mode.screen_h - mode.image_h) / 2;

  SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   myRGB[0] );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, myRGB[1] );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  myRGB[2] );
  SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, myRGB[3] );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

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

  // Reload OpenGL function pointers.  This only seems to be needed for Windows
  // Vista, but it shouldn't hurt on other systems.
  if(!loadFuncs())
    return false;

  // Check for some extensions that can potentially speed up operation
  // Don't use it if we've been instructed not to
  if(myOSystem->settings().getBool("gl_texrect"))
  {
    const char* extensions = (const char *) p_glGetString(GL_EXTENSIONS);
    myHaveTexRectEXT = strstr(extensions, "ARB_texture_rectangle") != NULL;
  }
  else
    myHaveTexRectEXT = false;

  // Initialize GL display
  p_glViewport(mode.image_x, mode.image_y, mode.image_w, mode.image_h);
  p_glShadeModel(GL_FLAT);
  p_glDisable(GL_CULL_FACE);
  p_glDisable(GL_DEPTH_TEST);
  p_glDisable(GL_ALPHA_TEST);
  p_glDisable(GL_LIGHTING);
  p_glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

  p_glMatrixMode(GL_PROJECTION);
  p_glLoadIdentity();
  p_glOrtho(0.0, mode.image_w, mode.image_h, 0.0, 0.0, 1.0);
  p_glMatrixMode(GL_MODELVIEW);
  p_glPushMatrix();
  p_glLoadIdentity();

  // Allocate GL textures
cerr << "dimensions: " << endl
	<< "  basew  = " << baseWidth << endl
	<< "  baseh  = " << baseHeight << endl
	<< "  scrw   = " << mode.screen_w << endl
	<< "  scrh   = " << mode.screen_h << endl
	<< "  imagex = " << mode.image_x << endl
	<< "  imagey = " << mode.image_y << endl
	<< "  imagew = " << mode.image_w << endl
	<< "  imageh = " << mode.image_h << endl
	<< endl;

  delete myBaseSurface;
  myBaseSurface = new FBSurfaceGL(*this, baseWidth, baseHeight,
                                   mode.image_w, mode.image_h);

  // Old textures currently in use by various UI items need to be
  // refreshed as well (only seems to be required for OSX)
  reloadSurfaces();

  // Make sure any old parts of the screen are erased
  p_glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapBuffers();
  p_glClear(GL_COLOR_BUFFER_BIT);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawMediaSource()
{
  MediaSource& mediasrc = myOSystem->console().mediaSource();

  // Copy the mediasource framebuffer to the RGB texture
  uInt8* currentFrame  = mediasrc.currentFrameBuffer();
  uInt8* previousFrame = mediasrc.previousFrameBuffer();
  uInt32 width         = mediasrc.width();
  uInt32 height        = mediasrc.height();
  uInt32 pitch         = myBaseSurface->pitch();
  uInt16* buffer       = (uInt16*) myBaseSurface->pixels();

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

        if(v != w || myRedrawEntireFrame)
        {
          // If we ever get to this point, we know the current and previous
          // buffers differ.  In that case, make sure the changes are
          // are drawn in postFrameUpdate()
          myDirtyFlag = true;

          buffer[pos] = buffer[pos+1] = (uInt16) myDefPalette[v];
        }
        pos += 2;
      }
      bufofsY    += width;
      screenofsY += pitch;
    }
  }
  else
  {
    // Phosphor mode always implies a dirty update,
    // so we don't care about myRedrawEntireFrame
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
        buffer[pos++] = (uInt16) myAvgPalette[v][w];
      }
      bufofsY    += width;
      screenofsY += pitch;
    }
  }

  // And blit the surface
  if(myDirtyFlag)
  {
    myBaseSurface->addDirtyRect(0, 0, 0, 0);
    myBaseSurface->update();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::postFrameUpdate()
{
//static int FCOUNT = 0;
  if(myDirtyFlag)
  {
    // Now show all changes made to the texture
    SDL_GL_SwapBuffers();
    myDirtyFlag = false;
//cerr << FCOUNT++ % 2 << " : SWAP buffers" << endl;
//cerr << "--------------------------------------------------------------------" << endl;
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
Uint32 FrameBufferGL::mapRGB(Uint8 r, Uint8 g, Uint8 b) const
{
  return myBaseSurface->mapRGB(r, g, b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::toggleFilter()
{
/*
  if(myBuffer.filter == GL_NEAREST)
  {
    myBuffer.filter = GL_LINEAR;
    myOSystem->settings().setString("gl_filter", "linear");
    showMessage("Filtering: GL_LINEAR");
  }
  else
  {
    myBuffer.filter = GL_NEAREST;
    myOSystem->settings().setString("gl_filter", "nearest");
    showMessage("Filtering: GL_NEAREST");
  }

  p_glBindTexture(myBuffer.target, myBuffer.texture);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_MAG_FILTER, myBuffer.filter);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_MIN_FILTER, myBuffer.filter);

  // The filtering has changed, so redraw the entire screen
  myRedrawEntireFrame = true;
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBufferGL::createSurface(int w, int h, bool isBase) const
{
  // Ignore 'isBase' argument; all GL surfaces are separate
  // Also, since this method will only be called for use in external
  // dialogs which cannot be scaled, the base and scaled parameters
  // are equal
  return new FBSurfaceGL((FrameBufferGL&)*this, w, h, w, h);
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
                         uInt32 scaleWidth, uInt32 scaleHeight)
  : myFB(buffer),
    myTexture(NULL),
    myTexID(0),
    myXOrig(0),
    myYOrig(0),
    myWidth(scaleWidth),
    myHeight(scaleHeight)
{
cerr << "  FBSurfaceGL::FBSurfaceGL: w = " << baseWidth << ", h = " << baseHeight << endl;

  // Fill buffer struct with valid data
  // This changes depending on the texturing used
  myTexCoord[0] = 0.0f;
  myTexCoord[1] = 0.0f;
  if(myFB.myHaveTexRectEXT)
  {
    myTexWidth    = baseWidth;
    myTexHeight   = baseHeight;
    myTexTarget   = GL_TEXTURE_RECTANGLE_ARB;
    myTexCoord[2] = (GLfloat) myTexWidth;
    myTexCoord[3] = (GLfloat) myTexHeight;
  }
  else
  {
    myTexWidth    = power_of_two(baseWidth);
    myTexHeight   = power_of_two(baseHeight);
    myTexTarget   = GL_TEXTURE_2D;
    myTexCoord[2] = (GLfloat) baseWidth / myTexWidth;
    myTexCoord[3] = (GLfloat) baseHeight / myTexHeight;
  }

  // Based on experimentation, the following is the fastest 16-bit
  // format for OpenGL (on all platforms)
  myTexFormat   = GL_BGRA;
  myTexType     = GL_UNSIGNED_SHORT_1_5_5_5_REV;
  myTexture     = SDL_CreateRGBSurface(SDL_SWSURFACE,
                    myTexWidth, myTexHeight, 16,
                    0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);

  switch(myTexture->format->BytesPerPixel)
  {
    case 2:  // 16-bit
      myPitch = myTexture->pitch/2;
      break;
    case 3:  // 24-bit
      myPitch = myTexture->pitch;
      break;
    case 4:  // 32-bit
      myPitch = myTexture->pitch/4;
      break;
    default:
      break;
  }

  // Associate the SDL surface with a GL texture object
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceGL::~FBSurfaceGL()
{
cerr << "  FBSurfaceGL::~FBSurfaceGL(): " << this << endl;

  if(myTexture)
    SDL_FreeSurface(myTexture);

  p_glDeleteTextures(1, &myTexID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + y * myPitch + x;
  while(x++ <= x2)
    *buffer++ = (uInt16) myFB.myDefPalette[color];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + y * myPitch + x;
  while(y++ <= y2)
  {
    *buffer = (uInt16) myFB.myDefPalette[color];
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color)
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
                           uInt32 tx, uInt32 ty, int color)
{
  const FontDesc& desc = font->desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ') return;
    chr = desc.defaultchar;
  }

  const Int32 w = font->getCharWidth(chr);
  const Int32 h = font->getFontHeight();
  chr -= desc.firstchar;
  const uInt32* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * h));

  uInt16* buffer = (uInt16*) myTexture->pixels + ty * myPitch + tx;
  for(int y = 0; y < h; ++y)
  {
    const uInt32 ptr = *tmp++;
    if(ptr)
    {
      uInt32 mask = 0x80000000;
      for(int x = 0; x < w; ++x, mask >>= 1)
        if(ptr & mask)
          buffer[x] = (uInt16) myFB.myDefPalette[color];
    }
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::drawBitmap(uInt32* bitmap, Int32 tx, Int32 ty,
                             int color, Int32 h)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + ty * myPitch + tx;

  for(int y = 0; y < h; ++y)
  {
    uInt32 mask = 0xF0000000;
    for(int x = 0; x < 8; ++x, mask >>= 4)
      if(bitmap[y] & mask)
        buffer[x] = (uInt16) myFB.myDefPalette[color];

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
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
  myXOrig = x;
  myYOrig = y;
#if 0
  // Only non-base surfaces can be arbitrarily 'moved'
  if(!myIsBaseSurface)
  {
    // Make sure pitch is valid
    recalc();

    myXOrig = x;
    myYOrig = y;
    myXOffset = myYOffset = myBaseOffset = 0;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setWidth(uInt32 w)
{
//cerr << "  BEFORE: w = " << myWidth <<", texcoord[2] = " << myTexCoord[2] << endl;

  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  myWidth = w;

  if(myFB.myHaveTexRectEXT)
    myTexCoord[2] = (GLfloat) myWidth;
  else
    myTexCoord[2] = (GLfloat) myWidth / myTexWidth;

//cerr << "  AFTER:  w = " << myWidth <<", texcoord[2] = " << myTexCoord[2] << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setHeight(uInt32 h)
{
//cerr << "  BEFORE: h = " << myHeight <<", texcoord[3] = " << myTexCoord[3] << endl;

  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  myHeight = h;

  if(myFB.myHaveTexRectEXT)
    myTexCoord[3] = (GLfloat) myHeight;
  else
    myTexCoord[3] = (GLfloat) myHeight / myTexHeight;

//cerr << "  AFTER:  h = " << myHeight <<", texcoord[3] = " << myTexCoord[3] << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::translateCoords(Int32& x, Int32& y) const
{
// TODO - this doesn't work if aspect ratio is used
  x -= myXOrig;
  y -= myYOrig;

/*
  // Wow, what a mess :)
  x = (Int32) ((x - myImageDim.x) / myWidthScaleFactor);
  y = (Int32) ((y - myImageDim.y) / myHeightScaleFactor);
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::update()
{
  if(mySurfaceIsDirty)
  {
    // Texturemap complete texture to surface so we have free scaling
    // and antialiasing 
    p_glBindTexture(myTexTarget, myTexID);
    p_glTexSubImage2D(myTexTarget, 0, 0, 0, myTexWidth, myTexHeight,
                      myTexFormat, myTexType, myTexture->pixels);
    p_glBegin(GL_QUADS);
      p_glTexCoord2f(myTexCoord[0], myTexCoord[1]);
      p_glVertex2i(myXOrig, myYOrig);

      p_glTexCoord2f(myTexCoord[2], myTexCoord[1]);
      p_glVertex2i(myXOrig + myWidth, myYOrig);

      p_glTexCoord2f(myTexCoord[2], myTexCoord[3]);
      p_glVertex2i(myXOrig + myWidth, myYOrig + myHeight);

      p_glTexCoord2f(myTexCoord[0], myTexCoord[3]);
      p_glVertex2i(myXOrig, myYOrig + myHeight);
    p_glEnd();
    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    myFB.myDirtyFlag = true;
  }
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

/*
  // Create an OpenGL texture from the SDL texture
  const string& filter = myOSystem->settings().getString("gl_filter");
  if(filter == "linear")
  {
    myBuffer.filter   = GL_LINEAR;
    myFilterParamName = "GL_LINEAR";
  }
  else if(filter == "nearest")
  {
    myBuffer.filter   = GL_NEAREST;
    myFilterParamName = "GL_NEAREST";
  }
*/

  p_glDeleteTextures(1, &myTexID);
  p_glGenTextures(1, &myTexID);
  p_glBindTexture(myTexTarget, myTexID);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//  p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, myBuffer.filter);
//  p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, myBuffer.filter);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Finally, create the texture in the most optimal format
  p_glTexImage2D(myTexTarget, 0, GL_RGB5,
                 myTexWidth, myTexHeight, 0,
                 myTexFormat, myTexType, myTexture->pixels);

  p_glEnable(myTexTarget);

cerr << "  ==> FBSurfaceGL::reload(): myTexID = " << myTexID << endl;
}

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Surface* FrameBufferGL::createSurface(int width, int height) const
{
  SDL_PixelFormat* fmt = myTexture->format;
  SDL_Surface* data =
    SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16,
                         fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  return data ? new GUI::Surface(width, height, data) : NULL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawSurface(const GUI::Surface* surface, Int32 x, Int32 y)
{
  SDL_Rect dstrect;
  dstrect.x = x;
  dstrect.y = y;
  SDL_Rect srcrect;
  srcrect.x = 0;
  srcrect.y = 0;
  srcrect.w = surface->myClipWidth;
  srcrect.h = surface->myClipHeight;

  SDL_BlitSurface(surface->myData, &srcrect, myTexture, &dstrect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::bytesToSurface(GUI::Surface* surface, int row,
                                   uInt8* data, int rowbytes) const
{
  SDL_Surface* s = surface->myData;
  uInt16* pixels = (uInt16*) s->pixels;
  pixels += (row * s->pitch/2);

  for(int c = 0; c < rowbytes; c += 3)
    *pixels++ = SDL_MapRGB(s->format, data[c], data[c+1], data[c+2]);
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myLibraryLoaded = false;

#endif  // DISPLAY_OPENGL
