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
    */
    void queryHardware(uInt32& w, uInt32& h, VariantList& renderers);

    /**
      This method is called to change to the given video mode.  If the mode
      is successfully changed, 'mode' holds the actual dimensions used.

      @param title The title for the created window
      @param mode  The video mode to use
      @param full  Whether this is a fullscreen or windowed mode

      @return  False on any errors, else true
    */
    bool setVideoMode(const string& title, VideoMode& mode, bool full);

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    void invalidate();

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w  The requested width of the new surface.
      @param h  The requested height of the new surface.
    */
    FBSurface* createSurface(int w, int h) const;

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    void grabMouse(bool grab);

    /**
      Set the icon for the main SDL window.
    */
    void setWindowIcon() { }  // Not currently needed on any supported systems

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
    // Enumeration created such that phosphor off/on is in LSB,
    // and Blargg off/on is in MSB
    enum FilterType {
      kNormal         = 0x00,
      kPhosphor       = 0x01,
      kBlarggNormal   = 0x10,
      kBlarggPhosphor = 0x11
    };
    FilterType myFilterType;

  private:
    // The SDL video buffer
    SDL_Window* myWindow;
    SDL_Renderer* myRenderer;

    // SDL initialization flags
    // This is set by the base FrameBuffer class, and read by the derived classes
    // If a FrameBuffer is successfully created, the derived classes must modify
    // it to point to the actual flags used by the SDL_Surface
    uInt32 myWindowFlags;

    // The lower-most base surface (will always be a TIA surface,
    // since Dialog surfaces are allocated by the Dialog class directly).
    FBSurfaceTIA* myTiaSurface;

    // Used by mapRGB (when palettes are created)
    SDL_PixelFormat* myPixelFormat;

    // The depth of the texture buffer
    uInt32 myDepth;

    // Indicates that the texture has been modified, and should be redrawn
    bool myDirtyFlag;
};

#endif
