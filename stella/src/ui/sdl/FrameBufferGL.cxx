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
// $Id: FrameBufferGL.cxx,v 1.9 2003-11-30 22:50:15 stephena Exp $
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
   :  myTexture(0),
      myFilterParam(GL_NEAREST)
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
bool FrameBufferGL::createScreen()
{
  uInt32 w = (uInt32) (myWidth  * theZoomLevel * theAspectRatio);
  uInt32 h = myHeight * theZoomLevel;

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

  glOrtho(0.0, (GLdouble) myScreen->w/(theZoomLevel * theAspectRatio),
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
  const uInt32* gamePalette = myMediaSource->palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) (((gamePalette[i] & 0x00ff0000) >> 16) * shade);
    g = (Uint8) (((gamePalette[i] & 0x0000ff00) >> 8) * shade);
    b = (Uint8) ((gamePalette[i] & 0x000000ff) * shade);

    myPalette[i] = SDL_MapRGB(myTexture->format, r, g, b);
  }

  theRedrawEntireFrameIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::init()
{
  // Get the desired width and height of the display
  myWidth  = myMediaSource->width() << 1;
  myHeight = myMediaSource->height();

  // Get the aspect ratio for the display
  // Since the display is already doubled horizontally, we half the
  // ratio that is provided
  theAspectRatio = myConsole->settings().getFloat("gl_aspect") / 2;
  if(theAspectRatio <= 0.0)
    theAspectRatio = 1.0;

  // Now create the OpenGL SDL screen
  Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
  if(SDL_Init(initflags) < 0)
    return false;

  // Check which system we are running under
  x11Available = false;
#ifdef UNIX
  SDL_VERSION(&myWMInfo.version);
  if(SDL_GetWMInfo(&myWMInfo) > 0)
    if(myWMInfo.subsystem == SDL_SYSWM_X11)
      x11Available = true;
#endif

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
  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, myRGB[0] );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, myRGB[1] );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, myRGB[2] );
  SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, myRGB[3] );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  // Create the screen
  if(!createScreen())
    return false;

  // Now check to see what color components were actually created
  SDL_GL_GetAttribute( SDL_GL_RED_SIZE, (int*)&myRGB[0] );
  SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, (int*)&myRGB[1] );
  SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, (int*)&myRGB[2] );
  SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, (int*)&myRGB[3] );

  // Create the texture surface and texture fonts
  createTextures();

  // Set up the palette *after* we know the color components
  // and the textures
  setupPalette(1.0);

  // Show some OpenGL info
  if(myConsole->settings().getBool("showinfo"))
  {
    ostringstream colormode;
    colormode << "Color   : " << myDepth << " bit, " << myRGB[0] << "-"
              << myRGB[1] << "-" << myRGB[2] << "-" << myRGB[3];

    cout << endl
         << "Vendor  : " << glGetString(GL_VENDOR) << endl
         << "Renderer: " << glGetString(GL_RENDERER) << endl
         << "Version : " << glGetString(GL_VERSION) << endl
         << colormode.str() << endl;   
  }

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
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawMediaSource()
{
  // Copy the mediasource framebuffer to the RGB texture
  uInt8* currentFrame  = myMediaSource->currentFrameBuffer();
  uInt8* previousFrame = myMediaSource->previousFrameBuffer();
  uInt32 width         = myMediaSource->width();
  uInt32 height        = myMediaSource->height();
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

  // Texturemap complete texture to surface so we have free scaling 
  // and antialiasing 
  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myTexture->w, myTexture->h,
                  GL_RGB, GL_UNSIGNED_SHORT_5_6_5, myTexture->pixels);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glColor3f(0.0, 0.0, 0.0);

  glBegin(GL_QUADS);
    glTexCoord2f(myTexCoord[0], myTexCoord[1]); glVertex2i(0, 0);
    glTexCoord2f(myTexCoord[2], myTexCoord[1]); glVertex2i(myWidth, 0);
    glTexCoord2f(myTexCoord[2], myTexCoord[3]); glVertex2i(myWidth, myHeight);
    glTexCoord2f(myTexCoord[0], myTexCoord[3]); glVertex2i(0, myHeight);
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
void FrameBufferGL::drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
  // First draw the box in the background, alpha-blended
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glColor4f(0.0, 0.0, 0.0, 0.7);
  glRecti(x, y, x+w, y+h);

  // Now draw the outer edges
  glLineWidth(theZoomLevel/2);
  glColor4f(0.8, 0.8, 0.8, 1.0);
  glBegin(GL_LINE_LOOP);
    glVertex2i(x,   y  );  // Top Left
    glVertex2i(x+w, y  );  // Top Right
    glVertex2i(x+w, y+h);  // Bottom Right
    glVertex2i(x,   y+h);  // Bottom Left
  glEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::drawText(uInt32 x, uInt32 y, const string& message)
{
  for(uInt32 i = 0; i < message.length(); i++)
    drawChar(x + i*8, y, (uInt32) message[i]);
}

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferGL::createTextures()
{
  uInt32 w = power_of_two(myWidth);
  uInt32 h = power_of_two(myHeight);

  myTexCoord[0] = 0.0f;
  myTexCoord[1] = 0.0f;
  myTexCoord[2] = (GLfloat) myWidth / w;
  myTexCoord[3] = (GLfloat) myHeight / h;

  myTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16,
    0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);

  if(myTexture == NULL)
    return false;

  // Create an OpenGL texture from the SDL texture
  bool showinfo = myConsole->settings().getBool("showinfo");
  string filter = myConsole->settings().getString("gl_filter");
  if(filter == "linear")
  {
    myFilterParam = GL_LINEAR;
    if(showinfo)
      cout << "Using GL_LINEAR filtering.\n\n";
  }
  else if(filter == "nearest")
  {
    myFilterParam = GL_NEAREST;
    if(showinfo)
      cout << "Using GL_NEAREST filtering.\n\n";
  }

  glGenTextures(1, &myTextureID);
  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myFilterParam);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myFilterParam);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
               myTexture->pixels);

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

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBufferGL::toggleFilter()
{
  if(myFilterParam == GL_NEAREST)
  {
    myFilterParam = GL_LINEAR;
    myConsole->settings().setString("gl_filter", "linear");
  }
  else
  {
    myFilterParam = GL_NEAREST;
    myConsole->settings().setString("gl_filter", "nearest");
  }

  glBindTexture(GL_TEXTURE_2D, myTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myFilterParam);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, myFilterParam);

  for(uInt32 i =0; i < 256; i++)
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
