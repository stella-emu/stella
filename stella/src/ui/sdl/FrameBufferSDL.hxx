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
// $Id: FrameBufferSDL.hxx,v 1.1 2003-10-26 19:40:39 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_SDL_HXX
#define FRAMEBUFFER_SDL_HXX

#include <SDL.h>
#include <SDL_syswm.h>

#include "FrameBuffer.hxx"
#include "bspf.hxx"

class Console;
class MediaSource;
class RectList;

class FrameBufferSDL : public FrameBuffer
{
  public:
    /**
      Creates a new SDL software framebuffer
    */
    FrameBufferSDL();

    /**
      Destructor
    */
    virtual ~FrameBufferSDL();

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
      @param fg  The color of the bounding sides
      @param bg  The color of the background
    */
    virtual void drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt8 fg, uInt8 bg);

    /**
      This routine should be called to draw text at the specified coordinates.

      @param x        The x coordinate
      @param y        The y coordinate
      @param message  The message text
      @param fg       The color of the text
    */
    virtual void drawText(uInt32 x, uInt32 y, const string& message, uInt8 fg);

    /**
      This routine should be called to draw character 'c' at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param c   The character to draw
      @param fg  The color of the character
    */
    virtual void drawChar(uInt32 x, uInt32 y, uInt32 c, uInt8 fg);

    /**
      This routine is called before any drawing is done (per-frame).
    */
    virtual void preFrameUpdate();

    /**
      This routine is called after any drawing is done (per-frame).
    */
    virtual void postFrameUpdate();

    /**
      This routine is called when the emulation has been paused.

      @param status  Toggle pause based on status
    */
    virtual void pause(bool status);

    /**
      Toggles between fullscreen and window mode.  Grabmouse and hidecursor
      activated when in fullscreen mode.
    */
    void toggleFullscreen();

    /**
      This routine is called when the user wants to resize the window.
      A '1' argument indicates that the window should increase in size, while '-1'
      indicates that the windows should decrease in size.  A '0' indicates that
      the window should be sized according to the current properties.
      Can't resize in fullscreen mode.  Will only resize up to the maximum size
      of the screen.
    */
    void resize(int mode);

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    void showCursor(bool show);

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    void grabMouse(bool grab);

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() { return isFullscreen; }

    /**
      Answers the current zoom level of the SDL 
    */
    uInt32 zoomLevel() { return theZoomLevel; }

    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    bool createScreen();

   /**
     Centers the game window onscreen.  Only works in X11 for now.
   */
    void centerScreen();

    /**
      Calculate the maximum window size that the current screen can hold.
      Only works in X11 for now.  If not running under X11, always return 4.
    */
    uInt32 maxWindowSizeForScreen();

    /**
      Set up the palette for a screen of any depth > 8.
      Scales the palette by 'shade'.
    */
    void setupPalette(float shade);

  private:
    // The SDL video buffer
    SDL_Surface* myScreen;

    // Used in the dirty update of the SDL surface
    RectList* myRectList;

    // SDL initialization flags
    uInt32 mySDLFlags;

    // SDL palette
    Uint32 palette[256];

    // Used to get window-manager specifics
    SDL_SysWMinfo myWMInfo;

    // Indicates if we are running under X11
    bool x11Available;

    // Indicates the current zoom level of the SDL screen
    uInt32 theZoomLevel;

    // Indicates the maximum zoom of the SDL screen
    uInt32 theMaxZoomLevel;

    // Indicates whether the window is currently centered
    bool isCentered;

    // Indicates if the mouse should be grabbed
    bool theGrabMouseIndicator;

    // Indicates if the mouse cursor should be hidden
    bool theHideCursorIndicator;

    // Indicates whether the game is currently in fullscreen
    bool isFullscreen;
};

#endif
