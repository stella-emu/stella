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
// $Id: FrameBuffer.hxx,v 1.1 2003-10-17 18:02:16 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_HXX
#define FRAMEBUFFER_HXX

#include "bspf.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "StellaEvent.hxx"

class Console;

/**
  This class implements a MAME-like user interface where Stella settings
  can be changed.  It also encapsulates the MediaSource.

  @author  Stephen Anthony
  @version $Id: FrameBuffer.hxx,v 1.1 2003-10-17 18:02:16 stephena Exp $
*/
class FrameBuffer
{
  public:
    /**
      Creates a new Frame Buffer
    */
    FrameBuffer();

    /**
      Destructor
    */
    virtual ~FrameBuffer(void);

    /**
      Send a keyboard event to the user interface.

      @param code  The StellaEvent code
      @param state The StellaEvent state
    */
    void sendKeyEvent(StellaEvent::KeyCode code, Int32 state);

    /**
      Send a joystick button event to the user interface.

      @param stick The joystick activated
      @param code  The StellaEvent joystick code
      @param state The StellaEvent state
    */
    void sendJoyEvent(StellaEvent::JoyStick stick, StellaEvent::JoyCode code,
         Int32 state);

    void showMainMenu(bool show);
    void showMessage(const string& message);

    uInt32 width()  { return myWidth; }
    uInt32 height() { return myHeight; }

    uInt16* pixels() const;

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() { return isFullscreen; }

  public:
    /**
      This routine should be called once the console is created to setup
      the video system for us to use.  Return false if any operation fails,
      otherwise return true.
    */
    virtual bool init(Console* console, MediaSource* mediasrc) = 0;

    /**
      This routine should be called anytime the display needs to be updated
    */
    virtual void update() = 0;

    /**
      Toggles between fullscreen and windowed mode.
    */
    virtual void toggleFullscreen() = 0;

  protected:
    // Enumeration representing the different types of user interface widgets
    enum Widget { W_NONE, MAIN_MENU, REMAP_MENU, INFO_MENU, FONTS_MENU };

    Widget currentSelectedWidget();
    Event::Type currentSelectedEvent();

    // Add binding between a StellaEvent key and a core event
    void addKeyBinding(Event::Type event, StellaEvent::KeyCode key);

    // Add binding between a StellaEvent joystick and a core event
    void addJoyBinding(Event::Type event, StellaEvent::JoyStick stick,
                       StellaEvent::JoyCode code);

    // Remove all bindings for this core event
    void deleteBinding(Event::Type event);

    // Move the cursor up 1 line, possibly scrolling the list of items
    void moveCursorUp();

    // Move the cursor down 1 line, possibly scrolling the list of items
    void moveCursorDown();

    // Move the list up 1 page and put the cursor at the top
    void movePageUp();

    // Move the list down 1 page and put the cursor at the top
    void movePageDown();

    // scan the mapping arrays and update the remap menu
    void loadRemapMenu();

    void initBase(Console* console, MediaSource* mediasrc);

  protected:
    // The Console for the system
    Console* myConsole;

    // The Mediasource for the system
    MediaSource* myMediaSource;

    // Indicates the current framerate of the system
    uInt32 myFrameRate;

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

    // Bounds for the window frame
    uInt32 myXStart, myYStart, myWidth, myHeight;

    // Table of bitmapped fonts.
    static const uInt8 ourFontData[2048];

    // Table of strings representing the various StellaEvent codes
    static const char* ourEventName[StellaEvent::LastKCODE];

    // Type of interface item currently slated for redraw
    Widget myCurrentWidget;

    // Indicates that an event is currently being remapped
    bool myRemapEventSelectedFlag;

    // Indicates the current selected event being remapped
    Event::Type mySelectedEvent;

    // The maximum number of vertical lines of text that can be onscreen
    uInt32 myMaxLines;

    // Keep track of current selected main menu item
    uInt32 myMainMenuIndex, myMainMenuItems;

    // Keep track of current selected remap menu item
    uInt32 myRemapMenuIndex, myRemapMenuLowIndex, myRemapMenuHighIndex;
    uInt32 myRemapMenuItems, myRemapMenuMaxLines;

    // Message timer
    Int32 myMessageTime;

    // Message text
    string myMessageText;

    // The width of the information menu, determined by the longest string
    uInt32 myInfoMenuWidth;

    // Holds information about the current selected ROM image
    string ourPropertiesInfo[6];

    // Holds static strings for the main menu
    static MainMenuItem ourMainMenu[3];

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

    // Indicates whether the emulator is currently in fullscreen mode
    bool isFullscreen; // FIXME - remove from here, its specific
};

#endif
