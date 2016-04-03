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

#include "System.hxx"
#include "TIATables.hxx"
#include "TIASnd.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIASound::TIASound()
  : myChannelMode(Hardware2Mono),
    myHWVol(100)
{
  // Build volume lookup table
  constexpr double ra = 1.0 / 30.0;
  constexpr double rb = 1.0 / 15.0;
  constexpr double rc = 1.0 / 7.5;
  constexpr double rd = 1.0 / 3.75;

  memset(myVolLUT, 0, sizeof(uInt16)*256*101);
  for(int i = 1; i < 256; ++i)
  {
    double r2 = 0.0;

    if(i & 0x01)  r2 += ra;
    if(i & 0x02)  r2 += rb;
    if(i & 0x04)  r2 += rc;
    if(i & 0x08)  r2 += rd;
    if(i & 0x10)  r2 += ra;
    if(i & 0x20)  r2 += rb;
    if(i & 0x40)  r2 += rc;
    if(i & 0x80)  r2 += rd;

    r2 = 1.0 / r2;
    uInt16 vol = uInt16(32768.0 * (1.0 - r2 / (1.0 + r2)) + 0.5);

    // Pre-calculate all possible volume levels
    for(int j = 0; j <= 100; ++j)
      myVolLUT[i][j] = vol * j / 100;
  }

  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::reset()
{
  myAud0State.reset();
  myAud1State.reset();
  while(!mySamples.empty()) mySamples.pop();

  for(int i = 0; i < 2; ++i)
  {
    myAudC0[i][0] = myAudC0[i][1] = 0;
    myAudC1[i][0] = myAudC1[i][1] = 0;
    myAudF0[i] = myAudF1[i] = 0;
    myAudV0[i] = myAudV1[i] = 0;
  }

  myDeferredC0 = myDeferredC1 = myDeferredF0 = myDeferredF1 =
    myDeferredV0 = myDeferredV1 = 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIASound::channels(uInt32 hardware, bool stereo)
{
  if(hardware == 1)
    myChannelMode = Hardware1;
  else
    myChannelMode = stereo ? Hardware2Stereo : Hardware2Mono;

  switch(myChannelMode)
  {
    case Hardware2Mono:   return "Hardware2Mono";
    case Hardware2Stereo: return "Hardware2Stereo";
    case Hardware1:       return "Hardware1";
    default:              return EmptyString;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::writeAudC0(uInt8 value, uInt32 clock)
{
  value &= 0x0F;
  if(clock <= Cycle1Phase1)  myAudC0[0][0] = value;
  if(clock <= Cycle1Phase2)  myAudC0[0][1] = value;
  if(clock <= Cycle2Phase1)  myAudC0[1][0] = value;
  if(clock <= Cycle2Phase2)  myAudC0[1][1] = value;
  else  // We missed both cycles, so defer write until next line
    myDeferredC0 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::writeAudC1(uInt8 value, uInt32 clock)
{
  value &= 0x0F;
  if(clock <= Cycle1Phase1)  myAudC1[0][0] = value;
  if(clock <= Cycle1Phase2)  myAudC1[0][1] = value;
  if(clock <= Cycle2Phase1)  myAudC1[1][0] = value;
  if(clock <= Cycle2Phase2)  myAudC1[1][1] = value;
  else  // We missed both cycles, so defer write until next line
    myDeferredC1 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::writeAudF0(uInt8 value, uInt32 clock)
{
  value &= 0x1F;
  if(clock <= Cycle1Phase1)  myAudF0[0] = value;
  if(clock <= Cycle2Phase1)  myAudF0[1] = value;
  else  // We missed both cycles, so defer write until next line
    myDeferredF0 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::writeAudF1(uInt8 value, uInt32 clock)
{
  value &= 0x1F;
  if(clock <= Cycle1Phase1)  myAudF1[0] = value;
  if(clock <= Cycle2Phase1)  myAudF1[1] = value;
  else  // We missed both cycles, so defer write until next line
    myDeferredF1 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::writeAudV0(uInt8 value, uInt32 clock)
{
  value &= 0x0F;
  if(clock <= Cycle1Phase2)  myAudV0[0] = value;
  if(clock <= Cycle2Phase2)  myAudV0[1] = value;
  else  // We missed both cycles, so defer write until next line
    myDeferredV0 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::writeAudV1(uInt8 value, uInt32 clock)
{
  value &= 0x0F;
  if(clock <= Cycle1Phase2)  myAudV1[0] = value;
  if(clock <= Cycle2Phase2)  myAudV1[1] = value;
  else  // We missed both cycles, so defer write until next line
    myDeferredV1 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::queueSamples()
{
  // Cycle 1
  bool aud0_c1 = updateAudioState(myAud0State, myAudF0[0], myAudC0[0]);
  bool aud1_c1 = updateAudioState(myAud1State, myAudF1[0], myAudC1[0]);

  // Cycle 2
  bool aud0_c2 = updateAudioState(myAud0State, myAudF0[1], myAudC0[1]);
  bool aud1_c2 = updateAudioState(myAud1State, myAudF1[1], myAudC1[1]);

  switch(myChannelMode)
  {
    case Hardware2Mono:   // mono sampling with 2 hardware channels
    {
      uInt32 idx1 = 0;
      if(aud0_c1) idx1 |= myAudV0[0];
      if(aud1_c1) idx1 |= (myAudV1[0] << 4);
      uInt16 vol1 = myVolLUT[idx1][myHWVol];
      mySamples.push(vol1);
      mySamples.push(vol1);

      uInt32 idx2 = 0;
      if(aud0_c2) idx2 |= myAudV0[1];
      if(aud1_c2) idx2 |= (myAudV1[1] << 4);
      uInt16 vol2 = myVolLUT[idx2][myHWVol];
      mySamples.push(vol2);
      mySamples.push(vol2);
      break;
    }
    case Hardware2Stereo: // stereo sampling with 2 hardware channels
    {
      mySamples.push(myVolLUT[aud0_c1 ? myAudV0[0] : 0][myHWVol]);
      mySamples.push(myVolLUT[aud1_c1 ? myAudV1[0] : 0][myHWVol]);

      mySamples.push(myVolLUT[aud0_c2 ? myAudV0[1] : 0][myHWVol]);
      mySamples.push(myVolLUT[aud1_c2 ? myAudV1[1] : 0][myHWVol]);

      break;
    }
    case Hardware1:       // mono/stereo sampling with only 1 hardware channel
    {
      uInt32 idx1 = 0;
      if(aud0_c1) idx1 |= myAudV0[0];
      if(aud1_c1) idx1 |= (myAudV1[0] << 4);
      mySamples.push(myVolLUT[idx1][myHWVol]);

      uInt32 idx2 = 0;
      if(aud0_c2) idx2 |= myAudV0[1];
      if(aud1_c2) idx2 |= (myAudV1[1] << 4);
      mySamples.push(myVolLUT[idx2][myHWVol]);
      break;
    }
  }

  /////////////////////////////////////////////////
  // End of line, allow deferred updates
  /////////////////////////////////////////////////

  // AUDC0
  if(myDeferredC0 != 0xff)  // write occurred after cycle2:phase2
  {
    myAudC0[0][0] = myAudC0[0][1] = myAudC0[1][0] = myAudC0[1][1] = myDeferredC0;
    myDeferredC0 = 0xff;
  }
  else                      // write occurred after cycle1:phase1
    myAudC0[0][0] = myAudC0[1][1];

  // AUDC1
  if(myDeferredC1 != 0xff)  // write occurred after cycle2:phase2
  {
    myAudC1[0][0] = myAudC1[0][1] = myAudC1[1][0] = myAudC1[1][1] = myDeferredC1;
    myDeferredC1 = 0xff;
  }
  else                      // write occurred after cycle1:phase1
    myAudC1[0][0] = myAudC1[1][1];

  // AUDF0
  if(myDeferredF0 != 0xff)  // write occurred after cycle2:phase2
  {
    myAudF0[0] = myAudF0[1] = myDeferredF0;
    myDeferredF0 = 0xff;
  }
  else                      // write occurred after cycle1:phase1
    myAudF0[0] = myAudF0[1];

  // AUDF1
  if(myDeferredF1 != 0xff)  // write occurred after cycle2:phase2
  {
    myAudF1[0] = myAudF1[1] = myDeferredF1;
    myDeferredF1 = 0xff;
  }
  else                      // write occurred after cycle1:phase1
    myAudF1[0] = myAudF1[1];

  // AUDV0
  if(myDeferredV0 != 0xff)  // write occurred after cycle2:phase2
  {
    myAudV0[0] = myAudV0[1] = myDeferredV0;
    myDeferredV0 = 0xff;
  }
  else                      // write occurred after cycle1:phase1
    myAudV0[0] = myAudV0[1];

  // AUDV1
  if(myDeferredV1 != 0xff)  // write occurred after cycle2:phase2
  {
    myAudV1[0] = myAudV1[1] = myDeferredV1;
    myDeferredV1 = 0xff;
  }
  else                      // write occurred after cycle1:phase1
    myAudV1[0] = myAudV1[1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIASound::volume(uInt32 percent)
{
  myHWVol = std::min(percent, 100u);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIASound::updateAudioState(AudioState& state, uInt32 audf, uInt32* audc)
{
  bool pulse_fb;  // pulse counter LFSR feedback

  // -- Logic updated on phase 1 of the audio clock --
  if(state.clk_en)
  {
    // Latch bit 4 of noise counter
    state.noise_cnt_4 = state.noise_cnt & 0x01;

    // Latch pulse counter hold condition
    switch(audc[0] & 0x03)
    {
      case 0x00:
        state.pulse_cnt_hold = false;
        break;
      case 0x01:
        state.pulse_cnt_hold = false;
        break;
      case 0x02:
        state.pulse_cnt_hold = ((state.noise_cnt & 0x1e) != 0x02);
        break;
      case 0x03:
        state.pulse_cnt_hold = !state.noise_cnt_4;
        break;
    }

    // Latch noise counter LFSR feedback
    switch(audc[0] & 0x03)
    {
      case 0x00:
        state.noise_fb = ((state.pulse_cnt & 0x01) ^ (state.noise_cnt & 0x01)) |
                         !((state.noise_cnt ? 1 : 0) | (state.pulse_cnt != 0x0a)) |
                         !(audc[0] & 0x0c);
        break;
      default:
        state.noise_fb = (((state.noise_cnt & 0x04) ? 1 : 0) ^ (state.noise_cnt & 0x01)) |
                         !state.noise_cnt;
        break;
    }
  }

  // Set (or clear) audio clock enable
  state.clk_en = (state.div_cnt == audf);

  // Increment clock divider counter
  if((state.div_cnt == audf) || (state.div_cnt == 31))
    state.div_cnt = 0;
  else
    state.div_cnt++;

  // -- Logic updated on phase 2 of the audio clock --
  if(state.clk_en)
  {
    // Evaluate pulse counter combinatorial logic
    switch(audc[1] >> 2)
    {
      case 0x00:
        pulse_fb = (((state.pulse_cnt & 0x02) ? 1 : 0) ^ (state.pulse_cnt & 0x01)) &
                   (state.pulse_cnt != 0x0a) && (audc[1] & 0x03);
        break;
      case 0x01:
        pulse_fb = !(state.pulse_cnt & 0x08);
        break;
      case 0x02:
        pulse_fb = !state.noise_cnt_4;
        break;
      case 0x03:
        pulse_fb = !((state.pulse_cnt & 0x02) || !(state.pulse_cnt & 0x0e));
        break;
    }

    // Increment noise counter
    state.noise_cnt >>= 1;
    if(state.noise_fb)
      state.noise_cnt |= 0x10;

    // Increment pulse counter
    if(!state.pulse_cnt_hold)
    {
      state.pulse_cnt = (~(state.pulse_cnt >> 1) & 0x07);
      if(pulse_fb)
        state.pulse_cnt |= 0x08;
    }
  }

  // Pulse generator output
  return (state.pulse_cnt & 0x01);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIASound::save(Serializer& out) const
{
#if 0
  try
  {
    out.putString(name());

    // Only get the TIA sound registers if sound is enabled
    if(myIsInitializedFlag)
    {
      out.putByte(myTIASound.get(TIARegister::AUDC0));
      out.putByte(myTIASound.get(TIARegister::AUDC1));
      out.putByte(myTIASound.get(TIARegister::AUDF0));
      out.putByte(myTIASound.get(TIARegister::AUDF1));
      out.putByte(myTIASound.get(TIARegister::AUDV0));
      out.putByte(myTIASound.get(TIARegister::AUDV1));
    }
    else
      for(int i = 0; i < 6; ++i)
        out.putByte(0);

    out.putInt(myLastRegisterSetCycle);
  }
  catch(...)
  {
    myOSystem.logMessage("ERROR: SoundSDL2::save", 0);
    return false;
  }
#endif
  return true;  // TODO
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIASound::load(Serializer& in)
{
#if 0
  try
  {
    if(in.getString() != name())
      return false;

    // Only update the TIA sound registers if sound is enabled
    // Make sure to empty the queue of previous sound fragments
    if(myIsInitializedFlag)
    {
      SDL_PauseAudio(1);
      myRegWriteQueue.clear();
      myTIASound.set(TIARegister::AUDC0, in.getByte());
      myTIASound.set(TIARegister::AUDC1, in.getByte());
      myTIASound.set(TIARegister::AUDF0, in.getByte());
      myTIASound.set(TIARegister::AUDF1, in.getByte());
      myTIASound.set(TIARegister::AUDV0, in.getByte());
      myTIASound.set(TIARegister::AUDV1, in.getByte());
      if(!myIsMuted) SDL_PauseAudio(0);
    }
    else
      for(int i = 0; i < 6; ++i)
        in.getByte();

    myLastRegisterSetCycle = in.getInt();
  }
  catch(...)
  {
    myOSystem.logMessage("ERROR: SoundSDL2::load", 0);
    return false;
  }
#endif
  return true;  // TODO
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIASound::AudioState::save(Serializer& out) const
{
  try
  {
    out.putBool(clk_en);
    out.putBool(noise_fb);
    out.putBool(noise_cnt_4);
    out.putBool(pulse_cnt_hold);
    out.putInt(div_cnt);
    out.putInt(noise_cnt);
    out.putInt(pulse_cnt);
  }
  catch(...)
  {
    // FIXME myOSystem.logMessage("ERROR: TIASnd_state::save", 0);
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIASound::AudioState::load(Serializer& in)
{
  try
  {
    clk_en = in.getBool();
    noise_fb = in.getBool();
    noise_cnt_4 = in.getBool();
    pulse_cnt_hold = in.getBool();
    div_cnt = in.getInt();
    noise_cnt = in.getInt();
    pulse_cnt = in.getInt();
  }
  catch(...)
  {
    // FIXME myOSystem.logMessage("ERROR: TIASnd_state::load", 0);
    return false;
  }
  return true;
}
