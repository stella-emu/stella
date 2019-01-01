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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_AUDIO_HXX
#define TIA_AUDIO_HXX

#include "bspf.hxx"
#include "AudioChannel.hxx"
#include "Serializable.hxx"

class AudioQueue;

class Audio : public Serializable
{
  public:
    Audio();

    void reset();

    void setAudioQueue(shared_ptr<AudioQueue> queue);

    void tick();

    AudioChannel& channel0();

    AudioChannel& channel1();

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    void phase1();

  private:
    shared_ptr<AudioQueue> myAudioQueue;

    uInt8 myCounter;

    AudioChannel myChannel0;
    AudioChannel myChannel1;

    Int16 myMixingTableSum[0x1e + 1];
    Int16 myMixingTableIndividual[0x0f + 1];

    Int16* myCurrentFragment;
    uInt32 mySampleIndex;

  private:
    Audio(const Audio&) = delete;
    Audio(Audio&&) = delete;
    Audio& operator=(const Audio&) = delete;
    Audio& operator=(Audio&&) = delete;
};

#endif // TIA_AUDIO_HXX
