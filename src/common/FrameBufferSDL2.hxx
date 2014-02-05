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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FRAMEBUFFER_SDL2_HXX
#define FRAMEBUFFER_SDL2_HXX

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

class OSystem;
class FBSurfaceUI;
class FBSurfaceTIA;
class TIA;

#include "bspf.hxx"
#include "FrameBuffer.hxx"

/**
  This class implements a standard SDL2 2D, hardware accelerated framebuffer.
  Behind the scenes, it may be using Direct3D, OpenGL(ES), etc.

  @author  Stephen Anthony
  @version $Id$
*/
class FrameBufferSDL2 : public FrameBuffer
{
  friend class FBSurfaceUI;
  friend class FBSurfaceTIA;

  public:
    /**
      Creates a new OpenGL framebuffer
    */
    FrameBufferSDL2(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferSDL2();

    /**
      Check if OpenGL is available on this system, and then opens it.
      If any errors occur, we shouldn't attempt to instantiate a
      FrameBufferSDL2 object.

      @param library  The filename of the OpenGL library
    */
    static bool loadLibrary(const string& library);

    //////////////////////////////////////////////////////////////////////
    // The following are derived from public methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      Toggles the use of grabmouse (only has effect in emulation mode).
      The method changes the 'grabmouse' setting and saves it.
    */
    void toggleGrabMouse();

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    void showCursor(bool show);

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() const;

    /**
      Set the title for the main window.
    */
    void setWindowTitle(const string& title);

    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

    /**
      Enable/disable NTSC filtering effects.
    */
    void enableNTSC(bool enable);
    bool ntscEnabled() const { return myFilterType & 0x10; }

    /**
      Set up the TIA/emulation palette for a screen of any depth > 8.

      @param palette  The array of colors
    */
    void setTIAPalette(const uInt32* palette);

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
    BufferType type() const { return kDoubleBuffer; }

    /**
      This method is called to query the TV effects in use by the FrameBuffer.
    */
    string effectsInfo() const;

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
      This method is called to query and initialize the video hardware
      for desktop and fullscreen resolution information.

      @return  False on any errors, else true
    */
    bool queryHardware(uInt32& w, uInt32& h, ResolutionList& res);

    /**
      This method is called to initialize the video subsystem
      with the given video mode.  Normally, it will also call setVidMode().

      @param mode  The video mode to use
      @param full  Whether this is a fullscreen or windowed mode

      @return  False on any errors, else true
    */
    bool initSubsystem(VideoMode& mode, bool full);

    /**
      This method is called to change to the given video mode.  If the mode
      is successfully changed, 'mode' holds the actual dimensions used.

      @param mode  The video mode to use

      @return  False on any errors (in which case 'mode' is invalid), else true
    */
    bool setVidMode(VideoMode& mode);

    /**
      Sets a hint that the underlying renderer may use; it is also free
      to ignore it completely.

      @param hint     The hint to set
      @param enabled  Whether the hint should be turned on or off
    */
    void setHint(FBHint hint, bool enabled);

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    void invalidate();

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one
    */
    FBSurface* createSurface(int w, int h, bool useBase = false) const;

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    void grabMouse(bool grab);

    /**
      Set the icon for the main SDL window.
    */
    void setWindowIcon();

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

    /**
      Change scanline intensity and interpolation.

      @param relative  If non-zero, change current intensity by
                       'relative' amount, otherwise set to 'absolute'
      @return  New current intensity
    */
    uInt32 enableScanlines(int relative, int absolute = 50);
    void enableScanlineInterpolation(bool enable);

  private:
    enum GLFunctionality {
      kGL_BASIC, kGL_VBO
    };
    bool loadFuncs(GLFunctionality functionality);

    // Enumeration created such that phosphor off/on is in LSB,
    // and Blargg off/on is in MSB
    enum FilterType {
      kNormal         = 0x00,
      kPhosphor       = 0x01,
      kBlarggNormal   = 0x10,
      kBlarggPhosphor = 0x11
    };
    FilterType myFilterType;

    static uInt32 power_of_two(uInt32 input)
    {
      uInt32 value = 1;
      while( value < input )
        value <<= 1;
      return value;
    }

  private:
    // The SDL video buffer
    SDL_Surface* myScreen;

    // SDL initialization flags
    // This is set by the base FrameBuffer class, and read by the derived classes
    // If a FrameBuffer is successfully created, the derived classes must modify
    // it to point to the actual flags used by the SDL_Surface
    uInt32 mySDLFlags;

    // The lower-most base surface (will always be a TIA surface,
    // since Dialog surfaces are allocated by the Dialog class directly).
    FBSurfaceTIA* myTiaSurface;

    // Used by mapRGB (when palettes are created)
    SDL_PixelFormat myPixelFormat;

    // The depth of the texture buffer
    uInt32 myDepth;

    // The size of color components for OpenGL
    Int32 myRGB[4];

    // Indicates that the texture has been modified, and should be redrawn
    bool myDirtyFlag;

    // Indicates if the OpenGL library has been properly loaded
    static bool myLibraryLoaded;

    // Indicates whether Vertex Buffer Objects (VBO) are available
    static bool myVBOAvailable;

    // Structure containing dynamically-loaded OpenGL function pointers
    #define OGL_DECLARE(NAME,RET,FUNC,PARAMS) RET (APIENTRY* NAME) PARAMS
    typedef struct {
      OGL_DECLARE(Clear,void,glClear,(GLbitfield));
      OGL_DECLARE(Enable,void,glEnable,(GLenum));
      OGL_DECLARE(Disable,void,glDisable,(GLenum));
      OGL_DECLARE(PushAttrib,void,glPushAttrib,(GLbitfield));
      OGL_DECLARE(GetString,const GLubyte*,glGetString,(GLenum));
      OGL_DECLARE(Hint,void,glHint,(GLenum, GLenum));
      OGL_DECLARE(ShadeModel,void,glShadeModel,(GLenum));
      OGL_DECLARE(MatrixMode,void,glMatrixMode,(GLenum));
      OGL_DECLARE(Ortho,void,glOrtho,(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble));
      OGL_DECLARE(Viewport,void,glViewport,(GLint, GLint, GLsizei, GLsizei));
      OGL_DECLARE(LoadIdentity,void,glLoadIdentity,(void));
      OGL_DECLARE(Translatef,void,glTranslatef,(GLfloat,GLfloat,GLfloat));
      OGL_DECLARE(EnableClientState,void,glEnableClientState,(GLenum));
      OGL_DECLARE(DisableClientState,void,glDisableClientState,(GLenum));
      OGL_DECLARE(VertexPointer,void,glVertexPointer,(GLint,GLenum,GLsizei,const GLvoid*));
      OGL_DECLARE(TexCoordPointer,void,glTexCoordPointer,(GLint,GLenum,GLsizei,const GLvoid*));
      OGL_DECLARE(DrawArrays,void,glDrawArrays,(GLenum,GLint,GLsizei));
      OGL_DECLARE(ReadPixels,void,glReadPixels,(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
      OGL_DECLARE(PixelStorei,void,glPixelStorei,(GLenum, GLint));
      OGL_DECLARE(TexEnvf,void,glTexEnvf,(GLenum, GLenum, GLfloat));
      OGL_DECLARE(GenTextures,void,glGenTextures,(GLsizei, GLuint*));
      OGL_DECLARE(DeleteTextures,void,glDeleteTextures,(GLsizei, const GLuint*));
      OGL_DECLARE(ActiveTexture,void,glActiveTexture,(GLenum));
      OGL_DECLARE(BindTexture,void,glBindTexture,(GLenum, GLuint));
      OGL_DECLARE(TexImage2D,void,glTexImage2D,(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
      OGL_DECLARE(TexSubImage2D,void,glTexSubImage2D,(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*));
      OGL_DECLARE(TexParameteri,void,glTexParameteri,(GLenum, GLenum, GLint));
      OGL_DECLARE(GetError,GLenum,glGetError,(void));
      OGL_DECLARE(Color4f,void,glColor4f,(GLfloat,GLfloat,GLfloat,GLfloat));
      OGL_DECLARE(BlendFunc,void,glBlendFunc,(GLenum,GLenum));
      OGL_DECLARE(GenBuffers,void,glGenBuffers,(GLsizei,GLuint*));
      OGL_DECLARE(BindBuffer,void,glBindBuffer,(GLenum,GLuint));
      OGL_DECLARE(BufferData,void,glBufferData,(GLenum,GLsizei,const void*,GLenum));
      OGL_DECLARE(DeleteBuffers,void,glDeleteBuffers,(GLsizei, const GLuint*));
    } GLpointers;
    GLpointers p_gl;
};

#endif
