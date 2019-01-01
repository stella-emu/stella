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
#include "StaggeredLogger.hxx"

#include "ThreadDebugging.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem, AudioSettings& audioSettings)
  : Sound(osystem),
    myIsInitializedFlag(false),
    myVolume(100),
    myVolumeFactor(0xffff),
    myDevice(0),
    myEmulationTiming(nullptr),
    myCurrentFragment(nullptr),
    myUnderrun(false),
    myAudioSettings(audioSettings)
{
  ASSERT_MAIN_THREAD;

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
  ASSERT_MAIN_THREAD;

  if (!myIsInitializedFlag) return;

  SDL_CloseAudioDevice(myDevice);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL2::openDevice()
{
  ASSERT_MAIN_THREAD;

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
  string pre_about = myAboutString;

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
  myAboutString = about();
  if(myAboutString != pre_about)
    myOSystem.logMessage(myAboutString, 1);

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
bool SoundSDL2::mute(bool state)
{
  bool oldstate = SDL_GetAudioDeviceStatus(myDevice) == SDL_AUDIO_PAUSED;
  if(myIsInitializedFlag)
    SDL_PauseAudioDevice(myDevice, state ? 1 : 0);

  return oldstate;
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
string SoundSDL2::about() const
{
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:   " << myVolume << "%" << endl
      << "  Channels: " << uInt32(myHardwareSpec.channels)
      << (myAudioQueue->isStereo() ? " (Stereo)" : " (Mono)") << endl
      << "  Preset:   ";
  switch (myAudioSettings.preset()) {
    case AudioSettings::Preset::custom:
      buf << "Custom" << endl;
      break;
    case AudioSettings::Preset::lowQualityMediumLag:
      buf << "Low quality, medium lag" << endl;
      break;
    case AudioSettings::Preset::highQualityMediumLag:
      buf << "High quality, medium lag" << endl;
      break;
    case AudioSettings::Preset::highQualityLowLag:
      buf << "High quality, low lag" << endl;
      break;
    case AudioSettings::Preset::veryHighQualityVeryLowLag:
      buf << "Very high quality, very low lag" << endl;
      break;
  }
  buf << "    Fragment size: " << uInt32(myHardwareSpec.samples) << " bytes" << endl
      << "    Sample rate:   " << uInt32(myHardwareSpec.freq) << " Hz" << endl;
  buf << "    Resampling:    ";
  switch(myAudioSettings.resamplingQuality())
  {
    case AudioSettings::ResamplingQuality::nearestNeightbour:
      buf << "Quality 1, nearest neighbor" << endl;
      break;
    case AudioSettings::ResamplingQuality::lanczos_2:
      buf << "Quality 2, Lanczos (a = 2)" << endl;
      break;
    case AudioSettings::ResamplingQuality::lanczos_3:
      buf << "Quality 3, Lanczos (a = 3)" << endl;
      break;
  }
  buf << "    Headroom:      " << std::fixed << std::setprecision(1)
      << (0.5 * myAudioSettings.headroom()) << " frames" << endl
      << "    Buffer size:   " << std::fixed << std::setprecision(1)
      << (0.5 * myAudioSettings.bufferSize()) << " frames" << endl;
  return buf.str();
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

  StaggeredLogger::Logger logger = [this](string msg) { myOSystem.logMessage(msg, 1); };

  Resampler::Format formatFrom =
    Resampler::Format(myEmulationTiming->audioSampleRate(), myAudioQueue->fragmentSize(), myAudioQueue->isStereo());
  Resampler::Format formatTo =
    Resampler::Format(myHardwareSpec.freq, myHardwareSpec.samples, myHardwareSpec.channels > 1);

  switch (myAudioSettings.resamplingQuality()) {
    case AudioSettings::ResamplingQuality::nearestNeightbour:
      myResampler = make_unique<SimpleResampler>(formatFrom, formatTo, nextFragmentCallback, logger);
      break;

    case AudioSettings::ResamplingQuality::lanczos_2:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo, nextFragmentCallback, 2, logger);
      break;

    case AudioSettings::ResamplingQuality::lanczos_3:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo, nextFragmentCallback, 3, logger);
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
