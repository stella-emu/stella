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
// $Id: SndDOS.cxx,v 1.2 2002-11-13 03:47:55 bwmott Exp $
//============================================================================

#include "SndDOS.hxx"
#include "dos.h"
#include "dos_sb.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundDOS::SoundDOS(bool activate)
    : myCurrentVolume(100),
      myFragmentSize(1024),
      myIsInitializedFlag(false),
      myUpdateLock(false),
      myIsMuted(false),
      mySampleRate(31400),
      mySampleQueue(mySampleRate)
{
  if(activate)
  {
    int bps = 8;
    int stereo = 0;
    int buffersize = myFragmentSize;
    int playback_freq = mySampleRate;

    if(sb_init(&playback_freq, &bps, &buffersize, &stereo) < 0)
    {
      cerr << "WARNING: Couldn't initialize audio system! " << endl;
      myIsInitializedFlag = false;
      mySampleRate = 0;
      return;
    }

    mySampleRate = playback_freq;
    myFragmentSize = buffersize;

    myIsInitializedFlag = true;
    myIsMuted = false;

    // Start playing audio
    sb_startoutput((sbmix_t)callback, (void*)this);
  }
  else
  {
    myIsInitializedFlag = false;
    myIsMuted = true;
    mySampleRate = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundDOS::~SoundDOS()
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundDOS::getSampleRate() const
{
  return myIsInitializedFlag ? mySampleRate : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundDOS::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::mute(bool state)
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

  if(myIsMuted)
  {
    sb_stopoutput();
  }
  else
  {
    sb_startoutput((sbmix_t)callback, (void*)this);
  }    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::close()
{
  if(myIsInitializedFlag)
  {
    sb_shutdown();
  }

  myIsInitializedFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::setSoundVolume(uInt32 percent)
{
  if(myIsInitializedFlag)
  {
    if((percent >= 0) && (percent <= 100))
    {
      // TODO: At some point we should support setting the volume...
      myCurrentVolume = percent;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::updateSound(MediaSource& mediaSource)
{
  if(myIsInitializedFlag)
  {
    // Move all of the generated samples into our private sample queue
    uInt8 buffer[4096];
    while(mediaSource.numberOfAudioSamples() > 0)
    {
      uInt32 size = mediaSource.dequeueAudioSamples(buffer, 4096);
      mySampleQueue.enqueue(buffer, size);
    }

    // Block until the sound interrupt has consumed all but 125 milliseconds
    // of the available audio samples
    uInt32 leave = mySampleRate / 8;
    while(mySampleQueue.size() > leave)
    {
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::callback(void* udata, void* stream, int len)
{
  SoundDOS* sound = (SoundDOS*)udata;

  if(!sound->myIsInitializedFlag)
  {
    return;
  }

  // Don't use samples unless there's at least 100 milliseconds worth of data
  if(sound->mySampleQueue.size() < (sound->mySampleRate / 10))
  {
    return;
  }

  sound->mySampleQueue.dequeue((uInt8*)stream, len);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundDOS::SampleQueue::SampleQueue(uInt32 capacity)
    : myCapacity(capacity),
      myBuffer(0),
      mySize(0),
      myHead(0),
      myTail(0)
{
  myBuffer = new uInt8[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundDOS::SampleQueue::~SampleQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundDOS::SampleQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundDOS::SampleQueue::dequeue(uInt8* buffer, uInt32 size)
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
void SoundDOS::SampleQueue::enqueue(uInt8* buffer, uInt32 size)
{
  disable();
  if((mySize + size) > myCapacity)
  {
    size = myCapacity - mySize;
  }
  enable();

  if(size == 0)
  {
    return;
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

  disable();
  mySize += size;
  enable();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundDOS::SampleQueue::size() const
{
  return mySize;
}

