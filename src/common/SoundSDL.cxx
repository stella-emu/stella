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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef SOUND_SUPPORT

#include <cmath>

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
    ostringstream buf;

    buf << "WARNING: Failed to initialize SDL audio system! \n"
        << "         " << SDL_GetError() << '\n';
    Logger::error(buf.view());
    return;
  }

  SDL_zero(mySpec);
  if(!myAudioSettings.enabled())
    Logger::info("Sound disabled\n");

  Logger::debug("SoundSDL::SoundSDL initialized");

  // Reserve 8K for the audio buffer; seems to be enough on most systems
  myBuffer.reserve(8_KB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  ASSERT_MAIN_THREAD;

  if(!myIsInitializedFlag)
    return;

  SDL_DestroyAudioStream(myStream);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::openDevice()
{
  ASSERT_MAIN_THREAD;

  mySpec = { SDL_AUDIO_F32, 2, static_cast<int>(myAudioSettings.sampleRate()) };

  if(myIsInitializedFlag)
  {
    SDL_DestroyAudioStream(myStream);
    myStream = nullptr;
  }

  myStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &mySpec, audioCallback, this);
  if(myStream == nullptr)
  {
    ostringstream buf;

    buf << "WARNING: Couldn't open SDL audio device! \n"
        << "         " << SDL_GetError() << '\n';
    Logger::error(buf.view());

    return myIsInitializedFlag = false;
  }
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

  myAudioQueue = audioQueue;
  myEmulationTiming = emulationTiming;
  myUnderrun = true;
  myCurrentFragment = nullptr;

  myAudioQueue->ignoreOverflows(!myAudioSettings.enabled());
//  myWavHandler.setSpeed(262 * 60 * 2. / myEmulationTiming->audioSampleRate());

  // Do we need to re-open the sound device?
  // Only do this when absolutely necessary
  if(myAudioSettings.sampleRate() != static_cast<uInt32>(mySpec.freq))
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
  const bool wasMuted = myVolumeFactor == 0;
  mute(!wasMuted);

  string message = "Sound ";
  message += !myAudioSettings.enabled()
    ? "disabled"
    : (wasMuted ? "unmuted" : "muted");

  myOSystem.frameBuffer().showTextMessage(message);
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

//     myWavHandler.pause(enable);
  }
  return wasPaused;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setVolume(uInt32 volume, bool persist)
{
  if(myIsInitializedFlag && (volume <= 100))
  {
    myVolumeFactor = myAudioSettings.enabled()
      ? static_cast<float>(volume) / 100.F
      : 0.F;

    SDL_SetAudioStreamGain(myStream, myVolumeFactor);
//     myWavHandler.setVolumeFactor(myVolumeFactor);

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
  ostringstream strval;
  (percent) ? strval << percent << "%" : strval << "Off";
  myOSystem.frameBuffer().showGaugeMessage("Volume", strval.view(), percent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SoundSDL::about() const
{
  ostringstream buf;
  buf << "Sound enabled:\n"
      << "  Volume:   " << myAudioSettings.volume() << "%\n"
      << "  Channels: " << static_cast<uInt32>(mySpec.channels)
      << (myAudioQueue->isStereo() ? " (Stereo)" : " (Mono)") << '\n'
      << "  Preset:   ";
  switch(myAudioSettings.preset())
  {
    using enum AudioSettings::Preset;
    case custom:
      buf << "Custom\n";
      break;
    case lowQualityMediumLag:
      buf << "Low quality, medium lag\n";
      break;
    case highQualityMediumLag:
      buf << "High quality, medium lag\n";
      break;
    case highQualityLowLag:
      buf << "High quality, low lag\n";
      break;
    case ultraQualityMinimalLag:
      buf << "Ultra quality, minimal lag\n";
      break;
    default:
      break;  // Not supposed to get here
  }
  buf << "    Sample rate:   " << static_cast<uInt32>(mySpec.freq) << " Hz\n";
  buf << "    Resampling:    ";
  switch(myAudioSettings.resamplingQuality())
  {
    using enum AudioSettings::ResamplingQuality;
    case nearestNeighbour:
      buf << "Quality 1, nearest neighbor\n";
      break;
    case lanczos_2:
      buf << "Quality 2, Lanczos (a = 2)\n";
      break;
    case lanczos_3:
      buf << "Quality 3, Lanczos (a = 3)\n";
      break;
    default:
      break;  // Not supposed to get here
  }
  buf << "    Headroom:      " << std::fixed << std::setprecision(1)
      << (0.5 * myAudioSettings.headroom()) << " frames\n"
      << "    Buffer size:   " << std::fixed << std::setprecision(1)
      << (0.5 * myAudioSettings.bufferSize()) << " frames\n";
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::initResampler()
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
    Resampler::Format(mySpec.freq, 1024, mySpec.channels > 1);

  switch(myAudioSettings.resamplingQuality())
  {
    using enum AudioSettings::ResamplingQuality;
    case nearestNeighbour:
      myResampler = make_unique<SimpleResampler>(formatFrom, formatTo,
                                                 nextFragmentCallback);
      break;

    case lanczos_2:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo,
                                                  nextFragmentCallback, 2);
      break;

    case lanczos_3:
      myResampler = make_unique<LanczosResampler>(formatFrom, formatTo,
                                                  nextFragmentCallback, 3);
      break;

    default:
      throw runtime_error("invalid resampling quality");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::audioCallback(void* object, SDL_AudioStream* stream,
                             int additional_amt, int)
{
  auto* self = static_cast<SoundSDL*>(object);
  std::vector<uInt8>& buf = self->myBuffer;

  // Make sure we always have enough room in the buffer
  if(additional_amt >= static_cast<int>(buf.capacity()))
    buf.resize(additional_amt);

  // The stream is 32-bit float (even though this callback is 8-bits), since
  // the resampler and TIA audio subsystem always generate float samples
  auto* s = reinterpret_cast<float*>(buf.data());
  self->myResampler->fillFragment(s, additional_amt >> 2);

  SDL_PutAudioStreamData(stream, buf.data(), additional_amt);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::playWav(const string& fileName, uInt32 position, uInt32 length)
{
#if 0
  return myWavHandler.play(fileName, position, length);
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::stopWav()
{
#if 0
  myWavHandler.stop();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::wavSize() const
{
#if 0
  return myWavHandler.size();
#endif
  return 0;
}

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::WavHandlerSDL::play(
    const string& fileName, uInt32 position, uInt32 length)
{
  // Load WAV file
  if(fileName != myFilename || myBuffer == nullptr)
  {
    if(myBuffer)
    {
      SDL_free(myBuffer);
      myBuffer = nullptr;
    }
    SDL_zero(mySpec);
    if(SDL_LoadWAV(fileName.c_str(), &mySpec, &myBuffer, &myLength) == nullptr)
      return false;

    // Set the callback function
    mySpec.callback = callback;
    mySpec.userdata = this;
  }
  if(position > myLength)
    return false;

  myFilename = fileName;

  myRemaining = length
    ? std::min(length, myLength - position)
    : myLength;
  myPos = myBuffer + position;

  // Open audio device
  if(!myDevice)
  {
    myDevice = SDL_OpenAudioDevice(device, 0, &mySpec, nullptr, 0);
    if(!myDevice)
      return false;

    // Play audio
    pause(false);
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandlerSDL::stop()
{
  if(myBuffer)
  {
    // Clean up
    myRemaining = 0;
    SDL_CloseAudioDevice(myDevice);  myDevice = 0;
    SDL_free(myBuffer);  myBuffer = nullptr;
  }
  if(myCvtBuffer)
  {
    myCvtBuffer.reset();
    myCvtBufferSize = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandlerSDL::processWav(uInt8* stream, uInt32 len)
{
  SDL_memset(stream, mySpec.silence, len);
  if(myRemaining)
  {
    if(mySpeed != 1.0)
    {
      const int origLen = len;
      len = std::round(len / mySpeed);
      const int newFreq =
        std::round(static_cast<double>(mySpec.freq) * origLen / len);

      if(len > myRemaining)  // NOLINT(readability-use-std-min-max)
        len = myRemaining;

      SDL_AudioCVT cvt;
      SDL_BuildAudioCVT(&cvt, mySpec.format, mySpec.channels, mySpec.freq,
                              mySpec.format, mySpec.channels, newFreq);
      SDL_assert(cvt.needed); // Obviously, this one is always needed.
      cvt.len = len * mySpec.channels;  // Mono 8 bit sample frames

      if(!myCvtBuffer || std::cmp_less(myCvtBufferSize, cvt.len * cvt.len_mult))
      {
        myCvtBufferSize = cvt.len * cvt.len_mult;
        myCvtBuffer = make_unique<uInt8[]>(myCvtBufferSize);
      }
      cvt.buf = myCvtBuffer.get();

      // Read original data into conversion buffer
      SDL_memcpy(cvt.buf, myPos, cvt.len);
      SDL_ConvertAudio(&cvt);
      // Mix volume adjusted WAV data into silent buffer
      SDL_MixAudioFormat(stream, cvt.buf, mySpec.format, cvt.len_cvt,
                         SDL_MIX_MAXVOLUME * SoundSDL::myVolumeFactor);
    }
    else
    {
      if(len > myRemaining)  // NOLINT(readability-use-std-min-max)
        len = myRemaining;

      // Mix volume adjusted WAV data into silent buffer
      SDL_MixAudioFormat(stream, myPos, mySpec.format, len,
                         SDL_MIX_MAXVOLUME * myVolumeFactor);
    }
    myPos += len;
    myRemaining -= len;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandlerSDL::wavCallback(void* object, SDL_AudioStream* stream,
                                          int additional_amt, int total_amt)
{
  static_cast<WavHandlerSDL*>(object)->processWav(
      stream, static_cast<uInt32>(len));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::WavHandlerSDL::~WavHandlerSDL()
{
  if(myDevice)
  {
    SDL_CloseAudioDevice(myDevice);
    SDL_FreeWAV(myBuffer);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::WavHandlerSDL::pause(bool state) const
{
  if(myDevice)
    if (state ? 1 : 0) {
      SDL_PauseAudioDevice(myDevice);
    }
  else {
    SDL_ResumeAudioDevice(myDevice);
  }
}
#endif

#endif  // SOUND_SUPPORT
