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
// $Id: Menu.hxx,v 1.2 2005-03-10 22:59:40 stephena Exp $
//============================================================================

#ifndef MENU_HXX
#define MENU_HXX

class Dialog;
class OSystem;
class OptionsDialog;

#include "Stack.hxx"
#include "bspf.hxx"

typedef FixedStack<Dialog *> DialogStack;

/**
  The base class for all menus in Stella.
  
  This class keeps track of all configuration menus. organizes them into
  a stack, and handles their events.

  @author  Stephen Anthony
  @version $Id: Menu.hxx,v 1.2 2005-03-10 22:59:40 stephena Exp $
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
    void handleKeyEvent(SDLKey key, SDLMod mod, uInt8 state);

// FIXME - add mouse and joystick handlers  

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
    void reset();

  private:
    OSystem* myOSystem;
    OptionsDialog* myOptionsDialog;
    DialogStack myDialogStack;
};

#endif
