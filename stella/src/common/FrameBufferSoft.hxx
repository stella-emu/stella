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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferSoft.hxx,v 1.11 2005-03-28 00:04:53 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_SOFT_HXX
#define FRAMEBUFFER_SOFT_HXX

#include <SDL.h>
#include <SDL_syswm.h>

class OSystem;

#include "bspf.hxx"
#include "FrameBuffer.hxx"

class RectList;


/**
  This class implements an SDL software framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferSoft.hxx,v 1.11 2005-03-28 00:04:53 stephena Exp $
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
      This routine is called to initialize software video mode.
      Return false if any operation fails, otherwise return true.
    */
    virtual bool initSubsystem();

    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen();

    /**
      Switches between the filtering options in software mode.
      Currently, none exist.
    */
    virtual void toggleFilter();

    /**
      This routine should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    virtual void drawMediaSource();

    /**
      This routine is called before any drawing is done (per-frame).
    */
    virtual void preFrameUpdate();

    /**
      This routine is called after any drawing is done (per-frame).
    */
    virtual void postFrameUpdate();

    /**
      This routine is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    virtual void scanline(uInt32 row, uInt8* data);

    /**
      This routine is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b)
      { return SDL_MapRGB(myScreen->format, r, g, b); }

    /**
      This routine is called to draw a horizontal line.

      @param x     The first x coordinate
      @param y     The y coordinate
      @param x2    The second x coordinate
      @param color The color of the line
    */
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color);

    /**
      This routine is called to draw a vertical line.

      @param x     The x coordinate
      @param y     The first y coordinate
      @param y2    The second y coordinate
      @param color The color of the line
    */
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color);

    /**
      This routine is called to draw a blended rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the box
      @param h      The height of the box
      @param color  FIXME
      @param level  FIXME
    */
    virtual void blendRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                           OverlayColor color, uInt32 level = 3);

    /**
      This routine is called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the area
    */
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          OverlayColor color);

    /**
      This routine is called to draw a framed rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the surrounding frame
    */
    virtual void frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                           OverlayColor color);

    /**
      This routine is called to draw the specified character.

      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    virtual void drawChar(uInt8 c, uInt32 x, uInt32 y, OverlayColor color);

    /**
      This routine is called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
      @param h      The height of the data image
    */
    virtual void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, OverlayColor color,
                            Int32 h = 8);

    /**
      This routine translates the given coordinates to their
      unzoomed/unscaled equivalents.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    inline virtual void translateCoords(Int32* x, Int32* y);

  private:
    // Used in the dirty update of the SDL surface
    RectList* myRectList;

    // GUI palette
    Uint32 myGUIPalette[5];
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
