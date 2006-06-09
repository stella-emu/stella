
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
// $Id: AtariVox.hxx,v 1.1 2006-06-09 02:45:11 urchlay Exp $
//============================================================================

#ifndef ATARIVOX_HXX
#define ATARIVOX_HXX

#include "bspf.hxx"
#include "Control.hxx"

/**
  Alex Herbert's AtariVox "controller": A speech synthesizer and
  storage device.

  @author  B. Watson
  @version $Id: AtariVox.hxx,v 1.1 2006-06-09 02:45:11 urchlay Exp $
*/
class AtariVox : public Controller
{
  public:
    /**
      Create a new AtariVox controller plugged into the specified jack

      @param jack The jack the controller is plugged into
      @param event The event object to use for events
    */
    AtariVox(Jack jack, const Event& event);

    /**
      Destructor
    */
    virtual ~AtariVox();

  public:
    /**
      Read the value of the specified digital pin for this controller.

      @param pin The pin of the controller jack to read
      @return The state of the pin
    */
    virtual bool read(DigitalPin pin);

    /**
      Read the resistance at the specified analog pin for this controller.
      The returned value is the resistance measured in ohms.

		The AtariVox doesn't use the analog pins.

      @param pin The pin of the controller jack to read
      @return The resistance at the specified pin
    */
    virtual Int32 read(AnalogPin pin);

    /**
      Write the given value to the specified digital pin for this
      controller.  Writing is only allowed to the pins associated
      with the PIA.  Therefore you cannot write to pin six.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    virtual void write(DigitalPin pin, bool value);

  private:
    // State of the output pins
    uInt8 myPinState;

	 // Shift register. Data comes in serially:
	 // 1 start bit, always 0
	 // 8 data bits, LSB first
	 // 1 stop bit, always 0
	 uInt16 myShiftRegister;

	 // How many bits have been shifted into the shift register?
	 uInt8 myShiftCount;
};

#endif
