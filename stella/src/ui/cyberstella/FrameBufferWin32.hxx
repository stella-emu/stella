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
// $Id: FrameBufferWin32.hxx,v 1.4 2003-11-24 23:56:10 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_WIN32_HXX
#define FRAMEBUFFER_WIN32_HXX

#include "FrameBuffer.hxx"
#include "bspf.hxx"

class Console;
class MediaSource;

/**
  This class implements a DirectX software framebuffer.
  Only fullscreen mode is supported for now.

  @author  Stephen Anthony
  @version $Id: FrameBufferWin32.hxx,v 1.4 2003-11-24 23:56:10 stephena Exp $
*/ 
class FrameBufferWin32 : public FrameBuffer
{
  public:
    /**
      Creates a new DirectDraw framebuffer
    */
    FrameBufferWin32();

    /**
      Destructor
    */
    virtual ~FrameBufferWin32();

    HWND hwnd() const { return myHWND; }
    bool windowActive() { return isWindowActive; }

    /**
      This routine should be called once the console is created to setup
      the video system for us to use.  Return false if any operation fails,
      otherwise return true.
    */
    virtual bool init();

    /**
      Set up the palette for a screen of depth = 8.

      @param shade  Scales the palette by the given amount
      @return       The result of the CreatePalette method
    */ 
    HRESULT setupPalette(float shade);

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
      This routine is called when the emulation has received
      a pause event.

      @param status  The received pause status
    */
    virtual void pauseEvent(bool status);
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
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() { return isFullscreen; }

    /**
      Answers the current zoom level of the window
    */
    uInt32 zoomLevel() { return theZoomLevel; }

  private:
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext);

    void cleanup();

    HWND myHWND;
    RECT myScreenRect;
    POINT myBlitOffset;

    // Stella objects
    SIZE myGameSize;
    BYTE myPalette[256];

    // DirectX
    IDirectDraw* myDD;
    IDirectDrawSurface* myPrimarySurface;
    IDirectDrawSurface* myBackSurface;
    DDSURFACEDESC myDDSurface;
    IDirectDrawPalette* myDDPalette;

    static LPCTSTR pszClassName;

    // Indicates the current zoom level of the window
    uInt32 theZoomLevel;

    // Indicates the maximum zoom of the window
    uInt32 theMaxZoomLevel;

    // Indicates whether the game is currently in fullscreen
    bool isFullscreen;

    // Indicates whether the window is currently active
    bool isWindowActive;
};

#endif
