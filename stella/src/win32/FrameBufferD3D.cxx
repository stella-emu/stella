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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferD3D.cxx,v 1.2 2007-09-03 18:37:24 stephena Exp $
//============================================================================

#ifdef DISPLAY_D3D

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferD3D.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Font.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferD3D::FrameBufferD3D(OSystem* osystem)
  : FrameBuffer(osystem),
    myTexture(NULL),
    myHaveTexRectEXT(false),
    myFilterParamName("GL_NEAREST"),
    myScaleFactor(1.0),
    myDirtyFlag(true)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferD3D::~FrameBufferD3D()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  p_glDeleteTextures(1, &myBuffer.texture);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferD3D::loadFuncs(const string& library)
{
  return myFuncsLoaded = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferD3D::initSubsystem(VideoMode mode)
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
string FrameBufferD3D::about()
{
  string extensions;
  if(myHaveTexRectEXT) extensions += "GL_TEXTURE_RECTANGLE_ARB  ";
  if(extensions == "") extensions = "None";

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
bool FrameBufferD3D::setVidMode(VideoMode mode)
{
  myScreenDim.x = myScreenDim.y = 0;
  myScreenDim.w = mode.screen_w;
  myScreenDim.h = mode.screen_h;

  myImageDim.x = mode.image_x;
  myImageDim.y = mode.image_y;
  myImageDim.w = mode.image_w;
  myImageDim.h = mode.image_h;

  // Activate stretching if its been requested and it makes sense to do so
  myScaleFactor = 1.0;
  if(fullScreen() && (mode.image_w < mode.screen_w) &&
     (mode.image_h < mode.screen_h))
  {
    const string& gl_fsmax = myOSystem->settings().getString("gl_fsmax");
    bool inUIMode =
      myOSystem->eventHandler().state() == EventHandler::S_LAUNCHER ||
      myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER;

    // Only stretch in certain modes
    if((gl_fsmax == "always") || 
       (inUIMode && gl_fsmax == "ui") ||
       (!inUIMode && gl_fsmax == "tia"))
    {
      float scaleX = float(myImageDim.w) / myScreenDim.w;
      float scaleY = float(myImageDim.h) / myScreenDim.h;

      if(scaleX > scaleY)
        myScaleFactor = float(myScreenDim.w) / myImageDim.w;
      else
        myScaleFactor = float(myScreenDim.h) / myImageDim.h;

      myImageDim.w = (Uint16) (myScaleFactor * myImageDim.w);
      myImageDim.h = (Uint16) (myScaleFactor * myImageDim.h);
      myImageDim.x = (myScreenDim.w - myImageDim.w) / 2;
      myImageDim.y = (myScreenDim.h - myImageDim.h) / 2;
    }
  }

  // Combine the zoom level and scaler into one quantity
  myScaleFactor *= (float) mode.zoom;

  GLdouble orthoWidth  = (GLdouble)
      (myImageDim.w / myScaleFactor);
  GLdouble orthoHeight = (GLdouble)
      (myImageDim.h / myScaleFactor);

  // Create screen containing GL context
if(myScreen)
{
  p_glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapBuffers();
  p_glClear(GL_COLOR_BUFFER_BIT);

}


  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

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

loadFuncs("");

  // Check for some extensions that can potentially speed up operation
cerr << "address = " << (int) p_glGetString << endl;
  const char* extensions = (const char *) p_glGetString(GL_EXTENSIONS);
  myHaveTexRectEXT = strstr(extensions, "ARB_texture_rectangle") != NULL;

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
  p_glOrtho(0.0, orthoWidth, orthoHeight, 0, -1.0, 1.0);
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
void FrameBufferD3D::drawMediaSource()
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
void FrameBufferD3D::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::postFrameUpdate()
{
  if(myDirtyFlag)
  {
    // Texturemap complete texture to surface so we have free scaling 
    // and antialiasing 
    uInt32 w = myBuffer.width, h = myBuffer.height;

    p_glTexSubImage2D(myBuffer.target, 0, 0, 0,
                      myBuffer.texture_width, myBuffer.texture_height,
                      myBuffer.format, myBuffer.type, myBuffer.pixels);
    p_glBegin(GL_QUADS);
      p_glTexCoord2f(myBuffer.tex_coord[0], myBuffer.tex_coord[1]); p_glVertex2i(0, 0);
      p_glTexCoord2f(myBuffer.tex_coord[2], myBuffer.tex_coord[1]); p_glVertex2i(w, 0);
      p_glTexCoord2f(myBuffer.tex_coord[2], myBuffer.tex_coord[3]); p_glVertex2i(w, h);
      p_glTexCoord2f(myBuffer.tex_coord[0], myBuffer.tex_coord[3]); p_glVertex2i(0, h);
    p_glEnd();

    // Now show all changes made to the texture
    SDL_GL_SwapBuffers();

    myDirtyFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::scanline(uInt32 row, uInt8* data)
{
  // Invert the row, since OpenGL rows start at the bottom
  // of the framebuffer
  row = myImageDim.h + myImageDim.y - row - 1;

  p_glPixelStorei(GL_PACK_ALIGNMENT, 1);
  p_glReadPixels(myImageDim.x, row, myImageDim.w, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::toggleFilter()
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
void FrameBufferD3D::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
  // Horizontal line
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = x2 - x + 1;
  tmp.h = 1;
  SDL_FillRect(myTexture, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
  // Vertical line
  SDL_Rect tmp;
  tmp.x = x;
  tmp.y = y;
  tmp.w = 1;
  tmp.h = y2 - y + 1;
  SDL_FillRect(myTexture, &tmp, myDefPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                             int color)
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
void FrameBufferD3D::drawChar(const GUI::Font* font, uInt8 chr,
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
void FrameBufferD3D::drawBitmap(uInt32* bitmap, Int32 tx, Int32 ty,
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
void FrameBufferD3D::translateCoords(Int32& x, Int32& y)
{
  // Wow, what a mess :)
  x = (Int32) ((x - myImageDim.x) / myScaleFactor);
  y = (Int32) ((y - myImageDim.y) / myScaleFactor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  myDirtyFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferD3D::enablePhosphor(bool enable, int blend)
{
  myUsePhosphor   = enable;
  myPhosphorBlend = blend;

  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferD3D::createTextures()
{
  if(myTexture)         SDL_FreeSurface(myTexture);
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
bool FrameBufferD3D::myFuncsLoaded = false;

#endif  // DISPLAY_D3D
