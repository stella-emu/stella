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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifdef SOUND_SUPPORT

#ifndef SOUND_SDL2_HXX
#define SOUND_SDL2_HXX

class OSystem;

#include <SDL.h>

#include "bspf.hxx"
#include "Sound.hxx"

/**
  This class implements the sound API for SDL2.

  @author Stephen Anthony
  @version $Id$
*/
class SoundSDL2 : public Sound
{
  public:
    /**
      Create a new sound object.  The open() method must be invoked
      before using the object.
    */
    SoundSDL2(OSystem& osystem);
    virtual ~SoundSDL2();

  public:
    /**
      Enables/disables the sound subsystem.

      @param state  True or false, to enable or disable the sound system
    */
    void setEnabled(bool state) override;

    /**
      Initializes the sound device.  This must be called before any
      calls are made to derived methods.

      @param stereo  The number of channels (mono -> 1, stereo -> 2)
    */
    void open(bool stereo) override;

    /**
      Should be called to close the sound device.  Once called the sound
      device can be started again using the open method.
    */
    void close() override;

    /**
      Set the mute state of the sound object.  While muted no sound is played.

      @param state  Mutes sound if true, unmute if false
    */
    void mute(bool state) override;

    /**
      Reset the sound device.
    */
    void reset() override;

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a percentage from 0 to 100.  Values outside
      this range indicate that the volume shouldn't be changed at all.

      @param volume  The new volume percentage level for the sound device
    */
    void setVolume(uInt32 volume) override;

    /**
      Adjusts the volume of the sound device based on the given direction.

      @param direction  Increase or decrease the current volume by a predefined
                        amount based on the direction (1 = increase, -1 = decrease)
    */
    void adjustVolume(Int8 direction) override;

  private:
    // Indicates if the sound subsystem is to be initialized
    bool myIsEnabled;

    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // Indicates if the sound is currently muted
    bool myIsMuted;

    // Current volume as a percentage (0 - 100)
    uInt32 myVolume;

    // Audio specification structure
    SDL_AudioSpec myHardwareSpec;

  private:
    // Callback function invoked by the SDL Audio library when it needs data
    static void getSamples(void* udata, uInt8* stream, int len);

    // Following constructors and assignment operators not supported
    SoundSDL2() = delete;
    SoundSDL2(const SoundSDL2&) = delete;
    SoundSDL2(SoundSDL2&&) = delete;
    SoundSDL2& operator=(const SoundSDL2&) = delete;
    SoundSDL2& operator=(SoundSDL2&&) = delete;
};

#endif

#endif  // SOUND_SUPPORT
