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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef SOUND_SUPPORT

#include <sstream>
#include <cassert>
#include <cmath>

#include "SDL_lib.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "SoundSDL2.hxx"
#include "AudioQueue.hxx"
#include "EmulationTiming.hxx"
#include "audio/SimpleResampler.hxx"
#include "audio/LanczosResampler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem)
  : Sound(osystem),
    myIsInitializedFlag(false),
    myVolume(100),
    myVolumeFactor(0xffff),
    myAudioQueue(0),
    myCurrentFragment(0)
{
  myOSystem.logMessage("SoundSDL2::SoundSDL2 started ...", 2);

  // The sound system is opened only once per program run, to eliminate
  // issues with opening and closing it multiple times
  // This fixes a bug most prevalent with ATI video cards in Windows,
  // whereby sound stopped working after the first video change
  SDL_AudioSpec desired;
  desired.freq   = myOSystem.settings().getInt("freq");
  desired.format = AUDIO_F32SYS;
  desired.channels = 2;
  desired.samples  = myOSystem.settings().getInt("fragsize");
  desired.callback = callback;
  desired.userdata = static_cast<void*>(this);

  ostringstream buf;
  if(SDL_OpenAudio(&desired, &myHardwareSpec) < 0)
  {
    buf << "WARNING: Couldn't open SDL audio system! " << endl
        << "         " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    return;
  }

  // Make sure the sample buffer isn't to big (if it is the sound code
  // will not work so we'll need to disable the audio support)
  if((float(myHardwareSpec.samples) / float(myHardwareSpec.freq)) >= 0.25)
  {
    buf << "WARNING: Sound device doesn't support realtime audio! Make "
        << "sure a sound" << endl
        << "         server isn't running.  Audio is disabled." << endl;
    myOSystem.logMessage(buf.str(), 0);

    SDL_CloseAudio();
    return;
  }

  myIsInitializedFlag = true;

  mute(true);

  myOSystem.logMessage("SoundSDL2::SoundSDL2 initialized", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::~SoundSDL2()
{
  if (!myIsInitializedFlag) return;

  SDL_CloseAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setEnabled(bool state)
{
  myOSystem.settings().setValue("sound", state);

  myOSystem.logMessage(state ? "SoundSDL2::setEnabled(true)" :
                                "SoundSDL2::setEnabled(false)", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::open(shared_ptr<AudioQueue> audioQueue, EmulationTiming* emulationTiming)
{
  this->emulationTiming = emulationTiming;

  myOSystem.logMessage("SoundSDL2::open started ...", 2);
  mute(true);

  if(!myOSystem.settings().getBool("sound"))
  {
    myOSystem.logMessage("Sound disabled\n", 1);
    return;
  }

  myAudioQueue = audioQueue;
  myUnderrun = true;
  myCurrentFragment = 0;

  // Adjust volume to that defined in settings
  setVolume(myOSystem.settings().getInt("volume"));

  // Show some info
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:      " << myVolume << endl
      << "  Frag size:   " << uInt32(myHardwareSpec.samples) << endl
      << "  Frequency:   " << uInt32(myHardwareSpec.freq) << endl
      << "  Channels:    " << uInt32(myHardwareSpec.channels)
      << endl;
  myOSystem.logMessage(buf.str(), 1);

  initResampler();

  // And start the SDL sound subsystem ...
  mute(false);

  myOSystem.logMessage("SoundSDL2::open finished", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::close()
{
  if(!myIsInitializedFlag) return;

  mute(true);

  if (myAudioQueue) myAudioQueue->closeSink(myCurrentFragment);
  myAudioQueue = 0;
  myCurrentFragment = 0;

  myOSystem.logMessage("SoundSDL2::close", 2);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(state ? 1 : 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::reset()
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setVolume(Int32 percent)
{
  if(myIsInitializedFlag && (percent >= 0) && (percent <= 100))
  {
    myOSystem.settings().setValue("volume", percent);
    myVolume = percent;

    SDL_LockAudio();
    myVolumeFactor = std::pow(static_cast<float>(percent) / 100.f, 2.f);
    SDL_UnlockAudio();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::adjustVolume(Int8 direction)
{
  ostringstream strval;
  string message;

  Int32 percent = myVolume;

  if(direction == -1)
    percent -= 2;
  else if(direction == 1)
    percent += 2;

  if((percent < 0) || (percent > 100))
    return;

  setVolume(percent);

  // Now show an onscreen message
  strval << percent;
  message = "Volume set to ";
  message += strval.str();

  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL2::getFragmentSize() const
{
  return myHardwareSpec.size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL2::getSampleRate() const
{
  return myHardwareSpec.freq;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::processFragment(float* stream, uInt32 length)
{
  myResampler->fillFragment(stream, length);

  for (uInt32 i = 0; i < length; i++) stream[i] = stream[i] * myVolumeFactor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::initResampler()
{
  Resampler::NextFragmentCallback nextFragmentCallback = [this] () -> Int16* {
    Int16* nextFragment = 0;

    if (myUnderrun)
      nextFragment = myAudioQueue->size() > emulationTiming->prebufferFragmentCount() ? myAudioQueue->dequeue(myCurrentFragment) : 0;
    else
      nextFragment = myAudioQueue->dequeue(myCurrentFragment);

    myUnderrun = nextFragment == 0;
    if (nextFragment) myCurrentFragment = nextFragment;

    return nextFragment;
  };

  myResampler = make_unique<LanczosResampler>(
    Resampler::Format(myAudioQueue->sampleRate(), myAudioQueue->fragmentSize(), myAudioQueue->isStereo()),
    Resampler::Format(myHardwareSpec.freq, myHardwareSpec.samples, myHardwareSpec.channels > 1),
    nextFragmentCallback,
    2
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::callback(void* udata, uInt8* stream, int len)
{
  SoundSDL2* self = static_cast<SoundSDL2*>(udata);

  if (self->myAudioQueue)
    self->processFragment(reinterpret_cast<float*>(stream), len >> 2);
  else
    SDL_memset(stream, 0, len);
}

#endif  // SOUND_SUPPORT
