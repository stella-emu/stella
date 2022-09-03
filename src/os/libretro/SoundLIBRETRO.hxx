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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef SOUND_SUPPORT

#ifndef SOUND_LIBRETRO_HXX
#define SOUND_LIBRETRO_HXX

#include <sstream>
#include <cassert>
#include <cmath>

#include "bspf.hxx"
#include "Logger.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "Sound.hxx"
#include "AudioQueue.hxx"
#include "EmulationTiming.hxx"
#include "AudioSettings.hxx"

/**
  This class implements the sound API for LIBRETRO.

  @author Stephen Anthony and Christian Speckner (DirtyHairy)
*/
class SoundLIBRETRO : public Sound
{
  public:
    /**
      Create a new sound object.  The init method must be invoked before
      using the object.
    */
    SoundLIBRETRO(OSystem& osystem, AudioSettings& audioSettings)
      : Sound(osystem),
        myAudioSettings{audioSettings}
    {
      Logger::debug("SoundLIBRETRO::SoundLIBRETRO started ...");
      Logger::debug("SoundLIBRETRO::SoundLIBRETRO initialized");
    }
    ~SoundLIBRETRO() override = default;

  public:
    /**
      Initializes the sound device.  This must be called before any
      calls are made to derived methods.
    */
    void open(shared_ptr<AudioQueue> audioQueue,
              EmulationTiming* emulationTiming) override
    {
      myEmulationTiming = emulationTiming;

      Logger::debug("SoundLIBRETRO::open started ...");

      audioQueue->ignoreOverflows(!myAudioSettings.enabled());

      myAudioQueue = audioQueue;
      myUnderrun = true;
      myCurrentFragment = nullptr;

      Logger::debug("SoundLIBRETRO::open finished");

      myIsInitializedFlag = true;
    }

    /**
      Should be called to close the sound device.  Once called the sound
      device can be started again using the open method.
    */
    void close() override
    {
      if (!myIsInitializedFlag)
        return;
      if (myAudioQueue)
        myAudioQueue->closeSink(myCurrentFragment);
      myAudioQueue.reset();
      myCurrentFragment = nullptr;

      Logger::debug("SoundLIBRETRO::close");
    }

    /**
      Empties the playback buffer.

      @param stream   Output audio buffer
      @param samples  Number of audio samples read
    */
    void dequeue(Int16* stream, uInt32* samples)
    {
      uInt32 outIndex = 0;

      while (myAudioQueue->size())
      {
        Int16* nextFragment = myAudioQueue->dequeue(myCurrentFragment);

        if (!nextFragment)
        {
          *samples = outIndex / 2;
          return;
        }

        myCurrentFragment = nextFragment;

        for (uInt32 i = 0; i < myAudioQueue->fragmentSize(); ++i)
        {
          Int16 sampleL, sampleR;

          if (myAudioQueue->isStereo())
          {
            sampleL = static_cast<Int16>(myCurrentFragment[2*i + 0]);
            sampleR = static_cast<Int16>(myCurrentFragment[2*i + 1]);
          }
          else
            sampleL = sampleR = static_cast<Int16>(myCurrentFragment[i]);

          stream[outIndex++] = sampleL;
          stream[outIndex++] = sampleR;
        }
      }
      *samples = outIndex / 2;
    }

  protected:
    //////////////////////////////////////////////////////////////////////
    // Most methods here aren't used at all.  See Sound class for
    // description, if needed.
    //////////////////////////////////////////////////////////////////////

    void setEnabled(bool enable) override { }
    void queryHardware(VariantList& devices) override { }
    void setVolume(uInt32 percent) override { }
    void adjustVolume(int direction = +1) override { }
    bool mute(bool state) override { return !myIsInitializedFlag; }
    bool toggleMute() override { return !myIsInitializedFlag; }
    string about() const override { return ""; }

    bool playWav(const char*, uInt32, uInt32) override { return false; }
    void stopWav() override { }
    uInt32 wavSize() const override { return 0; }

  private:
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag{false};

    shared_ptr<AudioQueue> myAudioQueue;

    EmulationTiming* myEmulationTiming{nullptr};

    Int16* myCurrentFragment{nullptr};
    bool myUnderrun{false};

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
