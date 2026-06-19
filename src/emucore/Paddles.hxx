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

#ifndef PADDLES_HXX
#define PADDLES_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  The standard Atari 2600 pair of paddle controllers.

  Paddle input is handled by four cooperating layers. This class (Paddles)
  interprets raw input events — analog axis, mouse, or keyboard/digital — and
  outputs a normalized [0,1] resistance via setPin(AnalogPin, Connection).
  AnalogReadout owns the RC circuit physics and converts that resistance into
  the INPT0-3 comparator output. TIA is the wiring layer that connects Paddles
  output to AnalogReadout input and exposes INPT0-3 to the CPU. LatchedInput
  handles INPT4/5 (fire buttons) independently. Paddles and AnalogReadout are
  fully decoupled: they communicate only through the AnalogReadout::Connection
  type.

  @author  Bradford W. Mott
*/
class Paddles : public Controller
{
  public:
    /**
      Create a new pair of paddle controllers plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller

      @param swappaddle Whether to swap the paddles plugged into this jack
      @param swapaxis   Whether to swap the axis on the paddle (x <-> y)
      @param swapdir    Whether to swap the direction for which an axis
                        causes movement (lesser axis values cause paddle
                        resistance to decrease instead of increase)
    */
    Paddles(Jack jack, const Event& event, const System& system,
            bool swappaddle, bool swapaxis, bool swapdir, bool altmap = false);
    ~Paddles() override = default;

  public:
    static constexpr int ANALOG_MIN_VALUE = -32768;
    static constexpr int ANALOG_MAX_VALUE = 32767;
    static constexpr int ANALOG_RANGE = ANALOG_MAX_VALUE - ANALOG_MIN_VALUE + 1;
    static constexpr float BASE_ANALOG_SENSE = 0.148643628F;
    static constexpr int MIN_ANALOG_SENSE = 0;
    static constexpr int MAX_ANALOG_SENSE = 30;
    static constexpr int MIN_ANALOG_LINEARITY = 25;
    static constexpr int MAX_ANALOG_LINEARITY = 100;
    static constexpr int MIN_ANALOG_CENTER = -10;
    static constexpr int MAX_ANALOG_CENTER = 30;
    static constexpr int MIN_DIGITAL_SENSE = 1;
    static constexpr int MAX_DIGITAL_SENSE = 20;
    static constexpr int MIN_DEJITTER = 0;
    static constexpr int MAX_DEJITTER = 10;
    static constexpr int MIN_MOUSE_RANGE = 1;
    static constexpr int MAX_MOUSE_RANGE = 100;

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update() override;

    /**
      Returns the name of this controller.
    */
    string name() const override { return "Paddles"; }

    /**
      Paddles are driven by the mouse (one analog axis each).
    */
    bool usesMouse() const override { return true; }

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
    bool setMouseControl(Controller::Type xtype, int xid,
                         Controller::Type ytype, int yid) override;

    /**
      Sets the x-center for analog paddles.

      @param xcenter  Value from -10 to 30, representing the center offset/860
    */
    static void setAnalogXCenter(int xcenter);

    /**
      Sets the y-center for analog paddles.

      @param ycenter  Value from -10 to 30, representing the center offset/860
    */
    static void setAnalogYCenter(int ycenter);

    /**
      Sets the linearity of analog paddles.

      @param linearity Value from 25 to 100
    */
    static void setAnalogLinearity(int linearity);

    /**
      Sets the sensitivity for analog paddles.

      @param sensitivity  Value from 0 to 30, where 20 equals 1
      @return  Resulting sensitivity
    */
    static float setAnalogSensitivity(int sensitivity);

    static float analogSensitivityValue(int sensitivity);


    /**
      @param strength  Value from 0 to 10
    */
    static void setDejitterBase(int strength);

    /**
      @param strength  Value from 0 to 10
    */
    static void setDejitterDiff(int strength);

    /**
      Sets the sensitivity for digital emulation of paddle movement.
      This is only used for *digital* events (ie, buttons or keys,
      or digital joystick axis events); Stelladaptors or the mouse are
      not modified.

      @param sensitivity  Value from 1 to MAX_DIGITAL_SENSE, with larger
                          values causing more movement
    */
    static void setDigitalSensitivity(int sensitivity);

    /**
      Sets the maximum upper range for digital/mouse emulation of paddle
      movement (ie, a value of 50 means to only use 50% of the possible
      range of movement).  Note that this specfically does not apply to
      Stelladaptor-like devices, which uses an absolute value range.

      @param range  Value from 1 to 100, representing the percentage
                    of the range to use
    */
    static void setDigitalPaddleRange(int range);


  private:
    // Upper limit of the normalized position range [0.0, POSITION_LIMIT].
    // Reduced below 1.0 by setDigitalPaddleRange() to shrink effective travel.
    inline static float POSITION_LIMIT = 1.F;

    // Pre-compute the events we care about based on given port
    // This will eliminate test for left or right port in update()
    Event::Type myAAxisValue, myBAxisValue,
                myADecEvent, myAIncEvent,
                myBDecEvent, myBIncEvent,
                myAFireEvent, myAButton1Event, myAButton2Event,
                myBFireEvent,
                myAxisMouseMotion;

    // The following are used for the various mouse-axis modes
    int myMPaddleID{-1};                    // paddle to emulate in 'automatic' mode
    int myMPaddleIDX{-1}, myMPaddleIDY{-1}; // paddles to emulate in 'specific axis' mode

    float myMouseScale{0.F};
    bool myKeyRepeatA{false}, myKeyRepeatB{false};
    float myPaddleRepeatA{0.F}, myPaddleRepeatB{0.F};
    std::array<float, 2> myPosition{0.5F, 0.5F};
    int myLastAxisX{0}, myLastAxisY{0};
    int myAxisDigitalZero{0}, myAxisDigitalOne{0};

    inline static int XCENTER = 0;
    inline static int YCENTER = 0;
    inline static float SENSITIVITY = 1.F;
    inline static float LINEARITY = 1.F;

    inline static int DIGITAL_SENSITIVITY = -1;
    inline static float DIGITAL_STEP = -1.F;  // full-speed normalized position step per frame
    inline static int DEJITTER_BASE = 0;
    inline static int DEJITTER_DIFF = 0;

    static void conditionAxisInput(int lastAxis, int& newAxis);

    void updateA();
    void updateB();

    /**
      Update the axes pin state according to the events currently set.
    */
    bool updateAnalogAxesA();
    bool updateAnalogAxesB();

    /**
      Update the paddle position from mouse motion and append the mouse buttons
      mapped to this paddle's fire button to 'fire' (at index 'n', advancing
      it) so they can be bound for replay within the input window.
    */
    void updateMouseA(std::array<Event::Type, MAX_PIN_EVENTS>& fire, size_t& n);
    void updateMouseB(std::array<Event::Type, MAX_PIN_EVENTS>& fire, size_t& n);

    /**
      Update the axes pin state according to the keyboard events currently set.
    */
    void updateDigitalAxesA();
    void updateDigitalAxesB();

  private:
    // Following constructors and assignment operators not supported
    Paddles() = delete;
    Paddles(const Paddles&) = delete;
    Paddles(Paddles&&) = delete;
    Paddles& operator=(const Paddles&) = delete;
    Paddles& operator=(Paddles&&) = delete;
};

#endif  // PADDLES_HXX
