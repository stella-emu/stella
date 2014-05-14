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

class OSystem;
class FBSurfaceSDL2;

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
  friend class FBSurfaceSDL2;

  public:
    /**
      Creates a new SDL2 framebuffer
    */
    FrameBufferSDL2(OSystem& osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferSDL2();

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
      This method is called to retrieve the R/G/B data from the given pixel.

      @param pixel  The pixel containing R/G/B data
      @param r      The red component of the color
      @param g      The green component of the color
      @param b      The blue component of the color
    */
    inline void getRGB(Uint32 pixel, Uint8* r, Uint8* g, Uint8* b) const
      { SDL_GetRGB(pixel, myPixelFormat, r, g, b); }

    /**
      This method is called to map a given R/G/B triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    inline Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
      { return SDL_MapRGB(myPixelFormat, r, g, b); }

    /**
      This method is called to query the buffering type of the FrameBuffer.
    */
    bool isDoubleBuffered() const { return myDblBufferedFlag; }

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
    */
    void queryHardware(uInt32& w, uInt32& h, VariantList& renderers);

    /**
      This method is called to change to the given video mode.

      @param title The title for the created window
      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    bool setVideoMode(const string& title, const VideoMode& mode);

    /**
      Enables/disables fullscreen mode.
    */
    void enableFullscreen(bool enable);

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    void invalidate();

    /**
      This method is called to create a surface with the given attributes.

      @param w     The requested width of the new surface.
      @param h     The requested height of the new surface.
      @param data  If non-null, use the given data values as a static surface
    */
    FBSurface* createSurface(uInt32 w, uInt32 h, const uInt32* data) const;

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    void grabMouse(bool grab);

    /**
      Set the icon for the main SDL window.
    */
    void setWindowIcon() { }  // Not currently needed on any supported systems

    /**
      This method is called to provide information about the FrameBuffer.
    */
    string about() const;

    /**
      This method is called after any drawing is done (per-frame).
    */
    void postFrameUpdate();

  private:
    // The SDL video buffer
    SDL_Window* myWindow;
    SDL_Renderer* myRenderer;

    // SDL initialization flags
    // This is set by the base FrameBuffer class, and read by the derived classes
    // If a FrameBuffer is successfully created, the derived classes must modify
    // it to point to the actual flags used by the SDL_Surface
    uInt32 myWindowFlags;

    // Used by mapRGB (when palettes are created)
    SDL_PixelFormat* myPixelFormat;

    // The depth of the texture buffer
    uInt32 myDepth;

    // Indicates that the texture has been modified, and should be redrawn
    bool myDirtyFlag;

    // Indicates whether the backend is using double buffering
    bool myDblBufferedFlag;
};

#endif
