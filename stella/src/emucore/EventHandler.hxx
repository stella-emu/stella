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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.hxx,v 1.108 2008-03-02 20:48:51 stephena Exp $
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <SDL.h>

class Console;
class OSystem;
class DialogContainer;
class EventMappingWidget;

#include "Array.hxx"
#include "Event.hxx"
#include "Control.hxx"
#include "StringList.hxx"
#include "bspf.hxx"


enum MouseButton {
  EVENT_LBUTTONDOWN,
  EVENT_LBUTTONUP,
  EVENT_RBUTTONDOWN,
  EVENT_RBUTTONUP,
  EVENT_WHEELDOWN,
  EVENT_WHEELUP
};

enum EventMode {
  kEmulationMode = 0,  // make sure these are set correctly,
  kMenuMode      = 1,  // since they'll be used as array indices
  kNumModes      = 2
};

/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontend will send translated events here, and the handler will
  check to see what the current 'mode' is.

  If in emulation mode, events received from the frontend are remapped and
  sent to the emulation core.  If in menu mode, the events are sent
  unchanged to the menu class, where (among other things) changing key
  mapping can take place.

  @author  Stephen Anthony
  @version $Id: EventHandler.hxx,v 1.108 2008-03-02 20:48:51 stephena Exp $
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler(OSystem* osystem);
 
    /**
      Destructor
    */
    virtual ~EventHandler();

    // Enumeration representing the different states of operation
    enum State {
      S_NONE,
      S_EMULATE,
      S_PAUSE,
      S_LAUNCHER,
      S_MENU,
      S_CMDMENU,
      S_DEBUGGER
    };

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    Event* event() { return myEvent; }

    /**
      Initialize state of this eventhandler.
    */
    void initialize();

    /**
      Set up any joysticks on the system.  This must be called *after* the
      framebuffer has been created, since SDL requires the video to be
      intialized before joysticks can be probed.
    */
    void setupJoysticks();

    /**
      Maps the given stelladaptors to specified ports on a real 2600

      @param sa1  Port for the first Stelladaptor to emulate (left or right)
      @param sa2  Port for the second Stelladaptor to emulate (left or right)
    */
    void mapStelladaptors(const string& sa1, const string& sa2);

    /**
      Collects and dispatches any pending events.  This method should be
      called regularly (at X times per second, where X is the game framerate).

      @param time  The current time in milliseconds.
    */
    void poll(uInt32 time);

    /**
      Set the default action for a joystick button to the given event

      @param event  The event we are assigning
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param button The joystick button
    */
    void setDefaultJoyMapping(Event::Type event, EventMode mode,
                              int stick, int button);

    /**
      Set the default for a joystick axis to the given event

      @param event  The event we are assigning
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  The value on the given axis
    */
    void setDefaultJoyAxisMapping(Event::Type event, EventMode mode,
                                  int stick, int axis, int value);

    /**
      Set the default for a joystick hat to the given event

      @param event  The event we are assigning
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  The value on the given axis
    */
    void setDefaultJoyHatMapping(Event::Type event, EventMode mode,
                                 int stick, int hat, int value);

    /**
      Returns the current state of the EventHandler

      @return The State type
    */
    inline State state() { return myState; }

    /**
      Returns the current launcher state (decide whether to enter launcher
      on game exit).
    */
    inline bool useLauncher() { return myUseLauncherFlag; }

    /**
      Resets the state machine of the EventHandler to the defaults

      @param state  The current state to set
    */
    void reset(State state);

    /**
      Refresh display according to the current state

      @param forceUpdate  Do a framebuffer update right away, instead
                          of waiting for the next frame
    */
    void refreshDisplay(bool forceUpdate = false);

    /**
      This method indicates that the system should terminate.
    */
    void quit() { handleEvent(Event::Quit, 1); }

    /**
      Sets the mouse to act as paddle 'num'

      @param num          The paddle which the mouse should emulate
      @param showmessage  Print a message to the framebuffer
    */
    void setPaddleMode(int num, bool showmessage = false);

    inline bool kbdAlt(int mod)
    {
  #ifndef MAC_OSX
      return (mod & KMOD_ALT);
  #else
      return ((mod & KMOD_META) && (mod & KMOD_SHIFT));
  #endif
    }

    inline bool kbdControl(int mod)
    {
  #ifndef MAC_OSX
      return (mod & KMOD_CTRL) > 0;
  #else
      return ((mod & KMOD_META) && !(mod & KMOD_SHIFT));
  #endif
    }

    inline bool kbdShift(int mod)
    {
      return (mod & KMOD_SHIFT);
    }

    void enterMenuMode(State state);
    void leaveMenuMode();
    bool enterDebugMode();
    void leaveDebugMode();
    void takeSnapshot();

    /**
      Send a resize event to the handler.
    */
    void handleResizeEvent();

    /**
      Send an event directly to the event handler.
      These events cannot be remapped.

      @param type  The event
      @param value The value for the event
    */
    void handleEvent(Event::Type type, Int32 value);

    inline bool frying() { return myFryingFlag; }

    inline SDL_Joystick* getJoystick(int i) { return ourJoysticks[i].stick; }

    StringList getActionList(EventMode mode);

    inline Event::Type eventForKey(int key, EventMode mode)
      { return myKeyTable[key][mode]; }
    inline Event::Type eventForJoyButton(int stick, int button, EventMode mode)
      { return myJoyTable[stick][button][mode]; }
    inline Event::Type eventForJoyAxis(int stick, int axis, int value, EventMode mode)
      { return myJoyAxisTable[stick][axis][(value > 0)][mode]; }
    inline Event::Type eventForJoyHat(int stick, int hat, int value, EventMode mode)
      { return myJoyHatTable[stick][hat][value][mode]; }

    Event::Type eventAtIndex(int idx, EventMode mode);
    string actionAtIndex(int idx, EventMode mode);
    string keyAtIndex(int idx, EventMode mode);

    /**
      Bind a key to an event/action and regenerate the mapping array(s)

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param key    The key to bind to this event
    */
    bool addKeyMapping(Event::Type event, EventMode mode, int key);

    /**
      Bind a joystick button to an event/action and regenerate the
      mapping array(s)

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param button The joystick button
    */
    bool addJoyMapping(Event::Type event, EventMode mode, int stick, int button);

    /**
      Bind a joystick axis direction to an event/action and regenerate
      the mapping array(s)

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  The value on the given axis
    */
    bool addJoyAxisMapping(Event::Type event, EventMode mode,
                           int stick, int axis, int value);

    /**
      Bind a joystick hat direction to an event/action and regenerate
      the mapping array(s)

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param axis   The joystick hat
      @param value  The value on the given hat
    */
    bool addJoyHatMapping(Event::Type event, EventMode mode,
                          int stick, int hat, int value);

    /**
      Erase the specified mapping

      @event  The event for which we erase all mappings
      @param mode   The mode where this event is active
    */
    void eraseMapping(Event::Type event, EventMode mode);

    /**
      Resets the event mappings to default values

      @param mode   The mode for which the defaults are set
    */
    void setDefaultMapping(EventMode mode);

    /**
      Joystick emulates 'impossible' directions (ie, left & right
      at the same time)

      @param allow  Whether or not to allow impossible directions
    */
    void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }

  private:
    /**
      Send a mouse motion event to the handler.

      @param event The mouse motion event generated by SDL
    */
    void handleMouseMotionEvent(SDL_Event& event);

    /**
      Send a mouse button event to the handler.

      @param event The mouse button event generated by SDL
    */
    void handleMouseButtonEvent(SDL_Event& event, int state);

    /**
      Send a joystick button event to the handler

      @param stick  The joystick number
      @param button The joystick button
      @param state  The state of the button (pressed or released)
    */
    void handleJoyEvent(int stick, int button, int state);

    /**
      Send a joystick axis event to the handler

      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  The value on the given axis
    */
    void handleJoyAxisEvent(int stick, int axis, int value);

    /**
      Send a joystick hat event to the handler

      @param stick  The joystick number
      @param axis   The joystick hat
      @param value  The value on the given hat
    */
    void handleJoyHatEvent(int stick, int hat, int value);
	
    /**
      Detects and changes the eventhandler state

      @param type  The event
      @return      True if the state changed, else false
    */
    inline bool eventStateChange(Event::Type type);

    /**
      The following methods take care of assigning action mappings.
    */
    void setActionMappings(EventMode mode);
    void setSDLMappings();
    void setKeymap();
    void setJoymap();
    void setJoyAxisMap();
    void setJoyHatMap();
    void setDefaultKeymap(EventMode mode);
    void setDefaultJoymap(EventMode mode);
    void setDefaultJoyAxisMap(EventMode mode);
    void setDefaultJoyHatMap(EventMode mode);
    void saveKeyMapping();
    void saveJoyMapping();
    void saveJoyAxisMapping();
    void saveJoyHatMapping();

    /**
      Tests if a mapping list is valid, both by length and by event count.

      @param list    The string containing the mappings, separated by ':'
      @param map     The result of parsing the string for int mappings
      @param length  The number of items that should be in the list

      @return      True if valid list, else false
    */
    bool isValidList(string& list, IntArray& map, uInt32 length);

    /**
      Tests if a given event should use continuous/analog values.

      @param event  The event to test for analog processing
      @return       True if analog, else false
    */
    inline bool eventIsAnalog(Event::Type event);

    void setEventState(State state);

  private:
    enum {
      kEmulActionListSize = 75,
      kMenuActionListSize = 13
    };

    // Structure used for action menu items
    struct ActionList {
      Event::Type event;
      const char* action;
      char* key;
    };

    // Joystick related items
    enum {
      kNumJoysticks  = 8,
      kNumJoyButtons = 24,
      kNumJoyAxis    = 16,
      kNumJoyHats    = 16
    };
    enum JoyType {
      JT_NONE,
      JT_REGULAR,
      JT_STELLADAPTOR_LEFT,
      JT_STELLADAPTOR_RIGHT
    };
    struct Stella_Joystick {
      SDL_Joystick* stick;
      JoyType       type;
      string        name;
    };
    enum JoyHat {
      kJHatUp,
      kJHatDown,
      kJHatLeft,
      kJHatRight,
      kJHatCentered
    };
    struct JoyMouse {   // Used for joystick to mouse emulation
      bool active;
      int x, y, x_amt, y_amt, amt, val, old_val;
    };

    // Global OSystem object
    OSystem* myOSystem;

    // Global Event object
    Event* myEvent;

    // Indicates current overlay object
    DialogContainer* myOverlay;

    // Array of key events, indexed by SDLKey
    Event::Type myKeyTable[SDLK_LAST][kNumModes];

    // Array of joystick button events
    Event::Type myJoyTable[kNumJoysticks][kNumJoyButtons][kNumModes];

    // Array of joystick axis events
    Event::Type myJoyAxisTable[kNumJoysticks][kNumJoyAxis][2][kNumModes];

    // Array of joystick hat events (we don't record diagonals)
    Event::Type myJoyHatTable[kNumJoysticks][kNumJoyHats][4][kNumModes];

    // Array of messages for each Event
    string ourMessageTable[Event::LastType];

    // Array of strings which correspond to the given SDL key
    string ourSDLMapping[SDLK_LAST];

    // Array of joysticks available to Stella
    Stella_Joystick ourJoysticks[kNumJoysticks];

    // Indicates the current state of the system (ie, which mode is current)
    State myState;

    // Indicates whether the mouse cursor is grabbed
    bool myGrabMouseFlag;

    // Indicates whether to use launcher mode when exiting a game
    bool myUseLauncherFlag;

    // Indicates whether the joystick emulates 'impossible' directions
    bool myAllowAllDirectionsFlag;

    // Indicates whether or not we're in frying mode
    bool myFryingFlag;

    // Indicates which paddle the mouse currently emulates
    Int8 myPaddleMode;

    // Type of device on each controller port (based on ROM properties)
    Controller::Type myController[2];

    // Holds static strings for the remap menu (emulation and menu events)
    static ActionList ourEmulActionList[kEmulActionListSize];
    static ActionList ourMenuActionList[kMenuActionListSize];

    // Lookup table for paddle button events
    static const Event::Type Paddle_Button[4];

    // Static lookup tables for Stelladaptor axis/button support
    static const Event::Type SA_Axis[2][2];
    static const Event::Type SA_Button[2][2];
};

#endif
