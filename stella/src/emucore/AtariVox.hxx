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
// $Id: AtariVox.hxx,v 1.11 2008-04-11 17:56:34 stephena Exp $
//============================================================================

#ifndef ATARIVOX_HXX
#define ATARIVOX_HXX

class SpeakJet;

#include "Control.hxx"
#include "SerialPort.hxx"

/**
  Richard Hutchinson's AtariVox "controller": A speech synthesizer and
  storage device.

  This code owes a great debt to Alex Herbert's AtariVox documentation and
  driver code.

  @author  B. Watson
  @version $Id: AtariVox.hxx,v 1.11 2008-04-11 17:56:34 stephena Exp $
*/
class AtariVox : public Controller
{
  public:
    /**
      Create a new AtariVox controller plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param port   The serial port object
      @param device Name of the port used for reading and writing
    */
    AtariVox(Jack jack, const Event& event, const SerialPort& port,
             const string& device);

    /**
      Destructor
    */
    virtual ~AtariVox();

  public:
    /**
      Write the given value to the specified digital pin for this
      controller.  Writing is only allowed to the pins associated
      with the PIA.  Therefore you cannot write to pin six.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    virtual void write(DigitalPin pin, bool value);

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    virtual void update();

    virtual string about() const;

#ifdef SPEAKJET_EMULATION
    SpeakJet* getSpeakJet() { return mySpeakJet; }
#endif

  private:
   void clockDataIn(bool value);
   void shiftIn(bool value);

  private:
    // How far off (in CPU cycles) can each write occur from when it's
    // supposed to happen? Eventually, this will become a user-settable
    // property... or it may turn out to be unnecessary.
    enum { TIMING_SLOP = 0 };

    // Instance of an real serial port on the system
    // Assuming there's a real AtariVox attached, we can send SpeakJet
    // bytes directly to it
    SerialPort* mySerialPort;

#ifdef SPEAKJET_EMULATION
    // Instance of SpeakJet which will actually do the talking for us.
    SpeakJet *mySpeakJet;
#endif

    // State of the output pins
    uInt8 myPinState;

    // How many bits have been shifted into the shift register?
    uInt8 myShiftCount;

    // Shift register. Data comes in serially:
    // 1 start bit, always 0
    // 8 data bits, LSB first
    // 1 stop bit, always 1
    uInt16 myShiftRegister;

    // When did the last data write start, in CPU cycles?
    // The real SpeakJet chip reads data at 19200 bits/sec. Alex's
    // driver code sends data at 62 CPU cycles per bit, which is
    // "close enough".
    uInt32 myLastDataWriteCycle;

    // Holds information concerning serial port usage
    string myAboutString;
};

#endif
