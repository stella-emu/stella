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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_AUDIO_HXX
#define TIA_AUDIO_HXX

class AudioQueue;

#include "bspf.hxx"
#include "AudioChannel.hxx"
#include "Serializable.hxx"

class Audio : public Serializable
{
  public:
    Audio();
    ~Audio() override = default;

    void reset();

    void setAudioQueue(const shared_ptr<AudioQueue>& queue);

    /**
      Enable/disable pushing audio samples. These are required for TimeMachine
      playback with sound.
    */
    void setAudioRewindMode(bool enable)
    {
    #ifdef GUI_SUPPORT
      myRewindMode = enable;
    #endif
    }

    FORCE_INLINE void tick();

    AudioChannel& channel0() { return myChannel0; }

    AudioChannel& channel1() { return myChannel1; }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    void createSample();
    void addSample(uInt8 sample0, uInt8 sample1);

  private:
    shared_ptr<AudioQueue> myAudioQueue;

    uInt8 myCounter{0};

    AudioChannel myChannel0;
    AudioChannel myChannel1;

    uInt32 mySumChannel0{0};
    uInt32 mySumChannel1{0};
    uInt32 mySumCt{0};

    std::array<Int16, 0x1e + 1> myMixingTableSum{};
    std::array<Int16, 0x0f + 1> myMixingTableIndividual{};

    Int16* myCurrentFragment{nullptr};
    uInt32 mySampleIndex{0};
  #ifdef GUI_SUPPORT
    bool myRewindMode{false};
    mutable ByteArray mySamples;
  #endif

  private:
    Audio(const Audio&) = delete;
    Audio(Audio&&) = delete;
    Audio& operator=(const Audio&) = delete;
    Audio& operator=(Audio&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Audio::tick()
{
  // Volume for each channel is sampled every color clock. the average of
  // these samples will be taken twice a scanline in the phase1() function
  mySumChannel0 += static_cast<uInt32>(myChannel0.actualVolume());
  mySumChannel1 += static_cast<uInt32>(myChannel1.actualVolume());
  mySumCt++;

  switch (myCounter) {
    case 9:
    case 81:
      myChannel0.phase0();
      myChannel1.phase0();
      break;

    case 37:
    case 149:
      myChannel0.phase1();
      myChannel1.phase1();
	  createSample();
      break;

    default:
      break;
  }

  if (++myCounter == 228) myCounter = 0;
}

#endif // TIA_AUDIO_HXX
