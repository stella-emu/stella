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

#ifndef AUDIO_CHANNEL_HXX
#define AUDIO_CHANNEL_HXX

#include "bspf.hxx"
#include "Serializable.hxx"

/**
  One of the two independent TIA audio channels. Implements the frequency
  divider, 5-bit pulse counter, 4-bit noise LFSR, and output volume
  controlled by the AUDC/AUDF/AUDV registers.

  @author  Christian Speckner (DirtyHairy)
*/
class AudioChannel : public Serializable
{
  public:
    AudioChannel() = default;
    ~AudioChannel() override = default;

    /**
      Reset channel to its power-on state.
     */
    void reset();

    /**
      Clock phase 0: advance the frequency divider. Called twice per scanline
      at TIA color clocks 9 and 81.
     */
    void phase0();

    /**
      Clock phase 1: advance the pulse and noise counters and update the
      output level. Called twice per scanline at TIA color clocks 37 and 149.
     */
    void phase1();

    /**
      The instantaneous output volume: volume register scaled by the low bit
      of the pulse counter.
     */
    uInt8 actualVolume() const {
      return (myPulseCounter & 0x01) * myAudv;
    }

    /**
      AUDC0/1 write: set the audio control register (waveform type, bits 3-0).
     */
    void audc(uInt8 value) {
      myAudc = value & 0x0f;
    }

    /**
      AUDF0/1 write: set the frequency divider reload value (bits 4-0).
     */
    void audf(uInt8 value) {
      myAudf = value & 0x1f;
    }

    /**
      AUDV0/1 write: set the output volume (bits 3-0).
     */
    void audv(uInt8 value) {
      myAudv = value & 0x0f;
    }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    // AUDC register value (waveform control, bits 3-0)
    uInt8 myAudc{0};
    // AUDF register value (frequency divider reload, bits 4-0)
    uInt8 myAudf{0};
    // AUDV register value (volume, bits 3-0)
    uInt8 myAudv{0};

    // Whether the channel clock is currently enabled
    bool myClockEnable{false};
    // Feedback bit computed from the noise counter
    bool myNoiseFeedback{false};
    // Bit 4 of the noise counter, sampled in phase 0
    bool myNoiseCounterBit4{false};
    // Whether the pulse counter is frozen by the current waveform type
    bool myPulseCounterHold{false};

    // Frequency divider counter; reloads from AUDF when it reaches zero
    uInt8 myDivCounter{0};
    // 5-bit pulse counter; its low bit gates the output volume
    uInt8 myPulseCounter{0};
    // 4-bit noise LFSR; feedback polynomial selected by AUDC
    uInt8 myNoiseCounter{0};

  private:
    // Following constructors and assignment operators not supported
    AudioChannel(const AudioChannel&) = delete;
    AudioChannel(AudioChannel&&) = delete;
    AudioChannel& operator=(const AudioChannel&) = delete;
    AudioChannel& operator=(AudioChannel&&) = delete;
};

#endif  // AUDIO_CHANNEL_HXX
