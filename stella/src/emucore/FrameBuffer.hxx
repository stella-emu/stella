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
// $Id: FrameBuffer.hxx,v 1.22 2005-03-26 19:26:47 stephena Exp $
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
  @version $Id: FrameBuffer.hxx,v 1.22 2005-03-26 19:26:47 stephena Exp $
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

      @param title     The title of the window
      @param width     The width of the framebuffer
      @param height    The height of the framebuffer
    */
    void initialize(const string& title, uInt32 width, uInt32 height);

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
      Returns the current width of the framebuffer.
      Note that this will take into account the current scaling (if any).

      @return  The current width
    */
    uInt32 width()  { return myWidth; }

    /**
      Returns the current height of the framebuffer.
      Note that this will take into account the current scaling (if any).

      @return  The current height
    */
    uInt32 height() { return myHeight; }

#if 0
FIXME
    /**
      This routine is called to get the width of the onscreen image.
    */
    uInt32 imageWidth() { return myDimensions.w; }

    /**
      This routine is called to get the height of the onscreen image.
    */
    uInt32 imageHeight() { return myDimensions.h; }
#endif

    /**
      This routine is called to get the width of the system desktop.
    */
    uInt32 screenWidth();

    /**
      This routine is called to get the height of the system desktop.
    */
    uInt32 screenHeight();

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
//FIXME cerr << "refresh() " << val++ << endl;
      theRedrawEntireFrameIndicator = true;
      myMenuRedraws = 2;
      if(now) drawMediaSource();
    }

    /**
      Toggles between fullscreen and window mode. either automatically
      or based on the given flag.  Grabmouse activated when in fullscreen mode.  

      @param given   Indicates whether to use the specified 'toggle' or
                     decide based on current status
      @param toggle  Set the fullscreen mode to this value
    */
    void toggleFullscreen(bool given = false, bool toggle = false);

    /**
      This routine is called when the user wants to resize the window.
      Mode = '1' means window should increase in size
      Mode = '-1' means window should decrease in size
      Mode = '0' means window should be resized to defaults

      @param mode  How the window should be resized
      @param zoom  The zoom level to use if something other than 0
    */
    void resize(Int8 mode, Int8 zoom = 0);

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
      Answers the current zoom level of the SDL window.
    */
    inline uInt32 zoomLevel() { return theZoomLevel; }

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
    */
    void setupPalette();

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
    virtual void blendRect(int x, int y, int w, int h,
                           OverlayColor color, int level = 3) = 0;

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
      This routine should be called to draw a framed rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the surrounding frame
    */
    virtual void frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
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

#if 0
FIXME
    /**
      This routine is called when the emulation has received
      a pause event.

      @param status  The received pause status
    */
    virtual void pauseEvent(bool status) = 0;
#endif


  protected:
    // The parent system for the framebuffer
    OSystem* myOSystem;

    // Bounds for the window frame
    uInt32 myWidth, myHeight;

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

    // Used to get window-manager specifics
    SDL_SysWMinfo myWMInfo;

    // Indicates the width/height and origin x/y of the onscreen image
    // (these may be different than the screen/window dimensions)
    SDL_Rect myDimensions;

    // Indicates if the system-specific WM information is available
    bool myWMAvailable;

    // Indicates the current zoom level of the SDL screen
    uInt32 theZoomLevel;

    // Indicates the maximum zoom of the SDL screen
    uInt32 theMaxZoomLevel;

    // The aspect ratio of the window
    float theAspectRatio;

    // The font object to use
    StellaFont* myFont;

  private:
    /**
      Set the icon for the main SDL window.
    */
    void setWindowIcon();

/*
    // Enumeration representing the different types of user interface widgets
    enum Widget { W_NONE, MAIN_MENU, REMAP_MENU, INFO_MENU };

    Widget currentSelectedWidget();
    Event::Type currentSelectedEvent();

    // Add binding between a StellaEvent key and a core event
    void addKeyBinding(Event::Type event, StellaEvent::KeyCode key);

    // Add binding between a StellaEvent joystick and a core event
    void addJoyBinding(Event::Type event, StellaEvent::JoyStick stick,
                       StellaEvent::JoyCode code);

    // Remove all bindings for this core event
    void deleteBinding(Event::Type event);

    // Draw the main menu
    void drawMainMenu();

    // Draw the remap menu
    void drawRemapMenu();

    // Draw the info menu
    void drawInfoMenu();

    // Move the cursor up 'amt' lines, possibly scrolling the list of items
    void moveCursorUp(uInt32 amt);

    // Move the cursor down 'amt' lines, possibly scrolling the list of items
    void moveCursorDown(uInt32 amt);

    // scan the mapping arrays and update the remap menu
    void loadRemapMenu();
*/
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

int val;
/*
    // Structure used for main menu items
    struct MainMenuItem
    {
      Widget widget;
      string action;
    };

    // Structure used for remap menu items
    struct RemapMenuItem
    {
      Event::Type event;
      string action;
      string key;
    };

    // Table of strings representing the various StellaEvent codes
    static const char* ourEventName[StellaEvent::LastKCODE];

    // Type of interface item currently slated for redraw
    Widget myCurrentWidget;

    // Indicates that an event is currently being remapped
    bool myRemapEventSelectedFlag;

    // Indicates the current selected event being remapped
    Event::Type mySelectedEvent;

    // Indicates if we are in menu mode
    bool myMenuMode;

    // The maximum number of vertical lines of text that can be onscreen
    Int32 myMaxRows;

    // The maximum number of characters of text in a row
    Int32 myMaxColumns;

    // Keep track of current selected main menu item
    uInt32 myMainMenuIndex, myMainMenuItems;

    // Keep track of current selected remap menu item
    Int32 myRemapMenuIndex, myRemapMenuLowIndex, myRemapMenuHighIndex;
    Int32 myRemapMenuItems, myRemapMenuMaxLines;

    // The width of the information menu, determined by the longest string
    Int32 myInfoMenuWidth;

    // Holds information about the current selected ROM image
    string ourPropertiesInfo[9];

    // Holds static strings for the main menu
    static MainMenuItem ourMainMenu[2];

    // Holds static strings for the remap menu
    static RemapMenuItem ourRemapMenu[57];

    // Holds the current key mappings
    Event::Type* myKeyTable;

    // Holds the number of items in the keytable array
    uInt32 myKeyTableSize;

    // Holds the current joystick mappings
    Event::Type* myJoyTable;

    // Holds the number of items in the joytable array
    uInt32 myJoyTableSize;
*/
};

#endif
