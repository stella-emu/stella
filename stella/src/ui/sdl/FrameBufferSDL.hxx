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
// $Id: FrameBufferSDL.hxx,v 1.8 2003-12-03 18:11:25 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_SDL_HXX
#define FRAMEBUFFER_SDL_HXX

#include <SDL.h>
#include <SDL_syswm.h>

#include "FrameBuffer.hxx"
#include "bspf.hxx"

/**
  This class is a base class for the SDL framebuffer and is derived from
  the core FrameBuffer class.

  It defines all common code shared between the software
  and OpenGL video modes, as well as required methods defined in
  the core FrameBuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferSDL.hxx,v 1.8 2003-12-03 18:11:25 stephena Exp $
*/
class FrameBufferSDL : public FrameBuffer
{
  public:
    /**
      Creates a new SDL framebuffer
    */
    FrameBufferSDL();

    /**
      Destructor
    */
    virtual ~FrameBufferSDL();


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
      Calculate the maximum window size that the current screen can hold.
      Only works in X11 for now.  If not running under X11, always return 4.
    */
    uInt32 maxWindowSizeForScreen();

    /**
      Set the title and icon for the main SDL window.
    */
    void setWindowAttributes();

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine is called when the emulation has been paused.

      @param status  Toggle pause based on status
    */
    void pauseEvent(bool status);

    //////////////////////////////////////////////////////////////////////
    // The following methods must be defined in child classes
    //////////////////////////////////////////////////////////////////////
    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen() = 0;

    /**
      Set up the palette for a screen of any depth > 8.
      Scales the palette by 'shade'.
    */
    virtual void setupPalette(float shade) = 0;

  protected:
    // The SDL video buffer
    SDL_Surface* myScreen;

    // SDL initialization flags
    uInt32 mySDLFlags;

    // SDL palette
    Uint32 myPalette[256];

    // Used to get window-manager specifics
    SDL_SysWMinfo myWMInfo;

    // Indicates if we are running under X11
    bool x11Available;

    // Indicates the current zoom level of the SDL screen
    uInt32 theZoomLevel;

    // Indicates the maximum zoom of the SDL screen
    uInt32 theMaxZoomLevel;

    // Indicates if the mouse should be grabbed
    bool theGrabMouseIndicator;

    // Indicates if the mouse cursor should be hidden
    bool theHideCursorIndicator;

    // The aspect ratio of the window
    float theAspectRatio;

    // Indicates whether the game is currently in fullscreen
    bool isFullscreen;
};

#endif
