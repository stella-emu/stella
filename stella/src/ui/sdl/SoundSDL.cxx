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
// $Id: SoundSDL.cxx,v 1.2 2002-10-12 15:24:49 stephena Exp $
//============================================================================

#include <SDL.h>

#include "SoundSDL.hxx"

static uInt8 _myCurrentVolume;
static MediaSource* _myMediaSource;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SoundSDL(bool activate)
    : myIsInitializedFlag(false),
      mySampleRate(31400),
      myFragSize(512),
      myIsMuted(true)
{
  myIsInitializedFlag = activate;

  if(!myIsInitializedFlag)
    return;

  if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
  {
    cerr << "Couldn't init SDL audio system: " << SDL_GetError() << endl;
    myIsInitializedFlag = false;
    mySampleRate = 0;
    return;
  }

  SDL_AudioSpec desired, obtained;

  desired.freq     = mySampleRate;
  desired.samples  = myFragSize;
  desired.format   = AUDIO_U8;
  desired.callback = fillAudio;
  desired.userdata = NULL;
  desired.channels = 1;

  if(SDL_OpenAudio(&desired, &obtained) < 0)
  {
    cerr << "Couldn't open SDL audio: " << SDL_GetError() << endl;
    myIsInitializedFlag = false;
    mySampleRate = 0;
    return;
  }

  myIsMuted = false;
  mySampleRate = obtained.freq;

  // Take care of the static stuff ...
  _myCurrentVolume = 0;
  _myMediaSource = (MediaSource*) NULL;

  SDL_PauseAudio(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::getSampleRate() const
{
  return myIsInitializedFlag ? mySampleRate : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::mute(bool state)
{
  if(!myIsInitializedFlag)
    return;

  // Ignore multiple calls to do the same thing
  if(myIsMuted == state)
    return;

  myIsMuted = state;

  SDL_PauseAudio(myIsMuted ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::close()
{
  if(myIsInitializedFlag)
    SDL_CloseAudio();

  myIsInitializedFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setSoundVolume(uInt32 percent)
{
  if(!myIsInitializedFlag)
    return;

  if((percent >= 0) && (percent <= 100))
    _myCurrentVolume = (int) (((float) percent / 100.0) * (float) SDL_MIX_MAXVOLUME);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setMediaSource(MediaSource& mediaSource)
{
  _myMediaSource = &mediaSource;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::fillAudio(void* udata, uInt8* stream, Int32 len)
{  
  if(!_myMediaSource)
    return;

  // Dequeue samples as long as full fragments are available
  if(_myMediaSource->numberOfAudioSamples() >= (uInt32) len)
  {
    uInt8 buffer[len];
    _myMediaSource->dequeueAudioSamples(buffer, (uInt32)len);
    SDL_MixAudio(stream, buffer, len, _myCurrentVolume);
  }
}
