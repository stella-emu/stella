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

#ifndef SOUND_SDL_HXX
#define SOUND_SDL_HXX

class OSystem;
class AudioQueue;
class EmulationTiming;
class AudioSettings;
class Resampler;

#include "SDL_lib.hxx"

#include "bspf.hxx"
#include "Sound.hxx"

/**
  This class implements the sound API for SDL.

  @author Stephen Anthony and Christian Speckner (DirtyHairy)
*/
class SoundSDL : public Sound
{
  public:
    /**
      Create a new sound object.  The init method must be invoked before
      using the object.
    */
    SoundSDL(OSystem& osystem, AudioSettings& audioSettings);
    ~SoundSDL() override;

  public:
    /**
      Enables/disables the sound subsystem.

      @param enable  Either true or false, to enable or disable the sound system
    */
    void setEnabled(bool enable) override;

    /**
      Initializes the sound device.  This must be called before any
      calls are made to derived methods.
    */
    void open(shared_ptr<AudioQueue> audioQueue,
              shared_ptr<const EmulationTiming> emulationTiming) override;

    /**
      Sets the sound mute state; sound processing continues.  When enabled,
      sound volume is 0; when disabled, sound volume returns to previously
      set level.

      @param enable  Mutes sound if true, unmute if false
    */
    void mute(bool enable) override;

    /**
      Toggles the sound mute state; sound processing continues.
      Switches between mute(true) and mute(false).
    */
    void toggleMute() override;

    /**
      Set the pause state of the sound object.  While paused, sound is
      neither played nor processed (ie, the sound subsystem is temporarily
      disabled).

      @param enable  Pause sound if true, unpause if false

      @return  The previous (old) pause state
    */
    bool pause(bool enable) override;

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a range from 0 to 100 (0 indicates mute).  Values
      outside this range indicate that the volume shouldn't be changed at all.

      @param volume  The new volume level for the sound device
    */
    void setVolume(uInt32 volume) override;

    /**
      Adjusts the volume of the sound device based on the given direction.

      @param direction  +1 indicates increase, -1 indicates decrease.
      */
    void adjustVolume(int direction = 1) override;

    /**
      This method is called to provide information about the sound device.
    */
    string about() const override;

    /**
      Play a WAV file.

      @param fileName  The name of the WAV file
      @param position  The position to start playing
      @param length    The played length

      @return  True if the WAV file can be played, else false
    */
    bool playWav(const string& fileName, uInt32 position = 0,
                 uInt32 length = 0) override;

    /**
      Stop any currently playing WAV file.
    */
    void stopWav() override;

    /**
      Get the size of the WAV file which remains to be played.

      @return  The remaining number of bytes
    */
    uInt32 wavSize() const override;

  private:
    /**
      This method is called to query the audio devices.

      @param devices  List of device names
    */
    void queryHardware(VariantList& devices) override;

    /**
      The actual sound device is opened only when absolutely necessary.
      Typically this will only happen once per program run, but it can also
      happen dynamically when changing sample rate and/or fragment size.
    */
    bool openDevice();

    void initResampler();

  private:
    AudioSettings& myAudioSettings;

    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag{false};

    // Audio specification structure
    SDL_AudioSpec myHardwareSpec{};

    SDL_AudioDeviceID myDevice{0};
    uInt32 myDeviceId{0};

    shared_ptr<AudioQueue> myAudioQueue;
    unique_ptr<Resampler> myResampler;

    shared_ptr<const EmulationTiming> myEmulationTiming;

    Int16* myCurrentFragment{nullptr};
    bool myUnderrun{false};

    string myAboutString;

    /**
      This class implements WAV file playback using the SDL sound API.
    */
    class WavHandlerSDL
    {
      public:
        explicit WavHandlerSDL() = default;
        ~WavHandlerSDL();

        bool play(const string& fileName, const char* device,
                  uInt32 position, uInt32 length);
        void stop();
        uInt32 size() const { return myBuffer ? myRemaining : 0; }

        void setSpeed(const double speed) { mySpeed = speed; }
        void pause(bool state) const;

      private:
        string myFilename;
        uInt32 myLength{0};
        SDL_AudioDeviceID myDevice{0};
        uInt8* myBuffer{nullptr};
        double mySpeed{1.0};
        unique_ptr<uInt8[]> myCvtBuffer;
        uInt32 myCvtBufferSize{0};
        SDL_AudioSpec mySpec{}; // audio output format
        uInt8* myPos{nullptr};  // pointer to the audio buffer to be played
        uInt32 myRemaining{0};  // remaining length of the sample we have to play

      private:
        // Callback function invoked by the SDL Audio library when it needs data
        void processWav(uInt8* stream, uInt32 len);
        static void callback(void* object, uInt8* stream, int len);

        // Following constructors and assignment operators not supported
        WavHandlerSDL(const WavHandlerSDL&) = delete;
        WavHandlerSDL(WavHandlerSDL&&) = delete;
        WavHandlerSDL& operator=(const WavHandlerSDL&) = delete;
        WavHandlerSDL& operator=(WavHandlerSDL&&) = delete;
    };

    WavHandlerSDL myWavHandler;

    static float myVolumeFactor;  // Current volume level (0 - 100)

  private:
    // Callback functions invoked by the SDL Audio library when it needs data
    static void callback(void* object, uInt8* stream, int len);

    // Following constructors and assignment operators not supported
    SoundSDL() = delete;
    SoundSDL(const SoundSDL&) = delete;
    SoundSDL(SoundSDL&&) = delete;
    SoundSDL& operator=(const SoundSDL&) = delete;
    SoundSDL& operator=(SoundSDL&&) = delete;
};

#endif

#endif  // SOUND_SUPPORT
