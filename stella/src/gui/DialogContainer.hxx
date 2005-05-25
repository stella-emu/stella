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
// $Id: DialogContainer.hxx,v 1.3 2005-05-25 23:22:11 stephena Exp $
//============================================================================

#ifndef DIALOG_CONTAINER_HXX
#define DIALOG_CONTAINER_HXX

class OSystem;

#include "Stack.hxx"
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "bspf.hxx"

typedef FixedStack<Dialog *> DialogStack;

/**
  The base class for groups of dialog boxes.  Each dialog box has a
  parent.  In most cases, the parent is itself a dialog box, but in the
  case of the lower-most dialog box, this class is its parent.
  
  This class keeps track of its children (dialog boxes), organizes them into
  a stack, and handles their events.

  @author  Stephen Anthony
  @version $Id: DialogContainer.hxx,v 1.3 2005-05-25 23:22:11 stephena Exp $
*/
class DialogContainer
{
  public:
    /**
      Create a new DialogContainer stack
    */
    DialogContainer(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~DialogContainer();

  public:
    /**
      Handle a keyboard event.

      @param key   keysym
      @param mod   modifiers
      @param state state of key
    */
    void handleKeyEvent(int key, int mod, uInt8 state);

    /**
      Handle a mouse motion event.

      @param x      The x location
      @param y      The y location
      @param button The currently pressed button
    */
    void handleMouseMotionEvent(int x, int y, int button);

    /**
      Handle a mouse button event.

      @param b     The mouse button
      @param x     The x location
      @param y     The y location
      @param state The state (pressed or released)
    */
    void handleMouseButtonEvent(MouseButton b, int x, int y, uInt8 state);

    /**
      Handle a joystick button event.

      @param x       The x location
      @param y       The y location
      @param stick   The joystick number
      @param button  The joystick button
      @param state   The state (pressed or released)
    */
    void handleJoyEvent(int x, int y, int stick, int button, uInt8 state);

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
      (Re)initialize the menuing system.  This is necessary if a new Console
      has been loaded, since in most cases the screen dimensions will have changed.
    */
    virtual void initialize() = 0;

  protected:
    OSystem* myOSystem;
    Dialog*  myBaseDialog;
    DialogStack myDialogStack;

    // For continuous events (keyDown)
    struct {
      int ascii;
      uInt8 flags;
      int keycode;
    } myCurrentKeyDown;
    int myKeyRepeatTime;
	
    // Position and time of last mouse click (used to detect double clicks)
    struct {
      int x, y;   // Position of mouse when the click occured
      int time;  // Time
      int count;  // How often was it already pressed?
    } myLastClick;
};

#endif
