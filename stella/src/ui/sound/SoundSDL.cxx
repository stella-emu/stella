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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundSDL.cxx,v 1.10 2004-04-27 00:50:52 stephena Exp $
//============================================================================

#include <SDL.h>

#include "TIASound.h"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include "System.hxx"

#include "SoundSDL.hxx"

//#define DIGITAL_SOUND

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SoundSDL(uInt32 fragsize, uInt32 queuesize)
    : myCurrentVolume(SDL_MIX_MAXVOLUME),
      myFragmentSize(fragsize),
      myIsInitializedFlag(false),
      myIsMuted(false),
      mySampleRate(31400),
      mySampleQueue(queuesize)
{
  if(1)
  {
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
      cerr << "WARNING: Couldn't initialize SDL audio system! " << endl;
      cerr << "         " << SDL_GetError() << endl;
      myIsInitializedFlag = false;
      mySampleRate = 0;
      return;
    }

    SDL_AudioSpec desired;
    desired.freq = mySampleRate;
    desired.format = AUDIO_U8;
    desired.channels = 1;
    desired.samples = myFragmentSize;
    desired.callback = callback;
    desired.userdata = (void*)this;

    if(SDL_OpenAudio(&desired, &myHardwareSpec) < 0)
    {
      cerr << "WARNING: Couldn't open SDL audio system! " << endl;
      cerr << "         " << SDL_GetError() << endl;
      myIsInitializedFlag = false;
      mySampleRate = 0;
      return;
    }

    // Make sure the sample buffer isn't to big (if it is the sound code
    // will not work so we'll need to disable the audio support)
    if(((float)myHardwareSpec.size / (float)myHardwareSpec.freq) >= 0.25)
    {
      cerr << "WARNING: Audio device doesn't support real time audio! Make ";
      cerr << "sure a sound" << endl;
      cerr << "         server isn't running.  Audio is disabled..." << endl;

      SDL_CloseAudio();

      myIsInitializedFlag = false;
      mySampleRate = 0;
      return;
    }

    myIsInitializedFlag = true;
    myIsMuted = false;
    mySampleRate = myHardwareSpec.freq;
    myFragmentSize = myHardwareSpec.samples;

//    cerr << "Freq: " << (int)mySampleRate << endl;
//    cerr << "Format: " << (int)myHardwareSpec.format << endl;
//    cerr << "Channels: " << (int)myHardwareSpec.channels << endl;
//    cerr << "Silence: " << (int)myHardwareSpec.silence << endl;
//    cerr << "Samples: " << (int)myHardwareSpec.samples << endl;
//    cerr << "Size: " << (int)myHardwareSpec.size << endl;

    // Now initialize the TIASound object which will actually generate sound
    Tia_sound_init(31400, mySampleRate);

    // And start the SDL sound subsystem ...
    SDL_PauseAudio(0);
  }
  else
  {
    myIsInitializedFlag = false;
    myIsMuted = true;
    mySampleRate = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  if(myIsInitializedFlag)
  {
    SDL_CloseAudio();
  }

  myIsInitializedFlag = false;
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
  {
    return;
  }

  // Ignore multiple calls to do the same thing
  if(myIsMuted == state)
  {
    return;
  }

  myIsMuted = state;

  SDL_PauseAudio(myIsMuted ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setVolume(Int32 percent)
{
  if(myIsInitializedFlag)
  {
    if((percent >= 0) && (percent <= 100))
    {
      SDL_LockAudio();
      myCurrentVolume = (uInt32)(((float)percent / 100.0) * SDL_MIX_MAXVOLUME);
      SDL_UnlockAudio();
    }
    else if(percent == -1)   // If -1 has been specified, play sound at default volume
    {
      myCurrentVolume = SDL_MIX_MAXVOLUME;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::update()
{
#if !defined(DIGITAL_SOUND)
  if(!myPauseStatus && myIsInitializedFlag)
  {
    // Make sure we have exclusive access to the sample queue
//    SDL_LockAudio();

    // Generate enough samples to keep the sample queue full to capacity
    uInt32 numbytes = mySampleQueue.capacity() - mySampleQueue.size();
    uInt8 buffer[numbytes];
    Tia_process(buffer, numbytes);
    mySampleQueue.enqueue(buffer, numbytes);

    // Release lock on the sample queue
//    SDL_UnlockAudio();
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::set(uInt16 addr, uInt8 value)
{
#if defined(DIGITAL_SOUND)
  // Calculate the number of samples that need to be generated based on the
  // number of CPU cycles which have passed since the last sound update
  uInt32 samplesToGenerate =
         (mySampleRate * (mySystem->cycles() - myLastSoundUpdateCycle)) / 1190000;

  // Update counters and create samples if there's one sample to generate
  // TODO: This doesn't handle rounding quite right (10/08/2002)
  if(samplesToGenerate >= 1)
  {
    uInt8 buffer[1024];

    for(Int32 sg = (Int32)samplesToGenerate; sg > 0; sg -= 1024)
    {
      Tia_process(buffer, ((sg >= 1024) ? 1024 : sg));
      mySampleQueue.enqueue(buffer, ((sg >= 1024) ? 1024 : sg));
    }
 	 	
    myLastSoundUpdateCycle = myLastSoundUpdateCycle +
        ((samplesToGenerate * 1190000) / mySampleRate);
  }

  if(addr != 0)
  {
    Update_tia_sound(addr, value);
  }
#else
  Update_tia_sound(addr, value);
#endif
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

    out.putLong(myLastSoundUpdateCycle);
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

    myLastSoundUpdateCycle = (Int32) in.getLong();

    // Only update the TIA sound registers if sound is enabled
    if(myIsInitializedFlag)
      Tia_set_registers(reg1, reg2, reg3, reg4, reg5, reg6);
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
void SoundSDL::callback(void* udata, uInt8* stream, int len)
{
  SoundSDL* sound = (SoundSDL*)udata;

  if(!sound->isSuccessfullyInitialized())
  {
    return;
  }

  if(sound->mySampleQueue.size() > 0)
  {
    Int32 offset;
    uInt8 buffer[2048];
    for(offset = 0; (offset < len) && (sound->mySampleQueue.size() > 0); )
    {
      uInt32 s = sound->mySampleQueue.dequeue(buffer, 
          (2048 > (len - offset) ? (len - offset) : 2048));
      SDL_MixAudio(stream + offset, buffer, s, sound->myCurrentVolume);
      offset += s;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SampleQueue::SampleQueue(uInt32 capacity)
    : myCapacity(capacity),
      myBuffer(0),
      mySize(0),
      myHead(0),
      myTail(0)
{
  myBuffer = new uInt8[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SampleQueue::~SampleQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::SampleQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::SampleQueue::dequeue(uInt8* buffer, uInt32 size)
{
  // We can only dequeue up to the number of items in the queue
  if(size > mySize)
  {
    size = mySize;
  }

  if((myHead + size) < myCapacity)
  {
    memcpy((void*)buffer, (const void*)(myBuffer + myHead), size);
    myHead += size;
  }
  else
  {
    uInt32 s1 = myCapacity - myHead;
    uInt32 s2 = size - s1;
    memcpy((void*)buffer, (const void*)(myBuffer + myHead), s1);
    memcpy((void*)(buffer + s1), (const void*)myBuffer, s2);
    myHead = (myHead + size) % myCapacity;
  }

  mySize -= size;

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::SampleQueue::enqueue(uInt8* buffer, uInt32 size)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll only enqueue the last myCapacity elements.
  if(size > myCapacity)
  {
    buffer += (size - myCapacity);
    size = myCapacity;
  }

  if((myTail + size) < myCapacity)
  {
    memcpy((void*)(myBuffer + myTail), (const void*)buffer, size);
    myTail += size;
  }
  else
  {
    uInt32 s1 = myCapacity - myTail;
    uInt32 s2 = size - s1;
    memcpy((void*)(myBuffer + myTail), (const void*)buffer, s1);
    memcpy((void*)myBuffer, (const void*)(buffer + s1), s2);
    myTail = (myTail + size) % myCapacity;
  }

  if((mySize + size) > myCapacity)
  {
    myHead = (myHead + ((mySize + size) - myCapacity)) % myCapacity;
    mySize = myCapacity;
  }
  else
  {
    mySize += size;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::SampleQueue::size() const
{
  return mySize;
}
