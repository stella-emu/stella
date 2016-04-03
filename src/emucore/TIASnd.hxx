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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

//-----------------------------------------------------------------------------
//
//  Title: TIA Audio Generator
//
//  Project - MMDC
//  Version - _VERSION
//  Author - Chris Brenner
//  Description - This code is translated more or less verbatim from my Verilog
//  code for my FPGA project. It provides core logic that can be used to achieve
//  cycle accurate emulation of the Atari 2600 TIA audio blocks.
//
//  The core logic is broken up into two functions. The updateAudioState()
//  function contains the logic for the clock divider and pulse generator. It is
//  used to update the state of the audio logic, and should be called once for
//  every audio clock cycle for each channel.
//
//  The queueSamples() function implements the volume control, and generates
//  audio samples. It depends on the state of the pulse generator of both audio
//  channels, so it internally calls the updateAudioState() function.
//
//  The constructor is called on startup in order to initialize  the audio logic
//  and volume control LUT.
//
//  The accuracy of the emulation is dependent upon the accuracy of the data
//  contained in the AUDCx, AUDFx, and AUDVx registers. It is important that these
//  registers contain the most recent value written prior to the current clock
//  cycle and phase.
//
//  The TIA audio clock is a 31.4 KHz two phase clock that occurs twice every
//  horizontal scan line. The mapping of the cycles and phases are as follows.
//  Cycle 1 - Phase 1: color clock 8
//  Cycle 1 - Phase 2: color clock 36
//  Cycle 2 - Phase 1: color clock 80
//  Cycle 2 - Phase 2: color clock 148
//
//  Since software can change the value of the registers in between clock cycles
//  and phases, it's necessary to develop a mechanism for keeping these registers
//  up to date when doing bulk processing. One method would be to time stamp the
//  register writes, and queue them into a FIFO, but I leave this design decision
//  to the emulator developer. The phase requirements are listed here.
//  AUDC0, AUDC1: used by the pulse generator at both phases
//  AUDF0, AUDF1: used by the clock divider at phase 1
//  AUDV0, AUDV1: used by the volume control at phase 2
//
//  In a real 2600, the volume control is analog, and is affected the instant when
//  AUDVx is written. However, since we generate audio samples at phase 2 of the
//  clock, the granularity of volume control updates is limited to our sample rate,
//  and changes that occur in between clocks result in only the last change prior
//  to phase 2 having an affect on the volume.
//
//  @author  Chris Brenner (original C implementation) and
//           Stephen Anthony (C++ conversion, integration into Stella)
//-----------------------------------------------------------------------------

#ifndef TIASOUND_HXX
#define TIASOUND_HXX

#include <queue>

#include "bspf.hxx"
#include "Serializable.hxx"

class TIASound : public Serializable
{
  public:
    /**
      Create a new TIA Sound object.
    */
    TIASound();

  public:
    /**
      Reset the sound emulation to its power-on state.
    */
    void reset();

    /**
      Selects the number of audio channels per sample.  There are two factors
      to consider: hardware capability and desired mixing.

      @param hardware  The number of channels supported by the sound system
      @param stereo    Whether to output the internal sound signals into 1
                       or 2 channels

      @return  Status of the channel configuration used
    */
    string channels(uInt32 hardware, bool stereo);

    /**
      Sets the specified sound register to the given value.

      @param value  Value to store in the register
      @param clock  Colour clock at which the write occurred
    */
    void writeAudC0(uInt8 value, uInt32 clock);
    void writeAudC1(uInt8 value, uInt32 clock);
    void writeAudF0(uInt8 value, uInt32 clock);
    void writeAudF1(uInt8 value, uInt32 clock);
    void writeAudV0(uInt8 value, uInt32 clock);
    void writeAudV1(uInt8 value, uInt32 clock);

    /**
      Create sound samples based on the current sound register settings
      in the specified buffer. NOTE: If channels is set to stereo then
      the buffer will need to be twice as long as the number of samples.
      The samples are stored in an internal queue, to be removed as
      necessary by getSamples() (invoked by the sound hardware callback).

      This method must be called once per scanline from the TIA class.
    */
    void queueSamples();

    /**
      Move specified number of samples from the internal queue into the
      given buffer.

      @param buffer   The location to move generated samples
      @param samples  The number of samples to move

      @return  The number of samples left to fill the buffer
               Should normally be 0, since we want to fill it completely
    */
    Int32 getSamples(uInt16* buffer, uInt32 samples)
    {
      while(mySamples.size() > 0 && samples--)
      {
        *buffer++ = mySamples.front();
        mySamples.pop();
      }
      return samples;
    }

    /**
      Set the volume of the samples created (0-100)
    */
    void volume(uInt32 percent);

    /**
      Saves the current state of this device to the given Serializer.

      @param out  The serializer device to save to.
      @return  The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out) const override;

    /**
      Loads the current state of this device from the given Serializer.

      @param in  The Serializer device to load from.
      @return  The result of the load.  True on success, false on failure.
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for this console class (used in error checking).

      @return  The name of the object
    */
    string name() const override { return "TIASound"; }

  private:
    struct AudioState : public Serializable
    {
      AudioState() { reset(); }
      void reset()
      {
        clk_en = noise_fb = noise_cnt_4 = pulse_cnt_hold = false;
        div_cnt = 0;  noise_cnt = pulse_cnt = 0;
      }
      bool save(Serializer& out) const override;
      bool load(Serializer& in) override;
      string name() const override { return "TIASound_AudioState"; }

      bool clk_en, noise_fb, noise_cnt_4, pulse_cnt_hold;
      uInt32 div_cnt, noise_cnt, pulse_cnt;
    };
    bool updateAudioState(AudioState& state, uInt32 audf, uInt32* audc);

    enum ChannelMode {
      Hardware2Mono,    // mono sampling with 2 hardware channels
      Hardware2Stereo,  // stereo sampling with 2 hardware channels
      Hardware1         // mono/stereo sampling with only 1 hardware channel
    };
    ChannelMode myChannelMode;

    AudioState myAud0State; // storage for AUD0 state
    AudioState myAud1State; // storage for AUD1 state

    uInt16 myVolLUT[256][101]; // storage for volume look-up table
    uInt32 myHWVol;            // actual output volume to use

    uInt32 myAudF0[2];      // audfx[0]: value at clock1-phase1
    uInt32 myAudF1[2];      // audfx[1]: value at clock2-phase1
    uInt32 myAudV0[2];      // audvx[0]: value at clock1-phase2 
    uInt32 myAudV1[2];      // audvx[1]: value at clock2-phase2
    uInt32 myAudC0[2][2];   // audcx[0][0]: value at clock1-phase1,
                            // audcx[0][1]: value at clock1:phase2
    uInt32 myAudC1[2][2];   // audcx[1][0]: value at clock2-phase1,
                            // audcx[1][1]: value at clock2:phase2

    // A value not equal to 0xff indicates that *both* cycles were missed
    // on the previous line, and must be updated on the next line
    uInt32 myDeferredC0, myDeferredC1;
    uInt32 myDeferredF0, myDeferredF1;
    uInt32 myDeferredV0, myDeferredV1;

    // Contains the samples previously created by queueSamples()
    // This will be periodically emptied by getSamples()
    queue<uInt16> mySamples;

    // The colour clock at which each cycle/phase ends
    // Any writes to sound registers that occur after a respective
    // cycle/phase are deferred until the next update interval
    static constexpr uInt32
      Cycle1Phase1 = 8, Cycle1Phase2 = 36, Cycle2Phase1 = 80, Cycle2Phase2 = 148;

  private:
    // Following constructors and assignment operators not supported
    TIASound(const TIASound&) = delete;
    TIASound(TIASound&&) = delete;
    TIASound& operator=(const TIASound&) = delete;
    TIASound& operator=(TIASound&&) = delete;
};

#endif
