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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <map>

class Console;
class OSystem;
class MouseControl;
class DialogContainer;
class PhysicalJoystick;

#include "Event.hxx"
#include "EventHandlerConstants.hxx"
#include "Control.hxx"
#include "StellaKeys.hxx"
#include "PKeyboardHandler.hxx"
#include "PJoystickHandler.hxx"
#include "Variant.hxx"
#include "bspf.hxx"

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
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler(OSystem& osystem);
    virtual ~EventHandler();

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    const Event& event() const { return myEvent; }

    /**
      Initialize state of this eventhandler.
    */
    void initialize();

    /**
      Maps the given Stelladaptor/2600-daptor(s) to specified ports on a real 2600.

      @param saport  How to map the ports ('lr' or 'rl')
    */
    void mapStelladaptors(const string& saport);

    /**
      Swaps the ordering of Stelladaptor/2600-daptor(s) devices.
    */
    void toggleSAPortOrder();

    /**
      Toggle whether the console is in 2600 or 7800 mode.
      Note that for now, this only affects whether the 7800 pause button is
      supported; there is no further emulation of the 7800 itself.
    */
    void set7800Mode();

    /**
      Collects and dispatches any pending events.  This method should be
      called regularly (at X times per second, where X is the game framerate).

      @param time  The current time in microseconds.
    */
    void poll(uInt64 time);

    /**
      Get/set the current state of the EventHandler

      @return The EventHandlerState type
    */
    EventHandlerState state() const { return myState; }
    void setState(EventHandlerState state);

    /**
      Resets the state machine of the EventHandler to the defaults

      @param state  The current state to set
    */
    void reset(EventHandlerState state);

    /**
      This method indicates that the system should terminate.
    */
    void quit() { handleEvent(Event::Quit, 1); }

    /**
      Sets the mouse axes and buttons to act as the controller specified in
      the ROM properties, otherwise disable mouse control completely

      @param enable  Whether to use the mouse to emulate controllers
                     Currently, this will be one of the following values:
                     'always', 'analog', 'never'
    */
    void setMouseControllerMode(const string& enable);

    void enterMenuMode(EventHandlerState state);
    void leaveMenuMode();
    bool enterDebugMode();
    void leaveDebugMode();
    void enterTimeMachineMenuMode(uInt32 numWinds, bool unwind);

    /**
      Send an event directly to the event handler.
      These events cannot be remapped.

      @param type  The event
      @param value The value for the event
    */
    void handleEvent(Event::Type type, Int32 value);

    /**
      Handle events that must be processed each time a new console is
      created.  Typically, these are events set by commandline arguments.
    */
    void handleConsoleStartupEvents();

    bool frying() const { return myFryingFlag; }

    StringList getActionList(EventMode mode) const;
    VariantList getComboList(EventMode mode) const;

    /** Used to access the list of events assigned to a specific combo event. */
    StringList getComboListForEvent(Event::Type event) const;
    void setComboListForEvent(Event::Type event, const StringList& events);

    /** Convert keys and physical joystick events into Stella events. */
    Event::Type eventForKey(StellaKey key, EventMode mode) const {
      return myPKeyHandler->eventForKey(key, mode);
    }
    Event::Type eventForJoyAxis(int stick, int axis, int value, EventMode mode) const {
      return myPJoyHandler->eventForAxis(stick, axis, value, mode);
    }
    Event::Type eventForJoyButton(int stick, int button, EventMode mode) const {
      return myPJoyHandler->eventForButton(stick, button, mode);
    }
    Event::Type eventForJoyHat(int stick, int hat, JoyHat value, EventMode mode) const {
      return myPJoyHandler->eventForHat(stick, hat, value, mode);
    }

    Event::Type eventAtIndex(int idx, EventMode mode) const;
    string actionAtIndex(int idx, EventMode mode) const;
    string keyAtIndex(int idx, EventMode mode) const;

    /**
      Bind a key to an event/action and regenerate the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param key    The key to bind to this event
    */
    bool addKeyMapping(Event::Type event, EventMode mode, StellaKey key);

    /**
      Bind a physical joystick axis direction to an event/action and regenerate
      the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  The value on the given axis
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyAxisMapping(Event::Type event, EventMode mode,
                           int stick, int axis, int value,
                           bool updateMenus = true);

    /**
      Bind a physical joystick button to an event/action and regenerate the
      mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param button The joystick button
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyButtonMapping(Event::Type event, EventMode mode, int stick, int button,
                             bool updateMenus = true);

    /**
      Bind a physical joystick hat direction to an event/action and regenerate
      the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param hat    The joystick hat
      @param value  The value on the given hat
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyHatMapping(Event::Type event, EventMode mode,
                          int stick, int hat, JoyHat value,
                          bool updateMenus = true);

    /**
      Erase the specified mapping.

      @param event  The event for which we erase all mappings
      @param mode   The mode where this event is active
    */
    void eraseMapping(Event::Type event, EventMode mode);

    /**
      Resets the event mappings to default values.

      @param event  The event which to (re)set (Event::NoType resets all)
      @param mode   The mode for which the defaults are set
    */
    void setDefaultMapping(Event::Type event, EventMode mode);

    /**
      Sets the combo event mappings to those in the 'combomap' setting
    */
    void setComboMap();

    /**
      Joystick emulates 'impossible' directions (ie, left & right
      at the same time).

      @param allow  Whether or not to allow impossible directions
    */
    void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }

    /**
      Changes to a new state based on the current state and the given event.

      @param type  The event
      @return      True if the state changed, else false
    */
    bool changeStateByEvent(Event::Type type);

    /**
      Get the current overlay in use.  The overlay won't always exist,
      so we should test if it's available.

      @return The overlay object
    */
    DialogContainer& overlay() const  { return *myOverlay; }
    bool hasOverlay() const { return myOverlay != nullptr; }

    /**
      Return a list of all physical joysticks currently in the internal database
      (first part of variant) and its internal ID (second part of variant).
    */
    VariantList physicalJoystickDatabase() const {
      return myPJoyHandler->database();
    }

    /**
      Remove the physical joystick identified by 'name' from the joystick
      database, only if it is not currently active.
    */
    void removePhysicalJoystickFromDatabase(const string& name);

    /**
      Enable/disable text events (distinct from single-key events).
    */
    virtual void enableTextEvents(bool enable) = 0;

    /**
      Handle changing mouse modes.
    */
    void handleMouseControl();

  protected:
    // Global OSystem object
    OSystem& myOSystem;

    /**
      Methods which are called by derived classes to handle specific types
      of input.
    */
    void handleTextEvent(char text);
    void handleMouseMotionEvent(int x, int y, int xrel, int yrel);
    void handleMouseButtonEvent(MouseButton b, bool pressed, int x, int y);
    void handleKeyEvent(StellaKey key, StellaMod mod, bool state) {
      myPKeyHandler->handleEvent(key, mod, state);
    }
    void handleJoyBtnEvent(int stick, int button, uInt8 state) {
      myPJoyHandler->handleBtnEvent(stick, button, state);
    }
    void handleJoyAxisEvent(int stick, int axis, int value) {
      myPJoyHandler->handleAxisEvent(stick, axis, value);
    }
    void handleJoyHatEvent(int stick, int hat, int value) {
      myPJoyHandler->handleHatEvent(stick, hat, value);
    }

    /**
      Collects and dispatches any pending events.
    */
    virtual void pollEvent() = 0;

    // Other events that can be received from the underlying event handler
    enum class SystemEvent {
      WINDOW_SHOWN,
      WINDOW_HIDDEN,
      WINDOW_EXPOSED,
      WINDOW_MOVED,
      WINDOW_RESIZED,
      WINDOW_MINIMIZED,
      WINDOW_MAXIMIZED,
      WINDOW_RESTORED,
      WINDOW_ENTER,
      WINDOW_LEAVE,
      WINDOW_FOCUS_GAINED,
      WINDOW_FOCUS_LOST
    };
    void handleSystemEvent(SystemEvent e, int data1 = 0, int data2 = 0);

    /**
      Add the given joystick to the list of physical joysticks available to the handler.
    */
    void addPhysicalJoystick(PhysicalJoystickPtr stick);

    /**
      Remove physical joystick at the current index.
    */
    void removePhysicalJoystick(int index);

  private:
    enum {
      kComboSize          = 16,
      kEventsPerCombo     = 8,
      kEmulActionListSize = 80 + kComboSize,
      kMenuActionListSize = 14
    };

    /**
      The following methods take care of assigning action mappings.
    */
    void setActionMappings(EventMode mode);
    void setDefaultKeymap(Event::Type, EventMode mode);
    void setDefaultJoymap(Event::Type, EventMode mode);
    void saveKeyMapping();
    void saveJoyMapping();
    void saveComboMapping();

  private:
    // Structure used for action menu items
    struct ActionList {
      Event::Type event;
      string action;
      string key;
      bool allow_combo;
    };

    // Global Event object
    Event myEvent;

    // Indicates current overlay object
    DialogContainer* myOverlay;

    // Handler for all keyboard-related events
    unique_ptr<PhysicalKeyboardHandler> myPKeyHandler;

    // Handler for all joystick addition/removal/mapping
    unique_ptr<PhysicalJoystickHandler> myPJoyHandler;

    // MouseControl object, which takes care of switching the mouse between
    // all possible controller modes
    unique_ptr<MouseControl> myMouseControl;

    // The event(s) assigned to each combination event
    Event::Type myComboTable[kComboSize][kEventsPerCombo];

    // Indicates the current state of the system (ie, which mode is current)
    EventHandlerState myState;

    // Indicates whether the virtual joystick emulates 'impossible' directions
    bool myAllowAllDirectionsFlag;

    // Indicates whether or not we're in frying mode
    bool myFryingFlag;

    // Sometimes an extraneous mouse motion event occurs after a video
    // state change; we detect when this happens and discard the event
    bool mySkipMouseMotion;

    // Whether the currently enabled console is emulating certain aspects
    // of the 7800 (for now, only the switches are notified)
    bool myIs7800;

    // Holds static strings for the remap menu (emulation and menu events)
    static ActionList ourEmulActionList[kEmulActionListSize];
    static ActionList ourMenuActionList[kMenuActionListSize];

    // Following constructors and assignment operators not supported
    EventHandler() = delete;
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;
};

#endif
