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
// $Id: FrameBufferD3D.hxx,v 1.2 2007-09-03 18:37:24 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_D3D_HXX
#define FRAMEBUFFER_D3D_HXX

#ifdef DISPLAY_D3D

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

#include <d3d9.h>
#include <d3dx9.h>

#include "bspf.hxx"
#include "FrameBuffer.hxx"

class OSystem;
class GUI::Font;

/**
  This class implements an SDL Direct3D framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferD3D.hxx,v 1.2 2007-09-03 18:37:24 stephena Exp $
*/
class FrameBufferD3D : public FrameBuffer
{
  public:
    /**
      Creates a new Direct3D framebuffer
    */
    FrameBufferD3D(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferD3D();

    /**
      Check if OpenGL is available on this system and dynamically load
      all required GL functions.  If any errors occur, we shouldn't attempt
      to instantiate a FrameBufferGL object.

      @param library  The filename of the OpenGL library
    */
    static bool loadFuncs(const string& library);

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize OpenGL video mode.
      Return false if any operation fails, otherwise return true.
    */
    virtual bool initSubsystem(VideoMode mode);

    /**
      This method is called to query the type of the FrameBuffer.
    */
    virtual BufferType type() { return kGLBuffer; }

    /**
      This method is called to provide information about the FrameBuffer.
    */
    virtual string about();

    /**
      This method is called to change to the given video mode.

      @param mode  The mode to use for rendering the mediasource
    */
    virtual bool setVidMode(VideoMode mode);

    /**
      Switches between the two filtering options in OpenGL.
      Currently, these are GL_NEAREST and GL_LINEAR.
    */
    virtual void toggleFilter();

    /**
      This method should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    virtual void drawMediaSource();

    /**
      This method is called before any drawing is done (per-frame).
    */
    virtual void preFrameUpdate();

    /**
      This method is called after any drawing is done (per-frame).
    */
    virtual void postFrameUpdate();

    /**
      This method is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    virtual void scanline(uInt32 row, uInt8* data);

    /**
      This method is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b)
      { return SDL_MapRGB(myTexture->format, r, g, b); }

    /**
      This method is called to draw a horizontal line.

      @param x     The first x coordinate
      @param y     The y coordinate
      @param x2    The second x coordinate
      @param color The color of the line
    */
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, int color);

    /**
      This method is called to draw a vertical line.

      @param x     The x coordinate
      @param y     The first y coordinate
      @param y2    The second y coordinate
      @param color The color of the line
    */
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, int color);

    /**
      This method is called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the area
    */
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          int color);

    /**
      This method is called to draw the specified character.

      @param font   The font to use to draw the character
      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    virtual void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y,
                          int color);

    /**
      This method is called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
      @param h      The height of the data image
    */
    virtual void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, int color,
                            Int32 h = 8);

    /**
      This method translates the given coordinates to their
      unzoomed/unscaled equivalents.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    inline virtual void translateCoords(Int32& x, Int32& y);

    /**
      This method adds a dirty rectangle
      (ie, an area of the screen that has changed)

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
    */
    virtual void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);

    /**
      Enable/disable phosphor effect.
    */
    virtual void enablePhosphor(bool enable, int blend);

  private:
    bool createTextures();

    inline uInt32 power_of_two(uInt32 input)
    {
      uInt32 value = 1;
      while( value < input )
        value <<= 1;
      return value;
    }

  private:
    // Points to the current texture data
    SDL_Surface* myTexture;

    // Holds all items specifically needed by GL commands
    struct glBufferType
    {
      GLuint  texture;
      GLsizei texture_width;
      GLsizei texture_height;
      GLfloat tex_coord[4];

      GLenum  target;
      GLenum  format;
      GLenum  type;
      GLint   filter;

      void*   pixels;
      int     width, height;
      int     pitch;
    };
    glBufferType myBuffer;

    // Optional GL extensions that may increase performance
    bool myHaveTexRectEXT;

    // The depth of the texture buffer
    uInt32 myDepth;

    // The size of color components for OpenGL
    uInt32 myRGB[4];

    // The name of the texture filtering to use
    string myFilterParamName;

    // The amount by which to scale the imagein fullscreen mode
    float myScaleFactor;

    // TODO - will be removed when textured dirty rect support is added
    bool myDirtyFlag;

    // Indicates if the OpenGL functions have been properly loaded
    static bool myFuncsLoaded;
};

#endif  // DISPLAY_D3D

#endif
