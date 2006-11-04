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
// $Id: FrameBufferSoft.hxx,v 1.36 2006-11-04 19:38:24 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_SOFT_HXX
#define FRAMEBUFFER_SOFT_HXX

#include <SDL.h>
#include <SDL_syswm.h>

class OSystem;
class GUI::Font;
class RectList;

#include "bspf.hxx"
#include "GuiUtils.hxx"
#include "FrameBuffer.hxx"


/**
  This class implements an SDL software framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferSoft.hxx,v 1.36 2006-11-04 19:38:24 stephena Exp $
*/
class FrameBufferSoft : public FrameBuffer
{
  public:
    /**
      Creates a new software framebuffer
    */
    FrameBufferSoft(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferSoft();

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize software video mode.
      Return false if any operation fails, otherwise return true.
    */
    virtual bool initSubsystem();

    /**
      This method is called to query the type of the FrameBuffer.
    */
    virtual BufferType type() { return kSoftBuffer; }

    /**
      This method is called to set the aspect ratio of the screen.
    */
    virtual void setAspectRatio();

    /**
      This method is called to change to the given scaler type.

      @param scaler  The scaler to use for rendering the mediasource
    */
    virtual void setScaler(Scaler scaler);

    /**
      This method is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen();

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
    virtual void scanline(uInt32 row, uInt8* data);

    /**
      This method is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b)
      { return SDL_MapRGB(myScreen->format, r, g, b); }

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
      Enable/disable phosphor effect.
    */
    virtual void enablePhosphor(bool enable, int blend);

    /**
      Completely erase contents of the screen.
    */
    virtual void cls();

  private:
    int myZoomLevel;
    int myPitch;

    enum RenderType {
      kSoftZoom,
      kPhosphor_16,
      kPhosphor_24,
      kPhosphor_32
    };
    RenderType myRenderType;

    // Used in the dirty update of the SDL surface
    RectList* myRectList;

    // Used in the dirty update of the overlay surface
    RectList* myOverlayRectList;
};

class RectList
{
  public:
    RectList(Uint32 size = 512);
    ~RectList();

    void add(SDL_Rect* rect);

    SDL_Rect* rects();
    Uint32 numRects();
    void start();

  private:
    Uint32 currentSize, currentRect;

    SDL_Rect* rectArray;
};

#endif
