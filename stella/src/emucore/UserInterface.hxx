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
// $Id: UserInterface.hxx,v 1.3 2003-09-26 17:35:05 stephena Exp $
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
  @version $Id: UserInterface.hxx,v 1.3 2003-09-26 17:35:05 stephena Exp $
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

    void sendKeymap(Event::Type table[StellaEvent::LastKCODE]);
    void sendJoymap(Event::Type table[StellaEvent::LastJSTICK][StellaEvent::LastJCODE]);

  public:
    bool drawPending() { return myCurrentWidget != NONE; }
    void showMainMenu(bool show);
    void showMessage(string& message);
    void update();

  private:
    // Enumeration representing the different types of user interface widgets
    enum Widget { NONE, MAIN_MENU, REMAP_MENU, INFO_MENU, MESSAGE };

    Widget currentSelectedWidget();
    Event::Type currentSelectedEvent();

    void moveCursorUp();
    void moveCursorDown();

    // Draw a bounded box centered horizontally
    void drawBoundedBox(uInt32 width, uInt32 height);

  private:
    // The Console for the system
    Console* myConsole;

    // The Mediasource for the system
    MediaSource* myMediaSource;

    // Bounds for the window frame
    uInt32 myXStart, myYStart, myWidth, myHeight;

    // Table of bitmapped fonts.  Holds A..Z and 0..9.
    static const uInt32 ourFontData[36];

    // Type of interface item currently slated for redraw
    Widget myCurrentWidget;

    // Indicates that an event is currently being remapped
    bool myRemapEventSelectedFlag;

    // Indicates the current selected event being remapped
    Event::Type mySelectedEvent;

    // Message timer
    Int32 myMessageTime;

    // Message text
    string myMessageText;
};

#endif
