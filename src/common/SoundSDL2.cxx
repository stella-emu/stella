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
#include "AudioSettings.hxx"
#include "audio/SimpleResampler.hxx"
#include "audio/LanczosResampler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem, AudioSettings& audioSettings)
  : Sound(osystem),
    myIsInitializedFlag(false),
    myVolume(100),
    myVolumeFactor(0xffff),
    myCurrentFragment(nullptr),
    myAudioSettings(audioSettings)
{
  myOSystem.logMessage("SoundSDL2::SoundSDL2 started ...", 2);

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    ostringstream buf;

    buf << "WARNING: Failed to initialize SDL audio system! " << endl
        << "         " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    return;
  }

  SDL_zero(myHardwareSpec);
  if(!openDevice())
    return;

  mute(true);

  myOSystem.logMessage("SoundSDL2::SoundSDL2 initialized", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::~SoundSDL2()
{
  if (!myIsInitializedFlag) return;

  SDL_CloseAudioDevice(myDevice);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL2::openDevice()
{
  SDL_AudioSpec desired;
  desired.freq   = myAudioSettings.sampleRate();
  desired.format = AUDIO_F32SYS;
  desired.channels = 2;
  desired.samples  = static_cast<Uint16>(myAudioSettings.fragmentSize());
  desired.callback = callback;
  desired.userdata = static_cast<void*>(this);

  if(myIsInitializedFlag)
    SDL_CloseAudioDevice(myDevice);
  myDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &myHardwareSpec,
                                 SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

  if(myDevice == 0)
  {
    ostringstream buf;

    buf << "WARNING: Couldn't open SDL audio device! " << endl
        << "         " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);

    return myIsInitializedFlag = false;
  }
  return myIsInitializedFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setEnabled(bool state)
{
  myAudioSettings.setEnabled(state);
  if (myAudioQueue) myAudioQueue->ignoreOverflows(!state);

  myOSystem.logMessage(state ? "SoundSDL2::setEnabled(true)" :
                               "SoundSDL2::setEnabled(false)", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::open(shared_ptr<AudioQueue> audioQueue,
                     EmulationTiming* emulationTiming)
{
  // Do we need to re-open the sound device?
  // Only do this when absolutely necessary
  if(myAudioSettings.sampleRate() != uInt32(myHardwareSpec.freq) ||
     myAudioSettings.fragmentSize() != uInt32(myHardwareSpec.samples))
    openDevice();

  myEmulationTiming = emulationTiming;

  myOSystem.logMessage("SoundSDL2::open started ...", 2);
  mute(true);

  audioQueue->ignoreOverflows(!myAudioSettings.enabled());
  if(!myAudioSettings.enabled())
  {
    myOSystem.logMessage("Sound disabled\n", 1);
    return;
  }

  myAudioQueue = audioQueue;
  myUnderrun = true;
  myCurrentFragment = nullptr;

  // Adjust volume to that defined in settings
  setVolume(myAudioSettings.volume());

  initResampler();

  // Show some info
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:      " << myVolume << endl
      << "  Frag size:   " << uInt32(myHardwareSpec.samples) << endl
      << "  Frequency:   " << uInt32(myHardwareSpec.freq) << endl
      << "  Channels:    " << uInt32(myHardwareSpec.channels) << endl
      << "  Resampling:  ";
  switch (myAudioSettings.resamplingQuality()) {
    case AudioSettings::ResamplingQuality::nearestNeightbour:
      buf << "quality 1, nearest neighbor" << endl;
      break;
    case AudioSettings::ResamplingQuality::lanczos_2:
      buf << "quality 2, nearest Lanczos (a = 2)" << endl;
      break;
    case AudioSettings::ResamplingQuality::lanczos_3:
      buf << "quality 3, nearest Lanczos (a = 3)" << endl;
      break;
    default:
      buf << "unknown resampler" << endl;
      break;
  }
  myOSystem.logMessage(buf.str(), 1);

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
  myAudioQueue.reset();
  myCurrentFragment = nullptr;

  myOSystem.logMessage("SoundSDL2::close", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudioDevice(myDevice, state ? 1 : 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setVolume(uInt32 percent)
{
  if(myIsInitializedFlag && (percent <= 100))
  {
    myAudioSettings.setVolume(percent);
    myVolume = percent;

    SDL_LockAudioDevice(myDevice);
    myVolumeFactor = static_cast<float>(percent) / 100.f;
    SDL_UnlockAudioDevice(myDevice);
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
  return myHardwareSpec.samples;
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
    Int16* nextFragment = nullptr;

    if (myUnderrun)
      nextFragment = myAudioQueue->size() >= myEmulationTiming->prebufferFragmentCount() ?
          myAudioQueue->dequeue(myCurrentFragment) : nullptr;
    else
      nextFragment = myAudioQueue->dequeue(myCurrentFragment);

    myUnderrun = nextFragment == nullptr;
    if (nextFragment) myCurrentFragment = nextFragment;

    return nextFragment;
  };

  Resampler::Format formatFrom =
    Resampler::Format(myEmulationTiming->audioSampleRate(), myAudioQueue->fragmentSize(), myAudioQueue->isStereo());
  Resampler::Format formatTo =
    Resampler::Format(myHardwareSpec.freq, myHardwareSpec.samples, myHardwareSpec.channels > 1);


  switch (myAudioSettings.resamplingQuality()) {
    case AudioSettings::ResamplingQuality::nearestNeightbour:
      myResampler = make_unique<SimpleResampler>(formatFrom, formatTo, nextFragmentCallback);
      break;

    case AudioSettings::ResamplingQuality::lanczos_2:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo, nextFragmentCallback, 2);
      break;

    case AudioSettings::ResamplingQuality::lanczos_3:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo, nextFragmentCallback, 3);
      break;

    default:
      throw runtime_error("invalid resampling quality");
  }
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
