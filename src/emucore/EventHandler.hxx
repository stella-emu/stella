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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
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

  @author  Stephen Anthony, Thomas Jentzsch
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
    void quit() { handleEvent(Event::Quit); }

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

      @param type      The event
      @param value     The value to use for the event
      @param repeated  Repeated key (true) or first press/release (false)
    */
    void handleEvent(Event::Type type, Int32 value = 1, bool repeated = false);

    /**
      Handle events that must be processed each time a new console is
      created.  Typically, these are events set by commandline arguments.
    */
    void handleConsoleStartupEvents();

    bool frying() const { return myFryingFlag; }

    StringList getActionList(Event::Group group) const;
    VariantList getComboList(EventMode mode) const;

    /** Used to access the list of events assigned to a specific combo event. */
    StringList getComboListForEvent(Event::Type event) const;
    void setComboListForEvent(Event::Type event, const StringList& events);

    /** Convert keys and physical joystick events into Stella events. */
    Event::Type eventForKey(EventMode mode, StellaKey key, StellaMod mod) const {
      return myPKeyHandler->eventForKey(mode, key, mod);
    }
    Event::Type eventForJoyAxis(EventMode mode, int stick, JoyAxis axis, JoyDir adir, int button) const {
      return myPJoyHandler->eventForAxis(mode, stick, axis, adir, button);
    }
    Event::Type eventForJoyButton(EventMode mode, int stick, int button) const {
      return myPJoyHandler->eventForButton(mode, stick, button);
    }
    Event::Type eventForJoyHat(EventMode mode, int stick, int hat, JoyHatDir hdir, int button) const {
      return myPJoyHandler->eventForHat(mode, stick, hat, hdir, button);
    }

    /** Get description of given event and mode. */
    string getMappingDesc(Event::Type event, EventMode mode) const {
      return myPKeyHandler->getMappingDesc(event, mode);
    }

    Event::Type eventAtIndex(int idx, Event::Group group) const;
    string actionAtIndex(int idx, Event::Group group) const;
    string keyAtIndex(int idx, Event::Group group) const;

    /**
      Bind a key to an event/action and regenerate the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param key    The key to bind to this event
      @param mod    The modifier to bind to this event
    */
    bool addKeyMapping(Event::Type event, EventMode mode, StellaKey key, StellaMod mod);

    /**
      Enable controller specific keyboard event mappings.
    */
    void defineKeyControllerMappings(const Controller::Type type, Controller::Jack port) {
      myPKeyHandler->defineControllerMappings(type, port);
    }

    /**
      Enable emulation keyboard event mappings.
    */
    void enableEmulationKeyMappings() {
      myPKeyHandler->enableEmulationMappings();
    }

    /**
      Bind a physical joystick axis direction to an event/action and regenerate
      the mapping array(s). The axis can be combined with a button. The button
      can also be mapped without an axis.

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param button The joystick button
      @param axis   The joystick axis
      @param adir   The given axis
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyMapping(Event::Type event, EventMode mode, int stick,
                       int button, JoyAxis axis = JoyAxis::NONE, JoyDir adir = JoyDir::NONE,
                       bool updateMenus = true);

    /**
      Bind a physical joystick hat direction to an event/action and regenerate
      the mapping array(s). The hat can be combined with a button.

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param button The joystick button
      @param hat    The joystick hat
      @param dir    The value on the given hat
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyHatMapping(Event::Type event, EventMode mode, int stick,
                          int button, int hat, JoyHatDir dir,
                          bool updateMenus = true);

    /**
      Enable controller specific keyboard event mappings.
    */
    void defineJoyControllerMappings(const Controller::Type type, Controller::Jack port) {
      myPJoyHandler->defineControllerMappings(type, port);
    }

    /**
      Enable emulation keyboard event mappings.
    */
    void enableEmulationJoyMappings() {
      myPJoyHandler->enableEmulationMappings();
    }

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

    void saveKeyMapping();
    void saveJoyMapping();

    void exitEmulation();

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
    void handleKeyEvent(StellaKey key, StellaMod mod, bool pressed, bool repeated) {
      myPKeyHandler->handleEvent(key, mod, pressed, repeated);
    }
    void handleJoyBtnEvent(int stick, int button, bool pressed) {
      myPJoyHandler->handleBtnEvent(stick, button, pressed);
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
    static constexpr Int32
      COMBO_SIZE           = 16,
      EVENTS_PER_COMBO     = 8,
    #ifdef PNG_SUPPORT
      PNG_SIZE             = 2,
    #else
      PNG_SIZE             = 0,
    #endif
      EMUL_ACTIONLIST_SIZE = 139 + PNG_SIZE + COMBO_SIZE,
      MENU_ACTIONLIST_SIZE = 18
    ;

    // Define event groups
    static const Event::EventSet MiscEvents;
    static const Event::EventSet AudioVideoEvents;
    static const Event::EventSet StateEvents;
    static const Event::EventSet ConsoleEvents;
    static const Event::EventSet JoystickEvents;
    static const Event::EventSet PaddlesEvents;
    static const Event::EventSet KeyboardEvents;
    static const Event::EventSet ComboEvents;
    static const Event::EventSet DebugEvents;

    /**
      The following methods take care of assigning action mappings.
    */
    void setActionMappings(EventMode mode);
    void setDefaultKeymap(Event::Type, EventMode mode);
    void setDefaultJoymap(Event::Type, EventMode mode);
    void saveComboMapping();

    StringList getActionList(EventMode mode) const;
    StringList getActionList(const Event::EventSet& events, EventMode mode = EventMode::kEmulationMode) const;
    // returns the action array index of the index in the provided group
    int getEmulActionListIndex(int idx, const Event::EventSet& events) const;
    int getActionListIndex(int idx, Event::Group group) const;

  private:
    // Structure used for action menu items
    struct ActionList {
      Event::Type event;
      string action;
      string key;
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
    Event::Type myComboTable[COMBO_SIZE][EVENTS_PER_COMBO];

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
    static ActionList ourEmulActionList[EMUL_ACTIONLIST_SIZE];
    static ActionList ourMenuActionList[MENU_ACTIONLIST_SIZE];

    // Following constructors and assignment operators not supported
    EventHandler() = delete;
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;
};

#endif
