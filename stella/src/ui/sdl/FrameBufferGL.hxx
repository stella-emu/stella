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
// $Id: FrameBufferGL.hxx,v 1.4 2003-11-17 17:43:39 stephena Exp $
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
  @version $Id: FrameBufferGL.hxx,v 1.4 2003-11-17 17:43:39 stephena Exp $
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

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBufferSDL.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen();

    /**
      Set up the palette for a screen of any depth > 8.
      Scales the palette by 'shade'.
    */
    virtual void setupPalette(float shade);

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

    // The OpenGL main texture handle
    GLuint myTextureID;

    // OpenGL texture coordinates for the main surface
    GLfloat myTexCoord[4];

    // The OpenGL font texture handles (one for each character)
    GLuint myFontTextureID[256];
};

#endif
