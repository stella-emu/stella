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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
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
#include "Logger.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "AudioQueue.hxx"
#include "EmulationTiming.hxx"
#include "AudioSettings.hxx"
#include "audio/SimpleResampler.hxx"
#include "audio/LanczosResampler.hxx"
#include "StaggeredLogger.hxx"
#include "ThreadDebugging.hxx"

#include "SoundSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem, AudioSettings& audioSettings)
  : Sound{osystem},
    myAudioSettings{audioSettings}
{
  ASSERT_MAIN_THREAD;

  Logger::debug("SoundSDL2::SoundSDL2 started ...");

  if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
  {
    ostringstream buf;

    buf << "WARNING: Failed to initialize SDL audio system! " << endl
        << "         " << SDL_GetError() << endl;
    Logger::error(buf.str());
    return;
  }

  queryHardware(myDevices);  // NOLINT

  SDL_zero(myHardwareSpec);
  if(!openDevice())
    return;

  Logger::debug("SoundSDL2::SoundSDL2 initialized");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::~SoundSDL2()
{
  ASSERT_MAIN_THREAD;

  if(!myIsInitializedFlag)
    return;

  if(myWavDevice)
  {
    SDL_CloseAudioDevice(myWavDevice);
    SDL_FreeWAV(myWavBuffer);
  }
  SDL_CloseAudioDevice(myDevice);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::queryHardware(VariantList& devices)
{
  ASSERT_MAIN_THREAD;

  const int numDevices = SDL_GetNumAudioDevices(0);

  // log the available audio devices
  ostringstream s;
  s << "Supported audio devices (" << numDevices << "):";
  Logger::debug(s.str());

  VarList::push_back(devices, "Default", 0);
  for(int i = 0; i < numDevices; ++i)
  {
    ostringstream ss;

    ss << "  " << i + 1 << ": " << SDL_GetAudioDeviceName(i, 0);
    Logger::debug(ss.str());

    VarList::push_back(devices, SDL_GetAudioDeviceName(i, 0), i + 1);
  }
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
  desired.userdata = this;

  if(myIsInitializedFlag)
    SDL_CloseAudioDevice(myDevice);

  myDeviceId = BSPF::clamp(myAudioSettings.device(), 0U,
                           static_cast<uInt32>(myDevices.size() - 1));
  const char* device = myDeviceId ? myDevices.at(myDeviceId).first.c_str() : nullptr;

  myDevice = SDL_OpenAudioDevice(device, 0, &desired, &myHardwareSpec,
                                 SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

  if(myDevice == 0)
  {
    ostringstream buf;

    buf << "WARNING: Couldn't open SDL audio device! " << endl
        << "         " << SDL_GetError() << endl;
    Logger::error(buf.str());

    return myIsInitializedFlag = false;
  }
  return myIsInitializedFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setEnabled(bool enable)
{
cerr << "setEnabled: " << enable << endl;
  mute(!enable);
  pause(!enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::open(shared_ptr<AudioQueue> audioQueue,
                     EmulationTiming* emulationTiming)
{
  const string pre_about = myAboutString;

  // Do we need to re-open the sound device?
  // Only do this when absolutely necessary
  if(myAudioSettings.sampleRate() != static_cast<uInt32>(myHardwareSpec.freq) ||
     myAudioSettings.fragmentSize() != static_cast<uInt32>(myHardwareSpec.samples) ||
     myAudioSettings.device() != myDeviceId)
    openDevice();

  myEmulationTiming = emulationTiming;
  myWavSpeed = 262 * 60 * 2. / myEmulationTiming->audioSampleRate();

  Logger::debug("SoundSDL2::open started ...");

  myAudioSettings.setEnabled(true);
  audioQueue->ignoreOverflows(!myAudioSettings.enabled());
  if(!myAudioSettings.enabled())
  {
    Logger::info("Sound disabled\n");
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
    Logger::info(myAboutString);

  // And start the SDL sound subsystem ...
  pause(false);

  Logger::debug("SoundSDL2::open finished");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::close()
{
  if(!myIsInitializedFlag)
    return;

  if(myAudioQueue)
    myAudioQueue->closeSink(myCurrentFragment);
  myAudioQueue.reset();
  myCurrentFragment = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::mute(bool state)
{
  myAudioSettings.setEnabled(!state);
  if(state)
  {
    SDL_LockAudioDevice(myDevice);
    myVolumeFactor = 0;
    SDL_UnlockAudioDevice(myDevice);
  }
  else
    setVolume(myAudioSettings.volume());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::toggleMute()
{
  const bool state = !myAudioSettings.enabled();
  mute(!state);

  string message = "Sound ";
  message += state ? "unmuted" : "muted";

  myOSystem.frameBuffer().showTextMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL2::pause(bool state)
{
  ASSERT_MAIN_THREAD;

  const bool oldstate = SDL_GetAudioDeviceStatus(myDevice) == SDL_AUDIO_PAUSED;
  if(myIsInitializedFlag)
    SDL_PauseAudioDevice(myDevice, state ? 1 : 0);
  if(myWavDevice)
    SDL_PauseAudioDevice(myWavDevice, state ? 1 : 0);

  return oldstate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setVolume(uInt32 volume)
{
  if(myIsInitializedFlag && (volume <= 100))
  {
    myAudioSettings.setVolume(volume);

    SDL_LockAudioDevice(myDevice);
    myVolumeFactor = myAudioSettings.enabled() ? static_cast<float>(volume) / 100.F : 0;
    SDL_UnlockAudioDevice(myDevice);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::adjustVolume(int direction)
{
  Int32 percent = myAudioSettings.volume();
  percent = BSPF::clamp(percent + direction * 2, 0, 100);

  // Enable audio if it is currently disabled
  const bool enabled = myAudioSettings.enabled();

  if(percent > 0 && direction && !enabled)
  {
    setEnabled(true);
    myOSystem.console().initializeAudio();
  }
  setVolume(percent);

  // Now show an onscreen message
  ostringstream strval;
  (percent) ? strval << percent << "%" : strval << "Off";
  myOSystem.frameBuffer().showGaugeMessage("Volume", strval.str(), percent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SoundSDL2::about() const
{
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:   " << myAudioSettings.volume() << "%" << endl
      << "  Device:   " << myDevices.at(myDeviceId).first << endl
      << "  Channels: " << static_cast<uInt32>(myHardwareSpec.channels)
      << (myAudioQueue->isStereo() ? " (Stereo)" : " (Mono)") << endl
      << "  Preset:   ";
  switch(myAudioSettings.preset())
  {
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
    case AudioSettings::Preset::ultraQualityMinimalLag:
      buf << "Ultra quality, minimal lag" << endl;
      break;
  }
  buf << "    Fragment size: " << static_cast<uInt32>(myHardwareSpec.samples)
      << " bytes" << endl
      << "    Sample rate:   " << static_cast<uInt32>(myHardwareSpec.freq)
      << " Hz" << endl;
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

  for(uInt32 i = 0; i < length; ++i)
    stream[i] *= myVolumeFactor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::initResampler()
{
  const Resampler::NextFragmentCallback nextFragmentCallback = [this] () -> Int16* {
    Int16* nextFragment = nullptr;

    if(myUnderrun)
      nextFragment = myAudioQueue->size() >= myEmulationTiming->prebufferFragmentCount()
        ? myAudioQueue->dequeue(myCurrentFragment)
        : nullptr;
    else
      nextFragment = myAudioQueue->dequeue(myCurrentFragment);

    myUnderrun = nextFragment == nullptr;
    if(nextFragment)
      myCurrentFragment = nextFragment;

    return nextFragment;
  };

  const Resampler::Format formatFrom =
    Resampler::Format(myEmulationTiming->audioSampleRate(),
    myAudioQueue->fragmentSize(), myAudioQueue->isStereo());
  const Resampler::Format formatTo =
    Resampler::Format(myHardwareSpec.freq, myHardwareSpec.samples,
    myHardwareSpec.channels > 1);

  switch(myAudioSettings.resamplingQuality())
  {
    case AudioSettings::ResamplingQuality::nearestNeightbour:
      myResampler = make_unique<SimpleResampler>(formatFrom, formatTo,
                                                 nextFragmentCallback);
      break;

    case AudioSettings::ResamplingQuality::lanczos_2:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo,
                                                  nextFragmentCallback, 2);
      break;

    case AudioSettings::ResamplingQuality::lanczos_3:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo,
                                                  nextFragmentCallback, 3);
      break;

    default:
      throw runtime_error("invalid resampling quality");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::callback(void* udata, uInt8* stream, int len)
{
  auto* self = static_cast<SoundSDL2*>(udata);

  if(self->myAudioQueue)
    self->processFragment(reinterpret_cast<float*>(stream), len >> 2);
  else
    SDL_memset(stream, 0, len);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL2::playWav(const string& fileName, const uInt32 position,
                        const uInt32 length)
{
  // Load WAV file
  if(fileName != myWavFilename || myWavBuffer == nullptr)
  {
    if(myWavBuffer)
    {
      SDL_FreeWAV(myWavBuffer);
      myWavBuffer = nullptr;
    }
    if(SDL_LoadWAV(fileName.c_str(), &myWavSpec, &myWavBuffer, &myWavLength) == nullptr)
      return false;
    // Set the callback function
    myWavSpec.callback = wavCallback;
    myWavSpec.userdata = nullptr;
    //myWavSpec.samples = 4096; // decrease for smaller samples;
  }
  if(position > myWavLength)
    return false;

  myWavFilename = fileName;

  myWavLen = length
    ? std::min(length, myWavLength - position)
    : myWavLength;
  myWavPos = myWavBuffer + position;

  // Open audio device
  if(!myWavDevice)
  {
    const char* device = myDeviceId ? myDevices.at(myDeviceId).first.c_str() : nullptr;

    myWavDevice = SDL_OpenAudioDevice(device, 0, &myWavSpec, nullptr, 0);
    if(!myWavDevice)
      return false;
    // Play audio
    SDL_PauseAudioDevice(myWavDevice, 0);
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::stopWav()
{
  if(myWavBuffer)
  {
    // Clean up
    myWavLen = 0;
    SDL_CloseAudioDevice(myWavDevice);
    myWavDevice = 0;
    SDL_FreeWAV(myWavBuffer);
    myWavBuffer = nullptr;
  }
  if(myWavCvtBuffer)
  {
    myWavCvtBuffer.reset();
    myWavCvtBufferSize = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL2::wavSize() const
{
  return myWavBuffer ? myWavLen : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::wavCallback(void* udata, uInt8* stream, int len)
{
  SDL_memset(stream, myWavSpec.silence, len);
  if(myWavLen)
  {
    if(myWavSpeed != 1.0)
    {
      const int origLen = len;
      len = std::round(len / myWavSpeed);
      const int newFreq =
        std::round(static_cast<double>(myWavSpec.freq) * origLen / len);

      if(static_cast<uInt32>(len) > myWavLen)
        len = myWavLen;

      SDL_AudioCVT cvt;
      SDL_BuildAudioCVT(&cvt, myWavSpec.format, myWavSpec.channels, myWavSpec.freq,
                              myWavSpec.format, myWavSpec.channels, newFreq);
      SDL_assert(cvt.needed); // Obviously, this one is always needed.
      cvt.len = len * myWavSpec.channels;  // Mono 8 bit sample frames

      if(!myWavCvtBuffer ||
          myWavCvtBufferSize < static_cast<uInt32>(cvt.len * cvt.len_mult))
      {
        myWavCvtBufferSize = cvt.len * cvt.len_mult;
        myWavCvtBuffer = make_unique<uInt8[]>(myWavCvtBufferSize);
      }
      cvt.buf = myWavCvtBuffer.get();

      // Read original data into conversion buffer
      SDL_memcpy(cvt.buf, myWavPos, cvt.len);
      SDL_ConvertAudio(&cvt);
      // Mix volume adjusted WAV data into silent buffer
      SDL_MixAudioFormat(stream, cvt.buf, myWavSpec.format, cvt.len_cvt,
                         SDL_MIX_MAXVOLUME * myVolumeFactor);
    }
    else
    {
      if(static_cast<uInt32>(len) > myWavLen)
        len = myWavLen;

      // Mix volume adjusted WAV data into silent buffer
      SDL_MixAudioFormat(stream, myWavPos, myWavSpec.format, len,
                         SDL_MIX_MAXVOLUME * myVolumeFactor);
    }
    myWavPos += len;
    myWavLen -= len;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float SoundSDL2::myVolumeFactor = 0.F;
SDL_AudioSpec SoundSDL2::myWavSpec;   // audio output format
uInt8* SoundSDL2::myWavPos = nullptr; // pointer to the audio buffer to be played
uInt32 SoundSDL2::myWavLen = 0;       // remaining length of the sample we have to play
double SoundSDL2::myWavSpeed = 1.0;
unique_ptr<uInt8[]> SoundSDL2::myWavCvtBuffer;
uInt32 SoundSDL2::myWavCvtBufferSize = 0;

#endif  // SOUND_SUPPORT
