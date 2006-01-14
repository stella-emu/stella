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
// $Id: FrameBufferGL.hxx,v 1.28 2006-01-14 22:29:34 stephena Exp $
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
#include "GuiUtils.hxx"
#include "FrameBuffer.hxx"


/**
  This class implements an SDL OpenGL framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferGL.hxx,v 1.28 2006-01-14 22:29:34 stephena Exp $
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
    virtual bool initSubsystem();

    /**
      This method is called to set the aspect ratio of the screen.
    */
    virtual void setAspectRatio();

    /**
      This method is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen();

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
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color);

    /**
      This method is called to draw a vertical line.

      @param x     The x coordinate
      @param y     The first y coordinate
      @param y2    The second y coordinate
      @param color The color of the line
    */
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color);

    /**
      This method is called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the area
    */
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          OverlayColor color);

    /**
      This method is called to draw the specified character.

      @param font   The font to use to draw the character
      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    virtual void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y,
                          OverlayColor color);

    /**
      This method is called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
      @param h      The height of the data image
    */
    virtual void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, OverlayColor color,
                            Int32 h = 8);

    /**
      This method translates the given coordinates to their
      unzoomed/unscaled equivalents.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    inline virtual void translateCoords(Int32* x, Int32* y);

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
      Enable/disable phosphor effect
    */
    virtual void enablePhosphor(bool enable);

  private:
    bool createTextures();

    void setDimensions(GLdouble* orthoWidth, GLdouble* orthoHeight);

    inline uInt32 power_of_two(uInt32 input)
    {
      uInt32 value = 1;
      while( value < input )
        value <<= 1;
      return value;
    }

  private:
    // The main texture buffer
    SDL_Surface* myTexture;

    // The possible OpenGL screenmodes to use
    SDL_Rect** myScreenmode;

    // The number of usable OpenGL screenmodes
    uInt32 myScreenmodeCount;

    // The depth of the texture buffer
    uInt32 myDepth;

    // The size of color components for OpenGL
    uInt32 myRGB[4];

    // The OpenGL main texture handle
    GLuint myTextureID;

    // OpenGL texture coordinates for the main surface
    GLfloat myTexCoord[4];

    // The texture filtering to use
    GLint myFilterParam;

    // The name of the texture filtering to use
    string myFilterParamName;

    // The scaling to use in fullscreen mode
    // This is separate from both zoomlevel and aspect ratio
    float myFSScaleFactor;

    // TODO - will be removed when textured dirty rect support is added
    bool myDirtyFlag;

    // Indicates if the OpenGL functions have been properly loaded
    static bool myFuncsLoaded;
};

#endif  // DISPLAY_OPENGL

#endif

