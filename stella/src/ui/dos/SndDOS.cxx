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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SndDOS.cxx,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
//============================================================================

#include <unistd.h>

#include "SndDOS.hxx"
#include "TIASound.h"
#include "sbdrv.h"

/**
  Compute the buffer size to use based on the given sample rate

  @param The sample rate to compute the buffer size for
*/
static unsigned long computeBufferSize(int sampleRate)
{
  int t;

  for(t = 7; t <= 12; ++t)
  {
    if((1 << t) > (sampleRate / 60))
    {
      return (1 << (t - 1));
    }
  }

  return 256;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundDOS::SoundDOS()
{
  int sampleRate = 15700;
  int DMABufferSize = computeBufferSize(sampleRate);

  if(OpenSB(sampleRate, DMABufferSize))
  {
    myEnabled = true;

    // Initialize TIA Sound Library
    Tia_sound_init(31400, sampleRate);

    // Start playing audio
    Start_audio_output(AUTO_DMA, 
        (void (*)(unsigned char*, short unsigned int))Tia_process);
  }
  else
  {
    // Oops, couldn't open SB so we're not enabled :-(
    myEnabled = false;
    return;
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundDOS::~SoundDOS()
{
  if(myEnabled)
  {
    CloseSB();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::set(Sound::Register reg, uInt8 value)
{
  if(!myEnabled)
    return;

  switch(reg) 
  {
    case AUDC0:
      Update_tia_sound(0x15, value);
      break;
    
    case AUDC1:
      Update_tia_sound(0x16, value);
      break;

    case AUDF0:
      Update_tia_sound(0x17, value);
      break;
    
    case AUDF1:
      Update_tia_sound(0x18, value);
      break;

    case AUDV0:
      Update_tia_sound(0x19, value);
      break;

    case AUDV1:
      Update_tia_sound(0x1A, value);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::mute(bool state)
{
  if(state)
  {
    Stop_audio_output();
  }
  else
  {
    Start_audio_output(AUTO_DMA, 
        (void (*)(unsigned char*, short unsigned int))Tia_process);
  }
}

