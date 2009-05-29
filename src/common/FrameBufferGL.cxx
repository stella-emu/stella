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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifdef DISPLAY_OPENGL

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>
#include <time.h>

#include "bspf.hxx"

#include "Console.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "Surface.hxx"
#include "TIA.hxx"
#include "GLShaderProgs.hxx"

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
OGL_DECLARE(void,glPushMatrix,(void));
OGL_DECLARE(void,glLoadIdentity,(void));
OGL_DECLARE(void,glBegin,(GLenum));
OGL_DECLARE(void,glEnd,(void));
OGL_DECLARE(void,glVertex2i,(GLint, GLint));
OGL_DECLARE(void,glTexCoord2f,(GLfloat, GLfloat));
OGL_DECLARE(void,glReadPixels,(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
OGL_DECLARE(void,glPixelStorei,(GLenum, GLint));
OGL_DECLARE(void,glTexEnvf,(GLenum, GLenum, GLfloat));
OGL_DECLARE(void,glGenTextures,(GLsizei, GLuint*));
OGL_DECLARE(void,glDeleteTextures,(GLsizei, const GLuint*));
OGL_DECLARE(void,glBindTexture,(GLenum, GLuint));
OGL_DECLARE(void,glTexImage2D,(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
OGL_DECLARE(void,glTexSubImage2D,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*));
OGL_DECLARE(void,glTexParameteri,(GLenum, GLenum, GLint));
OGL_DECLARE(GLuint,glCreateShader,(GLenum));
OGL_DECLARE(void,glDeleteShader,(GLuint));
OGL_DECLARE(void,glShaderSource,(GLuint, int, const char**, int));
OGL_DECLARE(void,glCompileShader,(GLuint));
OGL_DECLARE(GLuint,glCreateProgram,(void));
OGL_DECLARE(void,glDeleteProgram,(GLuint));
OGL_DECLARE(void,glAttachShader,(GLuint, GLuint));
OGL_DECLARE(void,glLinkProgram,(GLuint));
OGL_DECLARE(void,glUseProgram,(GLuint));
OGL_DECLARE(GLint,glGetUniformLocation,(GLuint, const char*));
OGL_DECLARE(void,glUniform1i,(GLint, GLint));
OGL_DECLARE(void,glUniform1f,(GLint, GLfloat));
OGL_DECLARE(void,glCopyTexImage2D,(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint));
OGL_DECLARE(void,glCopyTexSubImage2D,(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei));
OGL_DECLARE(void,glActiveTexture,(GLenum));
OGL_DECLARE(void,glGetIntegerv,(GLenum, GLint*));
OGL_DECLARE(void,glTexEnvi,(GLenum, GLenum, GLint));
OGL_DECLARE(void,glMultiTexCoord2f,(GLenum, GLfloat, GLfloat));
OGL_DECLARE(GLenum,glGetError,(void));


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL(OSystem* osystem)
  : FrameBuffer(osystem),
    myTiaSurface(NULL),
    myFilterParamName("GL_NEAREST"),
    myHaveTexRectEXT(false),
    myDirtyFlag(true)
{
  // We need a pixel format for palette value calculations
  // It's done this way (vs directly accessing a FBSurfaceGL object)
  // since the structure may be needed before any FBSurface's have
  // been created
  SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 16,
                     0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
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
bool FrameBufferGL::loadFuncs()
{
#define OGL_INIT(RET,FUNC,PARAMS) \
  p_ ## FUNC = (RET(APIENTRY*)PARAMS) SDL_GL_GetProcAddress(#FUNC); if(!p_ ## FUNC) return false

  if(myLibraryLoaded)
  {
    // Fill the function pointers for GL functions
    // If anything fails, we'll know it immediately, and return false
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
    OGL_INIT(void,glPushMatrix,(void));
    OGL_INIT(void,glLoadIdentity,(void));

    OGL_INIT(void,glBegin,(GLenum));
    OGL_INIT(void,glEnd,(void));
    OGL_INIT(void,glVertex2i,(GLint, GLint));
    OGL_INIT(void,glTexCoord2f,(GLfloat, GLfloat));

    OGL_INIT(void,glReadPixels,(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
    OGL_INIT(void,glPixelStorei,(GLenum, GLint));

    OGL_INIT(void,glTexEnvf,(GLenum, GLenum, GLfloat));
    OGL_INIT(void,glGenTextures,(GLsizei, GLuint*));
    OGL_INIT(void,glDeleteTextures,(GLsizei, const GLuint*));
    OGL_INIT(void,glBindTexture,(GLenum, GLuint));
    OGL_INIT(void,glTexImage2D,(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
    OGL_INIT(void,glTexSubImage2D,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*));
    OGL_INIT(void,glTexParameteri,(GLenum, GLenum, GLint));

    OGL_INIT(GLuint,glCreateShader,(GLenum));
    OGL_INIT(void,glDeleteShader,(GLuint));
    OGL_INIT(void,glShaderSource,(GLuint, int, const char**, int));
    OGL_INIT(void,glCompileShader,(GLuint));
    OGL_INIT(GLuint,glCreateProgram,(void));
    OGL_INIT(void,glDeleteProgram,(GLuint));
    OGL_INIT(void,glAttachShader,(GLuint, GLuint));
    OGL_INIT(void,glLinkProgram,(GLuint));
    OGL_INIT(void,glUseProgram,(GLuint));
    OGL_INIT(GLint,glGetUniformLocation,(GLuint, const char*));
    OGL_INIT(void,glUniform1i,(GLint, GLint));
    OGL_INIT(void,glUniform1f,(GLint, GLfloat));
    OGL_INIT(void,glCopyTexImage2D,(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint));
    OGL_INIT(void,glCopyTexSubImage2D,(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei));
    OGL_INIT(void,glActiveTexture,(GLenum));
    OGL_INIT(void,glGetIntegerv,(GLenum, GLint*));
    OGL_INIT(void,glTexEnvi,(GLenum, GLenum, GLint));
    OGL_INIT(void,glMultiTexCoord2f,(GLenum, GLfloat, GLfloat));
    OGL_INIT(GLenum,glGetError,(void));
  }
  else
    return false;

  // Grab OpenGL version number
  string version((const char *)p_glGetString(GL_VERSION));
  myGLVersion = atof(version.substr(0, 3).c_str());

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
  bool inUIMode =
    myOSystem->eventHandler().state() == EventHandler::S_LAUNCHER ||
    myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER;

  // Grab the initial width and height before it's updated below
  uInt32 baseWidth = mode.image_w / mode.gfxmode.zoom;
  uInt32 baseHeight = mode.image_h / mode.gfxmode.zoom;

  // Update the graphics filter options
  myUseTexture = true;  myTextureStag = false;
  const string& tv_tex = myOSystem->settings().getString("tv_tex");
  if(tv_tex == "stag")        myTextureStag = true;
  else if(tv_tex != "normal") myUseTexture = false;

  myUseBleed = true;
  const string& tv_bleed = myOSystem->settings().getString("tv_bleed");
  if(tv_bleed == "low")         myBleedQuality = 0;
  else if(tv_bleed == "medium") myBleedQuality = 1;
  else if(tv_bleed == "high")   myBleedQuality = 2;
  else  myUseBleed = false;

  myUseNoise = true;
  const string& tv_noise = myOSystem->settings().getString("tv_noise");
  if(tv_noise == "low")         myNoiseQuality = 5;
  else if(tv_noise == "medium") myNoiseQuality = 15;
  else if(tv_noise == "high")   myNoiseQuality = 25;
  else  myUseNoise = false;

  myUseGLPhosphor = myOSystem->settings().getBool("tv_phos");

  // Set the zoom level
  myZoomLevel = mode.gfxmode.zoom;

  // Aspect ratio and fullscreen stretching only applies to the TIA
  if(!inUIMode)
  {
    // Aspect ratio (depends on whether NTSC or PAL is detected)
    const string& frate = myOSystem->console().about().InitialFrameRate;
    int aspect =
      myOSystem->settings().getInt(frate == "60" ? "gl_aspectn" : "gl_aspectp");
    mode.image_w = (uInt16)(float(mode.image_w * aspect) / 100.0);

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
//  if(myOSystem->settings().getBool("gl_accel"))
//    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

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
  p_glViewport(0, 0, mode.screen_w, mode.screen_h);
  p_glShadeModel(GL_FLAT);
  p_glDisable(GL_CULL_FACE);
  p_glDisable(GL_DEPTH_TEST);
  p_glDisable(GL_ALPHA_TEST);
  p_glDisable(GL_LIGHTING);
  p_glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

  p_glMatrixMode(GL_PROJECTION);
  p_glLoadIdentity();
  p_glOrtho(0.0, mode.screen_w, mode.screen_h, 0.0, -1.0, 1.0);
  p_glMatrixMode(GL_MODELVIEW);
  p_glPushMatrix();
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

  // The framebuffer only takes responsibility for TIA surfaces
  // Other surfaces (such as the ones used for dialogs) are allocated
  // in the Dialog class
  delete myTiaSurface;  myTiaSurface = NULL;

  // Any previously allocated textures currently in use by various UI items
  // need to be refreshed as well (only seems to be required for OSX)
  resetSurfaces();

  if(!inUIMode)
  {
    // The actual TIA image is only half of that specified by baseWidth
    // The stretching can be done in hardware now that the TIA surface
    // and other UI surfaces are no longer tied together
    // Note that this may change in the future, when we add more
    // complex filters/scalers, but for now it's fine
    // Also note that TV filtering is only available with OpenGL 2.0+
    myTiaSurface = new FBSurfaceGL(*this, baseWidth>>1, baseHeight,
                                     mode.image_w, mode.image_h,
                                     myGLVersion >= 2.0);
    myTiaSurface->setPos(mode.image_x, mode.image_y);
    myTiaSurface->setFilter(myOSystem->settings().getString("gl_filter"));
  }

  // Make sure any old parts of the screen are erased
  p_glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapBuffers();
  p_glClear(GL_COLOR_BUFFER_BIT);

  return true;
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
    // Now show all changes made to the texture
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
                         uInt32 scaleWidth, uInt32 scaleHeight,
                         bool allowFiltering)
  : myFB(buffer),
    myTexture(NULL),
    myTexID(0),
    myFilterTexID(0),
    mySubMaskTexID(0),
    myNoiseMaskTexID(NULL),
    myPhosphorTexID(0),
    mySubpixelTexture(NULL),
    myNoiseTexture(NULL),
    myXOrig(0),
    myYOrig(0),
    myWidth(scaleWidth),
    myHeight(scaleHeight),
    myBleedProgram(0),
    myTextureProgram(0),
    myNoiseProgram(0),
    myPhosphorProgram(0),
    myTextureNoiseProgram(0),
    myNoiseNum(0),
    myTvFiltersEnabled(allowFiltering)
{
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

    // This is a quick fix, a better one will come later
    myTvFiltersEnabled = false;
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
  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE,
                  myTexWidth, myTexHeight, 16,
                  0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
  myPitch = myTexture->pitch >> 1;

  // Only do this if TV filters enabled, otherwise it won't be used anyway
  if(myTvFiltersEnabled)
  {
    // For a reason that hasn't been investigated yet, some of the filter and mask
    // texture coordinates need to be swapped in order for it not to render upside down

    myFilterTexCoord[0] = 0.0f;
    myFilterTexCoord[3] = 0.0f;

    if(myFB.myHaveTexRectEXT)
    {
      myFilterTexWidth = scaleWidth;
      myFilterTexHeight = scaleHeight;
      myFilterTexCoord[2] = (GLfloat) myFilterTexWidth;
      myFilterTexCoord[1] = (GLfloat) myFilterTexHeight;
    }
    else
    {
      myFilterTexWidth = power_of_two(scaleWidth);
      myFilterTexHeight = power_of_two(scaleHeight);
      myFilterTexCoord[2] = (GLfloat) scaleWidth / myFilterTexWidth;
      myFilterTexCoord[1] = (GLfloat) scaleHeight / myFilterTexHeight;
    }
  }

  // Only do this if TV and color bleed filters are enabled
  // This filer applies a color averaging of surrounding pixels for each pixel
  if(myTvFiltersEnabled && myFB.myUseBleed)
  {
    // Load shader programs. If it fails, don't use this filter.
    myBleedProgram = genShader(SHADER_BLEED);
    if(myBleedProgram == 0)
    {
      myFB.myUseBleed = false;
      cout << "ERROR: Failed to make bleed programs" << endl;
    }
  }

  // If the texture and noise filters are enabled together, we can use a single shader
  // Make sure we can use three textures at once first
  GLint texUnits;
  p_glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texUnits);
  if(myTvFiltersEnabled && texUnits >= 3 && myFB.myUseTexture && myFB.myUseNoise)
  {
    // Load shader program. If it fails, don't use this shader.
    myTextureNoiseProgram = genShader(SHADER_TEXNOISE);
    if(myTextureNoiseProgram == 0)
    {
      cout << "ERROR: Failed to make texture/noise program" << endl;

      // Load shader program. If it fails, don't use this filter.
      myTextureProgram = genShader(SHADER_TEX);
      if(myTextureProgram == 0)
      {
        myFB.myUseTexture = false;
        cout << "ERROR: Failed to make texture program" << endl;
      }

      // Load shader program. If it fails, don't use this filter.
      myNoiseProgram = genShader(SHADER_NOISE);
      if(myNoiseProgram == 0)
      {
        myFB.myUseNoise = false;
        cout << "ERROR: Failed to make noise program" << endl;
      }
    }
  }
  // Else, detect individual settings
  else if(myTvFiltersEnabled)
  {
    if(myFB.myUseTexture)
    {
      // Load shader program. If it fails, don't use this filter.
      myTextureProgram = genShader(SHADER_TEX);
      if(myTextureProgram == 0)
      {
        myFB.myUseTexture = false;
        cout << "ERROR: Failed to make texture program" << endl;
      }
    }

    if(myFB.myUseNoise)
    {
      // Load shader program. If it fails, don't use this filter.
      myNoiseProgram = genShader(SHADER_NOISE);
      if(myNoiseProgram == 0)
      {
        myFB.myUseNoise = false;
        cout << "ERROR: Failed to make noise program" << endl;
      }
    }
  }

  // Only do this if TV and color texture filters are enabled
  // This filer applies an RGB color pixel mask as well as a blackspace mask
  if(myTvFiltersEnabled && myFB.myUseTexture)
  {
    // Prepare subpixel texture
    mySubpixelTexture = SDL_CreateRGBSurface(SDL_SWSURFACE,
                  myFilterTexWidth, myFilterTexHeight, 16,
                  0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);

    uInt32 pCounter = 0;
    for (uInt32 y = 0; y < (uInt32)myFilterTexHeight; y++)
    {
      for (uInt32 x = 0; x < (uInt32)myFilterTexWidth; x++)
      {
        // Cause vertical offset for every other black row if enabled
        uInt32 offsetY;
        if (!myFB.myTextureStag || x % 6 < 3)
          offsetY = y;
        else
          offsetY = y + 2;

        // Make a row of black for the mask every so often
        if (offsetY % 4 == 0)
        {
          ((uInt16*)mySubpixelTexture->pixels)[pCounter] = 0x0000;
        }
        // Apply the coorect color mask
        else
        {
          ((uInt16*)mySubpixelTexture->pixels)[pCounter] = 0x7c00 >> ((x % 3) * 5);
        }
        pCounter++;
      }
    }
  }

  // Only do this if TV and noise filters are enabled
  // This filter applies a texture filled with gray pixel of random intensities
  if(myTvFiltersEnabled && myFB.myUseNoise)
  {
    // Get the current number of nose textures to use
    myNoiseNum = myFB.myNoiseQuality;

    // Allocate space for noise textures
    myNoiseTexture = new SDL_Surface*[myNoiseNum];
    myNoiseMaskTexID = new GLuint[myNoiseNum];

    // Prepare noise textures
    for(int i = 0; i < myNoiseNum; i++)
    {
      myNoiseTexture[i] = SDL_CreateRGBSurface(SDL_SWSURFACE,
                  myFilterTexWidth, myFilterTexHeight, 16,
                  0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
    }

    uInt32 pCounter = 0;
    for(int i = 0; i < myNoiseNum; i++)
    {
      pCounter = 0;

      // Attempt to make the numbers as random as possible
      int temp = (unsigned)time(0) + rand()/4;
      srand(temp);

      for (uInt32 y = 0; y < (uInt32)myFilterTexHeight; y++)
      {
        for (uInt32 x = 0; x < (uInt32)myFilterTexWidth; x++)
        {
          // choose random 0 - 2
          // 0 = 0x0000
          // 1 = 0x0421
          // 2 = 0x0842
          int num = rand() % 3;
          if (num == 0)
            ((uInt16*)myNoiseTexture[i]->pixels)[pCounter] = 0x0000;
          else if (num == 1)
            ((uInt16*)myNoiseTexture[i]->pixels)[pCounter] = 0x0421;
          else if (num == 2)
            ((uInt16*)myNoiseTexture[i]->pixels)[pCounter] = 0x0842;

          pCounter++;
        }
      }
    }
  }

  // Only do this if TV and phosphor filters are enabled
  // This filter merges the past screen with the current one, to give a phosphor burn-off effect
  if(myTvFiltersEnabled && myFB.myUseGLPhosphor)
  {
    // Load shader program. If it fails, don't use this filter.
    myPhosphorProgram = genShader(SHADER_PHOS);
    if(myPhosphorProgram == 0)
    {
      myFB.myUseGLPhosphor = false;
      cout << "ERROR: Failed to make phosphor program" << endl;
    }
  }

  // Associate the SDL surface with a GL texture object
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurfaceGL::~FBSurfaceGL()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  if(mySubpixelTexture)
    SDL_FreeSurface(mySubpixelTexture);

  if(myNoiseTexture)
    for(int i = 0; i < myNoiseNum; i++)
      SDL_FreeSurface(myNoiseTexture[i]);

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
  myXOrig = x;
  myYOrig = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setWidth(uInt32 w)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  myWidth = w;

  if(myFB.myHaveTexRectEXT)
    myTexCoord[2] = (GLfloat) myWidth;
  else
    myTexCoord[2] = (GLfloat) myWidth / myTexWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setHeight(uInt32 h)
{
  // This method can't be used with 'scaled' surface (aka TIA surfaces)
  // That shouldn't really matter, though, as all the UI stuff isn't scaled,
  // and it's the only thing that uses it
  myHeight = h;

  if(myFB.myHaveTexRectEXT)
    myTexCoord[3] = (GLfloat) myHeight;
  else
    myTexCoord[3] = (GLfloat) myHeight / myTexHeight;
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
    GLint loc;

    // Set a boolean to tell which filter is a first render (if any are applied).
    // Being a first render means using the Atari frame buffer instead of the
    // previous rendered data.
    bool firstRender = true;

    // Render as usual if no filters are used
    if(!myTvFiltersEnabled ||
       (!myFB.myUseTexture && !myFB.myUseNoise && !myFB.myUseBleed && !myFB.myUseGLPhosphor))
    {
      p_glUseProgram(0);

      // Texturemap complete texture to surface so we have free scaling
      // and antialiasing
      p_glActiveTexture(GL_TEXTURE0);
      p_glBindTexture(myTexTarget, myTexID);
      p_glTexSubImage2D(myTexTarget, 0, 0, 0, myTexWidth, myTexHeight,
                        GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, myTexture->pixels);

      // Pass in texture as a variable
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
    }

    // If TV filters are enabled
    if(myTvFiltersEnabled)
    {
      // If combined texture/noise program exists,
      // use the combined one; else do them separately
      if(myTextureNoiseProgram != 0)
      {
        p_glUseProgram(myTextureNoiseProgram);

        // Pass in subpixel mask texture
        p_glActiveTexture(GL_TEXTURE1);
        p_glBindTexture(myTexTarget, mySubMaskTexID);
        loc = p_glGetUniformLocation(myTextureNoiseProgram, "texMask");
        p_glUniform1i(loc, 1);

        // Choose random mask texture
        int num = rand() % myNoiseNum;
        // Pass in noise mask texture
        p_glActiveTexture(GL_TEXTURE2);
        p_glBindTexture(myTexTarget, myNoiseMaskTexID[num]);
        loc = p_glGetUniformLocation(myTextureNoiseProgram, "noiseMask");
        p_glUniform1i(loc, 2);

        renderThreeTexture(myTextureNoiseProgram, firstRender);

        // We have rendered, set firstRender to false
        firstRender = false;
      }
      else
      {
        // Check if texture filter is enabled
        if(myFB.myUseTexture)
        {
          p_glUseProgram(myTextureProgram);

          // Pass in subpixel mask texture
          p_glActiveTexture(GL_TEXTURE1);
          p_glBindTexture(myTexTarget, mySubMaskTexID);
          loc = p_glGetUniformLocation(myTextureProgram, "mask");
          p_glUniform1i(loc, 1);

          renderTwoTexture(myTextureProgram, firstRender);

          // We have rendered, set firstRender to false
          firstRender = false;
        }

        if(myFB.myUseNoise)
        {
          p_glUseProgram(myNoiseProgram);

          // Choose random mask texture
          int num = rand() % myNoiseNum;

          // Pass in noise mask texture
          p_glActiveTexture(GL_TEXTURE1);
          p_glBindTexture(myTexTarget, myNoiseMaskTexID[num]);
          loc = p_glGetUniformLocation(myNoiseProgram, "mask");
          p_glUniform1i(loc, 1);

          renderTwoTexture(myNoiseProgram, firstRender);

          // We have rendered, set firstRender to false
          firstRender = false;
        }
      }

      // Check if bleed filter is enabled
      if(myFB.myUseBleed)
      {
        p_glUseProgram(myBleedProgram);

        // Set some values based on high, medium, or low quality bleed. The high quality
        // scales by applying additional passes, the low and medium quality scales by using
        // a width and height based on the zoom level
        int passes;
        // High quality
        if(myFB.myBleedQuality == 2)
        {
          // Precalculate pixel shifts
          GLfloat pH = 1.0 / myHeight;
          GLfloat pW = 1.0 / myWidth;
          GLfloat pWx2 = pW * 2.0;

          loc = p_glGetUniformLocation(myBleedProgram, "pH");
          p_glUniform1f(loc, pH);
          loc = p_glGetUniformLocation(myBleedProgram, "pW");
          p_glUniform1f(loc, pW);
          loc = p_glGetUniformLocation(myBleedProgram, "pWx2");
          p_glUniform1f(loc, pWx2);

          // Set the number of passes based on zoom level
          passes = myFB.getZoomLevel();
        }
        // Medium and low quality
        else
        {
          // The scaling formula was produced through trial and error
          // Precalculate pixel shifts
          GLfloat pH = 1.0 / (myHeight / (0.35 * myFB.getZoomLevel()));
          GLfloat pW = 1.0 / (myWidth / (0.35 * myFB.getZoomLevel()));
          GLfloat pWx2 = pW * 2.0;

          loc = p_glGetUniformLocation(myBleedProgram, "pH");
          p_glUniform1f(loc, pH);
          loc = p_glGetUniformLocation(myBleedProgram, "pW");
          p_glUniform1f(loc, pW);
          loc = p_glGetUniformLocation(myBleedProgram, "pWx2");
          p_glUniform1f(loc, pWx2);

          // Medium quality
          if(myFB.myBleedQuality == 1)
            passes = 2;
          // Low quality
          else
            passes = 1;
        }

        // If we are using a texture effect, we need more bleed
        if (myFB.myUseTexture)
          passes <<= 1;

        for (int i = 0; i < passes; i++)
        {
          renderTexture(myBleedProgram, firstRender);

          // We have rendered, set firstRender to false
          firstRender = false;
        }
      }

      // Check if phosphor burn-off filter is enabled
      if(myFB.myUseGLPhosphor)
      {
        p_glUseProgram(myPhosphorProgram);

        // Pass in subpixel mask texture
        p_glActiveTexture(GL_TEXTURE1);
        p_glBindTexture(myTexTarget, myPhosphorTexID);
        loc = p_glGetUniformLocation(myPhosphorProgram, "mask");
        p_glUniform1i(loc, 1);

        renderTwoTexture(myPhosphorProgram, firstRender);

        p_glActiveTexture(GL_TEXTURE1);
        p_glBindTexture(myTexTarget, myPhosphorTexID);
        // We only need to copy the scaled size, which may be smaller than the texture width
        p_glCopyTexSubImage2D(myTexTarget, 0, 0, 0, myXOrig, myYOrig, myWidth, myHeight);

        // We have rendered, set firstRender to false
        firstRender = false;
      }
    }

    mySurfaceIsDirty = false;

    // Let postFrameUpdate() know that a change has been made
    myFB.myDirtyFlag = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::renderTexture(GLuint program, bool firstRender)
{
  GLint loc;
  GLfloat texCoord[4];

  p_glActiveTexture(GL_TEXTURE0);

  // If this is a first render, use the Atari frame buffer
  if(firstRender)
  {
    // Pass in Atari frame
    p_glBindTexture(myTexTarget, myTexID);
    p_glTexSubImage2D(myTexTarget, 0, 0, 0, myTexWidth, myTexHeight,
                      GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, myTexture->pixels);

    // Set the texture coord appropriately
    texCoord[0] = myTexCoord[0];
    texCoord[1] = myTexCoord[1];
    texCoord[2] = myTexCoord[2];
    texCoord[3] = myTexCoord[3];
  }
  else
  {
    // Copy frame buffer to texture, this isn't the fastest way to do it, but it's simple
    // (rendering directly to texture instead of copying may be faster)
    p_glBindTexture(myTexTarget, myFilterTexID);
    // We only need to copy the scaled size, which may be smaller than the texture width
    p_glCopyTexSubImage2D(myTexTarget, 0, 0, 0, myXOrig, myYOrig, myWidth, myHeight);

    // Set the texture coord appropriately
    texCoord[0] = myFilterTexCoord[0];
    texCoord[1] = myFilterTexCoord[1];
    texCoord[2] = myFilterTexCoord[2];
    texCoord[3] = myFilterTexCoord[3];
  }

  // Pass the texture to the program
  loc = p_glGetUniformLocation(program, "tex");
  p_glUniform1i(loc, 0);

  // Pass in texture as a variable
  p_glBegin(GL_QUADS);
    p_glTexCoord2f(texCoord[0], texCoord[1]);
    p_glVertex2i(myXOrig, myYOrig);

    p_glTexCoord2f(texCoord[2], texCoord[1]);
    p_glVertex2i(myXOrig + myWidth, myYOrig);

    p_glTexCoord2f(texCoord[2], texCoord[3]);
    p_glVertex2i(myXOrig + myWidth, myYOrig + myHeight);

    p_glTexCoord2f(texCoord[0], texCoord[3]);
    p_glVertex2i(myXOrig, myYOrig + myHeight);
  p_glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::renderTwoTexture(GLuint program, bool firstRender)
{
  GLint loc;
  GLfloat texCoord[4];

  p_glActiveTexture(GL_TEXTURE0);

  // If this is a first render, use the Atari frame buffer
  if(firstRender)
  {
    // Pass in Atari frame
    p_glBindTexture(myTexTarget, myTexID);
    p_glTexSubImage2D(myTexTarget, 0, 0, 0, myTexWidth, myTexHeight,
                      GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, myTexture->pixels);

    // Set the texture coord appropriately
    texCoord[0] = myTexCoord[0];
    texCoord[1] = myTexCoord[1];
    texCoord[2] = myTexCoord[2];
    texCoord[3] = myTexCoord[3];
  }
  else
  {
    // Copy frame buffer to texture, this isn't the fastest way to do it, but it's simple
    // (rendering directly to texture instead of copying may be faster)
    p_glBindTexture(myTexTarget, myFilterTexID);
    // We only need to copy the scaled size, which may be smaller than the texture width
    p_glCopyTexSubImage2D(myTexTarget, 0, 0, 0, myXOrig, myYOrig, myWidth, myHeight);

    // Set the filter texture coord appropriately
    texCoord[0] = myFilterTexCoord[0];
    texCoord[1] = myFilterTexCoord[1];
    texCoord[2] = myFilterTexCoord[2];
    texCoord[3] = myFilterTexCoord[3];
  }

  // Pass the texture to the program
  loc = p_glGetUniformLocation(program, "tex");
  p_glUniform1i(loc, 0);

  // Pass in textures as variables
  p_glBegin(GL_QUADS);
    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[0], texCoord[1]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[0], myFilterTexCoord[1]);
    p_glVertex2i(myXOrig, myYOrig);

    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[2], texCoord[1]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[2], myFilterTexCoord[1]);
    p_glVertex2i(myXOrig + myWidth, myYOrig);

    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[2], texCoord[3]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[2], myFilterTexCoord[3]);
    p_glVertex2i(myXOrig + myWidth, myYOrig + myHeight);

    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[0], texCoord[3]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[0], myFilterTexCoord[3]);
    p_glVertex2i(myXOrig, myYOrig + myHeight);
  p_glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::renderThreeTexture(GLuint program, bool firstRender)
{
  GLint loc;
  GLfloat texCoord[4];

  p_glActiveTexture(GL_TEXTURE0);

  // If this is a first render, use the Atari frame buffer
  if(firstRender)
  {
    // Pass in Atari frame
    p_glBindTexture(myTexTarget, myTexID);
    p_glTexSubImage2D(myTexTarget, 0, 0, 0, myTexWidth, myTexHeight,
                      GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, myTexture->pixels);

    // Set the texture coord appropriately
    texCoord[0] = myTexCoord[0];
    texCoord[1] = myTexCoord[1];
    texCoord[2] = myTexCoord[2];
    texCoord[3] = myTexCoord[3];
  }
  else
  {
    // Copy frame buffer to texture, this isn't the fastest way to do it, but it's simple
    // (rendering directly to texture instead of copying may be faster)
    p_glBindTexture(myTexTarget, myFilterTexID);
    // We only need to copy the scaled size, which may be smaller than the texture width
    p_glCopyTexSubImage2D(myTexTarget, 0, 0, 0, myXOrig, myYOrig, myWidth, myHeight);

    // Set the filter texture coord appropriately
    texCoord[0] = myFilterTexCoord[0];
    texCoord[1] = myFilterTexCoord[1];
    texCoord[2] = myFilterTexCoord[2];
    texCoord[3] = myFilterTexCoord[3];
  }

  // Pass the texture to the program
  loc = p_glGetUniformLocation(program, "tex");
  p_glUniform1i(loc, 0);

  // Pass in textures as variables
  p_glBegin(GL_QUADS);
    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[0], texCoord[1]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[0], myFilterTexCoord[1]);
    p_glMultiTexCoord2f(GL_TEXTURE2, myFilterTexCoord[0], myFilterTexCoord[1]);
    p_glVertex2i(myXOrig, myYOrig);

    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[2], texCoord[1]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[2], myFilterTexCoord[1]);
    p_glMultiTexCoord2f(GL_TEXTURE2, myFilterTexCoord[2], myFilterTexCoord[1]);
    p_glVertex2i(myXOrig + myWidth, myYOrig);

    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[2], texCoord[3]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[2], myFilterTexCoord[3]);
    p_glMultiTexCoord2f(GL_TEXTURE2, myFilterTexCoord[2], myFilterTexCoord[3]);
    p_glVertex2i(myXOrig + myWidth, myYOrig + myHeight);

    p_glMultiTexCoord2f(GL_TEXTURE0, texCoord[0], texCoord[3]);
    p_glMultiTexCoord2f(GL_TEXTURE1, myFilterTexCoord[0], myFilterTexCoord[3]);
    p_glMultiTexCoord2f(GL_TEXTURE2, myFilterTexCoord[0], myFilterTexCoord[3]);
    p_glVertex2i(myXOrig, myYOrig + myHeight);
  p_glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::free()
{
  p_glDeleteTextures(1, &myTexID);

  // The below is borken up a bit because of the possible combined texture/noise shader

  if(myFilterTexID)
    p_glDeleteTextures(1, &myFilterTexID);

  if(mySubMaskTexID)
    p_glDeleteTextures(1, &mySubMaskTexID);

  if(myTextureProgram)
    p_glDeleteProgram(myTextureProgram);

  if(myNoiseMaskTexID)
  {
    delete[] myNoiseTexture;
    p_glDeleteTextures(myNoiseNum, myNoiseMaskTexID);
    delete[] myNoiseMaskTexID;
  }

  if(myNoiseProgram)
    p_glDeleteProgram(myNoiseProgram);

  if(myPhosphorTexID)
  {
    p_glDeleteTextures(1, &myPhosphorTexID);
    p_glDeleteProgram(myPhosphorProgram);
  }

  if(myTextureNoiseProgram)
    p_glDeleteProgram(myTextureNoiseProgram);
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
  p_glEnable(myTexTarget);

  p_glGenTextures(1, &myTexID);
  p_glBindTexture(myTexTarget, myTexID);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Finally, create the texture in the most optimal format
  p_glTexImage2D(myTexTarget, 0, GL_RGB5,
                 myTexWidth, myTexHeight, 0,
                 GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, myTexture->pixels);

  // Do the same for the TV filter textures
  // Only do this if TV filters are enabled
  if(myTvFiltersEnabled)
  {
    // Generate the generic filter texture
    p_glGenTextures(1, &myFilterTexID);
    p_glBindTexture(myTexTarget, myFilterTexID);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Make the initial texture, this will get overwritten later
    p_glCopyTexImage2D(myTexTarget, 0, GL_RGB5, 0, 0, myFilterTexWidth, myFilterTexHeight, 0);

    // Only do this if TV and color texture filters are enabled
    if(myFB.myUseTexture)
    {
      // Generate the subpixel mask texture
      p_glGenTextures(1, &mySubMaskTexID);
      p_glBindTexture(myTexTarget, mySubMaskTexID);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      // Write the data
      p_glTexImage2D(myTexTarget, 0, GL_RGB5,
                     myFilterTexWidth, myFilterTexHeight, 0,
                     GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, mySubpixelTexture->pixels);
    }

    // Only do this if TV and noise filters are enabled
    if(myFB.myUseNoise)
    {
      // Generate the noise mask textures
      p_glGenTextures(myNoiseNum, myNoiseMaskTexID);
      for(int i = 0; i < myNoiseNum; i++)
      {
        p_glBindTexture(myTexTarget, myNoiseMaskTexID[i]);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Write the data
        p_glTexImage2D(myTexTarget, 0, GL_RGB5,
                       myFilterTexWidth, myFilterTexHeight, 0,
                       GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, myNoiseTexture[i]->pixels);
      }
    }

    // Only do this if TV and phosphor filters are enabled
    if(myFB.myUseGLPhosphor)
    {
      // Generate the noise mask textures
      p_glGenTextures(1, &myPhosphorTexID);
      p_glBindTexture(myTexTarget, myPhosphorTexID);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      // Make the initial texture, this will get overwritten later
      p_glCopyTexImage2D(myTexTarget, 0, GL_RGB5, 0, 0, myFilterTexWidth, myFilterTexHeight, 0);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurfaceGL::setFilter(const string& name)
{
  // We only do GL_NEAREST or GL_LINEAR for now
  GLint filter = GL_NEAREST;
  if(name == "linear")
    filter = GL_LINEAR;

  p_glBindTexture(myTexTarget, myTexID);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, filter);
  p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, filter);

  // Do the same for the filter textures
  // Only do this if TV filters are enabled
  if(myTvFiltersEnabled)
  {
    p_glBindTexture(myTexTarget, myFilterTexID);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, filter);
    p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, filter);

    // Only do this if TV and color texture filters are enabled
    if(myFB.myUseTexture)
    {
      p_glBindTexture(myTexTarget, mySubMaskTexID);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, filter);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, filter);
    }

    // Only do this if TV and noise filters are enabled
    if(myFB.myUseNoise)
    {
      for(int i = 0; i < myNoiseNum; i++)
      {
        p_glBindTexture(myTexTarget, myNoiseMaskTexID[i]);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, filter);
        p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, filter);
      }
    }

    // Only do this if TV and phosphor filters are enabled
    if(myFB.myUseGLPhosphor)
    {
      p_glBindTexture(myTexTarget, myPhosphorTexID);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MIN_FILTER, filter);
      p_glTexParameteri(myTexTarget, GL_TEXTURE_MAG_FILTER, filter);
    }
  }

  // The filtering has changed, so redraw the entire screen
  mySurfaceIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GLuint FBSurfaceGL::genShader(ShaderType type)
{
  string fFile = "";
  char* fCode = NULL;
  switch(type)
  {
    case SHADER_BLEED:
      fFile = "bleed.frag";
      fCode = (char*)GLShader::bleed_frag[0];
      break;
    case SHADER_TEX:
      fFile = "texture.frag";
      fCode = (char*)GLShader::texture_frag[0];
      break;
    case SHADER_NOISE:
      fFile = "noise.frag";
      fCode = (char*)GLShader::noise_frag[0];
      break;
    case SHADER_PHOS:
      fFile = "phosphor.frag";
      fCode = (char*)GLShader::phosphor_frag[0];
      break;
    case SHADER_TEXNOISE:
      fFile = "texture_noise.frag";
      fCode = (char*)GLShader::texture_noise_frag[0];
      break;
  }

  // First try opening an external fragment file
  // These shader files are stored in 'BASEDIR/shaders/'
  char* buffer = NULL;
  const string& filename =
    myFB.myOSystem->baseDir() + BSPF_PATH_SEPARATOR + "shaders" +
    BSPF_PATH_SEPARATOR + fFile;
  ifstream in(filename.c_str());
  if(in && in.is_open())
  {
    // Get file size
    in.seekg(0, std::ios::end);
    streampos size = in.tellg();

    // Reset position
    in.seekg(0);

    // Make buffer of proper size;
    buffer = new char[size+(streampos)1]; // +1 for '\0'

    // Read in file
    in.read(buffer, size);
    buffer[in.gcount()] = '\0';
    in.close();

    fCode = buffer;
  }

  // Make the shader program
  GLuint fShader = p_glCreateShader(GL_FRAGMENT_SHADER);
  GLuint program = p_glCreateProgram();
  p_glShaderSource(fShader, 1, (const char**)&fCode, NULL);
  p_glCompileShader(fShader);
  p_glAttachShader(program, fShader);
  p_glLinkProgram(program);

  // Go ahead and flag the shader for deletion so it is deleted once the program is
  p_glDeleteShader(fShader);

  // Clean up
  delete[] buffer;

  return program;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FrameBufferGL::myGLVersion = 0.0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::myLibraryLoaded = false;

#endif  // DISPLAY_OPENGL
