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
// $Id: FrameBufferGL.hxx,v 1.2 2004-06-20 23:30:48 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_GL_HXX
#define FRAMEBUFFER_GL_HXX

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

#include "FrameBuffer.hxx"
#include "FrameBufferSDL.hxx"
#include "bspf.hxx"

class Console;
class MediaSource;

/**
  This class implements an SDL OpenGL framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferGL.hxx,v 1.2 2004-06-20 23:30:48 stephena Exp $
*/
class FrameBufferGL : public FrameBufferSDL
{
  public:
    /**
      Creates a new SDL OpenGL framebuffer
    */
    FrameBufferGL();

    /**
      Destructor
    */
    virtual ~FrameBufferGL();

    /**
      Switches between the two filtering options in OpenGL.
      Currently, these are GL_NEAREST and GL_LINEAR.
    */
	void toggleFilter();

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBufferSDL.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen();

    /**
      This routine is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b)
      { return SDL_MapRGB(myTexture->format, r, g, b); }

    /**
      This routine is called to get the width of the onscreen image.
    */
    virtual uInt32 winWidth() { return (uInt32) (myWidth  * theZoomLevel * theAspectRatio); }

    /**
      This routine is called to get the height of the onscreen image.
    */
    virtual uInt32 winHeight() { return myHeight * theZoomLevel; }

    /**
      This routine is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    virtual void scanline(uInt32 row, uInt8* data);

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine should be called once the console is created to setup
      the video system for us to use.  Return false if any operation fails,
      otherwise return true.
    */
    virtual bool init();

    /**
      This routine should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    virtual void drawMediaSource();

    /**
      This routine should be called to draw a rectangular box with sides
      at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param w   The width of the box
      @param h   The height of the box
    */
    virtual void drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h);

    /**
      This routine should be called to draw text at the specified coordinates.

      @param x        The x coordinate
      @param y        The y coordinate
      @param message  The message text
    */
    virtual void drawText(uInt32 x, uInt32 y, const string& message);

    /**
      This routine should be called to draw character 'c' at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param c   The character to draw
    */
    virtual void drawChar(uInt32 x, uInt32 y, uInt32 c);

    /**
      This routine is called before any drawing is done (per-frame).
    */
    virtual void preFrameUpdate();

    /**
      This routine is called after any drawing is done (per-frame).
    */
    virtual void postFrameUpdate();

  private:

    bool createTextures();

    SDL_Rect viewport(uInt32 width, uInt32 height);

    uInt32 power_of_two(uInt32 input)
    {
      uInt32 value = 1;
      while( value < input )
        value <<= 1;
      return value;
    }

  private:
    // The main texture buffer
    SDL_Surface* myTexture;

    // The depth of the texture buffer
    uInt32 myDepth;

    // The size of color components for OpenGL
    uInt32 myRGB[4];

    // The OpenGL main texture handle
    GLuint myTextureID;

    // OpenGL texture coordinates for the main surface
    GLfloat myTexCoord[4];

    // The OpenGL font texture handles (one for each character)
    GLuint myFontTextureID[256];

    // The texture filtering to use
    GLint myFilterParam;
};

#endif
