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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferSoft.hxx,v 1.1 2003-11-09 23:53:20 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_SOFT_HXX
#define FRAMEBUFFER_SOFT_HXX

#include <SDL.h>
#include <SDL_syswm.h>

#include "FrameBuffer.hxx"
#include "FrameBufferSDL.hxx"
#include "bspf.hxx"

class Console;
class MediaSource;
class RectList;


/**
  This class implements an SDL software framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferSoft.hxx,v 1.1 2003-11-09 23:53:20 stephena Exp $
*/
class FrameBufferSoft : public FrameBufferSDL
{
  public:
    /**
      Creates a new SDL software framebuffer
    */
    FrameBufferSoft();

    /**
      Destructor
    */
    virtual ~FrameBufferSoft();

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBufferSDL.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen();

    /**
      Set up the palette for a screen of any depth > 8.
      Scales the palette by 'shade'.
    */
    virtual void setupPalette(float shade);

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine should be called once the console is created to setup
      the video system for us to use.  Return false if any operation fails,
      otherwise return true.
    */
    virtual bool init();

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

  private:
    // Used in the dirty update of the SDL surface
    RectList* myRectList;
};

/**

 */
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
