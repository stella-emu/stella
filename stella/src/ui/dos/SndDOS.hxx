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
// $Id: SndDOS.hxx,v 1.2 2002-11-13 03:47:55 bwmott Exp $
//============================================================================

#ifndef SOUNDDOS_HXX
#define SOUNDDOS_HXX

#include "bspf.hxx"
#include "MediaSrc.hxx"

/**
  This class implements aa sound class for the DOS front-end.  It supports
  SoundBlaster compatible sound cards.

  @author Bradford W. Mott
  @version $Id: SndDOS.hxx,v 1.2 2002-11-13 03:47:55 bwmott Exp $
*/
class SoundDOS
{
  public:
    /**
      Create a new sound object
    */
    SoundDOS(bool activate = true);
 
    /**
      Destructor
    */
    virtual ~SoundDOS();

  public: 
    /**
      Closes the sound device
    */
    void close();

    /**
      Return the playback sample rate for the sound device.
    
      @return The playback sample rate
    */
    uInt32 getSampleRate() const;

    /**
      Return true iff the sound device was successfully initialized.

      @return true iff the sound device was successfully initialized
    */
    bool isSuccessfullyInitialized() const;

    /**
      Set the mute state of the sound object.

      @param state Mutes sound if true, unmute if false
    */
    void mute(bool state);

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a precentage from 0 to 100.

      @param volume The new volume for the sound device
    */
    void setSoundVolume(uInt32 volume);

    /**
      Update the sound device using the audio sample from the specified
      media source.

      @param mediaSource The media source to get audio samples from.
    */
    void updateSound(MediaSource& mediaSource);

  private:
    /**
      A bounded queue class used to hold audio samples after they are
      produced by the MediaSource.
    */
    class SampleQueue
    {
      public:
        /**
          Create a new SampleQueue instance which can hold the specified
          number of samples.  If the queue ever reaches its capacity then
          older samples are discarded.
        */
        SampleQueue(uInt32 capacity);

        /**
          Destroy this SampleQueue instance.
        */
        virtual ~SampleQueue();

      public:
        /**
          Clear any samples stored in the queue.
        */
        void clear();

        /**
          Dequeue the upto the specified number of samples and store them
          in the buffer.  Returns the actual number of samples removed from
          the queue.

          @return the actual number of samples removed from the queue.
        */
        uInt32 dequeue(uInt8* buffer, uInt32 size);

        /**
          Enqueue the specified number of samples from the buffer.
        */
        void enqueue(uInt8* buffer, uInt32 size);

        /**
          Answers the number of samples currently in the queue.

          @return The number of samples in the queue.
        */
        uInt32 size() const;

      private:
        const uInt32 myCapacity;
        uInt8* myBuffer;
        volatile uInt32 mySize;
        volatile uInt32 myHead;
        volatile uInt32 myTail;
    };

  private:
    // Current volume
    uInt32 myCurrentVolume;

    // SDL fragment size
    uInt32 myFragmentSize;

    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // Mutex
    bool myUpdateLock;
    bool myCallbackLock;

    // Indicates if the sound is currently muted
    bool myIsMuted;

    // DSP sample rate
    uInt32 mySampleRate;

    // Queue which holds samples from the media source before they are played
    SampleQueue mySampleQueue;

  private:
    // Callback function invoked by the sound library when it needs data
    static void callback(void* udata, void* stream, int len);
};
#endif

