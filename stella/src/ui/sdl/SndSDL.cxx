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
// $Id: SndSDL.cxx,v 1.1 2002-08-15 00:29:40 stephena Exp $
//============================================================================

#include <SDL.h>

#include "TIASound.h"
#include "SndSDL.hxx"

#define FRAGSIZE  1 << 10

static int currentVolume = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void fill_audio(void* udata, Uint8* stream, int len)
{  
  Uint8 buffer[FRAGSIZE];

  if(len > FRAGSIZE)
    len = FRAGSIZE;

  Tia_process(buffer, len);
  SDL_MixAudio(stream, buffer, len, currentVolume);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SoundSDL(int volume, bool activate)
{
  myEnabled = activate;

  if(!myEnabled)
    return;

  if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
  {
    cerr << "Couldn't init SDL audio system: " << SDL_GetError() << endl;
    myEnabled = false;
    return;
  }

  SDL_AudioSpec desired, obtained;

  desired.freq     = 31400;
  desired.samples  = FRAGSIZE;
  desired.format   = AUDIO_U8;
  desired.callback = fill_audio;
  desired.userdata = NULL;
  desired.channels = 1;

  if(SDL_OpenAudio(&desired, &obtained) < 0)
  {
    cerr << "Couldn't open SDL audio: " << SDL_GetError() << endl;
    myEnabled = false;
    return;
  }
 
  /* Initialize the TIA Sound Library */
  Tia_sound_init(31400, obtained.freq);

  isMuted = false;
  setVolume(volume);

  SDL_PauseAudio(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  if(myEnabled)
    SDL_CloseAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::set(Sound::Register reg, uInt8 value)
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
      return;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::mute(bool state)
{
  if(!myEnabled)
    return;

  // Ignore multiple calls to do the same thing
  if(isMuted == state)
    return;

  isMuted = state;

  SDL_PauseAudio(isMuted ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setVolume(int percent)
{
  if(!myEnabled)
    return;

  if((percent >= 0) && (percent <= 100))
    currentVolume = (int) (((float) percent / 100.0) * (float) SDL_MIX_MAXVOLUME);
}
