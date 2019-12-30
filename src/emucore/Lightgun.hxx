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

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  This class handles the lighgun controller

  @author  Thomas Jentzsch
*/

class Lightgun : public Controller
{
public:
  /**
    Create a new pair of paddle controllers plugged into the specified jack

    @param jack   The jack the controller is plugged into
    @param event  The event object to use for events
    @param system The system using this controller
  */
  Lightgun(Jack jack, const Event& event, const System& system);
  virtual ~Lightgun() = default;

public:
  static constexpr int MAX_MOUSE_SENSE = 20;

  /**
    Update the entire digital and analog pin state according to the
    events currently set.
  */
  void update() override;

  /**
    Returns the name of this controller.
  */
  string name() const override { return "Lightgun"; }

  /**
    Answers whether the controller is intrinsically an analog controller.
  */
  bool isAnalog() const override { return true; }

  /**
    Sets the sensitivity for analog emulation of lightgun movement
    using a mouse.

    @param sensitivity  Value from 1 to MAX_MOUSE_SENSE, with larger
                        values causing more movement
  */
  static void setMouseSensitivity(int sensitivity);

private:
  // Pre-compute the events we care about based on given port
  // This will eliminate test for left or right port in update()
  Event::Type myPosValue, myFireEvent;

  int myCharge, myLastCharge;

  static constexpr int TRIGMIN = 1;
  static constexpr int TRIGMAX = 4096;

  static int MOUSE_SENSITIVITY;

  // Lookup table for associating paddle buttons with controller pins
  static const Controller::DigitalPin ourButtonPin;

private:
  // Following constructors and assignment operators not supported
  Lightgun() = delete;
  Lightgun(const Lightgun&) = delete;
  Lightgun(Lightgun&&) = delete;
  Lightgun& operator=(const Lightgun&) = delete;
  Lightgun& operator=(Lightgun&&) = delete;
};

#endif
