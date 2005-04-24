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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferGL.cxx,v 1.18 2005-04-24 01:57:46 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferGL.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "StellaFont.hxx"
#include "GuiUtils.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL(OSystem* osystem)
   :  FrameBuffer(osystem),
      myTexture(0),
      myScreenmode(0),
      myScreenmodeCount(0),
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
  glDeleteTextures(256, myFontTextureID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::initSubsystem()
{
  mySDLFlags |= SDL_OPENGL;

  // Get the aspect ratio for the display
  // Since the display is already doubled horizontally, we half the
  // ratio that is provided
  theAspectRatio = myOSystem->settings().getFloat("gl_aspect") / 2;
  if(theAspectRatio <= 0.0)
    theAspectRatio = 1.0;

  // Get the maximum size of a window for THIS screen
  theMaxZoomLevel = maxWindowSizeForScreen();

  // Check to see if window size will fit in the screen
  if((uInt32)myOSystem->settings().getInt("zoom") > theMaxZoomLevel)
    theZoomLevel = theMaxZoomLevel;
  else
    theZoomLevel = myOSystem->settings().getInt("zoom");

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
  // Create the texture surface and texture fonts
  createTextures();
#endif

  // Set up the palette *after* we know the color components
  // and the textures
  setupPalette();

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
  for(uInt8 i = 0; i < 5; i++)
    for(uInt8 j = 0; j < 3; j++)
      myGUIPalette[i][j] = (float)(myGUIColors[i][j]) / 255.0;

  return true;
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

  theRedrawEntireFrameIndicator = true;
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
      if(v == previousFrame[bufofs] && !theRedrawEntireFrameIndicator)
        continue;

      // x << 1 is times 2 ( doubling width )
      const uInt32 pos = screenofsY + (x << 1);
      buffer[pos] = buffer[pos+1] = (uInt16) myPalette[v];
    }
  }

  // If necessary, erase the screen
  if(theRedrawEntireFrameIndicator)
    glClear(GL_COLOR_BUFFER_BIT);

  // Texturemap complete texture to surface so we have free scaling 
  // and antialiasing 
  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexture->w, myTexture->h,
                  GL_RGB, GL_UNSIGNED_SHORT_5_6_5, myTexture->pixels);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  uInt32 w = myBaseDim.w, h = myBaseDim.h;
  glBegin(GL_QUADS);
    glTexCoord2f(myTexCoord[0], myTexCoord[1]); glVertex2i(0, 0);
    glTexCoord2f(myTexCoord[2], myTexCoord[1]); glVertex2i(w, 0);
    glTexCoord2f(myTexCoord[2], myTexCoord[3]); glVertex2i(w, h);
    glTexCoord2f(myTexCoord[0], myTexCoord[3]); glVertex2i(0, h);
  glEnd();

  // The frame doesn't need to be completely redrawn anymore
  theRedrawEntireFrameIndicator = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::preFrameUpdate()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::postFrameUpdate()
{
  // Now show all changes made to the textures
  SDL_GL_SwapBuffers();
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

  for(uInt32 i = 0; i < 256; i++)
  {
    glBindTexture(GL_TEXTURE_2D, myFontTextureID[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myFilterParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myFilterParam);
  }

  // The filtering has changed, so redraw the entire screen
  theRedrawEntireFrameIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color)
{
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glLineWidth(2);
  glColor4f(myGUIPalette[color][0],
            myGUIPalette[color][1],
            myGUIPalette[color][2],
            1.0);
  glBegin(GL_LINES);
    glVertex2i(x,  y);
    glVertex2i(x2, y);
  glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glLineWidth(2);
  glColor4f(myGUIPalette[color][0],
            myGUIPalette[color][1],
            myGUIPalette[color][2],
            1.0);
  glBegin(GL_LINES);
    glVertex2i(x, y );
    glVertex2i(x, y2);
  glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::blendRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                              OverlayColor color, uInt32 level)
{
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glColor4f(myGUIPalette[color][0],
            myGUIPalette[color][1],
            myGUIPalette[color][2],
            0.7);
  glRecti(x, y, x+w-1, y+h-1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                             OverlayColor color)
{
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glColor4f(myGUIPalette[color][0],
            myGUIPalette[color][1],
            myGUIPalette[color][2],
            1.0);
  glRecti(x, y, x+w-1, y+h-1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawChar(uInt8 chr, uInt32 xorig, uInt32 yorig,
                             OverlayColor color)
{
/*
  // If this character is not included in the font, use the default char.
  if(chr < myFont->desc().firstchar ||
     chr >= myFont->desc().firstchar + myFont->desc().size)
  {
    if (chr == ' ')
      return;
    chr = myFont->desc().defaultchar;
  }

  const Int32 w = myFont->getCharWidth(chr);
  const Int32 h = myFont->getFontHeight();
  chr -= myFont->desc().firstchar;
  const uInt16* tmp = myFont->desc().bits + (myFont->desc().offset ?
                      myFont->desc().offset[chr] : (chr * h));

  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    const uInt16 buffer = *tmp++;
    uInt16 mask = 0x8000;
//    if(ty + y < 0 || ty + y >= dst->h)
//      continue;

    for(int x = 0; x < w; x++, mask >>= 1)
    {
//      if (tx + x < 0 || tx + x >= dst->w)
//        continue;
      if ((buffer & mask) != 0)
      {
        rect.x = (x + xorig) * theZoomLevel;
        rect.y = (y + yorig) * theZoomLevel;
        rect.w = rect.h = theZoomLevel;
        SDL_FillRect(myScreen, &rect, myGUIPalette[color]);
      }
    }
  }
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawBitmap(uInt32* bitmap, Int32 xorig, Int32 yorig,
                                 OverlayColor color, Int32 h)
{
/*
  SDL_Rect rect;
  for(int y = 0; y < h; y++)
  {
    uInt32 mask = 0xF0000000;
//    if(ty + y < 0 || ty + y >= _screen.h)
//      continue;

    for(int x = 0; x < 8; x++, mask >>= 4)
    {
//      if(tx + x < 0 || tx + x >= _screen.w)
//        continue;
      if(bitmap[y] & mask)
      {
        rect.x = (x + xorig) * theZoomLevel;
        rect.y = (y + yorig) * theZoomLevel;
        rect.w = rect.h = theZoomLevel;
        SDL_FillRect(myScreen, &rect, myGUIPalette[color]);
      }
    }
  }
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::translateCoords(Int32* x, Int32* y)
{
  // Wow, what a mess :)
  *x = (Int32) (((*x - myImageDim.x) / (theZoomLevel * myFSScaleFactor * theAspectRatio)));
  *y = (Int32) (((*y - myImageDim.y) / (theZoomLevel * myFSScaleFactor)));
}

/*
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawChar(uInt32 x, uInt32 y, uInt32 c)
{
  if(c >= 256 )
    return;

  glBindTexture(GL_TEXTURE_2D, myFontTextureID[c]);
  glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2i(x,   y  );
    glTexCoord2f(1, 0); glVertex2i(x+8, y  );
    glTexCoord2f(1, 1); glVertex2i(x+8, y+8);
    glTexCoord2f(0, 1); glVertex2i(x,   y+8);
  glEnd();
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createTextures()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);

  glDeleteTextures(1, &myTextureID);
  glDeleteTextures(256, myFontTextureID);

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

/*
  // Now create the font textures.  There are 256 fonts of 8x8 pixels.
  // These will be stored in 256 textures of size 8x8.
  SDL_Surface* fontTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32,
  #if SDL_BYTEORDER == SDL_LIL_ENDIAN 
    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
  #else
    0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  #endif

  if(fontTexture == NULL)
    return false;

  // Create a texture for each character
  glGenTextures(256, myFontTextureID);

  for(uInt32 c = 0; c < 256; c++)
  {
    // First clear the texture
    SDL_Rect tmp;
    tmp.x = 0; tmp.y = 0; tmp.w = 8; tmp.h = 8;
    SDL_FillRect(fontTexture, &tmp,
                 SDL_MapRGBA(fontTexture->format, 0xff, 0xff, 0xff, 0x0));

    // Now fill the texture with font data
    for(uInt32 y = 0; y < 8; y++)
    {
      for(uInt32 x = 0; x < 8; x++)
      {
        if((ourFontData[(c << 3) + y] >> x) & 1)
        {
          tmp.x = x;
          tmp.y = y;
          tmp.w = tmp.h = 1;
          SDL_FillRect(fontTexture, &tmp,
            SDL_MapRGBA(fontTexture->format, 0x10, 0x10, 0x10, 0xff));
        }
      }
    }

    glBindTexture(GL_TEXTURE_2D, myFontTextureID[c]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myFilterParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myFilterParam);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               fontTexture->pixels);
  }

  SDL_FreeSurface(fontTexture);
*/

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

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

      // Figure out which dimension is closest to the 10% mark,
      // and calculate the scaling required to bring it to exactly 10%
      if(scaleX > scaleY)
        myFSScaleFactor = float(myScreenDim.w * 0.9) / myImageDim.w;
      else
        myFSScaleFactor = float(myScreenDim.h * 0.9) / myImageDim.h;

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

      // Figure out which dimension is closest to the 10% mark,
      // and calculate the scaling required to bring it to exactly 10%
      if(scaleX > scaleY)
        myFSScaleFactor = (myScreenDim.w * 0.9) / myImageDim.w;
      else
        myFSScaleFactor = (myScreenDim.h * 0.9) / myImageDim.h;

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
