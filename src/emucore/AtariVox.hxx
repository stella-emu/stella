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

#ifndef ATARIVOX_HXX
#define ATARIVOX_HXX

class OSystem;
class MT24LC256;
class SpeakJetBackend;
class FSNode;

#include "Control.hxx"

/**
  Richard Hutchinson's AtariVox "controller": a 32KB EEPROM accessed over
  I2C, plus a SpeakJet speech synthesizer.  The two are independent; the
  EEPROM is the same part the SaveKey uses, on the same pins.

  This code owes a great debt to Alex Herbert's AtariVox documentation and
  driver code.

  @author  B. Watson, Stephen Anthony
*/
class AtariVox : public Controller
{
  public:
    /**
      Create a new AtariVox controller plugged into the specified jack

      @param jack       The jack the controller is plugged into
      @param event      The event object to use for events
      @param osystem    The OSystem object to use
      @param system     The system using this controller
      @param portname   Serial port of a real AtariVox; speech is synthesized
                        in software when this is empty
      @param eepromfile The file containing the EEPROM data
      @param callback   Called to pass messages back to the parent controller
    */
    AtariVox(Jack jack, const Event& event, const OSystem& osystem,
             const System& system, const string& portname,
             const FSNode& eepromfile, const onMessageCallback& callback);
    ~AtariVox() override;

  public:
    using Controller::read;

    /**
      Read the value of the specified digital pin for this controller.

      @param pin The pin of the controller jack to read
      @return The state of the pin
    */
    bool read(DigitalPin pin) override;

    /**
      Write the given value to the specified digital pin for this
      controller.  Writing is only allowed to the pins associated
      with the PIA.  Therefore you cannot write to pin six.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    void write(DigitalPin pin, bool value) override;

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update() override;

    /**
      Returns the name of this controller.
    */
    string name() const override { return "AtariVox"; }

    /**
      Notification method invoked by the system after its reset method has
      been called.  It may be necessary to override this method for
      controllers that need to know a reset has occurred.
    */
    void reset() override;

    /**
      Force the EEPROM object to cleanup
    */
    void close() override;

    /** Erase the pages used by the current ROM to known state ($FF) */
    void eraseCurrent();

    /** Returns true if the page is used by the current ROM */
    bool isPageUsed(uInt32 page) const;

    string about(bool swappedPorts) const override;

    /**
      Save/load the controller state: the EEPROM's in-flight I2C transaction,
      then the SpeakJet shift state, so a state saved mid-byte doesn't drop
      the partially-shifted speech byte.
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
   void clockDataIn(bool value);

  private:
    // The 24LC256 EEPROM, the same part the SaveKey uses
    unique_ptr<MT24LC256> myEEPROM;

    // Produces the speech: either a real SpeakJet chip reached over a
    // serial port, or a software synthesizer
    unique_ptr<SpeakJetBackend> myBackend;

    // How many bits have been shifted into the shift register?
    uInt8 myShiftCount{0};

    // Shift register. Data comes in serially:
    // 1 start bit, always 0
    // 8 data bits, LSB first
    // 1 stop bit, always 1
    uInt16 myShiftRegister{0};

    // When did the last data write start, in CPU cycles?
    // The real SpeakJet chip reads data at 19200 bits/sec. Alex's
    // driver code sends data at 62 CPU cycles per bit, which is
    // "close enough".
    uInt64 myLastDataWriteCycle{0};

  private:
    // Following constructors and assignment operators not supported
    AtariVox() = delete;
    AtariVox(const AtariVox&) = delete;
    AtariVox(AtariVox&&) = delete;
    AtariVox& operator=(const AtariVox&) = delete;
    AtariVox& operator=(AtariVox&&) = delete;
};

#endif  // ATARIVOX_HXX
