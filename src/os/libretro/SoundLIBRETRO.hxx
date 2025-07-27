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
    }
    ~SoundLIBRETRO() override = default;

  public:
    /**
      Initializes the sound device.  This must be called before any
      calls are made to derived methods.
    */
    void open(shared_ptr<AudioQueue> audioQueue,
              shared_ptr<const EmulationTiming>) override
    {
      audioQueue->ignoreOverflows(!myAudioSettings.enabled());

      myAudioQueue = audioQueue;
      myUnderrun = true;
      myCurrentFragment = nullptr;

      myIsInitializedFlag = true;
    }

    /**
      Empties the playback buffer.

      @param stream   Output audio buffer
      @param samples  Number of audio samples read
    */
    void dequeue(Int16* stream, uInt32* samples)
    {
      uInt32 outIndex = 0;
      uInt32 frame    = myAudioSettings.sampleRate() / myOSystem.console().gameRefreshRate();

      while (myAudioQueue->size() && outIndex <= frame)
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
          Int16 sampleL = 0, sampleR = 0;

          if (myAudioQueue->isStereo())
          {
            sampleL = myCurrentFragment[2*i + 0];
            sampleR = myCurrentFragment[2*i + 1];
          }
          else
            sampleL = sampleR = myCurrentFragment[i];

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

    void setEnabled(bool) override { }
    void setVolume(uInt32, bool) override { }
    void adjustVolume(int) override { }
    void mute(bool) override { }
    void toggleMute() override { }
    bool pause(bool) override { return !myIsInitializedFlag; }
    string about() const override { return ""; }

  private:
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag{false};

    shared_ptr<AudioQueue> myAudioQueue;

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
