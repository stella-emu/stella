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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FRAMEBUFFER_GL_HXX
#define FRAMEBUFFER_GL_HXX

#ifdef DISPLAY_OPENGL

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

class OSystem;
class FBSurfaceGL;

#include "bspf.hxx"
#include "FrameBuffer.hxx"

/**
  This class implements an SDL OpenGL framebuffer.

  @author  Stephen Anthony
  @version $Id$
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

    /**
      Return version of the OpenGL library found by the OSystem
      (0 indicates that the libary was not loaded successfully).
      This will not return a valid version number until setVidMode()
      has been called at least once.
    */
    static float glVersion() { return myGLVersion; }

    //////////////////////////////////////////////////////////////////////
    // The following are derived from public methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

    /**
      This method is called to retrieve the R/G/B data from the given pixel.

      @param pixel  The pixel containing R/G/B data
      @param r      The red component of the color
      @param g      The green component of the color
      @param b      The blue component of the color
    */
    void getRGB(Uint32 pixel, Uint8* r, Uint8* g, Uint8* b) const
      { SDL_GetRGB(pixel, (SDL_PixelFormat*)&myPixelFormat, r, g, b); }

    /**
      This method is called to map a given R/G/B triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
      { return SDL_MapRGB((SDL_PixelFormat*)&myPixelFormat, r, g, b); }

    /**
      This method is called to query the type of the FrameBuffer.
    */
    BufferType type() const { return kGLBuffer; }

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
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one
    */
    FBSurface* createSurface(int w, int h, bool useBase = false) const;

    /**
      This method should be called anytime the TIA needs to be redrawn
      to the screen (full indicating that a full redraw is required).
    */
    void drawTIA(bool full);

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

    /**
      Enable/disable texture effect.
    */
    void enableTexture(bool enable);

  private:
    // The lower-most base surface (will always be a TIA surface,
    // since Dialog surfaces are allocated by the Dialog class directly).
    FBSurfaceGL* myTiaSurface;

    // Used by mapRGB (when palettes are created)
    SDL_PixelFormat myPixelFormat;

    // The depth of the texture buffer
    uInt32 myDepth;

    // The size of color components for OpenGL
    uInt32 myRGB[4];

    // The name of the texture filtering to use
    string myFilterParamName;

    // Optional GL extensions that may increase performance
    bool myHaveTexRectEXT;

    // Indicates that the texture has been modified, and should be redrawn
    bool myDirtyFlag;

    // Indicates whether or not color bleed filter is enabled
    bool myUseBleed;

    // Indicates the quality of the color bleed filter to use
    int myBleedQuality;

    // Indicates whether or not color texture filter is enabled
    bool myUseTexture;

    // Indicates whetehr or not color texture filter is staggered
    bool myTextureStag;

    // Indicates whether or not the noise filter is enabled
    bool myUseNoise;

    // Indicates the quality of the noise filter to use
    int myNoiseQuality;

    // Indicates whether or not the phosphor filter is enabled
    bool myUseGLPhosphor;

    // Indicates the OpenGL version found (0 indicates none)
    static float myGLVersion;

    // Indicates if the OpenGL library has been properly loaded
    static bool myLibraryLoaded;
};

/**
  A surface suitable for OpenGL rendering mode.

  @author  Stephen Anthony
  @version $Id$
*/
class FBSurfaceGL : public FBSurface
{
  friend class FrameBufferGL;

  public:
    FBSurfaceGL(FrameBufferGL& buffer,
                uInt32 baseWidth, uInt32 baseHeight,
                uInt32 scaleWidth, uInt32 scaleHeight,
                bool allowFiltering = false);
    virtual ~FBSurfaceGL();

    void hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color);
    void vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color);
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color);
    void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, uInt32 color);
    void drawBitmap(uInt32* bitmap, uInt32 x, uInt32 y, uInt32 color, uInt32 h = 8);
    void drawPixels(uInt32* data, uInt32 x, uInt32 y, uInt32 numpixels);
    void drawSurface(const FBSurface* surface, uInt32 x, uInt32 y);
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    void getPos(uInt32& x, uInt32& y) const;
    void setPos(uInt32 x, uInt32 y);
    uInt32 getWidth()  const { return myWidth;  }
    uInt32 getHeight() const { return myHeight; }
    void setWidth(uInt32 w);
    void setHeight(uInt32 h);
    void translateCoords(Int32& x, Int32& y) const;
    void update();
    void free();
    void reload();

  private:
    void setFilter(const string& name);

    /**
      This method generates an OpenGL shader program from a fragment shader.

      @param fragment The filename of the fragment shader (not including location)

      @return The generated shader program
    */
    enum ShaderType {
      SHADER_BLEED, SHADER_TEX, SHADER_NOISE, SHADER_PHOS, SHADER_TEXNOISE
    };
    GLuint genShader(ShaderType type);

    /**
      This method performs the final steps of rendering a single texture filter:
      passing the previously rendered screen to the given program and drawing
      to the screen. It does not include setting the program through
      p_glUseProgram() because this needs to be done before the custom program
      variables are set.

      @param program     The program to use to render the filter
      @param firstRender True if this is the first render for this frame, false if not
    */
    void renderTexture(GLuint program, bool firstRender);

    /**
      This method performs the final steps of rendering a two-texture filter:
      passing the previously rendered screen to the given program and drawing
      the previous texture and mask texture to the screen. It does not include
      setting the program through p_glUseProgram() because this needs to be
      done before the mask texture and custom program variables are set.

      @param program     The program to use to render the filter
      @param firstRender True if this is the first render for this frame, false if not
    */
    void renderTwoTexture(GLuint program, bool firstRender);

    /**
      This method performs the final steps of rendering a three-texture filter:
      passing the previously rendered screen to the given program and drawing
      the previous texture and two mask textures to the screen. It does not include
      setting the program through p_glUseProgram() because this needs to be
      done before the mask texture and custom program variables are set.

      @param program     The program to use to render the filter
      @param firstRender True if this is the first render for this frame, false if not
    */
    void renderThreeTexture(GLuint program, bool firstRender);

    inline void* pixels() const { return myTexture->pixels; }
    inline uInt32 pitch() const { return myPitch;           }

    static uInt32 power_of_two(uInt32 input)
    {
      uInt32 value = 1;
      while( value < input )
        value <<= 1;
      return value;
    }

  private:
    FrameBufferGL& myFB;
    SDL_Surface* myTexture;

    GLuint  myTexID;
    GLenum  myTexTarget;
    GLsizei myTexWidth;
    GLsizei myTexHeight;
    GLfloat myTexCoord[4];

    // The filter texture is what is used to hold data from screen after one
    // filter has been used. Needed since more than one filter is being used.
    // The size and texture coordinates are also used for the other filter
    // textures: mySubMaskTexID and myNoiseTexID
    GLuint myFilterTexID;
    GLsizei myFilterTexWidth;
    GLsizei myFilterTexHeight;
    GLfloat myFilterTexCoord[4];

    // The subpixel texture used for the texture filter
    GLuint mySubMaskTexID;
    // The noise textures used for the noise filter
    GLuint* myNoiseMaskTexID;
    // The past texture used for the phosphor filter
    GLuint myPhosphorTexID;

    // Surface for the subpixel texture filter mask
    SDL_Surface* mySubpixelTexture;
    // Surfaces for noise filter mask (array of pointers)
    SDL_Surface** myNoiseTexture;

    uInt32 myXOrig, myYOrig, myWidth, myHeight;
    bool mySurfaceIsDirty;
    uInt32 myPitch;

    // OpenGL shader programs
    GLuint myBleedProgram;         // Shader for color bleed filter
    GLuint myTextureProgram;       // Shader for color texture filter
    GLuint myNoiseProgram;         // Shader for noise filter
    GLuint myPhosphorProgram;      // Shader for the phosphor filter
    GLuint myTextureNoiseProgram;  // Shader for both color texture and noise filters

    // Used to save the number of noise textures to use at game launch
    int myNoiseNum;

    // Specifies whether the TV filters can be applied to this surface
    bool myTvFiltersEnabled;
};

#endif  // DISPLAY_OPENGL

#endif
