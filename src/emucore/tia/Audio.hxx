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

#ifndef AUDIO_HXX
#define AUDIO_HXX

class AudioQueue;

#include "bspf.hxx"
#include "AudioChannel.hxx"
#include "Serializable.hxx"

/**
  TIA audio subsystem. Drives the two-phase audio clock at the correct
  points in each scanline, accumulates per-clock channel volumes for
  downsampling, and pushes mixed 16-bit samples to the host audio layer
  via an AudioQueue.

  @author  Christian Speckner (DirtyHairy)
*/
class Audio : public Serializable
{
  public:
    Audio();
    ~Audio() override = default;

    /**
      Reset to initial state.
     */
    void reset();

    /**
      Set the audio output queue used to pass samples to the host audio system.
     */
    void setAudioQueue(const shared_ptr<AudioQueue>& queue);

    /**
      Enable/disable pushing audio samples. These are required for TimeMachine
      playback with sound.
    */
    void setAudioRewindMode(bool enable) { myRewindMode = enable; }

    /**
      Tick the given number of color clocks: accumulate channel volumes and
      drive the two-phase audio clock at the appropriate points in each
      scanline. A channel's volume can only change on a phase clock (AUDxx
      writes are applied in TIA::poke, between batches), so accumulation is
      done per segment between phase events rather than per color clock.
     */
    void tick(uInt32 colorClocks);

    /**
      Access audio channel 0.
     */
    AudioChannel& channel0() { return myChannel0; }

    /**
      Access audio channel 1.
     */
    AudioChannel& channel1() { return myChannel1; }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    /**
      Save/load the accumulated audio samples used for TimeMachine playback
      with sound. Unlike the core device state, these are only meaningful for
      rewind playback, so they travel with the (rewind-only) display state
      rather than every save state; this keeps normal save states a fixed size.
    */
    bool saveSamples(Serializer& out) const;
    bool loadSamples(Serializer& in);

  private:
    /**
      Build and push the next audio sample from the accumulated channel sums.
     */
    void createSample();

    /**
      Mix sample0 and sample1 via the precomputed lookup table and append to
      the current audio fragment.
     */
    void addSample(uInt8 sample0, uInt8 sample1);

  private:
    // Output queue shared with the host audio layer
    shared_ptr<AudioQueue> myAudioQueue;

    // Color clock position (0-227); drives the two-phase audio clock
    uInt8 myCounter{0};

    // Left and right audio channels
    AudioChannel myChannel0;
    AudioChannel myChannel1;

    // Accumulated volume samples for channel 0 (for downsampling)
    uInt32 mySumChannel0{0};
    // Accumulated volume samples for channel 1 (for downsampling)
    uInt32 mySumChannel1{0};
    // Number of samples in the current accumulation window
    uInt32 mySumCt{0};

    // Precomputed output levels for combined channel sums (indices 0-30)
    std::array<Int16, 0x1e + 1> myMixingTableSum{};
    // Precomputed output levels for a single channel (indices 0-15)
    std::array<Int16, 0x0f + 1> myMixingTableIndividual{};

    // Pointer to the audio output fragment currently being filled
    Int16* myCurrentFragment{nullptr};
    // Write index within the current fragment
    uInt32 mySampleIndex{0};

    bool myRewindMode{false};
    mutable ByteArray mySamples;

  private:
    // Following constructors and assignment operators not supported
    Audio(const Audio&) = delete;
    Audio(Audio&&) = delete;
    Audio& operator=(const Audio&) = delete;
    Audio& operator=(Audio&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Audio::tick(uInt32 colorClocks)
{
  // Volume for each channel is sampled every color clock; the average of
  // these samples is taken twice a scanline when phase1() fires. The volume
  // is constant between phase clocks, so each segment accumulates as a
  // single multiply instead of clock-by-clock.
  while (colorClocks > 0)
  {
    // Clocks up to and including the next phase event, or to the end of the
    // line when no event remains. Phase 0 fires at counters 9 and 81,
    // phase 1 at 37 and 149; the counter wraps after 227.
    enum class Event: uInt8 { phase0, phase1, lineWrap };
    uInt32 toEvent;  // NOLINT(cppcoreguidelines-init-variables)
    Event event;     // NOLINT(cppcoreguidelines-init-variables)

    if (myCounter < 10)       { toEvent = 10 - myCounter;  event = Event::phase0; }
    else if (myCounter < 38)  { toEvent = 38 - myCounter;  event = Event::phase1; }
    else if (myCounter < 82)  { toEvent = 82 - myCounter;  event = Event::phase0; }
    else if (myCounter < 150) { toEvent = 150 - myCounter; event = Event::phase1; }
    else                      { toEvent = 228 - myCounter; event = Event::lineWrap; }

    const uInt32 chunk = std::min(colorClocks, toEvent);

    mySumChannel0 += static_cast<uInt32>(myChannel0.actualVolume()) * chunk;
    mySumChannel1 += static_cast<uInt32>(myChannel1.actualVolume()) * chunk;
    mySumCt += chunk;
    myCounter = static_cast<uInt8>(myCounter + chunk);
    colorClocks -= chunk;

    // The event only fires if the batch actually reached it
    if (chunk == toEvent)
    {
      switch (event)
      {
        case Event::phase0:
          myChannel0.phase0();
          myChannel1.phase0();
          break;

        case Event::phase1:
          myChannel0.phase1();
          myChannel1.phase1();
          createSample();
          break;

        case Event::lineWrap:
          myCounter = 0;
          break;
      }
    }
  }
}

#endif  // AUDIO_HXX
