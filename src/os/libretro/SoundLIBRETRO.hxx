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
#include "FSNode.hxx"
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
        // Reuse a loaded WAV, else load it over the less recently played one
        const auto loaded = std::ranges::find_if(myWavs, [&](const Wav& w) {
          return !w.data.empty() && w.name == fileName;
        });
        const size_t idx = loaded != myWavs.end() ? SZT(loaded - myWavs.begin()) : 1 - myLast;
        const Wav& wav = myWavs[idx];

        // Whatever was playing stops, even if this WAV can't be played
        stop();
        if((loaded == myWavs.end() && !load(myWavs[idx], fileName)) ||
           position > U32(wav.data.size()))
          return false;

        myLast        = idx;
        myPos         = position;
        myEnd         = length ? std::min(position + length, U32(wav.data.size()))
                               : U32(wav.data.size());
        myRemaining   = myEnd - myPos;
        myAccumulator = 0.0;
        return true;
      }

      void stop() { myRemaining = 0; }

      uInt32 size() const { return myRemaining; }

      void mix(Int16* stream, uInt32 numSamples, uInt32 outputRate)
      {
        const Wav& wav = myWavs[myLast];
        if(!myRemaining || !wav.rate) return;

        const uInt32 frameSize = wav.channels * (wav.bits / 8);
        const double step = DBL(wav.rate) / outputRate;

        for(auto i = 0UZ; i < numSamples && myPos < myEnd; ++i)
        {
          const Int16 wavL = sample(wav, myPos);
          const Int16 wavR = (wav.channels > 1) ? sample(wav, myPos + wav.bits / 8) : wavL;

          stream[i * 2]     = I16(std::clamp(
              I32(stream[i * 2])     + wavL, -32768, 32767));
          stream[i * 2 + 1] = I16(std::clamp(
              I32(stream[i * 2 + 1]) + wavR, -32768, 32767));

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
      // A loaded PCM WAV file
      struct Wav
      {
        string    name;
        ByteArray data;
        uInt32    rate{0};
        uInt16    channels{1};
        uInt16    bits{8};
      };

      // Load a PCM WAV file, which may be an entry in a ZIP archive
      static bool load(Wav& wav, const string& fileName)
      {
        wav = Wav{};

        ByteArray file;
        try
        {
          FSNode(fileName, FSNode::ZipMode::Data).read(file);
        }
        catch(const std::runtime_error&)
        {
          return false;
        }

        const auto u16 = [&file](size_t at) {
          return U16(file[at] | (U32(file[at + 1]) << 8U));
        };
        const auto u32 = [&u16](size_t at) {
          return U32(u16(at) | (U32(u16(at + 2)) << 16U));
        };
        const auto isChunk = [&file](size_t at, const char* id) {
          return std::memcmp(file.data() + at, id, 4) == 0;
        };

        if(file.size() < 12 || !isChunk(0, "RIFF") || !isChunk(8, "WAVE"))
          return false;

        uInt16 format = 0;
        bool haveFmt = false, haveData = false;
        for(size_t at = 12; at + 8 <= file.size() && !(haveFmt && haveData); )
        {
          const size_t size = u32(at + 4), body = at + 8;
          const size_t avail = std::min(size, file.size() - body);

          if(isChunk(at, "fmt ") && avail >= 16)
          {
            format       = u16(body);
            wav.channels = u16(body + 2);
            wav.rate     = u32(body + 4);
            wav.bits     = u16(body + 14);
            haveFmt = true;
          }
          else if(isChunk(at, "data"))
          {
            wav.data.assign(file.begin() + I64(body), file.begin() + I64(body + avail));
            haveData = true;
          }
          at = body + size + (size & 1);
        }

        if(!haveFmt || !haveData || format != 1 || !wav.channels ||
           (wav.bits != 8 && wav.bits != 16))
        {
          wav = Wav{};
          return false;
        }

        // Whole frames only, so mix() never reads past the end
        const size_t frameSize = wav.channels * (wav.bits / 8U);
        wav.data.resize(wav.data.size() / frameSize * frameSize);
        wav.name = fileName;
        return !wav.data.empty();
      }

      static Int16 sample(const Wav& wav, uInt32 pos)
      {
        if(wav.bits == 8)
          return I16((I32(wav.data[pos]) - 128) << 8);
        return I16(wav.data[pos] | (U16(wav.data[pos + 1]) << 8));
      }

      // The two most recently played stay loaded, since KidVid alternates
      // between a tape's own file and one shared by all tapes
      std::array<Wav, 2> myWavs;
      // Index of the most recently played
      size_t    myLast{1};
      uInt32    myPos{0};
      uInt32    myEnd{0};
      uInt32    myRemaining{0};
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
