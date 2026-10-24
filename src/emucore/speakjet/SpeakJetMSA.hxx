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

#ifndef SPEAKJET_MSA_HXX
#define SPEAKJET_MSA_HXX

#include <deque>
#include <functional>

#include "SpeakJet.hxx"
#include "SpeakJetSynth.hxx"

/**
  The SpeakJet's sequencer: each command the chip executes becomes a run of
  segments, every one a set of register targets the synthesizer's registers
  slide to, and the synthesizer makes the sound sample by sample as they do.
  Nothing is held back: what comes next is only peeked at, as the chip reads
  ahead in its input buffer.

  The structure follows the MSA of Andrew Davie's speakjet library; every
  state, length, law and rate in it is measured from the chip.

  @author  Stephen Anthony
*/
class SpeakJetMSA : public SpeakJet::Sink
{
  public:
    /**
      What the chip can see coming in its input buffer: the next sound, with
      the Speed and Fast/Slow it will be said at, or code -1 when a pause, a
      delay or nothing comes first, and whether it is a pause or a delay.
    */
    struct Next
    {
      int code{-1};
      uInt8 speed{SpeakJet::INITIAL_SPEED};
      uInt32 scalePct{100};
      bool pause{false};
    };
    using Lookahead = std::function<Next()>;

    /**
      @param lookahead  Peeks at the input buffer
    */
    explicit SpeakJetMSA(Lookahead lookahead);
    ~SpeakJetMSA() override = default;

    /**
      Whether a sound or a pause is still being said.
    */
    bool busy() const { return !mySegments.empty(); }

    /**
      The next output sample, 1.0 being full scale.
    */
    double next();

    /**
      Drop whatever is being said; the controls stay.
    */
    void silence();

    /**
      Time passes with nothing said, so whatever comes next starts afresh.
    */
    void rest();

  public:
    void play(const SpeakJet::Utterance& utterance) override;
    void pause(uInt32 durationMs) override;
    void delay(uInt32 durationMs) override;
    void setParam(SpeakJet::Param param, uInt8 value) override;

  private:
    /**
      What a segment makes: silence, a stop's closure, voicing, the noise of
      oscillators 4 and 5, PE and PO's low sine, or a sound effect's program.
    */
    enum class Kind: uInt8 { Quiet, Closure, Voiced, Noise, Thump, Effect };

    /**
      One stretch of what is said: the registers it slides to, and how.
    */
    struct Segment
    {
      size_t samples{0};
      SpeakJetSynth::Registers target;

      // A glide's measured span, the least its slide may take; a stop's
      // states change within one period, so its registers jump
      size_t span{0};
      bool jump{false};

      // What it makes, its sound, whether it is that sound's last (where voicing
      // may end), and whether its noise carries on under the voicing after it
      Kind kind{Kind::Quiet};
      int code{-1};
      bool last{false};
      bool handsOver{false};

      // The Speed its slides run at, the Pitch it is said at, and the Volume,
      // which is the master register
      uInt8 speed{SpeakJet::INITIAL_SPEED};
      uInt8 pitch{SpeakJet::INITIAL_PITCH};
      uInt8 master{SpeakJet::INITIAL_VOLUME};

      // An effect's Bend, its tick in samples, and the plain tick its last
      // frame plays at under Fast or Slow
      uInt8 bend{SpeakJet::INITIAL_BEND};
      double tick{1.0};
      double lastTick{1.0};
    };

    /**
      Whether a code is made by voicing alone, the continuants and the
      voiced fricatives.
    */
    static bool voices(int code);

    /**
      Whether a code is DH or a voiced stop, a voice bar and its release.
    */
    static bool voicedStop(int code);

    /**
      Whether voicing runs through a code: either of those.
    */
    static bool voicedThrough(int code);

    /**
      How far Speed speeds the slides.
    */
    static double slewScale(uInt8 speed);

    /**
      How far Fast or Slow stretches the tick at a Speed: Fast takes a faster
      Speed's tick, Slow makes it half as long again short of a fixed part.
    */
    static double stretch(uInt32 scalePct, uInt8 speed);

    /**
      The fundamental a Pitch setting gives.
    */
    static double pitchHz(uInt8 setting);

    /**
      A voiced sound's segments, 'ms' long; a held repeat only carries on
      to the sound's end state, and 'into' is the code that follows at once.
    */
    void expand(int code, double ms, bool rearticulate, int into);

    /**
      What the join from 'code' into 'next' adds to the sound's length, and
      the closure a stop takes of it.
    */
    double extra(int code, const Next& next, double& closureMs) const;

    /**
      Running straight into the stop 'into', the registers 'code' slides to
      and the slide's length; false if 'into' has no entry state.
    */
    bool entry(int code, int into, SpeakJetSynth::Registers& to, size_t& samples) const;

    /**
      The open join of the sound being said, once the chip can see what comes
      next: its last segment takes the difference, and a stop's entry slide.
    */
    void join();

    /**
      A voiceless stop running straight into voicing, 'after' segments queued
      behind it, gives the end of its time to that voicing's slide.
    */
    void handOver(int code, int next, size_t after);

    /**
      Silence 'ms' long, or a stop's closure, and voicing stops.
    */
    void quiet(double ms, Kind kind = Kind::Quiet);

    /**
      A voiceless sound or an effect, 'ms' long; an effect steps its program
      once a tick, 'tickMs' long, and its last frame once every 'lastTickMs'.
    */
    void unvoiced(int code, double ms, double tickMs, double lastTickMs);

    /**
      The next sample of the synthesizer, its envelope restarted if it has
      been silent.
    */
    double synth(const SpeakJetSynth::Registers& regs);

    /**
      The next sample of silence: every volume at zero, the output stage
      settling from what it last made.
    */
    double still();

    /**
      Whether voicing runs straight on after the front segment.
    */
    bool runsOn() const;

    /**
      Start the front segment: what the registers slide from and to.
    */
    void begin();

  private:
    Lookahead myLookahead;
    SpeakJetSynth mySynth;

    std::deque<Segment> mySegments;

    // What the segment before the front one made
    Kind myLastKind{Kind::Quiet};

    // Samples into the front segment, the registers now, the slide they are
    // on and its length
    size_t myPos{0};
    SpeakJetSynth::Registers myRegs, myFrom, myTo;
    size_t mySlide{1};

    // The fundamental, gliding to each Pitch
    double myF0{0.0};

    // A run of voicing: whether it is sounding, whether it started from
    // nothing, and how long it has run
    bool myLive{false};
    bool myCold{false};
    double myRunMs{0.0};

    // The next run starts from nothing: after silence, a voiceless stop or
    // an affricate
    bool myColdNext{true};

    // A pause just went by, so a stop next still has its closure
    bool myAfterPause{false};

    // The closure the next sound, a stop, opens with, and the time a stop
    // before it gave the next sound to start under its noise
    double myClosureMs{0.0};
    double myHandOverMs{0.0};

    /**
      A sound whose join is open, the chip not yet seeing what follows: its
      code, its length without the join, and the segments queued after its last.
    */
    struct Join
    {
      bool open{false};
      int code{-1};
      double ms{0.0};
      size_t after{0};
    };
    Join myJoin;

    // Whether the synthesizer is sounding, and whether a stop's jump is
    // waiting for the next period, as the chip's states change there
    bool mySounding{false};
    bool myJumpWait{false};

    // Whether the front segment fades the voicing out at its end, once known
    bool myEndKnown{false};
    bool myEnds{false};

    // The controls
    uInt8 myVolume{SpeakJet::INITIAL_VOLUME};
    uInt8 mySpeed{SpeakJet::INITIAL_SPEED};
    uInt8 myPitch{SpeakJet::INITIAL_PITCH};
    uInt8 myBend{SpeakJet::INITIAL_BEND};

    // The last sound said, for a repeat and a lead into a nasal; -1 once a
    // pause separates it from the next
    int myLastCode{-1};

  private:
    SpeakJetMSA() = delete;
    SpeakJetMSA(const SpeakJetMSA&) = delete;
    SpeakJetMSA(SpeakJetMSA&&) = delete;
    SpeakJetMSA& operator=(const SpeakJetMSA&) = delete;
    SpeakJetMSA& operator=(SpeakJetMSA&&) = delete;
};

#endif  // SPEAKJET_MSA_HXX
