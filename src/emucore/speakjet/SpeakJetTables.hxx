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

#ifndef SPEAKJET_TABLES_HXX
#define SPEAKJET_TABLES_HXX

#include <optional>

#include "bspf.hxx"

/**
  How long the SpeakJet takes over its sounds and the joins between them,
  and how its parameters scale length and level, as measured on the chip.

  @author  Stephen Anthony
*/
class SpeakJetTables
{
  public:
    // Lifts the recordings to sit level with the TIA at the same master volume
    static constexpr double SPEECH_GAIN = 2.818;

    /**
      How Speed scales a sound's duration, relative to the default Speed.
    */
    static double speedFactor(uInt8 speed);

    /**
      How Pitch shifts the fundamental, which is the setting in Hz, relative
      to the default.
    */
    static double pitchFactor(uInt8 pitch);

    /**
      The level a voiced sound recorded at 'atPitch' needs at 'pitch': lower
      voices are quieter, at close to constant energy per glottal pulse.
    */
    static double pitchGain(uInt8 pitch, uInt8 atPitch);

    /**
      How Pitch scales a voiced sound's length, relative to the default.
    */
    static double pitchDuration(uInt8 pitch);

    /**
      The same inside a phrase, where above the default only a fifth of the
      isolated sound's lengthening shows.
    */
    static double phraseDuration(uInt8 pitch);

    /**
      How Volume scales amplitude, relative to the default.
    */
    static double volumeFactor(uInt8 volume);

    /**
      The part of a sound, in ms, that Speed, Fast and Slow leave unstretched.
    */
    static double fixedMs(uInt8 code);

    /**
      What the chip adds to a phrase at a join from one sound into the next,
      in ms at the default Speed; negative is shorter than the two alone.
    */
    static double joinMs(uInt8 from, uInt8 to);

    /**
      What one more copy of a sound adds, in ms, where that is known.
    */
    static std::optional<double> repeatMs(uInt8 code);

    /**
      Whether the chip holds a repeated sound rather than starting it again.
    */
    static bool holdsRepeat(uInt8 code);

    /**
      The near-silent closure before a stop's burst, in ms, when a sound runs
      straight into it.
    */
    static double closureMs(uInt8 code);

    /**
      Time a quiet code takes mid-phrase that its trimmed recording lacks.
    */
    static uInt32 quietPadMs(uInt8 code);

  private:
    SpeakJetTables() = delete;
    ~SpeakJetTables() = delete;
    SpeakJetTables(const SpeakJetTables&) = delete;
    SpeakJetTables(SpeakJetTables&&) = delete;
    SpeakJetTables& operator=(const SpeakJetTables&) = delete;
    SpeakJetTables& operator=(SpeakJetTables&&) = delete;
};

#endif  // SPEAKJET_TABLES_HXX
