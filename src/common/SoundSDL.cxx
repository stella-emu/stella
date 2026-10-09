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

#ifdef SOUND_SUPPORT

#include <cmath>
#include <iomanip>

#include "SDL_lib.hxx"
#include "Logger.hxx"
#include "FrameBuffer.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "AudioQueue.hxx"
#include "EmulationTiming.hxx"
#include "AudioSettings.hxx"
#include "audio/SimpleResampler.hxx"
#include "audio/LanczosResampler.hxx"
#include "ThreadDebugging.hxx"

#include "SoundSDL.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SoundSDL(OSystem& osystem, AudioSettings& audioSettings)
  : Sound{osystem},
    myAudioSettings{audioSettings}
{
  ASSERT_MAIN_THREAD;

  Logger::debug("SoundSDL::SoundSDL started ...");

  if(!SDL_InitSubSystem(SDL_INIT_AUDIO))
  {
    Logger::error(std::format("WARNING: Failed to initialize SDL audio system! \n"
      "         {}\n", SDL_GetError()));
    return;
  }

  SDL_zero(mySpec);
  if(!myAudioSettings.enabled())
    Logger::info("Sound disabled\n");

  Logger::debug("SoundSDL::SoundSDL initialized");

  // Reserve 8K for the audio buffer; seems to be enough on most systems
  myBuffer.resize(8_KB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  ASSERT_MAIN_THREAD;

  // The WAV stream must go before the audio subsystem is shut down below
  myWavHandler.close();

  if(!myIsInitializedFlag)
    return;

  SDL_DestroyAudioStream(myStream);
  SDL_CloseAudioDevice(myDevice);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::openDevice()
{
  ASSERT_MAIN_THREAD;

  const auto SOUND_ERROR = [this] -> bool
  {
    Logger::error(std::format("WARNING: Couldn't open SDL audio device! \n"
      "         {}\n", SDL_GetError()));
    return myIsInitializedFlag = false;
  };

  if(myIsInitializedFlag)
  {
    SDL_DestroyAudioStream(myStream);
    myStream = nullptr;
    // This also unbinds the WAV stream, which is rebound below
    SDL_CloseAudioDevice(myDevice);
  }

  mySpec = { SDL_AUDIO_F32, 2, I32(myAudioSettings.sampleRate()) };

  myDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &mySpec);
  if(myDevice == 0)
    return SOUND_ERROR();
  myStream = SDL_CreateAudioStream(&mySpec, nullptr);
  if(!myStream)
    return SOUND_ERROR();
  if(!SDL_BindAudioStream(myDevice, myStream))
    return SOUND_ERROR();
  if(!SDL_SetAudioStreamGetCallback(myStream, audioCallback, this))
    return SOUND_ERROR();
  myWavHandler.rebind(myDevice);

  return myIsInitializedFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setEnabled(bool enable)
{
  if(myIsInitializedFlag)
  {
    mute(!enable);
    pause(!enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::open(shared_ptr<AudioQueue> audioQueue,
                    shared_ptr<const EmulationTiming> emulationTiming)
{
  if(!myAudioSettings.enabled())
  {
    Logger::info("Sound disabled\n");
    return;
  }
  pause(true);

  const string pre_about = myAboutString;

  myAudioQueue = std::move(audioQueue);
  myEmulationTiming = std::move(emulationTiming);
  myUnderrun = true;
  myCurrentFragment = nullptr;

  myAudioQueue->ignoreOverflows(!myAudioSettings.enabled());
  myWavHandler.setSpeed(262 * 60 * 2. / myEmulationTiming->audioSampleRate());

  // Do we need to re-open the sound device?
  // Only do this when absolutely necessary
  if(myAudioSettings.sampleRate() != U32(mySpec.freq))
    openDevice();

  Logger::debug("SoundSDL::open started ...");

  // Adjust volume to that defined in settings
  setVolume(myAudioSettings.volume());

  // Initialize resampler; must be done after the sound device has opened
  initResampler();

  // Show some info
  myAboutString = about();
  if(myAboutString != pre_about)
    Logger::info(myAboutString);

  // And start the SDL sound subsystem ...
  pause(false);

  Logger::debug("SoundSDL::open finished");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::mute(bool enable)
{
  if(enable)
    setVolume(0, false);
  else
    setVolume(myAudioSettings.volume());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::toggleMute()
{
  const bool wasMuted = (myVolumeFactor == 0.F);
  mute(!wasMuted);

  myOSystem.frameBuffer().showTextMessage(
    std::format("Sound {}", !myAudioSettings.enabled()
      ? "disabled"
      : (wasMuted ? "unmuted" : "muted")));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::pause(bool enable)
{
  ASSERT_MAIN_THREAD;

  const bool wasPaused = SDL_AudioStreamDevicePaused(myStream);
  if(myIsInitializedFlag)
  {
    if(enable) SDL_PauseAudioStreamDevice(myStream);
    else       SDL_ResumeAudioStreamDevice(myStream);

    myWavHandler.pause(enable);
  }
  return wasPaused;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setVolume(uInt32 volume, bool persist)
{
  if(myIsInitializedFlag && (volume <= 100))
  {
    myVolumeFactor = myAudioSettings.enabled()
      ? FLT(volume) / 100.F
      : 0.F;

    SDL_SetAudioStreamGain(myStream, myVolumeFactor);
    myWavHandler.setVolumeFactor(myVolumeFactor);

    if(persist)
      myAudioSettings.setVolume(volume);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::adjustVolume(int direction)
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
  myOSystem.frameBuffer().showGaugeMessage("Volume",
    percent ? std::format("{}%", percent) : "Off",
    percent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SoundSDL::about() const
{
  const string_view presetStr = [this] -> string_view {
    switch(myAudioSettings.preset())
    {
      using enum AudioSettings::Preset;
      case custom:                  return "Custom\n";
      case lowQualityMediumLag:     return "Low quality, medium lag\n";
      case highQualityMediumLag:    return "High quality, medium lag\n";
      case highQualityLowLag:       return "High quality, low lag\n";
      case ultraQualityMinimalLag:  return "Ultra quality, minimal lag\n";
      default:                      return "\n";
    }
  }();

  const string_view resampleStr = [this] -> string_view {
    switch(myAudioSettings.resamplingQuality())
    {
      using enum AudioSettings::ResamplingQuality;
      case nearestNeighbour: return "Quality 1, nearest neighbor\n";
      case lanczos_2:        return "Quality 2, Lanczos (a = 2)\n";
      case lanczos_3:        return "Quality 3, Lanczos (a = 3)\n";
      default:               return "\n";
    }
  }();

  return std::format(
    "Sound enabled:\n"
    "  Volume:   {}%\n"
    "  Channels: {}{}\n"
    "  Preset:   {}"
    "    Sample rate:   {} Hz\n"
    "    Resampling:    {}"
    "    Headroom:      {:.1f} frames\n"
    "    Buffer size:   {:.1f} frames\n",
    myAudioSettings.volume(),
    U32(mySpec.channels),
    myAudioQueue->isStereo() ? " (Stereo)" : " (Mono)",
    presetStr,
    U32(mySpec.freq),
    resampleStr,
    0.5 * myAudioSettings.headroom(),
    0.5 * myAudioSettings.bufferSize()
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::initResampler()
{
  const Resampler::NextFragmentCallback nextFragmentCallback = [this] -> Int16* {
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
    Resampler::Format(mySpec.freq, 1024, mySpec.channels > 1);

  switch(myAudioSettings.resamplingQuality())
  {
    using enum AudioSettings::ResamplingQuality;
    case nearestNeighbour:
      myResampler = std::make_unique<SimpleResampler>(formatFrom, formatTo,
                                                 nextFragmentCallback);
      break;

    case lanczos_2:
      myResampler = std::make_unique<LanczosResampler>(formatFrom, formatTo,
                                                  nextFragmentCallback, 2);
      break;

    case lanczos_3:
      myResampler = std::make_unique<LanczosResampler>(formatFrom, formatTo,
                                                  nextFragmentCallback, 3);
      break;

    default:
      throw std::runtime_error("invalid resampling quality");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::audioCallback(void* object, SDL_AudioStream* stream,
                             int additional_amt, int)
{
  auto* self = static_cast<SoundSDL*>(object);
  if(self->myResampler)
  {
    std::vector<uInt8>& buf = self->myBuffer;

    // Make sure we always have enough room in the buffer
    if(std::cmp_greater(additional_amt, buf.size()))
      buf.resize(additional_amt);

    // The stream is 32-bit float (even though this callback is 8-bits), since
    // the resampler and TIA audio subsystem always generate float samples
    auto* s = reinterpret_cast<float*>(buf.data());
    // SDL documents additional_amt as a byte count, never negative, so the
    // unsigned shift below is fine
    self->myResampler->fillFragment(s, U32(additional_amt) >> 2U);

    SDL_PutAudioStreamData(stream, buf.data(), additional_amt);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::playWav(const string& fileName, uInt32 position, uInt32 length)
{
  if(myStream)
    return myWavHandler.play(SDL_GetAudioStreamDevice(myStream),
                             fileName, position, length);
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::stopWav()
{
  myWavHandler.stop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::wavSize() const
{
  return myWavHandler.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::WavHandler::play(SDL_AudioDeviceID device,
    const string& fileName, uInt32 position, uInt32 length)
{
  // One stream is created on first use, then reused for every WAV
  if(myStream == nullptr)
  {
    myStream = SDL_CreateAudioStream(nullptr, nullptr);
    if(myStream == nullptr)
      return false;
    if(!SDL_SetAudioStreamGetCallback(myStream, WavHandler::wavCallback, this) ||
       !SDL_BindAudioStream(device, myStream))
    {
      SDL_DestroyAudioStream(myStream);  myStream = nullptr;
      return false;
    }
    SDL_SetAudioStreamGain(myStream, myVolumeFactor);
  }

  // wavCallback runs under this same lock, so it never sees a half-changed WAV
  SDL_LockAudioStream(myStream);
  const bool loaded = [&] -> bool {
    // Stop the current WAV, dropping whatever of it is still queued
    myRemaining = 0;
    SDL_ClearAudioStream(myStream);

    // Load WAV file
    if(fileName != myFilename || myBuffer == nullptr)
    {
      if(myBuffer)
      {
        SDL_free(myBuffer);
        myBuffer = nullptr;
      }
      SDL_zero(mySpec);
      if(!SDL_LoadWAV(fileName.c_str(), &mySpec, &myBuffer, &myLength))
        return false;
    }

    if(position > myLength || !SDL_SetAudioStreamFormat(myStream, &mySpec, nullptr))
      return false;

    myFilename = fileName;
    myRemaining = length
      ? std::min(length, myLength - position)
      : myLength;
    myPos = myBuffer + position;

    return true;
  }();
  SDL_UnlockAudioStream(myStream);

  return loaded;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandler::stop()
{
  if(myBuffer)
  {
    // Clean up under the stream lock, as in play()
    SDL_LockAudioStream(myStream);
    myRemaining = 0;
    SDL_ClearAudioStream(myStream);
    SDL_free(myBuffer);  myBuffer = nullptr;
    SDL_UnlockAudioStream(myStream);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandler::close()
{
  stop();
  SDL_DestroyAudioStream(myStream);  myStream = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandler::rebind(SDL_AudioDeviceID device) const
{
  if(myStream)
    SDL_BindAudioStream(device, myStream);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandler::setVolumeFactor(float volumeFactor)
{
  myVolumeFactor = volumeFactor;
  if(myStream)
    SDL_SetAudioStreamGain(myStream, myVolumeFactor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandler::pause(bool state) const
{
  if(myStream)
  {
    if(state) SDL_PauseAudioStreamDevice(myStream);
    else      SDL_ResumeAudioStreamDevice(myStream);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandler::wavCallback(void* object, SDL_AudioStream* stream,
                                       int additional_amt, int)
{
  auto* self = static_cast<WavHandler*>(object);
  auto len = U32(additional_amt);
  auto& remaining = self->myRemaining;

  if(remaining)
  {
    if(self->mySpeed != 1.0)
      len = U32(std::round(len / self->mySpeed));

    if(len > remaining)  // NOLINT(readability-use-std-min-max)
      len = remaining;

    SDL_PutAudioStreamData(stream, self->myPos, len);

    self->myPos += len;
    remaining -= len;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::WavHandler::~WavHandler()
{
  ASSERT_MAIN_THREAD;

  if(myBuffer)
    SDL_free(myBuffer);
}

#endif  // SOUND_SUPPORT
