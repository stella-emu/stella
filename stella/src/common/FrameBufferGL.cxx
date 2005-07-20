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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferGL.cxx,v 1.36 2005-07-20 17:33:02 stephena Exp $
//============================================================================

#ifdef DISPLAY_OPENGL

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferGL.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Font.hxx"
#include "GuiUtils.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL(OSystem* osystem)
   :  FrameBuffer(osystem),
      myTexture(NULL),
      myScreenmode(0),
      myScreenmodeCount(0),
      myTextureID(0),
      myFilterParam(GL_NEAREST),
      myFilterParamName("GL_NEAREST"),
      myFSScaleFactor(1.0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::~FrameBufferGL()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  glDeleteTextures(1, &myTextureID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::initSubsystem()
{
  mySDLFlags |= SDL_OPENGL;

  // Set up the OpenGL attributes
  myDepth = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
  switch(myDepth)
  {
    case 8:
      myRGB[0] = 3;
      myRGB[1] = 3;
      myRGB[2] = 2;
      myRGB[3] = 0;
      break;

    case 15:
      myRGB[0] = 5;
      myRGB[1] = 5;
      myRGB[2] = 5;
      myRGB[3] = 0;
      break;

    case 16:
      myRGB[0] = 5;
      myRGB[1] = 6;
      myRGB[2] = 5;
      myRGB[3] = 0;
      break;

    case 24:
      myRGB[0] = 8;
      myRGB[1] = 8;
      myRGB[2] = 8;
      myRGB[3] = 0;
      break;

    case 32:
      myRGB[0] = 8;
      myRGB[1] = 8;
      myRGB[2] = 8;
      myRGB[3] = 8;
      break;

    default:  // This should never happen
      break;
  }

  // Get the valid OpenGL screenmodes
  myScreenmodeCount = 0;
  myScreenmode = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL);
  if((myScreenmode != (SDL_Rect**) -1) && (myScreenmode != (SDL_Rect**) 0))
    for(uInt32 i = 0; myScreenmode[i]; ++i)
      myScreenmodeCount++;

  // Create the screen
  if(!createScreen())
    return false;

  // Now check to see what color components were actually created
  SDL_GL_GetAttribute( SDL_GL_RED_SIZE, (int*)&myRGB[0] );
  SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, (int*)&myRGB[1] );
  SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, (int*)&myRGB[2] );
  SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, (int*)&myRGB[3] );

#ifndef TEXTURES_ARE_LOST
  // Create the texture surface
  createTextures();
#endif

  // Show some OpenGL info
  if(myOSystem->settings().getBool("showinfo"))
  {
    cout << "Video rendering: OpenGL mode" << endl;

    ostringstream colormode;
    colormode << "  Color   : " << myDepth << " bit, " << myRGB[0] << "-"
              << myRGB[1] << "-" << myRGB[2] << "-" << myRGB[3];

    cout << "  Vendor  : " << glGetString(GL_VENDOR) << endl
         << "  Renderer: " << glGetString(GL_RENDERER) << endl
         << "  Version : " << glGetString(GL_VERSION) << endl
         << colormode.str() << endl
         << "  Filter  : " << myFilterParamName << endl
         << endl;
  }

  // Precompute the GUI palette
  // We abuse the concept of 'enum' by referring directly to the integer values
  for(uInt8 i = 0; i < kNumColors-256; i++)
    myPalette[i+256] = mapRGB(ourGUIColors[i][0], ourGUIColors[i][1], ourGUIColors[i][2]);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::setAspectRatio()
{
  theAspectRatio = myOSystem->settings().getFloat("gl_aspect") / 2;
  if(theAspectRatio <= 0.0)
    theAspectRatio = 1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createScreen()
{
  SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   myRGB[0] );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, myRGB[1] );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  myRGB[2] );
  SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, myRGB[3] );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  // Set the screen coordinates
  GLdouble orthoWidth  = 0.0;
  GLdouble orthoHeight = 0.0;
  setDimensions(&orthoWidth, &orthoHeight);

  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  glPushAttrib(GL_ENABLE_BIT);

  // Center the image horizontally and vertically
  glViewport(myImageDim.x, myImageDim.y, myImageDim.w, myImageDim.h);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  glOrtho(0.0, orthoWidth, orthoHeight, 0.0, 0.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

#ifdef TEXTURES_ARE_LOST
  createTextures();
#endif

  // Make sure any old parts of the screen are erased
  // Do it for both buffers!
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapBuffers();
  glClear(GL_COLOR_BUFFER_BIT);

  refreshTIA();
  refreshOverlay();
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

  register uInt32 y;
  for(y = 0; y < height; ++y )
  {
    const uInt32 bufofsY    = y * width;
    const uInt32 screenofsY = y * myTexture->w;

    register uInt32 x;
    for(x = 0; x < width; ++x )
    {
      const uInt32 bufofs = bufofsY + x;
      uInt8 v = currentFrame[bufofs];
      if(v != previousFrame[bufofs] || theRedrawTIAIndicator)
      {
        // If we ever get to this point, we know the current and previous
        // buffers differ.  In that case, make sure the changes are
        // are drawn in postFrameUpdate()
        theRedrawTIAIndicator = true;

        // x << 1 is times 2 ( doubling width )
        const uInt32 pos = screenofsY + (x << 1);
        buffer[pos] = buffer[pos+1] = (uInt16) myPalette[v];
      }
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
  // Do the following twice, since OpenGL mode is double-buffered,
  // and we need the contents placed in both buffers
  if(theRedrawTIAIndicator || theRedrawOverlayIndicator)
  {
    // Texturemap complete texture to surface so we have free scaling 
    // and antialiasing 
    uInt32 w = myBaseDim.w, h = myBaseDim.h;

    glBindTexture(GL_TEXTURE_2D, myTextureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexture->w, myTexture->h,
                    GL_RGB, GL_UNSIGNED_SHORT_5_6_5, myTexture->pixels);
    glBegin(GL_QUADS);
      glTexCoord2f(myTexCoord[0], myTexCoord[1]); glVertex2i(0, 0);
      glTexCoord2f(myTexCoord[2], myTexCoord[1]); glVertex2i(w, 0);
      glTexCoord2f(myTexCoord[2], myTexCoord[3]); glVertex2i(w, h);
      glTexCoord2f(myTexCoord[0], myTexCoord[3]); glVertex2i(0, h);
    glEnd();

    // Now show all changes made to the textures
    SDL_GL_SwapBuffers();

    glBegin(GL_QUADS);
      glTexCoord2f(myTexCoord[0], myTexCoord[1]); glVertex2i(0, 0);
      glTexCoord2f(myTexCoord[2], myTexCoord[1]); glVertex2i(w, 0);
      glTexCoord2f(myTexCoord[2], myTexCoord[3]); glVertex2i(w, h);
      glTexCoord2f(myTexCoord[0], myTexCoord[3]); glVertex2i(0, h);
    glEnd();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::scanline(uInt32 row, uInt8* data)
{
  // Invert the row, since OpenGL rows start at the bottom
  // of the framebuffer
  row = myImageDim.h + myImageDim.y - row - 1;

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(myImageDim.x, row, myImageDim.w, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::toggleFilter()
{
  if(myFilterParam == GL_NEAREST)
  {
    myFilterParam = GL_LINEAR;
    myOSystem->settings().setString("gl_filter", "linear");
    showMessage("Filtering: GL_LINEAR");
  }
  else
  {
    myFilterParam = GL_NEAREST;
    myOSystem->settings().setString("gl_filter", "nearest");
    showMessage("Filtering: GL_NEAREST");
  }

  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myFilterParam);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myFilterParam);

  // The filtering has changed, so redraw the entire screen
  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color)
{
  SDL_Rect tmp;

  // Horizontal line
  tmp.x = x;
  tmp.y = y;
  tmp.w = x2 - x + 1;
  tmp.h = 1;
  SDL_FillRect(myTexture, &tmp, myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
  SDL_Rect tmp;

  // Vertical line
  tmp.x = x;
  tmp.y = y;
  tmp.w = 1;
  tmp.h = y2 - y + 1;
  SDL_FillRect(myTexture, &tmp, myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::blendRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                              OverlayColor color, uInt32 level)
{
// FIXME - make this do alpha-blending
//         for now, just do a normal fill
  fillRect(x, y, w, h, color);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                             OverlayColor color)
{
  SDL_Rect tmp;

  // Fill the rectangle
  tmp.x = x;
  tmp.y = y;
  tmp.w = w;
  tmp.h = h;
  SDL_FillRect(myTexture, &tmp, myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawChar(const GUI::Font* FONT, uInt8 chr,
                             uInt32 xorig, uInt32 yorig, OverlayColor color)
{
  GUI::Font* font = (GUI::Font*) FONT;

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

  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    const uInt16 buffer = *tmp++;
    uInt16 mask = 0x8000;

    for(int x = 0; x < w; x++, mask >>= 1)
    {
      if ((buffer & mask) != 0)
      {
        rect.x = x + xorig;
        rect.y = y + yorig;
        rect.w = rect.h = 1;
        SDL_FillRect(myTexture, &rect, myPalette[color]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawBitmap(uInt32* bitmap, Int32 xorig, Int32 yorig,
                                 OverlayColor color, Int32 h)
{
  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    uInt32 mask = 0xF0000000;

    for(int x = 0; x < 8; x++, mask >>= 4)
    {
      if(bitmap[y] & mask)
      {
        rect.x = x + xorig;
        rect.y = y + yorig;
        rect.w = rect.h = 1;
        SDL_FillRect(myTexture, &rect, myPalette[color]);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::translateCoords(Int32* x, Int32* y)
{
  // Wow, what a mess :)
  *x = (Int32) (((*x - myImageDim.x) / (theZoomLevel * myFSScaleFactor * theAspectRatio)));
  *y = (Int32) (((*y - myImageDim.y) / (theZoomLevel * myFSScaleFactor)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createTextures()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  glDeleteTextures(1, &myTextureID);

  uInt32 w = power_of_two(myBaseDim.w);
  uInt32 h = power_of_two(myBaseDim.h);

  myTexCoord[0] = 0.0f;
  myTexCoord[1] = 0.0f;
  myTexCoord[2] = (GLfloat) myBaseDim.w / w;
  myTexCoord[3] = (GLfloat) myBaseDim.h / h;

  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16,
    0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);

  if(myTexture == NULL)
    return false;

  // Create an OpenGL texture from the SDL texture
  string filter = myOSystem->settings().getString("gl_filter");
  if(filter == "linear")
  {
    myFilterParam     = GL_LINEAR;
    myFilterParamName = "GL_LINEAR";
  }
  else if(filter == "nearest")
  {
    myFilterParam     = GL_NEAREST;
    myFilterParamName = "GL_NEAREST";
  }

  glGenTextures(1, &myTextureID);
  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myFilterParam);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myFilterParam);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
               myTexture->pixels);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glEnable(GL_TEXTURE_2D);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::setDimensions(GLdouble* orthoWidth, GLdouble* orthoHeight)
{
  // We always know the initial image width and height
  // We have to determine final image dimensions as well as screen dimensions
  myImageDim.x = 0;
  myImageDim.y = 0;
  myImageDim.w = (Uint16) (myBaseDim.w * theZoomLevel * theAspectRatio);
  myImageDim.h = (Uint16) (myBaseDim.h * theZoomLevel);
  myScreenDim  = myImageDim;

  myFSScaleFactor = 1.0f;

  // Determine if we're in fullscreen or windowed mode
  // In fullscreen mode, we clip the SDL screen to known resolutions
  // In windowed mode, we use the actual image resolution for the SDL screen
  if(mySDLFlags & SDL_FULLSCREEN)
  {
    float scaleX = 0.0f;
    float scaleY = 0.0f;

    if(myOSystem->settings().getBool("gl_fsmax") &&
       myDesktopDim.w != 0 && myDesktopDim.h != 0)
    {
      // Use the largest available screen size
      myScreenDim.w = myDesktopDim.w;
      myScreenDim.h = myDesktopDim.h;

      scaleX = float(myImageDim.w) / myScreenDim.w;
      scaleY = float(myImageDim.h) / myScreenDim.h;

      if(scaleX > scaleY)
        myFSScaleFactor = float(myScreenDim.w) / myImageDim.w;
      else
        myFSScaleFactor = float(myScreenDim.h) / myImageDim.h;

      myImageDim.w = (Uint16) (myFSScaleFactor * myImageDim.w);
      myImageDim.h = (Uint16) (myFSScaleFactor * myImageDim.h);
    }
    else if(myOSystem->settings().getBool("gl_fsmax") &&
            myScreenmode != (SDL_Rect**) -1)
    {
      // Use the largest available screen size
      myScreenDim.w = myScreenmode[0]->w;
      myScreenDim.h = myScreenmode[0]->h;

      scaleX = float(myImageDim.w) / myScreenDim.w;
      scaleY = float(myImageDim.h) / myScreenDim.h;

      if(scaleX > scaleY)
        myFSScaleFactor = float(myScreenDim.w) / myImageDim.w;
      else
        myFSScaleFactor = float(myScreenDim.h) / myImageDim.h;

      myImageDim.w = (Uint16) (myFSScaleFactor * myImageDim.w);
      myImageDim.h = (Uint16) (myFSScaleFactor * myImageDim.h);
    }
    else if(myScreenmode == (SDL_Rect**) -1)
    {
      // All modes are available, so use the exact image resolution
      myScreenDim.w = myImageDim.w;
      myScreenDim.h = myImageDim.h;
    }
    else  // otherwise, search for a valid screenmode
    {
      for(uInt32 i = myScreenmodeCount-1; i >= 0; i--)
      {
        if(myImageDim.w <= myScreenmode[i]->w && myImageDim.h <= myScreenmode[i]->h)
        {
          myScreenDim.w = myScreenmode[i]->w;
          myScreenDim.h = myScreenmode[i]->h;
          break;
        }
      }
    }

    // Now calculate the OpenGL coordinates
    myImageDim.x = (myScreenDim.w - myImageDim.w) / 2;
    myImageDim.y = (myScreenDim.h - myImageDim.h) / 2;

    *orthoWidth  = (GLdouble) (myImageDim.w / (theZoomLevel * theAspectRatio * myFSScaleFactor));
    *orthoHeight = (GLdouble) (myImageDim.h / (theZoomLevel * myFSScaleFactor));
  }
  else
  {
    *orthoWidth  = (GLdouble) (myImageDim.w / (theZoomLevel * theAspectRatio));
    *orthoHeight = (GLdouble) (myImageDim.h / theZoomLevel);
  }
/*
  cerr << "myImageDim.x = " << myImageDim.x << ", myImageDim.y = " << myImageDim.y << endl;
  cerr << "myImageDim.w = " << myImageDim.w << ", myImageDim.h = " << myImageDim.h << endl;
  cerr << "myScreenDim.w = " << myScreenDim.w << ", myScreenDim.h = " << myScreenDim.h << endl;
  cerr << endl;
*/
}

#endif  // DISPLAY_OPENGL
