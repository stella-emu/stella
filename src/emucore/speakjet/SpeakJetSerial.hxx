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

#ifndef SPEAKJET_SERIAL_HXX
#define SPEAKJET_SERIAL_HXX

class SerialPort;

#include "SpeakJet.hxx"
#include "SpeakJetBackend.hxx"

/**
  Speaks through a real SpeakJet chip, in an AtariVox plugged into one of
  the host's serial ports.  The command stream is passed through untouched,
  and the chip itself reports whether it can accept more data.  A decoder
  follows the stream too, only so a state records the parameters the chip
  was given.

  @author  B. Watson, Stephen Anthony
*/
class SpeakJetSerial : public SpeakJetBackend, public SpeakJet::Sink
{
  public:
    /**
      Create a backend using the given host serial port.

      @param portname  Name of the serial port to use
    */
    explicit SpeakJetSerial(string_view portname);
    ~SpeakJetSerial() override;

  public:
    /**
      Whether the port actually opened.  A configured port that is absent,
      typically because the device has been unplugged, leaves this false so
      the caller can fall back to synthesizing the speech.
    */
    bool isOpen() const { return myIsOpen; }

    void write(uInt8 code) override;
    bool ready() override;
    string about() const override { return myAboutString; }

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  public:
    // The chip makes the sound; the decoder only keeps track of the stream
    void play(const SpeakJet::Utterance&) override { }
    void pause(uInt32) override { }
    void delay(uInt32) override { }
    void setParam(SpeakJet::Param, uInt8) override { }

  private:
    // Instance of a real serial port on the system
    // Assuming there's a real AtariVox attached, we can send SpeakJet
    // bytes directly to it
    unique_ptr<SerialPort> mySerialPort;

    // Follows the bytes sent to the chip
    SpeakJet myDecoder;

    // When using software flow control, assume the device starts in READY mode
    bool myReadyStateSoftFlow{true};

    // False when the configured port could not be opened
    bool myIsOpen{false};

    // Some USB-Serial adaptors send the CTS signal inverted; we detect
    // that when opening the port, and flip the signal when necessary
    bool myCTSFlip{false};

    // Holds information concerning serial port usage
    string myAboutString;

  private:
    // Following constructors and assignment operators not supported
    SpeakJetSerial() = delete;
    SpeakJetSerial(const SpeakJetSerial&) = delete;
    SpeakJetSerial(SpeakJetSerial&&) = delete;
    SpeakJetSerial& operator=(const SpeakJetSerial&) = delete;
    SpeakJetSerial& operator=(SpeakJetSerial&&) = delete;
};

#endif  // SPEAKJET_SERIAL_HXX
