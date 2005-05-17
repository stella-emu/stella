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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBuffer.hxx,v 1.33 2005-05-17 18:42:22 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_HXX
#define FRAMEBUFFER_HXX

#include <SDL.h>
#include <SDL_syswm.h>

#include "bspf.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "StellaFont.hxx"
#include "StellaEvent.hxx"
#include "GuiUtils.hxx"

class StellaFont;
class Console;
class OSystem;

/**
  This class encapsulates the MediaSource and is the basis for the video
  display in Stella.  All graphics ports should derive from this class for
  platform-specific video stuff.

  All GUI elements (ala ScummVM) are drawn here as well.

  @author  Stephen Anthony
  @version $Id: FrameBuffer.hxx,v 1.33 2005-05-17 18:42:22 stephena Exp $
*/
class FrameBuffer
{
  public:
    /**
      Creates a new Frame Buffer
    */
    FrameBuffer(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBuffer();

    /**
      Get the font object of the framebuffer

      @return The font reference
    */
    StellaFont& font() const { return *myFont; }

    /**
      (Re)initializes the framebuffer display.  This must be called before any
      calls are made to derived methods.

      @param title   The title of the window
      @param width   The width of the framebuffer
      @param height  The height of the framebuffer
      @param aspect  Whether to use the aspect ratio setting
    */
    void initialize(const string& title, uInt32 width, uInt32 height,
                    bool aspect = true);

    /**
      Updates the display, which depending on the current mode could mean
      drawing the mediasource, any pending menus, etc.
    */
    void update();

    /**
      Shows a message onscreen.

      @param message  The message to be shown
    */
    void showMessage(const string& message);

    /**
      Returns the current width of the framebuffer *before* any scaling.

      @return  The current unscaled width
    */
    inline const uInt32 baseWidth()  { return myBaseDim.w; }

    /**
      Returns the current height of the framebuffer *before* any scaling.

      @return  The current unscaled height
    */
    inline const uInt32 baseHeight() { return myBaseDim.h; }

    /**
      Returns the current width of the framebuffer image.
      Note that this will take into account the current scaling (if any).

      @return  The current width
    */
    inline const uInt32 imageWidth()  { return myImageDim.w; }

    /**
      Returns the current height of the framebuffer image.
      Note that this will take into account the current scaling (if any).

      @return  The current height
    */
    inline const uInt32 imageHeight() { return myImageDim.h; }

     /**
      Sets the pause status.  While pause is selected, the
      MediaSource will not be updated.

      @param status  Toggle pause based on status
    */
    void pause(bool status);

    /**
      Indicates that a redraw should be done, since the window contents
      are dirty.

      @param now  Determine if the refresh should be done right away or in
                  the next frame
    */
    void refresh(bool now = false)
    {
//      cerr << "refresh() " << myNumRedraws++ << endl;
      theRedrawEntireFrameIndicator = true;
      myMenuRedraws = 2;
      if(now) update();
    }

    /**
      Toggles between fullscreen and window mode.
      Grabmouse activated when in fullscreen mode.  
    */
    void toggleFullscreen();

    /**
      Enables/disables fullscreen mode.
      Grabmouse activated when in fullscreen mode.  

      @param enable  Set the fullscreen mode to this value
    */
    void setFullscreen(bool enable);

    /**
      This routine is called when the user wants to resize the window.
      Size = 'PreviousSize' means window should decrease in size
      Size = 'NextSize' means window should increase in size
      Size = 'GivenSize' means window should be resized to quantity given in 'zoom'

      @param size  Described above
      @param zoom  The zoom level to use if size is set to 'sGiven'
    */
    void resize(Size size, Int8 zoom = 0);

    /**
      Sets the state of the cursor (hidden or grabbed) based on the
      current mode.
    */
    void setCursorState();

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
    bool fullScreen();

    /**
      Answers the current zoom level of the framebuffer image in the X axis.
    */
    inline const float zoomLevelX() { return theZoomLevel * theAspectRatio; }

    /**
      Answers the current zoom level of the framebuffer image in the X axis.
    */
    inline const float zoomLevelY() { return (float) theZoomLevel; }

    /**
      Calculate the maximum window size that the current screen can hold.
      Only works in X11/Win32 for now, otherwise always return 4.
    */
    uInt32 maxWindowSizeForScreen();

    /**
      Set the title for the main SDL window.
    */
    void setWindowTitle(const string& title);

    /**
      Set up the palette for a screen of any depth > 8.

      @param palette  The array of colors
    */
    void setPalette(const uInt32* palette);

    /**
      This routine should be called to draw a rectangular box with sides
      at the specified coordinates.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the box
      @param h      The height of the box
      @param colorA FIXME
      @param colorB FIXME
    */
    void box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
             OverlayColor colorA, OverlayColor colorB);

    /**
      This routine should be called to draw a framed rectangle.
      I'm not exactly sure what it is, so I can't explain it :)

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the surrounding frame
    */
    void frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                   OverlayColor color);

    /**
      Indicate that the specified area should be redrawn.
      Currently we just redraw the entire screen.
    */
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h) { refresh(); }

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and must be implemented
    // in derived classes.
    //////////////////////////////////////////////////////////////////////
    /**
      This routine is called to initialize the subsystem-specific video mode.
    */
    virtual bool initSubsystem() = 0;

    /**
      This routine is called whenever the screen needs to be recreated.
      It updates the global screen variable.
    */
    virtual bool createScreen() = 0;

    /**
      Switches between the filtering options in the video subsystem.
    */
    virtual void toggleFilter() = 0;

    /**
      This routine should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    virtual void drawMediaSource() = 0;

    /**
      This routine is called before any drawing is done (per-frame).
    */
    virtual void preFrameUpdate() = 0;

    /**
      This routine is called after any drawing is done (per-frame).
    */
    virtual void postFrameUpdate() = 0;

    /**
      This routine is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    virtual void scanline(uInt32 row, uInt8* data) = 0;

    /**
      This routine is called to map a given r,g,b triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) = 0;

    /**
      This routine should be called to draw a horizontal line.

      @param x     The first x coordinate
      @param y     The y coordinate
      @param x2    The second x coordinate
      @param color The color of the line
    */
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color) = 0;

    /**
      This routine should be called to draw a vertical line.

      @param x     The x coordinate
      @param y     The first y coordinate
      @param y2    The second y coordinate
      @param color The color of the line
    */
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color) = 0;

    /**
      This routine should be called to draw a blended rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the box
      @param h      The height of the box
      @param color  FIXME
      @param level  FIXME
    */
    virtual void blendRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                           OverlayColor color, uInt32 level = 3) = 0;

    /**
      This routine should be called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the area
    */
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          OverlayColor color) = 0;

    /**
      This routine should be called to draw the specified character.

      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    virtual void drawChar(uInt8 c, uInt32 x, uInt32 y, OverlayColor color) = 0;

    /**
      This routine should be called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
      @param h      The height of the data image
    */
    virtual void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, OverlayColor color,
                            Int32 h = 8) = 0;

    /**
      This routine should be called to translate the given coordinates
      to their unzoomed/unscaled equivalents.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    virtual void translateCoords(Int32* x, Int32* y) = 0;

  protected:
    // The parent system for the framebuffer
    OSystem* myOSystem;

    // Dimensions of the base image, before zooming.
    // All external GUI items should refer to these dimensions,
    //  since this is the *real* size of the image.
    // The other sizes are simply scaled versions of these dimensions.
    SDL_Rect myBaseDim;

    // Dimensions of the actual image, after zooming
    SDL_Rect myImageDim;

    // Dimensions of the SDL window (not always the same as the image)
    SDL_Rect myScreenDim;

    // Dimensions of the desktop area
    SDL_Rect myDesktopDim;

    // Indicates if the entire frame should be redrawn
    bool theRedrawEntireFrameIndicator;

    // The SDL video buffer
    SDL_Surface* myScreen;

    // SDL initialization flags
    uInt32 mySDLFlags;

    // SDL palette
    Uint32 myPalette[256];

    // Holds the palette for GUI elements
    uInt8 myGUIColors[5][3];

    // Indicates the current zoom level of the SDL screen
    uInt32 theZoomLevel;

    // Indicates the maximum zoom of the SDL screen
    uInt32 theMaxZoomLevel;

    // The aspect ratio of the window
    float theAspectRatio;

    // Indicates whether to use aspect ratio correction
    bool theUseAspectRatioFlag;

    // The font object to use
    StellaFont* myFont;

  private:
    /**
      Set the icon for the main SDL window.
    */
    void setWindowIcon();

  private:
    // Indicates the current framerate of the system
    uInt32 myFrameRate;

    // Indicates the current pause status
    bool myPauseStatus;

    // Indicates if the menus should be redrawn
    bool theMenuChangedIndicator;

    // Message timer
    Int32 myMessageTime;

    // Message text
    string myMessageText;

    // Number of times menu have been drawn
    uInt32 myMenuRedraws;

    // Indicates how many times the framebuffer has been redrawn
    // Used only for debugging purposes
    uInt32 myNumRedraws;
};

#endif
