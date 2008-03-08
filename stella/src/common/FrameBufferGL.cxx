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
// $Id: FrameBufferGL.cxx,v 1.98 2008-03-08 13:22:12 stephena Exp $
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
    myTexture(NULL),
    myHaveTexRectEXT(false),
    myFilterParamName("GL_NEAREST"),
    myWidthScaleFactor(1.0),
    myHeightScaleFactor(1.0),
    myDirtyFlag(true)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::~FrameBufferGL()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  p_glDeleteTextures(1, &myBuffer.texture);
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
bool FrameBufferGL::initSubsystem(VideoMode mode)
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
bool FrameBufferGL::setVidMode(VideoMode mode)
{
  bool inUIMode =
    myOSystem->eventHandler().state() == EventHandler::S_LAUNCHER ||
    myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER;

  myScreenDim.x = myScreenDim.y = 0;
  myScreenDim.w = mode.screen_w;
  myScreenDim.h = mode.screen_h;

  myImageDim.x = mode.image_x;
  myImageDim.y = mode.image_y;
  myImageDim.w = mode.image_w;
  myImageDim.h = mode.image_h;

  // Normally, we just scale to the given zoom level
  myWidthScaleFactor  = (float) mode.zoom;
  myHeightScaleFactor = (float) mode.zoom;

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
      float scaleX = float(myImageDim.w) / myScreenDim.w;
      float scaleY = float(myImageDim.h) / myScreenDim.h;

      if(scaleX > scaleY)
        stretchFactor = float(myScreenDim.w) / myImageDim.w;
      else
        stretchFactor = float(myScreenDim.h) / myImageDim.h;
    }
  }
  myWidthScaleFactor  *= stretchFactor;
  myHeightScaleFactor *= stretchFactor;

  // Activate aspect ratio correction in TIA mode
  int iaspect = myOSystem->settings().getInt("gl_aspect");
  float aspectFactor = 1.0;
  if(!inUIMode && iaspect < 100)
  {
    aspectFactor = float(iaspect) / 100.0;
    myWidthScaleFactor *= aspectFactor;
  }

  // Now re-calculate the dimensions
  myImageDim.w = (Uint16) (stretchFactor * aspectFactor * myImageDim.w);
  myImageDim.h = (Uint16) (stretchFactor * myImageDim.h);
  if(!fullScreen()) myScreenDim.w = myImageDim.w;
  myImageDim.x = (myScreenDim.w - myImageDim.w) / 2;
  myImageDim.y = (myScreenDim.h - myImageDim.h) / 2;

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
  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 0, mySDLFlags);
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
  p_glViewport(myImageDim.x, myImageDim.y, myImageDim.w, myImageDim.h);
  p_glShadeModel(GL_FLAT);
  p_glDisable(GL_CULL_FACE);
  p_glDisable(GL_DEPTH_TEST);
  p_glDisable(GL_ALPHA_TEST);
  p_glDisable(GL_LIGHTING);
  p_glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

  p_glMatrixMode(GL_PROJECTION);
  p_glLoadIdentity();
  p_glOrtho(0.0, myImageDim.w, myImageDim.h, 0.0, 0.0, 1.0);
  p_glMatrixMode(GL_MODELVIEW);
  p_glLoadIdentity();

  // Allocate GL textures
  createTextures();

  p_glEnable(myBuffer.target);

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
  uInt16* buffer       = (uInt16*) myTexture->pixels;

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

        if(v != w || theRedrawTIAIndicator)
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
      screenofsY += myBuffer.pitch;
    }
  }
  else
  {
    // Phosphor mode always implies a dirty update,
    // so we don't care about theRedrawTIAIndicator
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
      screenofsY += myBuffer.pitch;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    // Texturemap complete texture to surface so we have free scaling 
    // and antialiasing 
    uInt32 w = myImageDim.w, h = myImageDim.h;

    p_glTexSubImage2D(myBuffer.target, 0, 0, 0,
                      myBuffer.texture_width, myBuffer.texture_height,
                      myBuffer.format, myBuffer.type, myBuffer.pixels);
    p_glBegin(GL_QUADS);
      p_glTexCoord2f(myBuffer.tex_coord[0], myBuffer.tex_coord[1]); p_glVertex2i(0, 0);
      p_glTexCoord2f(myBuffer.tex_coord[2], myBuffer.tex_coord[1]); p_glVertex2i(w, 0);
      p_glTexCoord2f(myBuffer.tex_coord[2], myBuffer.tex_coord[3]); p_glVertex2i(w, h);
      p_glTexCoord2f(myBuffer.tex_coord[0], myBuffer.tex_coord[3]); p_glVertex2i(0, h);
    p_glEnd();

    // Overlay UI dialog boxes


    // Now show all changes made to the texture
    SDL_GL_SwapBuffers();

    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::scanline(uInt32 row, uInt8* data) const
{
  // Invert the row, since OpenGL rows start at the bottom
  // of the framebuffer
  row = myImageDim.h + myImageDim.y - row - 1;

  p_glPixelStorei(GL_PACK_ALIGNMENT, 1);
  p_glReadPixels(myImageDim.x, row, myImageDim.w, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::toggleFilter()
{
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
  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + y * myBuffer.pitch + x;
  while(x++ <= x2)
    *buffer++ = (uInt16) myDefPalette[color];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + y * myBuffer.pitch + x;
  while(y++ <= y2)
  {
    *buffer = (uInt16) myDefPalette[color];
    buffer += myBuffer.pitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color)
{
  // Fill the rectangle
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(myTexture, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawChar(const GUI::Font* font, uInt8 chr,
                             uInt32 tx, uInt32 ty, int color)
{
  // If this character is not included in the font, use the default char.
  if(chr < font->desc().firstchar ||
     chr >= font->desc().firstchar + font->desc().size)
  {
    if (chr == ' ')
      return;
    chr = font->desc().defaultchar;
  }

  const Int32 w = font->getCharWidth(chr);
  const Int32 h = font->getFontHeight();
  chr -= font->desc().firstchar;
  const uInt16* tmp = font->desc().bits + (font->desc().offset ?
                      font->desc().offset[chr] : (chr * h));

  uInt16* buffer = (uInt16*) myTexture->pixels + ty * myBuffer.pitch + tx;
  for(int y = 0; y < h; ++y)
  {
    const uInt16 ptr = *tmp++;
    uInt16 mask = 0x8000;

    for(int x = 0; x < w; ++x, mask >>= 1)
      if(ptr & mask)
        buffer[x] = (uInt16) myDefPalette[color];

    buffer += myBuffer.pitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawBitmap(uInt32* bitmap, Int32 tx, Int32 ty,
                               int color, Int32 h)
{
  uInt16* buffer = (uInt16*) myTexture->pixels + ty * myBuffer.pitch + tx;

  for(int y = 0; y < h; ++y)
  {
    uInt32 mask = 0xF0000000;
    for(int x = 0; x < 8; ++x, mask >>= 4)
      if(bitmap[y] & mask)
        buffer[x] = (uInt16) myDefPalette[color];

    buffer += myBuffer.pitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawSurface(const GUI::Surface* surface, Int32 x, Int32 y)
{
  SDL_Rect clip;
  clip.x = x;
  clip.y = y;

  SDL_BlitSurface(surface->myData, 0, myTexture, &clip);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::bytesToSurface(GUI::Surface* surface, int row,
                                   uInt8* data) const
{
  SDL_Surface* s = surface->myData;
  uInt16* pixels = (uInt16*) s->pixels;
  pixels += (row * s->pitch/2);

  int rowsize = surface->getWidth() * 3;
  for(int c = 0; c < rowsize; c += 3)
    *pixels++ = SDL_MapRGB(s->format, data[c], data[c+1], data[c+2]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::translateCoords(Int32& x, Int32& y) const
{
  // Wow, what a mess :)
  x = (Int32) ((x - myImageDim.x) / myWidthScaleFactor);
  y = (Int32) ((y - myImageDim.y) / myHeightScaleFactor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  myDirtyFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;

  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createTextures()
{
  if(myTexture)
  {
    p_glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapBuffers();
    p_glClear(GL_COLOR_BUFFER_BIT);
    SDL_FreeSurface(myTexture);
  }
  if(myBuffer.texture)  p_glDeleteTextures(1, &myBuffer.texture);
  memset(&myBuffer, 0, sizeof(glBufferType));
  myBuffer.filter = GL_NEAREST;

  // Fill buffer struct with valid data
  // This changes depending on the texturing used
  myBuffer.width  = myBaseDim.w;
  myBuffer.height = myBaseDim.h;
  myBuffer.tex_coord[0] = 0.0f;
  myBuffer.tex_coord[1] = 0.0f;
  if(myHaveTexRectEXT)
  {
    myBuffer.texture_width  = myBuffer.width;
    myBuffer.texture_height = myBuffer.height;
    myBuffer.target         = GL_TEXTURE_RECTANGLE_ARB;
    myBuffer.tex_coord[2]   = (GLfloat) myBuffer.texture_width;
    myBuffer.tex_coord[3]   = (GLfloat) myBuffer.texture_height;
  }
  else
  {
    myBuffer.texture_width  = power_of_two(myBuffer.width);
    myBuffer.texture_height = power_of_two(myBuffer.height);
    myBuffer.target         = GL_TEXTURE_2D;
    myBuffer.tex_coord[2]   = (GLfloat) myBuffer.width / myBuffer.texture_width;
    myBuffer.tex_coord[3]   = (GLfloat) myBuffer.height / myBuffer.texture_height;
  }

  // Create a texture that best suits the current display depth and system
  // This code needs to be Apple-specific, otherwise performance is
  // terrible on a Mac Mini
#if defined(MAC_OSX)
  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE,
                myBuffer.texture_width, myBuffer.texture_height, 16,
                0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
#else
  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE,
                myBuffer.texture_width, myBuffer.texture_height, 16,
                0x0000f800, 0x000007e0, 0x0000001f, 0x00000000);
#endif
  if(myTexture == NULL)
    return false;

  myBuffer.pixels = myTexture->pixels;
  switch(myTexture->format->BytesPerPixel)
  {
    case 2:  // 16-bit
      myBuffer.pitch = myTexture->pitch/2;
      break;
    case 3:  // 24-bit
      myBuffer.pitch = myTexture->pitch;
      break;
    case 4:  // 32-bit
      myBuffer.pitch = myTexture->pitch/4;
      break;
    default:
      break;
  }

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

  p_glGenTextures(1, &myBuffer.texture);
  p_glBindTexture(myBuffer.target, myBuffer.texture);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_MIN_FILTER, myBuffer.filter);
  p_glTexParameteri(myBuffer.target, GL_TEXTURE_MAG_FILTER, myBuffer.filter);

  // Finally, create the texture in the most optimal format
  GLenum tex_intformat;
#if defined (MAC_OSX)
  tex_intformat   = GL_RGB5;
  myBuffer.format = GL_BGRA;
  myBuffer.type   = GL_UNSIGNED_SHORT_1_5_5_5_REV;
#else
  tex_intformat   = GL_RGB;
  myBuffer.format = GL_RGB;
  myBuffer.type   = GL_UNSIGNED_SHORT_5_6_5;
#endif
  p_glTexImage2D(myBuffer.target, 0, tex_intformat,
                 myBuffer.texture_width, myBuffer.texture_height, 0,
                 myBuffer.format, myBuffer.type, myBuffer.pixels);

  return true;
}

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
bool FrameBufferGL::myLibraryLoaded = false;

#endif  // DISPLAY_OPENGL
