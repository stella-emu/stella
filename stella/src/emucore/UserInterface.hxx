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
// $Id: UserInterface.hxx,v 1.2 2003-09-26 00:32:00 stephena Exp $
//============================================================================

#ifndef USERINTERFACE_HXX
#define USERINTERFACE_HXX

#include "bspf.hxx"
#include "Event.hxx"

class Console;
class MediaSource;

/**
  This class implements a MAME-like user interface where Stella settings
  can be changed.

  @author  Stephen Anthony
  @version $Id: UserInterface.hxx,v 1.2 2003-09-26 00:32:00 stephena Exp $
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

    // Enumeration representing the different types of menus
    enum MenuType { MENU_NONE, MENU_MAIN, MENU_REMAP, MENU_INFO };

  public:
    MenuType currentSelectedMenu();
    Event::Type currentSelectedItem();

    bool drawPending() { return myCurrentMenu != MENU_NONE; }
    void showMenu(MenuType type);
    void update();
    void moveCursorUp();
    void moveCursorDown();

  private:
    // Clears the internal framebuffer
    void cls();

  private:
    // The Console for the system
    Console* myConsole;

    // The Mediasource for the system
    MediaSource* myMediaSource;

    // A buffer containing the current interface element to be drawn
    Int16* myBuffer;

    // Indicates the size of the framebuffer
    uInt32 myBufferSize;

    // Bounds for the window frame
    uInt32 myXStart, myYStart, myWidth, myHeight;

    // Table of bitmapped fonts.  Holds A..Z and 0..9.
    static const uInt32 ourFontData[36];

    // Indicates if buffers are dirty (have been modified)
    bool myBufferDirtyFlag;

    // Menu type currently slated for redraw
    MenuType myCurrentMenu;
};

#endif
