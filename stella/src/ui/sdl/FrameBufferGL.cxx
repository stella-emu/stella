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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferGL.cxx,v 1.2 2003-11-09 23:53:20 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferSDL.hxx"
#include "FrameBufferGL.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::FrameBufferGL()
   :  myTexture(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferGL::~FrameBufferGL()
{
  if(myTexture)
    SDL_FreeSurface(myTexture);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createScreen()
{
  int w = myWidth  * theZoomLevel;
  int h = myHeight * theZoomLevel;

  myScreen = SDL_SetVideoMode(w, h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    cerr << "ERROR: Unable to open SDL window: " << SDL_GetError() << endl;
    return false;
  }

  glPushAttrib(GL_ENABLE_BIT);
  glViewport(0, 0, myScreen->w, myScreen->h);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  glOrtho(0.0, (GLdouble) myScreen->w/theZoomLevel,
          (GLdouble) myScreen->h/theZoomLevel, 0.0, 0.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  theRedrawEntireFrameIndicator = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::setupPalette(float shade)
{
// FIXME - OpenGL should be able to shade the texture itself
  const uInt32* gamePalette = myMediaSource->palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b, a;

    r = (Uint8) (((gamePalette[i] & 0x00ff0000) >> 16) * shade);
    g = (Uint8) (((gamePalette[i] & 0x0000ff00) >> 8) * shade);
    b = (Uint8) ((gamePalette[i] & 0x000000ff) * shade);
    a = 0xff;

  #if SDL_BYTEORDER == SDL_LIL_ENDIAN 
    myPalette[i] = (a << 24) | (b << 16) | (g << 8) | r;
  #else
    myPalette[i] = (r << 24) | (g << 16) | (b << 8) | a;
  #endif
  }

  theRedrawEntireFrameIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::init()
{
  // Get the desired width and height of the display
  myWidth  = myMediaSource->width() << 1;
  myHeight = myMediaSource->height();

  // Now create the OpenGL SDL screen
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
  if(SDL_Init(initflags) < 0)
    return false;

  // Check which system we are running under
  x11Available = false;
  SDL_VERSION(&myWMInfo.version);
  if(SDL_GetWMInfo(&myWMInfo) > 0)
    if(myWMInfo.subsystem == SDL_SYSWM_X11)
      x11Available = true;

  // Get the maximum size of a window for THIS screen
  theMaxZoomLevel = maxWindowSizeForScreen();

  // Check to see if window size will fit in the screen
  if((uInt32)myConsole->settings().getInt("zoom") > theMaxZoomLevel)
    theZoomLevel = theMaxZoomLevel;
  else
    theZoomLevel = myConsole->settings().getInt("zoom");

  mySDLFlags = SDL_OPENGL;
  mySDLFlags |= myConsole->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;

  // Set the window title and icon
  ostringstream name;
  name << "Stella: \"" << myConsole->properties().get("Cartridge.Name") << "\"";
  SDL_WM_SetCaption(name.str().c_str(), "stella");

  // Set up the OpenGL attributes
  int rgb_size[3];
  int bpp = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
  switch(bpp)
  {
    case 8:
      rgb_size[0] = 3;
      rgb_size[1] = 3;
      rgb_size[2] = 2;
      break;

    case 15:
    case 16:
      rgb_size[0] = 5;
      rgb_size[1] = 5;
      rgb_size[2] = 5;
      break;

    default:
      rgb_size[0] = 8;
      rgb_size[1] = 8;
      rgb_size[2] = 8;
      break;
  }
  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, rgb_size[0] );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, rgb_size[1] );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, rgb_size[2] );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, bpp );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  // Create the screen
  if(!createScreen())
    return false;
  setupPalette(1.0);

  // Show some OpenGL info
  if(myConsole->settings().getBool("showinfo"))
  {
    cout << endl
         << "Vendor  : " << glGetString(GL_VENDOR) << endl
         << "Renderer: " << glGetString(GL_RENDERER) << endl
         << "Version : " << glGetString(GL_VERSION) << endl;
  }

  // Create the texture surface and texture fonts
  createTextures();

  // Make sure that theUseFullScreenFlag sets up fullscreen mode correctly
  theGrabMouseIndicator  = myConsole->settings().getBool("grabmouse");
  theHideCursorIndicator = myConsole->settings().getBool("hidecursor");
  if(myConsole->settings().getBool("fullscreen"))
  {
    grabMouse(true);
    showCursor(false);
    isFullscreen = true;
  }
  else
  {
    // Keep mouse in game window if grabmouse is selected
    grabMouse(theGrabMouseIndicator);

    // Show or hide the cursor depending on the 'hidecursor' argument
    showCursor(!theHideCursorIndicator);
  }

  // Set up global GL stuff
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawMediaSource() // FIXME - maybe less copying can be done?
{
  // Copy the mediasource framebuffer to the RGB texture
  uInt8* currentFrame  = myMediaSource->currentFrameBuffer();
  uInt8* previousFrame = myMediaSource->previousFrameBuffer();
  uInt32 width  = myMediaSource->width();
  uInt32 height = myMediaSource->height();
  uInt32* buffer = (uInt32*) myTexture->pixels;

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

      // x << 1 is times 2 ( doubling width ) WIDTH_FACTOR
      const uInt32 pos = screenofsY + (x << 1);
      buffer[pos] = buffer[pos+1] = myPalette[v];
    }
  }

  // Texturemap complete texture to surface so we have free scaling 
  // and antialiasing 
  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexture->w, myTexture->h,
                  GL_RGBA, GL_UNSIGNED_BYTE, myTexture->pixels);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glColor3f(0.0, 0.0, 0.0);

  glBegin(GL_QUADS);
    glTexCoord2f(myTexCoord[0], myTexCoord[1]); glVertex2i(0, 0);
    glTexCoord2f(myTexCoord[2], myTexCoord[1]); glVertex2i(myWidth, 0);
    glTexCoord2f(myTexCoord[2], myTexCoord[3]); glVertex2i(myWidth, myHeight);
    glTexCoord2f(myTexCoord[0], myTexCoord[3]); glVertex2i(0, myHeight);
  glEnd();
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
void FrameBufferGL::drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // First draw the box in the background, alpha-blended
  glEnable(GL_BLEND);
  glColor4f(0.0, 0.0, 0.0, 0.7);
  glRecti(x, y, x+w, y+h);

  // Now draw the outer edges
  glDisable(GL_BLEND);
  glColor3f(0.8, 0.8, 0.8);
  glBegin(GL_LINE_LOOP);
    glVertex2i(x,   y  );  // Top Left
    glVertex2i(x+w, y  );  // Top Right
    glVertex2i(x+w, y+h);  // Bottom Right
    glVertex2i(x,   y+h);  // Bottom Left
  glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawText(uInt32 xorig, uInt32 yorig, const string& message)
{
  glBindTexture(GL_TEXTURE_2D, myFontTextureID);
  glEnable(GL_BLEND);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

  // We place the loop here to avoid multiple calls to glBegin/glEnd
  glBegin(GL_QUADS);
  for(uInt32 i = 0; i < message.length(); i++)
  {
    uInt32 x = xorig + i*8;
    uInt32 y = yorig;
    uInt8 c = message[i];

    glTexCoord2f(myFontCoord[c].minX, myFontCoord[c].minY); glVertex2i(x,   y  );
    glTexCoord2f(myFontCoord[c].maxX, myFontCoord[c].minY); glVertex2i(x+8, y  );
    glTexCoord2f(myFontCoord[c].maxX, myFontCoord[c].maxY); glVertex2i(x+8, y+8);
    glTexCoord2f(myFontCoord[c].minX, myFontCoord[c].maxY); glVertex2i(x,   y+8);
  }
  glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawChar(uInt32 x, uInt32 y, uInt32 c)
{
  if(c >= 256 )
    return;

  glBindTexture(GL_TEXTURE_2D, myFontTextureID);
  glEnable(GL_BLEND);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glBegin(GL_QUADS);
    glTexCoord2f(myFontCoord[c].minX, myFontCoord[c].minY); glVertex2i(x,   y  );
    glTexCoord2f(myFontCoord[c].maxX, myFontCoord[c].minY); glVertex2i(x+8, y  );
    glTexCoord2f(myFontCoord[c].maxX, myFontCoord[c].maxY); glVertex2i(x+8, y+8);
    glTexCoord2f(myFontCoord[c].minX, myFontCoord[c].maxY); glVertex2i(x,   y+8);
  glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createTextures()
{
  uInt32 w = power_of_two(myWidth);
  uInt32 h = power_of_two(myHeight);

  myTexCoord[0] = 0.0f;
  myTexCoord[1] = 0.0f;
  myTexCoord[2] = (GLfloat) myWidth / w;
  myTexCoord[3] = (GLfloat) myHeight / h;

  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
  #if SDL_BYTEORDER == SDL_LIL_ENDIAN 
    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
  #else
    0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  #endif

  if(myTexture == NULL)
    return false;

  // Create an OpenGL texture from the SDL texture
  bool showinfo = myConsole->settings().getBool("showinfo");
  string filter = myConsole->settings().getString("gl_filter");
  GLint param = GL_NEAREST;
  if(filter == "linear")
  {
    param = GL_LINEAR;
    if(showinfo)
      cout << "Using GL_LINEAR filtering.\n\n";
  }
  else if(filter == "nearest")
  {
    param = GL_NEAREST;
    if(showinfo)
      cout << "Using GL_NEAREST filtering.\n\n";
  }

  glGenTextures(1, &myTextureID);
  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               myTexture->pixels);

  // Now create the font texture.  There are 256 fonts of 8x8 pixels.
  // These will be stored in a texture of size 256x64, which is 32 characters
  // per line, and 8 lines.
  SDL_Surface* fontTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, 256, 64, 32,
  #if SDL_BYTEORDER == SDL_LIL_ENDIAN 
    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
  #else
    0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  #endif

  if(fontTexture == NULL)
    return false;

  // First clear the texture
  SDL_Rect tmp;
  tmp.x = 0; tmp.y = 0; tmp.w = 256; tmp.h = 64;
  SDL_FillRect(fontTexture, &tmp,
               SDL_MapRGBA(fontTexture->format, 0xff, 0xff, 0xff, 0x0));

  // Now fill the texture with font data
  for(uInt32 lines = 0; lines < 8; lines++)
  {
    for(uInt32 x = lines*32; x < (lines+1)*32; x++)
    {
      for(uInt32 y = 0; y < 8; y++)
      {
        for(uInt32 z = 0; z < 8; z++)
        {
          if((ourFontData[(x << 3) + y] >> z) & 1)
          {
            tmp.x = ((x-lines*32)<<3) + z;
            tmp.y = y + lines*8;
            tmp.w = tmp.h = 1;
            SDL_FillRect(fontTexture, &tmp,
                SDL_MapRGBA(fontTexture->format, 0x10, 0x10, 0x10, 0xff));
          }
        }
      }
    }
  }

  // Generate the character coordinates
  for(uInt32 i = 0; i < 256; i++)
  {
    uInt32 row = i / 32;
    uInt32 col = i - (row*32);

    myFontCoord[i].minX = (GLfloat) (col*8)   / 256;
    myFontCoord[i].maxX = (GLfloat) (col*8+8) / 256;
    myFontCoord[i].minY = (GLfloat) (row*8)   / 64;
    myFontCoord[i].maxY = (GLfloat) (row*8+8) / 64;
  }

  glGenTextures(1, &myFontTextureID);
  glBindTexture(GL_TEXTURE_2D, myFontTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               fontTexture->pixels);

  SDL_FreeSurface(fontTexture);

  return true;
}
