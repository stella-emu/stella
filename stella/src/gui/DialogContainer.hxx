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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DialogContainer.hxx,v 1.12 2005-12-24 22:50:53 stephena Exp $
//============================================================================

#ifndef DIALOG_CONTAINER_HXX
#define DIALOG_CONTAINER_HXX

class Dialog;
class OSystem;

#include "EventHandler.hxx"
#include "Stack.hxx"
#include "bspf.hxx"

typedef FixedStack<Dialog *> DialogStack;

/**
  The base class for groups of dialog boxes.  Each dialog box has a
  parent.  In most cases, the parent is itself a dialog box, but in the
  case of the lower-most dialog box, this class is its parent.
  
  This class keeps track of its children (dialog boxes), organizes them into
  a stack, and handles their events.

  @author  Stephen Anthony
  @version $Id: DialogContainer.hxx,v 1.12 2005-12-24 22:50:53 stephena Exp $
*/
class DialogContainer
{
  friend class EventHandler;

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
      Update the dialog container with the current time.
      This is useful if we want to trigger events at some specified time.

      @param time  The current time in microseconds
    */
    void updateTime(uInt32 time);

    /**
      Handle a keyboard event.

      @param unicode  Unicode translation
      @param key      Actual key symbol
      @param mod      Modifiers
      @param state    Pressed or released
    */
    void handleKeyEvent(int unicode, int key, int mod, uInt8 state);

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

      @param stick   The joystick number
      @param button  The joystick button
      @param state   The state (pressed or released)
    */
    void handleJoyEvent(int stick, int button, uInt8 state);

    /**
      Handle a joystick axis event.

      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  Value associated with given axis
    */
    void handleJoyAxisEvent(int stick, int axis, int value);

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
      Redraw all dialogs on the stack
    */
    void refresh() { myRefreshFlag = true; }

    /**
      (Re)initialize the menuing system.  This is necessary if a new Console
      has been loaded, since in most cases the screen dimensions will have changed.
    */
    virtual void initialize() = 0;

    // Emulation of mouse using joystick axis events
    static JoyMouse ourJoyMouse;

  private:
    void handleJoyMouse(uInt32);
    void reset();

  protected:
    OSystem* myOSystem;
    Dialog*  myBaseDialog;
    DialogStack myDialogStack;

    enum {
      kDoubleClickDelay = 500,
      kKeyRepeatInitialDelay = 400,
      kKeyRepeatSustainDelay = 50,
      kClickRepeatInitialDelay = kKeyRepeatInitialDelay,
      kClickRepeatSustainDelay = kKeyRepeatSustainDelay
    };

    // Indicates the most current time (in milliseconds) as set by updateTime()
    uInt32 myTime;

    // Indicates a full refresh of all dialogs is required
    bool myRefreshFlag;

    // For continuous events (keyDown)
    struct {
      int ascii;
      int keycode;
      uInt8 flags;
    } myCurrentKeyDown;
    uInt32 myKeyRepeatTime;

    // For continuous events (mouseDown)
    struct {
      int x;
      int y;
      int button;
    } myCurrentMouseDown;
    uInt32 myClickRepeatTime;
	
    // Position and time of last mouse click (used to detect double clicks)
    struct {
      int x, y;     // Position of mouse when the click occured
      uInt32 time;  // Time
      int count;    // How often was it already pressed?
    } myLastClick;
};

#endif
