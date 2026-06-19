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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CONTROLLER_HXX
#define CONTROLLER_HXX

class Controller;
class ControllerLowLevel;
class Event;
class System;

#include <functional>

#include "bspf.hxx"
#include "Serializable.hxx"
#include "AnalogReadout.hxx"
#include "Event.hxx"

/**
  A controller is a device that plugs into either the left or right
  controller jack of the Video Computer System (VCS).  The pins of
  the controller jacks are mapped as follows:

                           -------------
                           \ 1 2 3 4 5 /
                            \ 6 7 8 9 /
                             ---------

            Left Controller             Right Controller

    pin 1   D4  PIA SWCHA               D0  PIA SWCHA
    pin 2   D5  PIA SWCHA               D1  PIA SWCHA
    pin 3   D6  PIA SWCHA               D2  PIA SWCHA
    pin 4   D7  PIA SWCHA               D3  PIA SWCHA
    pin 5   D7  TIA INPT1 (Dumped)      D7  TIA INPT3 (Dumped)
    pin 6   D7  TIA INPT4 (Latched)     D7  TIA INPT5 (Latched)
    pin 7   +5                          +5
    pin 8   GND                         GND
    pin 9   D7  TIA INPT0 (Dumped)      D7  TIA INPT2 (Dumped)

  Each of the pins connected to the PIA can be configured as an
  input or output pin.  The "dumped" TIA pins are used to charge
  a capacitor.  A potentiometer is sometimes connected to these
  pins for analog input.

  This is a base class for all controllers.  It provides a view
  of the controller from the perspective of the controller's jack.

  Digital pins may be either static or bound to an input event.
  A static pin (setPin) keeps a fixed value until changed.  A pin bound
  to an event (bindPin) instead reports that event's value sampled at
  the current position within the input window (via currentInputPos), so the
  pin can change part way through the window exactly as the user's input did.
  This is what lets a program reading a pin at several points within one
  window observe a press/release between reads; the values come from the
  Event transition schedule.

  @author  Bradford W. Mott
*/
class Controller : public Serializable
{
  /**
    Various classes that need special access to the underlying controller state
  */
  friend class M6532;
  friend class ControllerLowLevel;

  public:
    static constexpr int MIN_DIGITAL_DEADZONE = 0;
    static constexpr int MAX_DIGITAL_DEADZONE = 29;
    static constexpr int MIN_ANALOG_DEADZONE = 0;
    static constexpr int MAX_ANALOG_DEADZONE = 29;
    static constexpr int MIN_MOUSE_SENSE = 1;
    static constexpr int MAX_MOUSE_SENSE = 20;

    /**
      Enumeration of the controller jacks
    */
    enum class Jack: uInt8 { Left = 0, Right = 1, Left2 = 2, Right2 = 3 };

    /**
      Enumeration of the digital pins of a controller port
    */
    enum class DigitalPin: uInt8 { One, Two, Three, Four, Six };

    /**
      Enumeration of the analog pins of a controller port
    */
    enum class AnalogPin: uInt8 { Five, Nine };

    /**
      Enumeration of the controller types
    */
    enum class Type: uInt8
    {
      Unknown,
      AmigaMouse, AtariMouse, AtariVox, BoosterGrip, CompuMate,
      Driving, Genesis, Joystick, Keyboard, KidVid, MindLink,
      Paddles, PaddlesIAxis, PaddlesIAxDr, SaveKey, TrakBall,
      Lightgun, QuadTari, Joy2BPlus,
      LastType
    };

    /**
      Callback type for analog pin updates
    */
    using onAnalogPinUpdateCallback = std::function<void(AnalogPin)>;

    /**
      Callback type for general controller messages
    */
    using onMessageCallback = std::function<void(string_view)>;
    using onMessageCallbackForced = std::function<void(string_view, bool force)>;

  public:
    /**
      Create a new controller plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
      @param type   The type for this controller
    */
    Controller(Jack jack, const Event& event, const System& system,
               Type type);
    ~Controller() override = default;

    /**
      Returns the jack that this controller is plugged into.
    */
    Jack jack() const { return myJack; }

    /**
      Returns the type of this controller.
    */
    Type type() const { return myType; }

    /**
      Read the entire state of all digital pins for this controller.
      Note that this method must use the lower 4 bits, and zero the upper bits.

      @return The state of all digital pins
    */
    virtual uInt8 read();

    /**
      Read the value of the specified digital pin for this controller.

      @param pin The pin of the controller jack to read
      @return The state of the pin
    */
    virtual bool read(DigitalPin pin);

    /**
      Read the resistance at the specified analog pin for this controller.
      The returned value is the resistance measured in ohms.

      @param pin The pin of the controller jack to read
      @return The resistance at the specified pin
    */
    virtual AnalogReadout::Connection read(AnalogPin pin);

    /**
      Write the given value to the specified digital pin for this
      controller.  Writing is only allowed to the pins associated
      with the PIA.  Therefore you cannot write to pin six.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    virtual void write(DigitalPin pin, bool value) { }

    /**
      Called after *all* digital pins have been written on Port A.
      Most controllers don't do anything in this case.

      @param value  The entire contents of the SWCHA register
    */
    virtual void controlWrite(uInt8 value) { }

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    virtual void update() { }

    /**
      Returns the name of this controller.
    */
    virtual string name() const { return ""; }

    /**
      Whether the mouse is the appropriate input device for emulating this
      controller (trackballs, paddles, driving controllers, ...).  Used to
      decide whether to enable/grab the mouse.  This describes the input
      method only, NOT the controller's signalling: e.g. a trackball is
      digital behind the scenes (gray code on the digital pins) yet is
      mouse-driven, so returns true.  Specific controllers override as needed.
    */
    virtual bool usesMouse() const { return false; }

    /**
      Notification method invoked by the system after its reset method has
      been called.  It may be necessary to override this method for
      controllers that need to know a reset has occurred.
    */
    virtual void reset() { }

    /**
      Notification method invoked by the system indicating that the
      console is about to be destroyed.  It may be necessary to override
      this method for controllers that need cleanup before exiting.
    */
    virtual void close() { }

    /**
      Determines how this controller will treat values received from the
      X/Y axis and left/right buttons of the mouse.  Since not all controllers
      use the mouse the same way (or at all), it's up to the specific class to
      decide how to use this data.

      In the current implementation, the left button is tied to the X axis,
      and the right one tied to the Y axis.

      @param xtype  The controller to use for x-axis data
      @param xid    The controller ID to use for x-axis data (-1 for no id)
      @param ytype  The controller to use for y-axis data
      @param yid    The controller ID to use for y-axis data (-1 for no id)

      @return  Whether the controller supports using the mouse
    */
    virtual bool setMouseControl(
      Controller::Type xtype, int xid, Controller::Type ytype, int yid)
    { return false; }

    /**
      Returns more detailed information about this controller.
    */
    virtual string about(bool swappedPorts) const
    {
      return std::format("{} in {} port", name(),
          ((myJack == Jack::Left) ^ swappedPorts) ? "left" : "right");
    }

    /**
      Saves the current state of this controller to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out) const override;

    /**
      Loads the current state of this controller from the given Serializer.

      @param in The serializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    bool load(Serializer& in) override;

    /**
      Inject a callback to be notified on analog pin updates.
    */
    void setOnAnalogPinUpdateCallback(onAnalogPinUpdateCallback callback) {
      myOnAnalogPinUpdateCallback = std::move(callback);
    }

    /**
      Returns the display name of the given controller type
    */
    static string_view getName(Type type);

    /**
      Returns the property name of the given controller type
    */
    static string_view getPropName(Type type);

    /**
      Returns the controller type of the given property name
    */
    static Type getType(string_view propName);

    /**
      Sets the dead zone amount for real analog joysticks.

      @param deadZone Value from 0 to 29
    */
    static void setDigitalDeadZone(int deadZone) {
      DIGITAL_DEAD_ZONE = digitalDeadZoneValue(deadZone);
    }

    /**
      Sets the dead zone for analog paddles.

      @param deadZone Value from 0 to 29
    */
    static void setAnalogDeadZone(int deadZone) {
      ANALOG_DEAD_ZONE = analogDeadZoneValue(deadZone);
    }

    /**
      Retrieves the effective digital dead zone value
    */
    static constexpr int digitalDeadZoneValue(int deadZone) {
      deadZone = BSPF::clamp(deadZone, MIN_DIGITAL_DEADZONE, MAX_DIGITAL_DEADZONE);
      return 3200 + deadZone * 1000;
    }

    /**
      Retrieves the effective analog dead zone value
    */
    static constexpr int analogDeadZoneValue(int deadZone) {
      deadZone = BSPF::clamp(deadZone, MIN_ANALOG_DEADZONE, MAX_ANALOG_DEADZONE);
      return deadZone * ((32768 / 2 + MAX_DIGITAL_DEADZONE / 2) / MAX_DIGITAL_DEADZONE);
    }

    static int digitalDeadZone() { return DIGITAL_DEAD_ZONE; }
    static int analogDeadZone()  { return ANALOG_DEAD_ZONE;  }

    /**
      Sets the sensitivity for analog emulation movement
      using a mouse.

      @param sensitivity  Value from 1 to MAX_MOUSE_SENSE, with larger
                          values causing more movement
    */
    static void setMouseSensitivity(int sensitivity) {
      MOUSE_SENSITIVITY = BSPF::clamp(sensitivity, MIN_MOUSE_SENSE, MAX_MOUSE_SENSE);
    }

    /**
      Set auto fire state.

      @param enable The new autofire state
    */
    static void setAutoFire(bool enable) {
      AUTO_FIRE = enable;
    }

    /**
      Sets the auto fire rate. 0 disables auto fire.

      @param rate   Auto fire rate (0..30/25) in Hz
      @param isNTSC NTSC or PAL frame rate
    */
    static void setAutoFireRate(int rate, bool isNTSC = true) {
      rate = BSPF::clamp(rate, 0, isNTSC ? 30 : 25);
      AUTO_FIRE_RATE = 32 * 1024 * rate / (isNTSC ? 60 : 50);
    }

  protected:
    /**
      Derived classes *must* use these accessor/mutator methods.
      The read/write methods above are meant to be used at a higher level.
    */
    bool setPin(DigitalPin pin, bool value) {
      // A static value overrides any event binding on this pin
      myDigitalPinEvent[static_cast<int>(pin)].fill(Event::NoType);
      return myDigitalPinState[static_cast<int>(pin)] = value;
    }
    bool getPin(DigitalPin pin) const {
      return myDigitalPinState[static_cast<int>(pin)];
    }

    /**
      Bind a digital pin to one or more input events so that read() reflects
      their value at the current position within the input window (active low:
      the pin reads as pressed when any bound event is active).  This lets the
      pin change within the window as the user's input did, instead of being
      latched once per window.  Several events cover a button with multiple
      sources, e.g. a fire button that the mouse buttons also trigger.
    */
    bool bindPin(DigitalPin pin, SpanOf<Event::Type> events) {
      auto& bound = myDigitalPinEvent[static_cast<int>(pin)];
      bound.fill(Event::NoType);

      bool pressed = false;
      size_t i = 0;
      for(const Event::Type event: events)
      {
        bound[i++] = event;
        pressed |= myEvent.get(event) != 0;
      }

      // Keep the static state current for getPin()/debugger display
      return myDigitalPinState[static_cast<int>(pin)] = !pressed;
    }

    bool bindPin(DigitalPin pin, Event::Type event) {
      return bindPin(pin, SpanOf<Event::Type>{&event, 1});
    }

    void setPin(AnalogPin pin, AnalogReadout::Connection value) {
      myAnalogPinValue[static_cast<int>(pin)] = value;
      if(myOnAnalogPinUpdateCallback)
        myOnAnalogPinUpdateCallback(pin);
    }

    AnalogReadout::Connection getPin(AnalogPin pin) const {
      return myAnalogPinValue[static_cast<int>(pin)];
    }

    void resetDigitalPins() {
      setPin(DigitalPin::One,   true);
      setPin(DigitalPin::Two,   true);
      setPin(DigitalPin::Three, true);
      setPin(DigitalPin::Four,  true);
      setPin(DigitalPin::Six,   true);
    }

    void resetAnalogPins() {
      setPin(AnalogPin::Five, AnalogReadout::disconnect());
      setPin(AnalogPin::Nine, AnalogReadout::disconnect());
    }

    /**
      Checks for the next auto fire event.

      @param pressed  True if the fire button is currently pressed
      @return  The result of the auto fire event check
    */
    bool getAutoFireState(bool pressed) { return autoFireCheck(pressed, myFireDelay); }

    /**
      Whether auto fire is currently active.  When it is, the fire button
      generates its own timing and is set via setPin() rather than bound to an
      event for replay within the input window.
    */
    static bool autoFireActive() { return AUTO_FIRE && AUTO_FIRE_RATE; }

    /**
      Drive a digital "fire" button from one or more events (the fire event
      plus any mouse buttons currently mapped to it).  Normally the pin is
      bound for replay within the input window, so each source can change the
      button mid-window (see bindPin); while autofire is generating its own
      timing the pin can't be event-bound and is set statically instead.
      'fireDelay' is the controller's per-pin autofire counter.
    */
    void updateFireButton(DigitalPin pin, int& fireDelay,
                          SpanOf<Event::Type> events) {
      if(autoFireActive())
      {
        bool pressed = false;
        for(const Event::Type event: events)
          pressed |= myEvent.get(event) != 0;
        setPin(pin, !autoFireCheck(pressed, fireDelay));
      }
      else
        bindPin(pin, events);
    }

    /**
      The current position within the input window, in CPU cycles from its
      start, used to sample event-bound pins at read time.  Sourced from
      System::cycles() via Event.
    */
    uInt64 currentInputPos() const;

  protected:
    /// Up to this many input events may be bound to one digital pin (e.g. a
    /// fire button also driven by both mouse buttons)
    static constexpr size_t MAX_PIN_EVENTS = 3;

    /// Specifies which jack the controller is plugged in
    const Jack myJack;

    /// Reference to the event object this controller uses
    const Event& myEvent;

    /// Pointer to the System object (used for timing purposes)
    const System& mySystem;

    /// Specifies which type of controller this is (defined by child classes)
    const Type myType{Type::Unknown};

    /// The callback that is dispatched whenver an analog pin has changed
    onAnalogPinUpdateCallback myOnAnalogPinUpdateCallback{nullptr};

    /// Defines the dead zone of analog joysticks for digital Atari controllers
    inline static int DIGITAL_DEAD_ZONE{3200};

    /// Defines the dead zone of analog joysticks for analog Atari controllers
    inline static int ANALOG_DEAD_ZONE{0};

    inline static int MOUSE_SENSITIVITY{-1};

    /// Defines the state of auto fire
    inline static bool AUTO_FIRE{false};

    /// Defines the speed of auto fire
    inline static int AUTO_FIRE_RATE{0};

    /// Delay[frames] until the next fire event
    int myFireDelay{0};
    int myFireDelayP1{0}; // required for paddles only

  private:
    static bool autoFireCheck(bool pressed, int& delay)
    {
      if(AUTO_FIRE && AUTO_FIRE_RATE && pressed)
      {
        delay -= AUTO_FIRE_RATE;
        if(delay <= 0)
          delay += 32 * 1024;
        return delay > 16 * 1024;
      }
      delay = 0;
      return pressed;
    }

    /// The boolean value on each digital pin
    std::array<bool, 5> myDigitalPinState{true, true, true, true, true};

    /// Input events bound to each digital pin, replayed within the input
    /// window by read() (active low, OR'd over the slots); unused slots and a
    /// fully static pin are NoType
    BSPF::array2D<Event::Type, 5, MAX_PIN_EVENTS> myDigitalPinEvent{};

    /// The analog value on each analog pin
    std::array<AnalogReadout::Connection, 2>
      myAnalogPinValue{AnalogReadout::disconnect(), AnalogReadout::disconnect()};

    // Must match Controller::Type enum order; size enforced by static_assert below
    struct ControllerInfo {
      string_view name;
      string_view propName;
    };
    static constexpr auto CONTROLLER_INFO = std::to_array<ControllerInfo>({
        { "Unknown",       "AUTO"          },
        { "Amiga mouse",   "AMIGAMOUSE"    },
        { "Atari mouse",   "ATARIMOUSE"    },
        { "AtariVox",      "ATARIVOX"      },
        { "Booster Grip",  "BOOSTERGRIP"   },
        { "CompuMate",     "COMPUMATE"     },
        { "Driving",       "DRIVING"       },
        { "Sega Genesis",  "GENESIS"       },
        { "Joystick",      "JOYSTICK"      },
        { "Keyboard",      "KEYBOARD"      },
        { "Kid Vid",       "KIDVID"        },
        { "MindLink",      "MINDLINK"      },
        { "Paddles",       "PADDLES"       },
        { "Paddles_IAxis", "PADDLES_IAXIS" },
        { "Paddles_IAxDr", "PADDLES_IAXDR" },
        { "SaveKey",       "SAVEKEY"       },
        { "Trak-Ball",     "TRAKBALL"      },
        { "Light Gun",     "LIGHTGUN"      },
        { "QuadTari",      "QUADTARI"      },
        { "Joy 2B+",       "JOY_2B+"       }
    });
    static_assert(CONTROLLER_INFO.size() == static_cast<size_t>(Type::LastType),
        "CONTROLLER_INFO must have an entry for each Controller::Type");

  private:
    // Following constructors and assignment operators not supported
    Controller() = delete;
    Controller(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller& operator=(Controller&&) = delete;
};

#endif  // CONTROLLER_HXX
