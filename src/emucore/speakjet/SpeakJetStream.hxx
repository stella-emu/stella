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

#ifndef SPEAKJET_STREAM_HXX
#define SPEAKJET_STREAM_HXX

class OSystem;
class System;

#include <deque>

#include "SpeakJet.hxx"
#include "SpeakJetBackend.hxx"
#include "SpeakJetMSA.hxx"

/**
  Speaks without a real AtariVox attached, by running the chip itself: its
  sequencer and synthesizer make every sound.

  The chip runs in emulated time.  Each frame it makes as many samples as
  the CPU cycles since the last frame are worth, taking the next command
  from its 64-byte input buffer whenever it has finished the last, and they
  are queued as continuous PCM.  A command stays in the buffer until it has
  been carried out, so READY is the chip's Buffer Half Full line, as on the
  real AtariVox.

  The structure follows Andrew Davie's speakjet library.

  @author  Stephen Anthony
*/
class SpeakJetStream : public SpeakJetBackend
{
  public:
    /**
      @param osystem   The OSystem object to use
      @param system    The system the AtariVox is attached to
      @param deadPort  A serial port that was configured but could not be
                       opened, named in about() so the fallback is visible
    */
    SpeakJetStream(const OSystem& osystem, const System& system,
                   string_view deadPort = "");
    ~SpeakJetStream() override;

  public:
    void write(uInt8 code) override;
    bool ready() override;
    void reset() override;
    void update() override;
    string about() const override { return myAboutString; }

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    /**
      One command in the input buffer, with the value a two-byte one takes.
    */
    struct Command
    {
      uInt8 code{0};
      uInt8 data{0};
      uInt8 size{1};
    };

    /**
      Drop all speech, queued or being said; the controls stay.
    */
    void silence();

    /**
      Whether the chip has nothing to say, so time can pass unrendered.
    */
    bool idle() const;

    /**
      Carry out commands until one has something to say or none are left.
    */
    void execute();

    /**
      The next sound the chip can see coming, for the sequencer.
    */
    SpeakJetMSA::Next lookahead() const;

    /**
      How many bytes a command takes, its value included.
    */
    static uInt8 sizeOf(uInt8 code);

    /**
      The CPU clock to pace by.
    */
    uInt64 cyclesPerSecond() const;

  private:
    const OSystem& myOSystem;
    const System& mySystem;

    SpeakJetMSA myMSA;
    SpeakJet myDecoder;

    // The input buffer: its commands, the bytes held (the one being carried
    // out included) and that one's size, and a two-byte command's first half
    std::deque<Command> myQueue;
    size_t myBuffered{0};
    size_t myExecuting{0};
    int myHalfCode{-1};

    // The CPU cycle count at the last update, and the fraction of a sample
    // those cycles were worth that has not been made yet
    uInt64 myLastCycle{0};
    double mySampleFraction{0.0};

    // The chip has been silent, so the next speech gets its cushion back first
    bool myStarting{true};

    vector<Int16> myPCM;
    string myAboutString;

  private:
    SpeakJetStream() = delete;
    SpeakJetStream(const SpeakJetStream&) = delete;
    SpeakJetStream(SpeakJetStream&&) = delete;
    SpeakJetStream& operator=(const SpeakJetStream&) = delete;
    SpeakJetStream& operator=(SpeakJetStream&&) = delete;
};

#endif  // SPEAKJET_STREAM_HXX
