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
// $Id: FrameBufferSoft.hxx,v 1.6 2005-02-21 20:41:19 stephena Exp $
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
  @version $Id: FrameBufferSoft.hxx,v 1.6 2005-02-21 20:41:19 stephena Exp $
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
      This routine is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b)
      { return SDL_MapRGB(myScreen->format, r, g, b); }

    /**
      This routine should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    virtual void drawMediaSource();

    /**
      This routine should be called to draw a rectangular box with sides
      at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param w   The width of the box
      @param h   The height of the box
    */
    virtual void drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h);

    /**
      This routine should be called to draw text at the specified coordinates.

      @param x        The x coordinate
      @param y        The y coordinate
      @param message  The message text
    */
    virtual void drawText(uInt32 x, uInt32 y, const string& message);

    /**
      This routine should be called to draw character 'c' at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param c   The character to draw
    */
    virtual void drawChar(uInt32 x, uInt32 y, uInt32 c);

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

  private:
    // Used in the dirty update of the SDL surface
    RectList* myRectList;
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
