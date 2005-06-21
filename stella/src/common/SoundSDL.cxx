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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundSDL.cxx,v 1.18 2005-06-21 18:46:33 stephena Exp $
//============================================================================

#include <sstream>
#include <cassert>
#include <cmath>
#include <SDL.h>

#include "TIASound.h"
#include "FrameBuffer.hxx"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"

#include "SoundSDL.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SoundSDL(OSystem* osystem)
    : Sound(osystem),
      myIsEnabled(osystem->settings().getBool("sound")),
      myIsInitializedFlag(false),
      myFragmentSizeLogBase2(0),
      myIsMuted(false)
{
  initialize(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  // Close the SDL audio system if it's initialized
  closeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setEnabled(bool enable)
{
  myIsEnabled = enable;
  myOSystem->settings().setBool("sound", enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::initialize(bool forcerestart)
{
  // Check whether to start the sound subsystem
  if(!myIsEnabled)
  {
    closeAudio();
    if(myOSystem->settings().getBool("showinfo"))
      cout << "Sound disabled." << endl << endl;
    return;
  }

  // Clear the sound queue  FIXME - still an annoying partial sound playing?
  SDL_PauseAudio(1);
  Tia_clear_registers();
  myRegWriteQueue.clear();
  SDL_PauseAudio(0);

  if(forcerestart && myIsInitializedFlag)
    closeAudio();

  bool isAlreadyInitialized = (SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) > 0;

  if(!isAlreadyInitialized)
  {
    myIsInitializedFlag = false;
    myIsMuted = false;
    myLastRegisterSetCycle = 0;

    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
      cerr << "WARNING: Couldn't initialize SDL audio system! " << endl;
      cerr << "         " << SDL_GetError() << endl;
      return;
    }
    else
    {
      uInt32 fragsize = myOSystem->settings().getInt("fragsize");

      SDL_AudioSpec desired;
      desired.freq = 31400;
      desired.format = AUDIO_U8;
      desired.channels = 1;
      desired.samples = fragsize;
      desired.callback = callback;
      desired.userdata = (void*)this;

      if(SDL_OpenAudio(&desired, &myHardwareSpec) < 0)
      {
        cerr << "WARNING: Couldn't open SDL audio system! " << endl;
        cerr << "         " << SDL_GetError() << endl;
        return;
      }

      // Make sure the sample buffer isn't to big (if it is the sound code
      // will not work so we'll need to disable the audio support)
      if(((float)myHardwareSpec.samples / (float)myHardwareSpec.freq) >= 0.25)
      {
        cerr << "WARNING: Sound device doesn't support realtime audio! Make ";
        cerr << "sure a sound" << endl;
        cerr << "         server isn't running.  Audio is disabled." << endl;

        SDL_CloseAudio();
        return;
      }

      myIsInitializedFlag = true;
      myIsMuted = false;
      myFragmentSizeLogBase2 = log((double)myHardwareSpec.samples) / log(2.0);

      /*
        cerr << "Freq: " << (int)myHardwareSpec.freq << endl;
        cerr << "Format: " << (int)myHardwareSpec.format << endl;
        cerr << "Channels: " << (int)myHardwareSpec.channels << endl;
        cerr << "Silence: " << (int)myHardwareSpec.silence << endl;
        cerr << "Samples: " << (int)myHardwareSpec.samples << endl;
        cerr << "Size: " << (int)myHardwareSpec.size << endl;
      */

      // Now initialize the TIASound object which will actually generate sound
      Tia_sound_init(31400, myHardwareSpec.freq);

      // And start the SDL sound subsystem ...
      SDL_PauseAudio(0);

      // Adjust volume to that defined in settings
      myVolume = myOSystem->settings().getInt("volume");
      setVolume(myVolume);

      // Show some info
      if(myOSystem->settings().getBool("showinfo"))
        cout << "Sound enabled:" << endl
             << "  Volume   : "  << myVolume << endl
             << "  Frag size: "  << fragsize << endl << endl;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    // Ignore multiple calls to do the same thing
    if(myIsMuted == state)
    {
      return;
    }

    myIsMuted = state;

    SDL_PauseAudio(myIsMuted ? 1 : 0);
    myRegWriteQueue.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::reset()
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(1);
    myIsMuted = false;
    myLastRegisterSetCycle = 0;
    myRegWriteQueue.clear();
    SDL_PauseAudio(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setVolume(Int32 percent)
{
  if(myIsInitializedFlag)
  {
    if((percent >= 0) && (percent <= 100))
    {
      myOSystem->settings().setInt("volume", percent);
      SDL_LockAudio();
      myVolume = percent;
      Tia_volume(percent);
      SDL_UnlockAudio();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::adjustVolume(Int8 direction)
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

  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::adjustCycleCounter(Int32 amount)
{
  myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setFrameRate(uInt32 framerate)
{
  myDisplayFrameRate = framerate;
  myLastRegisterSetCycle = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::set(uInt16 addr, uInt8 value, Int32 cycle)
{
  SDL_LockAudio();

  // First, calulate how many seconds would have past since the last
  // register write on a real 2600
  double delta = (((double)(cycle - myLastRegisterSetCycle)) / 
      (1193191.66666667));

  // Now, adjust the time based on the frame rate the user has selected
// FIXME - not sure this is needed anymore, since the display framerate
//         and sound framerate are always locked in sync; hence 1:1
  delta = delta * (myDisplayFrameRate / 60.0);//FIXME (double)myOSystem->console().frameRate());

  RegWrite info;
  info.addr = addr;
  info.value = value;
  info.delta = delta;
  myRegWriteQueue.enqueue(info);

  // Update last cycle counter to the current cycle
  myLastRegisterSetCycle = cycle;

  SDL_UnlockAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::processFragment(uInt8* stream, Int32 length)
{
  if(!myIsInitializedFlag)
    return;

  // If there are excessive items on the queue then we'll remove some
  if(myRegWriteQueue.duration() > (myFragmentSizeLogBase2 / myDisplayFrameRate))
  {
    double removed = 0.0;
    while(removed < ((myFragmentSizeLogBase2 - 1) / myDisplayFrameRate))
    {
      RegWrite& info = myRegWriteQueue.front();
      removed += info.delta;
      Update_tia_sound(info.addr, info.value);
      myRegWriteQueue.dequeue();
    }
//    cout << "Removed Items from RegWriteQueue!" << endl;
  }

  double position = 0.0;
  double remaining = length;

  while(remaining > 0.0)
  {
    if(myRegWriteQueue.size() == 0)
    {
      // There are no more pending TIA sound register updates so we'll
      // use the current settings to finish filling the sound fragment
      Tia_process(stream + (uInt32)position, length - (uInt32)position);

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

      // How long will the remaing samples in the fragment take to play
      double duration = remaining / (double)myHardwareSpec.freq;

      // Does the register update occur before the end of the fragment?
      if(info.delta <= duration)
      {
        // If the register update time hasn't already passed then
        // process samples upto the point where it should occur
        if(info.delta > 0.0)
        {
          // Process the fragment upto the next TIA register write.  We
          // round the count passed to Tia_process up if needed.
          double samples = (myHardwareSpec.freq * info.delta);
          Tia_process(stream + (uInt32)position, (uInt32)samples +
              (uInt32)(position + samples) - 
              ((uInt32)position + (uInt32)samples));
          position += samples;
          remaining -= samples;
        }
        Update_tia_sound(info.addr, info.value);
        myRegWriteQueue.dequeue();
      }
      else
      {
        // The next register update occurs in the next fragment so finish
        // this fragment with the current TIA settings and reduce the register
        // update delay by the corresponding amount of time
        Tia_process(stream + (uInt32)position, length - (uInt32)position);
        info.delta -= duration;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::callback(void* udata, uInt8* stream, int len)
{
  SoundSDL* sound = (SoundSDL*)udata;
  sound->processFragment(stream, (Int32)len);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::closeAudio()
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    myIsInitializedFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::load(Deserializer& in)
{
  string device = "TIASound";

  try
  {
    if(in.getString() != device)
      return false;

    uInt8 reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0, reg6 = 0;
    reg1 = (uInt8) in.getLong();
    reg2 = (uInt8) in.getLong();
    reg3 = (uInt8) in.getLong();
    reg4 = (uInt8) in.getLong();
    reg5 = (uInt8) in.getLong();
    reg6 = (uInt8) in.getLong();

    myLastRegisterSetCycle = (Int32) in.getLong();

    // Only update the TIA sound registers if sound is enabled
    // Make sure to empty the queue of previous sound fragments
    if(myIsInitializedFlag)
    {
      SDL_PauseAudio(1);
      myRegWriteQueue.clear();
      Tia_set_registers(reg1, reg2, reg3, reg4, reg5, reg6);
      SDL_PauseAudio(0);
    }
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in load state for " << device << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::save(Serializer& out)
{
  string device = "TIASound";

  try
  {
    out.putString(device);

    uInt8 reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0, reg6 = 0;

    // Only get the TIA sound registers if sound is enabled
    if(myIsInitializedFlag)
      Tia_get_registers(&reg1, &reg2, &reg3, &reg4, &reg5, &reg6);

    out.putLong(reg1);
    out.putLong(reg2);
    out.putLong(reg3);
    out.putLong(reg4);
    out.putLong(reg5);
    out.putLong(reg6);

    out.putLong(myLastRegisterSetCycle);
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in save state for " << device << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::RegWriteQueue::RegWriteQueue(uInt32 capacity)
    : myCapacity(capacity),
      myBuffer(0),
      mySize(0),
      myHead(0),
      myTail(0)
{
  myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SoundSDL::RegWriteQueue::duration()
{
  double duration = 0.0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].delta;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
  {
    grow();
  }

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::RegWrite& SoundSDL::RegWriteQueue::front()
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::grow()
{
  RegWrite* buffer = new RegWrite[myCapacity * 2];
  for(uInt32 i = 0; i < mySize; ++i)
  {
    buffer[i] = myBuffer[(myHead + i) % myCapacity];
  }
  myHead = 0;
  myTail = mySize;
  myCapacity = myCapacity * 2;
  delete[] myBuffer;
  myBuffer = buffer;
}
