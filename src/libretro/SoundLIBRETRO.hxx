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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef SOUND_SUPPORT

#ifndef SOUND_LIBRETRO_HXX
#define SOUND_LIBRETRO_HXX

class OSystem;
class AudioQueue;
class EmulationTiming;
class AudioSettings;

#include "bspf.hxx"
#include "Sound.hxx"

/**
  This class implements the sound API for LIBRTRO.

  @author Stephen Anthony and Christian Speckner (DirtyHairy)
*/
class SoundLIBRETRO : public Sound
{
  public:
    /**
      Create a new sound object.  The init method must be invoked before
      using the object.
    */
    SoundLIBRETRO(OSystem& osystem, AudioSettings& audioSettings);

    /**
      Destructor
    */
    virtual ~SoundLIBRETRO();

  public:
    /**
      Enables/disables the sound subsystem.

      @param enable  Either true or false, to enable or disable the sound system
    */
    void setEnabled(bool enable) override { }

    /**
      Initializes the sound device.  This must be called before any
      calls are made to derived methods.
    */
    void open(shared_ptr<AudioQueue> audioQueue, EmulationTiming* emulationTiming) override;

    /**
      Should be called to close the sound device.  Once called the sound
      device can be started again using the open method.
    */
    void close() override;

    /**
      Set the mute state of the sound object.  While muted no sound is played.

      @param state Mutes sound if true, unmute if false

      @return  The previous (old) mute state
    */
    bool mute(bool state) override { return !myIsInitializedFlag; }

    /**
      Toggles the sound mute state.  While muted no sound is played.

      @return  The previous (old) mute state
    */
    bool toggleMute() override { return !myIsInitializedFlag; }

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a percentage from 0 to 100.  Values outside
      this range indicate that the volume shouldn't be changed at all.

      @param percent  The new volume percentage level for the sound device
    */
    void setVolume(uInt32 percent) override { }

    /**
      Adjusts the volume of the sound device based on the given direction.

      @param increase  Increase or decrease the current volume by a predefined
                       amount
    */
    void adjustVolume(bool increase) override { return nullptr; }

    /**
      This method is called to provide information about the sound device.
    */
    string about() const override { return ""; }

  public:
    /**
      Empties the playback buffer.

      @param stream   Output audio buffer
      @param samples  Number of audio samples read
    */
    void dequeue(Int16* stream, uInt32* samples);

  private:
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    shared_ptr<AudioQueue> myAudioQueue;

    EmulationTiming* myEmulationTiming;

    Int16* myCurrentFragment;
    bool myUnderrun;

    AudioSettings& myAudioSettings;

  private:
    // Following constructors and assignment operators not supported
    SoundLIBRETRO() = delete;
    SoundLIBRETRO(const SoundLIBRETRO&) = delete;
    SoundLIBRETRO(SoundLIBRETRO&&) = delete;
    SoundLIBRETRO& operator=(const SoundLIBRETRO&) = delete;
    SoundLIBRETRO& operator=(SoundLIBRETRO&&) = delete;
};

#endif

#endif  // SOUND_SUPPORT
