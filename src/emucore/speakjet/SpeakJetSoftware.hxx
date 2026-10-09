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

#ifndef SPEAKJET_SOFTWARE_HXX
#define SPEAKJET_SOFTWARE_HXX

class OSystem;

#include <cstdlib>

#include "SpeakJet.hxx"
#include "SpeakJetBackend.hxx"
#include "SpeakJetSamples.hxx"
#include "SpeakJetVoice.hxx"

/**
  Speaks without a real AtariVox attached.  Voiced sounds are made as the
  chip makes them, the rest are played from recordings of the chip, and
  each is joined to the next as the chip joins them, then handed to the
  sound system as continuous PCM.

  @author  Stephen Anthony
*/
class SpeakJetSoftware : public SpeakJetBackend, public SpeakJet::Sink
{
  public:
    /**
      @param osystem   The OSystem object to use
      @param deadPort  A serial port that was configured but could not be
                       opened, named in about() so the fallback is visible
    */
    explicit SpeakJetSoftware(const OSystem& osystem, string_view deadPort = "");
    ~SpeakJetSoftware() override;

  public:
    // Any byte means the phrase goes on, even a command between two sounds
    void write(uInt8 code) override { myIdleFrames = 0; myDecoder.write(code); }
    bool ready() override;
    void reset() override;
    void update() override;
    string about() const override { return myAboutString; }

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  public:
    void play(const SpeakJet::Utterance& utterance) override;
    void pause(uInt32 durationMs) override;
    void delay(uInt32 durationMs) override;
    void setParam(SpeakJet::Param param, uInt8 value) override;
    void endOfPhrase() override { releasePending(-1); flush(); showPhrase(); }

  private:
    using Samples = SpeakJetDSP::Samples;
    using Clip = SpeakJetSamples::Clip;
    using Part = SpeakJetVoice::Part;

    // Drop all speech, queued or still being made; the parameters stay
    void silence();

    /**
      Release the sound held back awaiting its successor.

      @param nextCode    The sound following it directly, whose join with it
                         is then applied; -1 when nothing follows, which lets
                         its decay sound
      @param nextTiming  That sound's Speed and Fast/Slow factor, which the
                         join takes its timing from
    */
    void releasePending(int nextCode, double nextTiming = 1.0);

    // The closure before a stop, filled with the last sound dying away
    void playClosure(double closureMs, int nextCode);

    // Crossfade onto the held-back tail, then queue for playback
    void append(sShortSpan pcm, uInt32 xfadeMs);

    // Queue the held-back tail, ending the current run of sound
    void flush();

    // Take up to 'count' samples from the front of the sounding decay
    Samples takeDecay(size_t count);

    // Print the phrase just spoken, if SPEAKJET_PHRASES asks for it
    void showPhrase();

    // The rate everything is rendered at
    uInt32 rate() const { return mySamples.rate(); }

  private:
    const OSystem& myOSystem;

    SpeakJet myDecoder;
    SpeakJetSamples mySamples;
    SpeakJetVoice myVoice;

    // The last sound, held back because the join the chip makes depends on
    // what follows it; a repeat adds a part rather than a second copy
    vector<Part> myPendingParts;
    Samples myPendingSound;
    Samples myPendingDecay;
    int myPendingCode{-1};

    // Its clip's ramps, and whether the sound before it ran straight in
    size_t myPendingHead{0};
    size_t myPendingTail{0};
    bool myPendingTrimHead{false};

    // The fundamental its clip was recorded at, which sizes phase searches
    double myPendingF0{SpeakJetSamples::SAMPLE_F0};

    // The Speed and Bend it was spoken at, which a command before the next
    // sound may since have changed
    uInt8 myPendingSpeed{SpeakJet::DEFAULT_SPEED};
    uInt8 myPendingBend{SpeakJet::DEFAULT_BEND};

    // The sound before the one being released
    int myPrevCode{-1};

    // Held back from the queue so the next sound can crossfade onto it
    Samples myTail;

    // The decay now sounding, which plays only while nothing else does
    Samples myDecay;

    // The last 30ms queued, and whether the next sound is aligned to it
    Samples myContext;
    bool myAlignNext{false};

    // How much the next join should overlap, set by the sound just played
    uInt32 myNextXfadeMs{8};

    // Nothing is playing, so the next sound primes the queue first
    bool myStarting{true};

    // A pause just played, so the next sound still gets its closure
    bool myAfterPause{false};

    // Sound runs straight on into whatever comes next, inside an utterance
    bool myFlowing{false};

    // Frames since the last byte; once nothing follows, the tail is released
    uInt32 myIdleFrames{0};

    // Current MSA parameters
    uInt8 myVolume{SpeakJet::DEFAULT_VOLUME};
    uInt8 mySpeed{SpeakJet::DEFAULT_SPEED};
    uInt8 myPitch{SpeakJet::DEFAULT_PITCH};
    uInt8 myBend{SpeakJet::DEFAULT_BEND};

    string myAboutString;

    // With SPEAKJET_TRACE set, every piece queued is printed with what made it
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const bool myTrace{std::getenv("SPEAKJET_TRACE") != nullptr};
    const char* myKind{"?"};
    int myTraceCode{-1};
    uInt64 myQueued{0};
    uInt64 myFrame{0};

    // With SPEAKJET_PHRASES set, each phrase's sounds are printed as it ends
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const bool myShowPhrases{std::getenv("SPEAKJET_PHRASES") != nullptr};
    string myPhrase;

  private:
    SpeakJetSoftware() = delete;
    SpeakJetSoftware(const SpeakJetSoftware&) = delete;
    SpeakJetSoftware(SpeakJetSoftware&&) = delete;
    SpeakJetSoftware& operator=(const SpeakJetSoftware&) = delete;
    SpeakJetSoftware& operator=(SpeakJetSoftware&&) = delete;
};

#endif  // SPEAKJET_SOFTWARE_HXX
