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

#ifndef SPEAKJET_BACKEND_HXX
#define SPEAKJET_BACKEND_HXX

#include "bspf.hxx"
#include "Serializable.hxx"

/**
  Abstract interface to whatever produces the AtariVox's speech.

  The AtariVox recovers a byte stream from the controller pins and passes it
  here.  An implementation either forwards those bytes to a real SpeakJet
  chip, or synthesizes the speech itself.  Every implementation saves the
  same state layout, since a state may be loaded with a different backend
  than it was saved with.

  @author  Stephen Anthony
*/
class SpeakJetBackend : public Serializable
{
  public:
    SpeakJetBackend() = default;
    ~SpeakJetBackend() override = default;

    /**
      Send one byte of the SpeakJet command stream to the device.

      @param code  The byte to send
    */
    virtual void write(uInt8 code) = 0;

    /**
      Whether the device can accept more data; this drives the READY pin.

      @return  True if more data can be sent, else false
    */
    virtual bool ready() = 0;

    /**
      Notification that the console has been reset.
    */
    virtual void reset() { }

    /**
      Called once per frame, for a backend that holds output back waiting to
      see what comes next.
    */
    virtual void update() { }

    /**
      Describes this backend, for the controller's 'about' text.

      @return  Text appended to the AtariVox description
    */
    virtual string about() const = 0;

  private:
    // Following constructors and assignment operators not supported
    SpeakJetBackend(const SpeakJetBackend&) = delete;
    SpeakJetBackend(SpeakJetBackend&&) = delete;
    SpeakJetBackend& operator=(const SpeakJetBackend&) = delete;
    SpeakJetBackend& operator=(SpeakJetBackend&&) = delete;
};

#endif  // SPEAKJET_BACKEND_HXX
