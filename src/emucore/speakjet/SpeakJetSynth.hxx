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

#ifndef SPEAKJET_SYNTH_HXX
#define SPEAKJET_SYNTH_HXX

#include <complex>

#include "bspf.hxx"

/**
  The SpeakJet's synthesizer, one output sample at a time from what its
  registers hold, in the chip's own units.  Oscillators 1 to 3 are the
  formants: each envelope period restarts them under the grain envelope, with
  the pedestal beneath.  Oscillators 4 and 5 are the noise, at half the level,
  their frequencies jittered by the distortion.  Every volume scales with the
  master volume.  What the chip computes is held at its own rate, then goes
  through the AtariVox's output stage.

  The structure follows the synthesizer of Andrew Davie's speakjet library;
  every shape and constant in it is measured from the chip.

  @author  Stephen Anthony
*/
class SpeakJetSynth
{
  public:
    // The rate every sample is made at
    static constexpr uInt32 RATE = 32000;

    /**
      What the registers hold.
    */
    struct Registers
    {
      // Oscillators 1 to 5: frequency in Hz (0 to 3999), volume (0 to 31);
      // 1 to 3 by formant band
      std::array<double, 5> hz{500.0, 1500.0, 2500.0, 0.0, 0.0};
      std::array<double, 5> volume{};

      // The envelope's frequency in Hz, oscillator 4 and 5's distortion (0 to
      // 255), and the master volume (0 to 127)
      double envelopeHz{0.0};
      double distortion{0.0};
      double master{0.0};

      // Oscillators 1 to 3 under the envelope or running free, its shape (0 the
      // grain, 1 raised cosine, 2 rising saw, 3 square), 4 and 5 half enveloped
      bool enveloped{true};
      uInt8 shape{0};
      bool noiseHalf{false};
    };

  public:
    SpeakJetSynth();
    ~SpeakJetSynth() = default;

    /**
      The next sample, as the output stage gives it.

      @param regs  The registers
      @return  The sample, 1.0 being full scale
    */
    double sample(const Registers& regs);

    /**
      As at power-on: silent, the output stage at rest.
    */
    void reset();

    /**
      Sound starts again after silence: the envelope begins a period at the
      next sample, and oscillators 1 to 3 start from phase zero.
    */
    void restart();

    /**
      Whether the next sample, at voicing frequency 'f0', starts a period.
    */
    bool startsPeriod(double f0) const;

  private:
    /**
      The output stage after the hold: the analogue low-pass carried exactly
      to each hold edge, then the high-pass.
    */
    struct Path {
      double dt{0.0};
      double w2{0.0}, wq{0.0};
      std::complex<double> p1, p2;
      // The low-pass over one whole sample: the state's own part, the input's
      std::array<double, 4> e{};
      std::array<double, 2> g{};
      double h0{0.0}, h1{0.0};
    };

    /**
      The low-pass over 't' seconds: the state's own part, the held input's.
    */
    static void lowpassOver(const Path& p, double t, std::array<double, 4>& e,
                            std::array<double, 2>& g);

    /**
      One output sample through the hold and the stage, 'in' being what the
      chip computes at it.
    */
    double stageStep(double in);

    /**
      One sample of oscillators 4 and 5 at their volumes, 'tau' seconds into
      the period.
    */
    double noise(const Registers& regs, double tau);

  private:
    Path myPath;

    // Where each formant oscillator is in its cycle, where the envelope is in
    // its period, and samples since the period started (negative before any)
    std::array<double, 3> myPhase{};
    double myCycle{0.0};
    double mySince{-1.0};

    // The noise oscillators' phases, their held jitter, chip samples since it
    // was drawn, and the generator drawing it
    std::array<double, 2> myNoisePhase{};
    std::array<double, 2> myNoiseJitter{};
    double myNoiseHeld{0.0};
    uInt32 myNoiseSeed{1};

    // Samples made, the chip tick held and its value, the last input, the
    // low-pass's state, and the high-pass's last input and output
    uInt64 myAt{0};
    uInt64 myTick{~uInt64{0}};
    double myHeld{0.0};
    double myLast{0.0};
    std::array<double, 2> myLow{};
    double myLowLast{0.0};
    double myHighLast{0.0};

  private:
    SpeakJetSynth(const SpeakJetSynth&) = delete;
    SpeakJetSynth(SpeakJetSynth&&) = delete;
    SpeakJetSynth& operator=(const SpeakJetSynth&) = delete;
    SpeakJetSynth& operator=(SpeakJetSynth&&) = delete;
};

#endif  // SPEAKJET_SYNTH_HXX
