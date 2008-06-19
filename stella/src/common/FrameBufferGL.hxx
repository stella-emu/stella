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
// $Id: FrameBufferGL.hxx,v 1.54 2008-06-19 12:01:30 stephena Exp $
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
  @version $Id: FrameBufferGL.hxx,v 1.54 2008-06-19 12:01:30 stephena Exp $
*/
class FrameBufferGL : public FrameBuffer
{
  friend class FBSurfaceGL;

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
    // The following are derived from public methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

    /**
      This method is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
      { return SDL_MapRGB(myTexture->format, r, g, b); }

    /**
      This method is called to query the type of the FrameBuffer.
    */
    BufferType type() const { return kGLBuffer; }

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one
    */
    FBSurface* createSurface(int w, int h, bool useBase = false) const;

    /**
      This method is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    void scanline(uInt32 row, uInt8* data) const;

  protected:
    //////////////////////////////////////////////////////////////////////
    // The following are derived from protected methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize the video subsystem
      with the given video mode.  Normally, it will also call setVidMode().

      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    bool initSubsystem(VideoMode& mode);

    /**
      This method is called to change to the given video mode.  If the mode
      is successfully changed, 'mode' holds the actual dimensions used.

      @param mode  The video mode to use

      @return  False on any errors (in which case 'mode' is invalid), else true
    */
    bool setVidMode(VideoMode& mode);

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
      This method is called to provide information about the FrameBuffer.
    */
    string about() const;

    /**
      This method is called after any drawing is done (per-frame).
    */
    void postFrameUpdate();

  private:
    bool loadFuncs();

    bool createTextures();

    static uInt32 power_of_two(uInt32 input)
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

/**
  A surface suitable for software rendering mode.

  @author  Stephen Anthony
  @version $Id: FrameBufferGL.hxx,v 1.54 2008-06-19 12:01:30 stephena Exp $
*/
class FBSurfaceGL : public FBSurface
{
  public:
    FBSurfaceGL(const FrameBufferGL& buffer, SDL_Surface* surface,
                  uInt32 w, uInt32 h, bool isBase);
    virtual ~FBSurfaceGL();

    void hLine(uInt32 x, uInt32 y, uInt32 x2, int color);
    void vLine(uInt32 x, uInt32 y, uInt32 y2, int color);
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color);
    void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, int color);
    void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, int color, Int32 h = 8);
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    void getPos(uInt32& x, uInt32& y) const;
    void setPos(uInt32 x, uInt32 y);
    void setWidth(uInt32 w);
    void setHeight(uInt32 h);
    void translateCoords(Int32& x, Int32& y) const;
    void update();

  private:
    void recalc();

  private:
    const FrameBufferGL& myFB;
    SDL_Surface* myTexture;
    uInt32 myWidth, myHeight;
    bool myIsBaseSurface;
    bool mySurfaceIsDirty;
    int myBaseOffset;
    int myPitch;

    uInt32 myXOrig, myYOrig;
    uInt32 myXOffset, myYOffset;
};

#endif  // DISPLAY_OPENGL

#endif
