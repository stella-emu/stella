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
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundSDL.hxx,v 1.2 2004-06-13 05:03:26 bwmott Exp $
//============================================================================

#ifndef SOUNDSDL_HXX
#define SOUNDSDL_HXX

#include <SDL.h>

#include "Sound.hxx"
#include "bspf.hxx"
#include "MediaSrc.hxx"

/**
  This class implements the sound API for SDL.

  @author Stephen Anthony and Bradford W. Mott
  @version $Id: SoundSDL.hxx,v 1.2 2004-06-13 05:03:26 bwmott Exp $
*/
class SoundSDL : public Sound
{
  public:
    /**
      Create a new sound object.  The init method must be invoked before
      using the object.
    */
    SoundSDL(uInt32 fragsize);
 
    /**
      Destructor
    */
    virtual ~SoundSDL();

  public:
    /**
      Return true iff the sound device was successfully initialized.

      @return true iff the sound device was successfully initialized
    */
    virtual bool isSuccessfullyInitialized() const;

    /**
      Set the mute state of the sound object.  While muted no sound is played.

      @param state Mutes sound if true, unmute if false
    */
    virtual void mute(bool state);

    /**
      Resets the sound device.
    */
    virtual void reset();

    /**
      Sets the sound register to a given value.

      @param addr The register address
      @param value The value to save into the register
      @param cycle The CPU cycle at which the register is being updated
    */
    virtual void set(uInt16 addr, uInt8 value, Int32 cycle);

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a percentage from 0 to 100.  Values outside
      this range indicate that the volume shouldn't be changed at all.

      @param percent The new volume percentage level for the sound device
    */
    virtual void setVolume(Int32 percent);

  public:
    /**
      Loads the current state of this device from the given Deserializer.

      @param in The deserializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    bool load(Deserializer& in);

    /**
      Saves the current state of this device to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out);

  protected:
    /**
      Invoked by the sound callback to process the next sound fragment.

      @param stream Pointer to the start of the fragment
      @param length Length of the fragment
    */
    void processFragment(uInt8* stream, Int32 length);

  protected:
    // Struct to hold information regarding a TIA sound register write
    struct RegWrite
    {
      uInt16 addr;
      uInt8 value;
      double delta;
    };

    /**
      A queue class used to hold TIA sound register writes before being
      processed while creating a sound fragment.
    */
    class RegWriteQueue
    {
      public:
        /**
          Create a new queue instance with the specified initial
          capacity.  If the queue ever reaches its capacity then it will
          automatically increase its size.
        */
        RegWriteQueue(uInt32 capacity = 512);

        /**
          Destroy this queue instance.
        */
        virtual ~RegWriteQueue();

      public:
        /**
          Clear any items stored in the queue.
        */
        void clear();

        /**
          Dequeue the first object in the queue.
        */
        void dequeue();

        /**
          Return the duration of all the items in the queue.
        */
        double duration();

        /**
          Enqueue the specified object.
        */
        void enqueue(const RegWrite& info);

        /**
          Return the item at the front on the queue.

          @return The item at the front of the queue.
        */
        RegWrite& front();

        /**
          Answers the number of items currently in the queue.

          @return The number of items in the queue.
        */
        uInt32 size() const;

      private:
        // Increase the size of the queue
        void grow();

      private:
        uInt32 myCapacity;
        RegWrite* myBuffer;
        uInt32 mySize;
        uInt32 myHead;
        uInt32 myTail;
    };

  private:
    // Audio specification structure
    SDL_AudioSpec myHardwareSpec;
    
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // Indicates if the sound is currently muted
    bool myIsMuted;

    // Current volume as a percentage (0 - 100)
    uInt32 myVolume;

    // Queue of TIA register writes
    RegWriteQueue myRegWriteQueue;

  private:
    // Callback function invoked by the SDL Audio library when it needs data
    static void callback(void* udata, uInt8* stream, int len);
};

#endif