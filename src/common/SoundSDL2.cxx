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

namespace {
  inline Int16 applyVolume(Int16 sample, Int32 volumeFactor)
  {
    return static_cast<Int16>(static_cast<Int32>(sample) * volumeFactor / 0xffff);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem)
  : Sound(osystem),
    myIsInitializedFlag(false),
    myVolume(100),
    myVolumeFactor(0xffff),
    myAudioQueue(0),
    myCurrentFragment(0),
    myFragmentBufferSize(0)
{
  myOSystem.logMessage("SoundSDL2::SoundSDL2 started ...", 2);

#ifdef BSPF_WINDOWS
  // TODO - remove the following code once we convert to the new sound
  //        core, and use 32-bit floating point samples and do
  //        our own resampling
  SDL_setenv("SDL_AUDIODRIVER", "directsound", true);
#endif

  // The sound system is opened only once per program run, to eliminate
  // issues with opening and closing it multiple times
  // This fixes a bug most prevalent with ATI video cards in Windows,
  // whereby sound stopped working after the first video change
  SDL_AudioSpec desired;
  // desired.freq   = myOSystem.settings().getInt("freq");
  desired.freq = 48000;
  desired.format = AUDIO_S16SYS;
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
void SoundSDL2::open(shared_ptr<AudioQueue> audioQueue)
{
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
  myTimeIndex = 0;
  myFragmentIndex = 0;
  myFragmentBufferSize = static_cast<uInt32>(
    ceil(
      1.5 * static_cast<double>(myHardwareSpec.samples) / static_cast<double>(myAudioQueue->fragmentSize())
          * static_cast<double>(myAudioQueue->sampleRate()) / static_cast<double>(myHardwareSpec.freq)
    )
  );

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
    myVolumeFactor = static_cast<Int32>(floor(static_cast<double>(0xffff) * static_cast<double>(myVolume) / 100.));
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
void SoundSDL2::processFragment(Int16* stream, uInt32 length)
{
  if (myUnderrun && myAudioQueue->size() > 0) {
    myUnderrun = false;
    myCurrentFragment = myAudioQueue->dequeue(myCurrentFragment);
    myFragmentIndex = 0;
  }

  if (!myCurrentFragment) {
    memset(stream, 0, 2 * length);
    return;
  }

  const bool isStereoTIA = myAudioQueue->isStereo();
  const bool isStereo = myHardwareSpec.channels == 2;
  const uInt32 sampleRateTIA = myAudioQueue->sampleRate();
  const uInt32 sampleRate = myHardwareSpec.freq;
  const uInt32 fragmentSize = myAudioQueue->fragmentSize();
  const uInt32 outputSamples = isStereo ? (length >> 1) : length;

  for (uInt32 i = 0; i < outputSamples; i++) {
    myTimeIndex += sampleRateTIA;

    if (myTimeIndex >= sampleRate) {
      myFragmentIndex += myTimeIndex / sampleRate;
      myTimeIndex %= sampleRate;
    }

    if (myFragmentIndex >= fragmentSize) {
      myFragmentIndex %= fragmentSize;

      Int16* nextFragment = myAudioQueue->dequeue(myCurrentFragment);
      if (nextFragment)
        myCurrentFragment = nextFragment;
      else
        myUnderrun = true;
    }

    if (isStereo) {
      if (isStereoTIA) {
        stream[2*i]     = applyVolume(myCurrentFragment[2*myFragmentIndex], myVolumeFactor);
        stream[2*i + 1] = applyVolume(myCurrentFragment[2*myFragmentIndex + 1], myVolumeFactor);
      } else {
        stream[2*i] = stream[2*i + 1] = applyVolume(myCurrentFragment[myFragmentIndex], myVolumeFactor);
      }
    } else {
      if (isStereoTIA) {
        stream[i] = applyVolume(
          (myCurrentFragment[2*myFragmentIndex] / 2) + (myCurrentFragment[2*myFragmentIndex + 1] / 2),
          myVolumeFactor
        );
      } else {
        stream[i] = applyVolume(myCurrentFragment[myFragmentIndex], myVolumeFactor);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::callback(void* udata, uInt8* stream, int len)
{
  SoundSDL2* self = static_cast<SoundSDL2*>(udata);

  if (self->myAudioQueue)
    self->processFragment(reinterpret_cast<Int16*>(stream), len >> 1);
  else
    SDL_memset(stream, 0, len);
}

#endif  // SOUND_SUPPORT
