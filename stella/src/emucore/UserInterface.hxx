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
// $Id: UserInterface.hxx,v 1.6 2003-09-30 01:22:45 stephena Exp $
//============================================================================

#ifndef USERINTERFACE_HXX
#define USERINTERFACE_HXX

#include "bspf.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"

class Console;
class MediaSource;

/**
  This class implements a MAME-like user interface where Stella settings
  can be changed.

  @author  Stephen Anthony
  @version $Id: UserInterface.hxx,v 1.6 2003-09-30 01:22:45 stephena Exp $
*/
class UserInterface
{
  public:
    /**
      Creates a new User Interface

      @param console  The Console object
      @param mediasrc The MediaSource object to draw into
    */
    UserInterface(Console* console, MediaSource* mediasrc);

    /**
      Destructor
    */
    virtual ~UserInterface(void);

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

  public:
    void showMainMenu(bool show);
    void showMessage(const string& message);
    void update();

  private:
    // Enumeration representing the different types of user interface widgets
    enum Widget { W_NONE, MAIN_MENU, REMAP_MENU, INFO_MENU };

    Widget currentSelectedWidget();
    Event::Type currentSelectedEvent();

    // Move the cursor up 1 line, possibly scrolling the list of items
    void moveCursorUp();

    // Move the cursor down 1 line, possibly scrolling the list of items
    void moveCursorDown();

    // Move the list up 1 page and put the cursor at the top
    void movePageUp();

    // Move the list down 1 page and put the cursor at the top
    void movePageDown();

    // Draw a bounded box at the specified coordinates
    void drawBoundedBox(uInt32 x, uInt32 y, uInt32 width, uInt32 height);

    // Draw message text at specified coordinates
    void drawText(uInt32 x, uInt32 y, const string& message);

    // scan the mapping arrays and update the remap menu
    void loadRemapMenu();

    // Add binding between a StellaEvent key and a core event
    void addKeyBinding(Event::Type event, StellaEvent::KeyCode key);

    // Add binding between a StellaEvent joystick and a core event
    void addJoyBinding(Event::Type event, StellaEvent::JoyStick stick,
                       StellaEvent::JoyCode code);

    // Remove all bindings for this core event
    void deleteBinding(Event::Type event);


  private:
    // The Console for the system
    Console* myConsole;

    // The Mediasource for the system
    MediaSource* myMediaSource;

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
};

#endif
