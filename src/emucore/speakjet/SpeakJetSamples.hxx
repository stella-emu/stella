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

#ifndef SPEAKJET_SAMPLES_HXX
#define SPEAKJET_SAMPLES_HXX

#include "SpeakJetDSP.hxx"

/**
  The SpeakJet's sounds as recorded from the chip, one WAV per MSA code
  named for its mnemonic, e.g. 'IY.wav'.  Each Bend setting has its own set
  in a 'bendNN' directory, and the voiced codes have sets per Pitch in
  'pitchNN' directories.  Sets are loaded on first use.

  @author  Stephen Anthony
*/
class SpeakJetSamples
{
  public:
    using Samples = SpeakJetDSP::Samples;

    // One recorded code: the sound, then the tail the chip leaves when
    // nothing follows it
    struct Clip {
      Samples body;
      Samples decay;

      // Where the sound itself starts and ends, past the rise and fall the
      // chip puts at the ends of an utterance; both come off at a join
      size_t head{0};
      size_t tail{0};
    };
    using SampleSet = std::array<Clip, 127>;

    // The fundamental the default recordings were made at, in Hz
    static constexpr double SAMPLE_F0 = 87.6;

    // A sound is never cut back past this share of itself
    static constexpr size_t RAMP_DIVISOR = 3;

  public:
    /**
      @param path  The directory holding the samples, with a trailing separator
    */
    explicit SpeakJetSamples(const string& path);

    // Where the samples live, and how many of the default set were found
    const string& path() const { return myPath; }
    uInt32 found() const { return myFound; }

    // Whether there are sets per Bend and per Pitch
    bool perBend() const { return myPerBend; }
    bool perPitch() const { return myPerPitch; }

    // The rate every sample is resampled to; 0 when nothing loaded
    uInt32 rate() const { return myRate; }

    /**
      The set to speak with at a Bend, loading it if need be; the default
      set when that Bend was never recorded.
    */
    const SampleSet& setFor(uInt8 bend);

    /**
      The set recorded at a Bend as loaded so far, with no fallback.
    */
    const SampleSet& recorded(uInt8 bend) const { return mySamples[bend]; }

    /**
      The clip for a code recorded at the Pitch nearest 'pitch', if there is
      a set for it; such a clip already has that length, level and pitch.

      @param atPitch  Set to the Pitch it was recorded at
      @return  The clip, or nullptr if there is no Pitch set to use
    */
    const Clip* pitchClip(uInt8 code, uInt8 pitch, uInt8 bend, uInt8& atPitch);

    /**
      The set recorded at one of the sampled Pitch settings, at a Bend.
    */
    const SampleSet& pitchSet(uInt8 bend, uInt8 pitch);

    /**
      Find where a clip's own sound starts and ends within its body.
    */
    static void findRamps(Clip& clip, uInt32 rate);

  private:
    /**
      Load the samples for one Bend, if not already loaded.

      @return  How many of the 127 were found
    */
    uInt32 loadSet(uInt8 bend);

    /**
      Load one directory's samples into a set.

      @param dir   Directory to read, with a trailing separator
      @param only  Lowest code to bother with
    */
    uInt32 loadInto(SampleSet& set, const string& dir, uInt8 only);

    // One Pitch set at one Bend, by its place among the sampled settings
    const SampleSet& pitchSetAt(size_t bend, size_t want);

    // Read a PCM WAV file as 16-bit mono; false if missing or unsupported
    static bool readWav(const string& path, Samples& samples, uInt32& rate);

    /**
      Drop leading and trailing silence, relative to the file's own peak.

      @return  What was cut from the end, which the decay continues from
    */
    static Samples trimSilence(Samples& samples);

    /**
      Move the start of a decay out of a body that still holds it.

      @return  What was moved, which the decay continues from
    */
    static Samples decayTail(Samples& samples, uInt32 rate);

  private:
    const string myPath;

    // One set per Bend: the same code at two Bend values differs more than
    // unrelated speech does, so every setting is recorded
    std::array<SampleSet, 16> mySamples;
    std::array<bool, 16> myLoaded{};

    // The voiced codes' sets per Pitch, at each Bend
    BSPF::array2D<SampleSet, 16, 30> myPitchSamples;
    BSPF::array2D<bool, 16, 30> myPitchLoaded{};

    uInt32 myFound{0};
    bool myPerBend{false};
    bool myPerPitch{false};
    uInt32 myRate{0};

  private:
    SpeakJetSamples() = delete;
    SpeakJetSamples(const SpeakJetSamples&) = delete;
    SpeakJetSamples(SpeakJetSamples&&) = delete;
    SpeakJetSamples& operator=(const SpeakJetSamples&) = delete;
    SpeakJetSamples& operator=(SpeakJetSamples&&) = delete;
};

#endif  // SPEAKJET_SAMPLES_HXX
