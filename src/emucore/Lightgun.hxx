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

#ifndef LIGHTGUN_HXX
#define LIGHTGUN_HXX

/**
  This class handles the lightgun controller

  @author  Thomas Jentzsch
*/

class Lightgun : public Controller
{
public:
  /**
    Create a new lightgun controller plugged into the specified jack

    @param jack   The jack the controller is plugged into
    @param event  The event object to use for events
    @param system The system using this controller
  */
  Lightgun(Jack jack, const Event& event, const System& system, const FrameBuffer& frameBuffer);
  virtual ~Lightgun() = default;

public:
  using Controller::read;

  /**
    Read the value of the specified digital pin for this controller.

    @param pin The pin of the controller jack to read
    @return The state of the pin
  */
  bool read(DigitalPin pin) override;

  /**
    Update the entire digital and analog pin state according to the
    events currently set.
  */
  void update() override;

  /**
    Returns the name of this controller.
  */
  string name() const override { return "Lightgun"; }

private:
  const FrameBuffer& myFrameBuffer;

  static constexpr Int32 X_OFS = -21;
  static constexpr Int32 Y_OFS =   5;

private:
  // Following constructors and assignment operators not supported
  Lightgun() = delete;
  Lightgun(const Lightgun&) = delete;
  Lightgun(Lightgun&&) = delete;
  Lightgun& operator=(const Lightgun&) = delete;
  Lightgun& operator=(Lightgun&&) = delete;
};

#endif
