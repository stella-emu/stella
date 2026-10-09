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

#ifndef SPEAKJET_VOICE_HXX
#define SPEAKJET_VOICE_HXX

class SpeakJetSamples;
namespace SpeakJetChip {
  struct Noise;
}

#include <complex>

#include "SpeakJet.hxx"
#include "SpeakJetDSP.hxx"

/**
  Makes the SpeakJet's voiced sounds as the chip does: each period a grain
  of three oscillators under a fixed envelope, sliding between the states
  in SpeakJetChip at the chip's rate, plus that period's residual from the
  recordings, held at the chip's own rate and filtered as its output is.

  @author  Stephen Anthony
*/
class SpeakJetVoice
{
  public:
    using Samples = SpeakJetDSP::Samples;

    // One stretch of a sound with the Pitch and Volume in force for it, and
    // the length it takes
    struct Part {
      double targetMs{0.0};
      double pitch{1.0};
      double gain{1.0};
      uInt8 setting{SpeakJet::DEFAULT_PITCH};
      uInt8 volume{SpeakJet::DEFAULT_VOLUME};
    };

  public:
    explicit SpeakJetVoice(SpeakJetSamples& samples);

    // Whether a code is made here rather than played from its recording
    static bool makes(int code);

    // Whether a made sound runs on from a recorded code, sliding out of it
    static bool carries(int code);

    /**
      Speak one voiced sound, carrying the voicing on from the sound before
      when it runs straight in.

      @param code   The sound
      @param parts  Its parts, each with its final length
      @param bend   The Bend it is spoken at
      @param speed  The Speed it is spoken at, which scales its slides
      @param cold   Voicing starts from nothing, as it does from silence
      @param ends   Voicing stops after it, so it fades out
      @param into   The code that follows at once, or -1
      @return  The sound, gain applied
    */
    Samples speak(int code, const vector<Part>& parts, uInt8 bend, uInt8 speed,
                  bool cold, bool ends, int into);

    // Whether voicing is still running on from the last sound
    bool live() const { return myVoicing.live; }

    // Voicing stops, so the next sound starts it afresh
    void stop() { myVoicing.live = false; }

    // The next made sound slides out of this recorded code, or out of
    // nothing for -1
    void carryFrom(int code) { myCarryCode = code; }

    // Forget the voicing, as at power-on
    void reset() { myVoicing = Voicing{}; }

  private:
    // The points of a sound its states are measured at: a glide's onset near
    // its start, then the start and end states
    enum class Point: uInt8 { Onset, Start, End };

    // The output stage after the hold: the analogue low-pass carried exactly
    // to each hold edge, then the high-pass
    struct Path {
      double rate{0.0}, dt{0.0};
      double w2{0.0}, wq{0.0};
      std::complex<double> p1, p2;
      // The low-pass over one whole sample: the state's own part, the input's
      std::array<double, 4> e{};
      std::array<double, 2> g{};
      double h0{0.0}, h1{0.0};
    };

    // The hold and output stage's memory: the chip tick held and its value,
    // the last input, the low-pass's state, the high-pass's last in and out
    struct Stage {
      uInt64 tick{~uInt64{0}};
      double held{0.0}, last{0.0};
      std::array<double, 2> x{};
      double lowLast{0.0}, highLast{0.0};
    };

    // The voicing, running on from one made sound into the next
    struct Voicing {
      bool live{false};

      // The oscillators' settings now, and where each is in its cycle
      std::array<double, 3> hz{}, level{}, phase{};
      double pedestal{0.0};

      // The fundamental, the position in the period, time since the trigger
      double f0{0.0};
      double cycle{0.0};
      double since{-1.0};

      // Samples made, and the hold and output stage they go through
      uInt64 at{0};
      Stage stage;

      // 1ms delay, so a period's residual can start 1ms before its trigger
      vector<double> delay;
      size_t head{0};
      vector<double> active;
      uInt64 activeAt{0};

      // The state the last residual was taken for, to blend from
      int resCode{-1};
      Point resAt{Point::Start};
      uInt8 resBend{SpeakJet::DEFAULT_BEND};

      // Time since the run started, whether from nothing, and the slide's start
      double runMs{0.0};
      bool cold{false};
      bool warm{false};
      size_t slideStart{0};

      // A voiced fricative's noise: osc 4 and 5 phases, their held jitter,
      // chip samples since it was drawn, and the generator drawing it
      std::array<double, 2> noisePhase{}, noiseJitter{};
      double noiseHeld{0.0};
      uInt32 noiseSeed{1};
    };

  private:
    /**
      A code's recorded voicing period less the synthesised one at one point
      of the sound, worked out on first use from that Bend's recording.

      @return  Empty if the recording has no clear periods
    */
    const vector<double>& residual(int code, Point which, uInt8 bend) const;

    /**
      One sample of a voiced fricative's noise, advancing its oscillators.

      @param tau   Seconds since this period's trigger
      @param rate  The output rate
      @return  The noise, before its level
    */
    double voicedNoise(const SpeakJetChip::Noise& noise, double tau, double rate);

    // The output stage at an output rate
    static Path pathFor(double rate);

    // The low-pass over 't' seconds: the state's own part, the held input's
    static void lowpassOver(const Path& p, double t, std::array<double, 4>& e,
                            std::array<double, 2>& g);

    // One output sample 'at' through the hold and the stage, 'in' being what
    // the chip computes at it
    static double stageStep(const Path& p, Stage& s, uInt64 at, double in);

    // A whole signal held at the chip's rate and through the stage, from rest
    static vector<double> chipOutput(const vector<double>& raw, uInt32 rate);

    // The stage's response at 'hz', the hold's included
    static std::complex<double> pathGain(const Path& p, double hz);

    // What the chip would compute for the stage to give 'y', up to its Nyquist
    static void toChip(vector<double>& y, uInt32 rate);

  private:
    SpeakJetSamples& mySamples;

    Voicing myVoicing;

    // A recorded code the next made sound slides out of, or -1
    int myCarryCode{-1};

    // residual() per Bend, code and point, worked out once
    mutable BSPF::array2D<vector<double>, 16 * 72, 3> myResidual;
    mutable BSPF::array2D<bool, 16 * 72, 3> myResidualDone{};

  private:
    SpeakJetVoice() = delete;
    SpeakJetVoice(const SpeakJetVoice&) = delete;
    SpeakJetVoice(SpeakJetVoice&&) = delete;
    SpeakJetVoice& operator=(const SpeakJetVoice&) = delete;
    SpeakJetVoice& operator=(SpeakJetVoice&&) = delete;
};

#endif  // SPEAKJET_VOICE_HXX
