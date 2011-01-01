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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FRAMEBUFFER_GP2X_HXX
#define FRAMEBUFFER_GP2X_HXX

#include <SDL.h>

class OSystem;
class GUI::Font;

#include "bspf.hxx"
#include "FrameBuffer.hxx"


/**
  This class implements an SDL hardware framebuffer for the GP2X device.

  @author  Stephen Anthony
  @version $Id$
*/
class FrameBufferGP2X : public FrameBuffer
{
  public:
    /**
      Creates a new software framebuffer
    */
    FrameBufferGP2X(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferGP2X();

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize software video mode.
      Return false if any operation fails, otherwise return true.
    */
    virtual bool initSubsystem(VideoMode mode);

    /**
      This method is called to query the type of the FrameBuffer.
    */
    virtual BufferType type() const { return kSoftBuffer; }

    /**
      This method is called to provide information about the FrameBuffer.
    */
    virtual string about() const;

    /**
      This method is called to change to the given videomode type.

      @param mode  The video mode to use for rendering the mediasource
    */
    bool setVidMode(VideoMode mode);

    /**
      Switches between the filtering options in software mode.
      Currently, none exist.
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
    virtual void scanline(uInt32 row, uInt8* data) const;

    /**
      This method is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
      { return SDL_MapRGB(myScreen->format, r, g, b); }

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
      This method should be called to draw an SDL surface.

      @param surface The data to draw
      @param x       The x coordinate
      @param y       The y coordinate
    */
    void drawSurface(const GUI::Surface* surface, Int32 x, Int32 y);

    /**
      This method should be called to convert and copy a given row of RGB
      data into an SDL surface.

      @param surface  The data to draw
      @param row      The row of the surface the data should be placed in
      @param data     The data in uInt8 R/G/B format
      @param rowbytes The number of bytes in row of 'data'
    */
    void bytesToSurface(GUI::Surface* surface, int row,
                        uInt8* data, int rowbytes) const;

    /**
      This method translates the given coordinates to their
      unzoomed/unscaled equivalents.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    virtual void translateCoords(Int32& x, Int32& y) const;

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

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    virtual void showCursor(bool show);

  private:
    // Origin at which to access the FrameBuffer
    // This point is treated as (0, 0) wrt the TIA image
    uInt16* myBasePtr;

    // Indicates that the buffer is dirty, and should be redrawn/flipped
    bool myDirtyFlag;

    // Pitch (in bytes) of the current screen
    int myPitch;

    // surface pixel format
    SDL_PixelFormat* myFormat;

    // the height of the display for TV output
    int myTvHeight;
};

#endif
