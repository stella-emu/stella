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
// $Id: FrameBufferGL.hxx,v 1.51 2008-02-06 13:45:19 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_GL_HXX
#define FRAMEBUFFER_GL_HXX

#ifdef DISPLAY_OPENGL

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

class OSystem;
class GUI::Font;

#include "bspf.hxx"
#include "FrameBuffer.hxx"

/**
  This class implements an SDL OpenGL framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferGL.hxx,v 1.51 2008-02-06 13:45:19 stephena Exp $
*/
class FrameBufferGL : public FrameBuffer
{
  public:
    /**
      Creates a new OpenGL framebuffer
    */
    FrameBufferGL(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferGL();

    /**
      Check if OpenGL is available on this system, and then opens it.
      If any errors occur, we shouldn't attempt to instantiate a
      FrameBufferGL object.

      @param library  The filename of the OpenGL library
    */
    static bool loadLibrary(const string& library);

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize OpenGL video mode.
      Return false if any operation fails, otherwise return true.
    */
    bool initSubsystem(VideoMode mode);

    /**
      This method is called to query the type of the FrameBuffer.
    */
    BufferType type() const { return kGLBuffer; }

    /**
      This method is called to provide information about the FrameBuffer.
    */
    string about() const;

    /**
      This method is called to change to the given video mode.

      @param mode  The mode to use for rendering the mediasource
    */
    bool setVidMode(VideoMode mode);

    /**
      Switches between the two filtering options in OpenGL.
      Currently, these are GL_NEAREST and GL_LINEAR.
    */
    void toggleFilter();

    /**
      This method should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    void drawMediaSource();

    /**
      This method is called before any drawing is done (per-frame).
    */
    void preFrameUpdate();

    /**
      This method is called after any drawing is done (per-frame).
    */
    void postFrameUpdate();

    /**
      This method is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    void scanline(uInt32 row, uInt8* data) const;

    /**
      This method is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
      { return SDL_MapRGB(myTexture->format, r, g, b); }

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param width   The requested width of the new surface.
      @param height  The requested height of the new surface.
    */
    GUI::Surface* createSurface(int width, int height) const;

    /**
      This method is called to draw a horizontal line.

      @param x     The first x coordinate
      @param y     The y coordinate
      @param x2    The second x coordinate
      @param color The color of the line
    */
    void hLine(uInt32 x, uInt32 y, uInt32 x2, int color);

    /**
      This method is called to draw a vertical line.

      @param x     The x coordinate
      @param y     The first y coordinate
      @param y2    The second y coordinate
      @param color The color of the line
    */
    void vLine(uInt32 x, uInt32 y, uInt32 y2, int color);

    /**
      This method is called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the area
    */
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color);

    /**
      This method is called to draw the specified character.

      @param font   The font to use to draw the character
      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, int color);

    /**
      This method is called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
      @param h      The height of the data image
    */
    void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, int color, Int32 h = 8);

    /**
      This method should be called to draw an SDL surface.

      @param surface The data to draw
      @param x       The x coordinate
      @param y       The y coordinate
    */
    void drawSurface(const GUI::Surface* surface, Int32 x, Int32 y);

    /**
      This method should be called to convert and copy a given row of RGB
      data into an SDL surface.

      @param surface The data to draw
      @param row     The row of the surface the data should be placed in
      @param data    The data in uInt8 R/G/B format
    */
    void bytesToSurface(GUI::Surface* surface, int row, uInt8* data) const;

    /**
      This method translates the given coordinates to their
      unzoomed/unscaled equivalents.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    void translateCoords(Int32& x, Int32& y) const;

    /**
      This method adds a dirty rectangle
      (ie, an area of the screen that has changed)

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
    */
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);

    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

  private:
    bool loadFuncs();

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

    // The amount by which to scale the image in each dimension in fullscreen mode
    float myWidthScaleFactor, myHeightScaleFactor;

    // Indicates that the texture has been modified, and should be redrawn
    bool myDirtyFlag;

    // Indicates if the OpenGL library has been properly loaded
    static bool myLibraryLoaded;
};

#endif  // DISPLAY_OPENGL

#endif
