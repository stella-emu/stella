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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Menu.hxx,v 1.6 2005-05-03 19:11:27 stephena Exp $
//============================================================================

#ifndef MENU_HXX
#define MENU_HXX

class Properties;
class OSystem;

#include <SDL.h>

#include "Stack.hxx"
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "OptionsDialog.hxx"
#include "bspf.hxx"

typedef FixedStack<Dialog *> DialogStack;

/**
  The base class for all menus in Stella.
  
  This class keeps track of all configuration menus. organizes them into
  a stack, and handles their events.

  @author  Stephen Anthony
  @version $Id: Menu.hxx,v 1.6 2005-05-03 19:11:27 stephena Exp $
*/
class Menu
{
  public:
    /**
      Create a new menu stack
    */
    Menu(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~Menu();

  public:
    /**
      Handle a keyboard event.

      @param key   keysym
      @param mod   modifiers
      @param state state of key
    */
    void handleKeyEvent(SDLKey key, SDLMod mod, uInt8 state); //FIXME - this shouldn't refer to SDL directly

    /**
      Handle a mouse motion event.

      @param x      The x location
      @param y      The y location
      @param button The currently pressed button
    */
    void handleMouseMotionEvent(Int32 x, Int32 y, Int32 button);

    /**
      Handle a mouse button event.

      @param b     The mouse button
      @param x     The x location
      @param y     The y location
      @param state The state (pressed or released)
    */
    void handleMouseButtonEvent(MouseButton b, Int32 x, Int32 y, uInt8 state);

// FIXME - add joystick handler

    /**
      (Re)initialize the menuing system.  This is necessary if a new Console
      has been loaded, since in most cases the screen dimensions will have changed.
    */
    void initialize();

    /**
      Draw the stack of menus.
    */
    void draw();

    /**
      Add a dialog box to the stack
    */
    void addDialog(Dialog* d);

    /**
      Remove the topmost dialog box from the stack
    */
    void removeDialog();

    /**
      Reset dialog stack to the main configuration menu
    */
    void reStack();

    /**
      Adds the specified game info to the appropriate menu item

      @param props  The properties of the current game
    */
    void setGameProfile(Properties& props) { myOptionsDialog->setGameProfile(props); }

  private:
    OSystem* myOSystem;
    OptionsDialog* myOptionsDialog;
    DialogStack myDialogStack;

    // For continuous events (keyDown)
    struct {
      uInt16 ascii;
      uInt8 flags;
      uInt32 keycode;
    } myCurrentKeyDown;
    uInt32 myKeyRepeatTime;
	
    // Position and time of last mouse click (used to detect double clicks)
    struct {
      Int16 x, y;   // Position of mouse when the click occured
      uInt32 time;  // Time
      Int32 count;  // How often was it already pressed?
    } myLastClick;
};

#endif
