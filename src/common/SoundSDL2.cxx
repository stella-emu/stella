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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifdef SOUND_SUPPORT

#include <sstream>
#include <cassert>
#include <cmath>
#include <SDL.h>

#include "TIA.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "SoundSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(OSystem& osystem)
  : Sound(osystem),
    myIsEnabled(false),
    myIsInitializedFlag(false),
    myIsMuted(true),
    myVolume(100)
{
  myOSystem.logMessage("SoundSDL2::SoundSDL2 started ...", 2);

  // The sound system is opened only once per program run, to eliminate
  // issues with opening and closing it multiple times
  // This fixes a bug most prevalent with ATI video cards in Windows,
  // whereby sound stopped working after the first video change
  SDL_AudioSpec desired;
  SDL_memset(&desired, 0, sizeof(desired));
  desired.freq   = myOSystem.settings().getInt("freq");
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples  = myOSystem.settings().getInt("fragsize");
  desired.callback = getSamples;
  desired.userdata = static_cast<void*>(this);

  ostringstream buf;
  if(SDL_OpenAudio(&desired, &myHardwareSpec) < 0)
  {
    buf << "WARNING: Couldn't open SDL audio system! " << endl
        << "         " << SDL_GetError() << endl;
    myOSystem.logMessage(buf.str(), 0);
    return;
  }

  myIsInitializedFlag = true;
  SDL_PauseAudio(1);

  myOSystem.logMessage("SoundSDL2::SoundSDL2 initialized", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::~SoundSDL2()
{
  // Close the SDL audio system if it's initialized
  if(myIsInitializedFlag)
  {
//FIXME    SDL_PauseAudio(1);
    SDL_CloseAudio();
    myIsEnabled = myIsInitializedFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setEnabled(bool state)
{
  myOSystem.settings().setValue("sound", state);

  myOSystem.logMessage(state ? "SoundSDL2::setEnabled(true)" : 
                                "SoundSDL2::setEnabled(false)", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::open(bool stereo)
{
  myOSystem.logMessage("SoundSDL2::open started ...", 2);
  myIsEnabled = false;
  mute(true);
  if(!myIsInitializedFlag || !myOSystem.settings().getBool("sound"))
  {
    myOSystem.logMessage("Sound disabled\n", 1);
    return;
  }

  // Now initialize the TIASound object which will actually generate sound
  const string& chanResult =
      myOSystem.console().tia().sound().channels(myHardwareSpec.channels, stereo);

  // Adjust volume to that defined in settings
  myVolume = myOSystem.settings().getInt("volume");
  setVolume(myVolume);

  // Show some info
  ostringstream buf;
  buf << "Sound enabled:"  << endl
      << "  Volume:      " << myVolume << endl
      << "  Frag size:   " << uInt32(myHardwareSpec.samples) << endl
      << "  Frequency:   " << uInt32(myHardwareSpec.freq) << endl
      << "  Channels:    " << uInt32(myHardwareSpec.channels)
                           << " (" << chanResult << ")" << endl
      << endl;
  myOSystem.logMessage(buf.str(), 1);

  // And start the SDL sound subsystem ...
  myIsEnabled = true;
  SDL_PauseAudio(0);//myIsMuted ? 1 : 0);

  myOSystem.logMessage("SoundSDL2::open finished", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::close()
{
  if(myIsInitializedFlag)
  {
    myIsEnabled = false;
    SDL_PauseAudio(1);
    if(myOSystem.hasConsole())
      myOSystem.console().tia().sound().reset();
    myOSystem.logMessage("SoundSDL2::close", 2);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    myIsMuted = state;
    if(myOSystem.hasConsole())
      myOSystem.console().tia().sound().volume(myIsMuted ? 0 : myVolume);

    //    SDL_PauseAudio(myIsMuted ? 1 : 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::reset()
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(1);
    if(myOSystem.hasConsole())
      myOSystem.console().tia().sound().reset();
//    mute(myIsMuted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setVolume(uInt32 volume)
{
  if(myIsInitializedFlag && (volume <= 100))
  {
    myOSystem.settings().setValue("volume", volume);
    myVolume = volume;
    if(myOSystem.hasConsole())
      myOSystem.console().tia().sound().volume(volume);
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
void SoundSDL2::getSamples(void* udata, uInt8* stream, int len)
{
  SoundSDL2* sound = static_cast<SoundSDL2*>(udata);
  if(sound->myIsEnabled)
  {
    // The callback is requesting 8-bit data, but the TIA sound emulator
    // deals in 16-bit data
    // So, we need to convert the pointer and half the length
    uInt16* buffer = reinterpret_cast<uInt16*>(stream);
    Int32 left =
      sound->myOSystem.console().tia().sound().getSamples(buffer, uInt32(len) >> 1);
    if(left > 0)  // Is silence required?
      SDL_memset(buffer, 0, uInt32(left) << 1);
  }
  else
    SDL_memset(stream, 0, len);  // Write 'silence'
}

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::processFragment(Int16* stream, uInt32 length)
{
  uInt32 channels = myHardwareSpec.channels;
  length = length / channels;

  // If there are excessive items on the queue then we'll remove some
  if(myRegWriteQueue.duration() > myFragmentSizeLogDiv1)
  {
    double removed = 0.0;
    while(removed < myFragmentSizeLogDiv2)
    {
      RegWrite& info = myRegWriteQueue.front();
      removed += info.delta;
      myTIASound.set(info.addr, info.value);
      myRegWriteQueue.dequeue();
    }
  }

  double position = 0.0;
  double remaining = length;

  while(remaining > 0.0)
  {
    if(myRegWriteQueue.size() == 0)
    {
      // There are no more pending TIA sound register updates so we'll
      // use the current settings to finish filling the sound fragment
      myTIASound.process(stream + (uInt32(position) * channels),
          length - uInt32(position));

      // Since we had to fill the fragment we'll reset the cycle counter
      // to zero.  NOTE: This isn't 100% correct, however, it'll do for
      // now.  We should really remember the overrun and remove it from
      // the delta of the next write.
      myLastRegisterSetCycle = 0;
      break;
    }
    else
    {
      // There are pending TIA sound register updates so we need to
      // update the sound buffer to the point of the next register update
      RegWrite& info = myRegWriteQueue.front();

      // How long will the remaining samples in the fragment take to play
      double duration = remaining / myHardwareSpec.freq;

      // Does the register update occur before the end of the fragment?
      if(info.delta <= duration)
      {
        // If the register update time hasn't already passed then
        // process samples upto the point where it should occur
        if(info.delta > 0.0)
        {
          // Process the fragment upto the next TIA register write.  We
          // round the count passed to process up if needed.
          double samples = (myHardwareSpec.freq * info.delta);
          myTIASound.process(stream + (uInt32(position) * channels),
              uInt32(samples) + uInt32(position + samples) - 
              (uInt32(position) + uInt32(samples)));

          position += samples;
          remaining -= samples;
        }
        myTIASound.set(info.addr, info.value);
        myRegWriteQueue.dequeue();
      }
      else
      {
        // The next register update occurs in the next fragment so finish
        // this fragment with the current TIA settings and reduce the register
        // update delay by the corresponding amount of time
        myTIASound.process(stream + (uInt32(position) * channels),
            length - uInt32(position));
        info.delta -= duration;
        break;
      }
    }
  }
}
#endif

#endif  // SOUND_SUPPORT
