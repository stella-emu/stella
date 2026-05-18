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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
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
#include <algorithm>
#include <cstring>
#include <fstream>

#include "libretro.h"
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
extern void post_message(const char* msg, retro_log_level level, unsigned duration_ms);

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
      const uInt32 frame = myAudioSettings.sampleRate() / myOSystem.console().gameRefreshRate();

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

      if(myWavHandler.size())
        myWavHandler.mix(stream, *samples, myAudioSettings.sampleRate());
    }

    bool playWav(const string& fileName, uInt32 position, uInt32 length) override {
      if(myWavHandler.play(fileName, position, length))
        return true;
      const string msg = "KidVid: WAV file not found: " + fileName;
      post_message(msg.c_str(), RETRO_LOG_WARN, 5000);
      return false;
    }
    void stopWav() override { myWavHandler.stop(); }
    uInt32 wavSize() const override { return myWavHandler.size(); }

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
    class WavHandler
    {
    public:
      bool play(const string& fileName, uInt32 position, uInt32 length)
      {
        if(fileName != myFilename || myBuffer.empty())
        {
          myBuffer.clear();
          myFilename.clear();

          std::ifstream f(fileName, std::ios::binary);
          if(!f) return false;

          auto read32 = [&](uInt32& v) { f.read(reinterpret_cast<char*>(&v), 4); };
          auto read16 = [&](uInt16& v) { f.read(reinterpret_cast<char*>(&v), 2); };

          char tag[4];
          uInt32 u32{}; uInt16 u16{};

          f.read(tag, 4); if(std::memcmp(tag, "RIFF", 4) != 0) return false;
          read32(u32);
          f.read(tag, 4); if(std::memcmp(tag, "WAVE", 4) != 0) return false;

          uInt16 audioFormat = 0, channels = 0, bitsPerSample = 0;
          uInt32 sampleRate = 0, dataSize = 0;
          bool haveFmt = false, haveData = false;

          while(f && !(haveFmt && haveData))
          {
            char id[4]; uInt32 chunkSize{};
            f.read(id, 4);
            read32(chunkSize);
            if(!f) break;

            const auto chunkStart = f.tellg();

            if(!std::memcmp(id, "fmt ", 4) && chunkSize >= 16)
            {
              read16(audioFormat);
              read16(channels);
              read32(sampleRate);
              read32(u32);  // byte rate
              read16(u16);  // block align
              read16(bitsPerSample);
              haveFmt = true;
            }
            else if(!std::memcmp(id, "data", 4))
            {
              myBuffer.resize(chunkSize);
              f.read(reinterpret_cast<char*>(myBuffer.data()), chunkSize);
              dataSize = static_cast<uInt32>(f.gcount());
              haveData = true;
            }

            f.seekg(chunkStart +
                    static_cast<std::streamoff>(chunkSize) +
                    static_cast<std::streamoff>(chunkSize & 1));
          }

          if(!haveFmt || !haveData || audioFormat != 1) return false;
          if(!channels || (bitsPerSample != 8 && bitsPerSample != 16)) return false;

          myFilename      = fileName;
          mySampleRate    = sampleRate;
          myChannels      = channels;
          myBitsPerSample = bitsPerSample;
          myLength        = dataSize;
          myBuffer.resize(dataSize);
        }

        if(position > myLength) return false;
        myPos         = position;
        myEnd         = length ? std::min(position + length, myLength) : myLength;
        myRemaining   = myEnd - myPos;
        myAccumulator = 0.0;
        return true;
      }

      void stop() { myRemaining = 0; }

      uInt32 size() const { return myRemaining; }

      void mix(Int16* stream, uInt32 numSamples, uInt32 outputRate)
      {
        if(!myRemaining || !mySampleRate) return;

        const uInt32 frameSize = myChannels * (myBitsPerSample / 8);
        const double step = static_cast<double>(mySampleRate) / outputRate;

        for(size_t i = 0; i < numSamples && myPos < myEnd; ++i)
        {
          const Int16 wavL = sample(myPos);
          const Int16 wavR = (myChannels > 1) ? sample(myPos + myBitsPerSample / 8) : wavL;

          stream[i * 2]     = static_cast<Int16>(std::clamp(
              static_cast<int>(stream[i * 2])     + wavL, -32768, 32767));
          stream[i * 2 + 1] = static_cast<Int16>(std::clamp(
              static_cast<int>(stream[i * 2 + 1]) + wavR, -32768, 32767));

          myAccumulator += step;
          while(myAccumulator >= 1.0 && myPos < myEnd)
          {
            myAccumulator -= 1.0;
            myPos += frameSize;
          }
        }

        myRemaining = (myPos < myEnd) ? (myEnd - myPos) : 0;
      }

    private:
      Int16 sample(uInt32 pos) const
      {
        if(myBitsPerSample == 8)
          return static_cast<Int16>((static_cast<int>(myBuffer[pos]) - 128) << 8);
        return static_cast<Int16>(myBuffer[pos] | (static_cast<uInt16>(myBuffer[pos + 1]) << 8));
      }

      string    myFilename;
      ByteArray myBuffer;
      uInt32    myLength{0};
      uInt32    myPos{0};
      uInt32    myEnd{0};
      uInt32    myRemaining{0};
      uInt32    mySampleRate{0};
      uInt16    myChannels{1};
      uInt16    myBitsPerSample{8};
      double    myAccumulator{0.0};
    };

    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag{false};

    shared_ptr<AudioQueue> myAudioQueue;

    Int16* myCurrentFragment{nullptr};
    bool myUnderrun{false};

    AudioSettings& myAudioSettings;
    WavHandler myWavHandler;

  private:
    // Following constructors and assignment operators not supported
    SoundLIBRETRO() = delete;
    SoundLIBRETRO(const SoundLIBRETRO&) = delete;
    SoundLIBRETRO(SoundLIBRETRO&&) = delete;
    SoundLIBRETRO& operator=(const SoundLIBRETRO&) = delete;
    SoundLIBRETRO& operator=(SoundLIBRETRO&&) = delete;
};

#endif  // SOUND_LIBRETRO_HXX

#endif  // SOUND_SUPPORT
