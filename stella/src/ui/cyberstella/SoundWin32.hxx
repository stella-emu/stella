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
// $Id: SoundWin32.hxx,v 1.3 2003-11-19 21:06:27 stephena Exp $
//============================================================================

#ifndef SOUND_WIN32_HXX
#define SOUND_WIN32_HXX

#include <dsound.h>

#include "bspf.hxx"
#include "MediaSrc.hxx"
#include "Sound.hxx"

/**
  This class implements a sound class using the
  Win32 DirectSound API.

  @author  Stephen Anthony
  @version $Id: SoundWin32.hxx,v 1.3 2003-11-19 21:06:27 stephena Exp $
*/
class SoundWin32 : public Sound
{
  public:
    /**
      Create a new sound object
    */
    SoundWin32();

    /**
      Destructor
    */
    virtual ~SoundWin32();

  public:
    /**
      Closes the sound device
    */
    void closeDevice();

    /**
      Return the playback sample rate for the sound device.

      @return The playback sample rate
    */
    uInt32 getSampleRate() const;

    /**
      Return true iff the sound device was successfully initlaized.

      @return true iff the sound device was successfully initlaized.
    */
    bool isSuccessfullyInitialized() const;

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a percentage from 0 to 100.  A -1 indicates
      that the volume shouldn't be changed at all.

      @param percent The new volume percentage level for the sound device
    */
    void setVolume(Int32 percent);

    /**
      Update the sound device using the audio sample from the
      media source.
    */
    void update();

    /**
      Initialize the DirectSound subsystem/

      @return The result of initialization.
    */
    HRESULT Initialize(HWND hwnd);

  private:
    // Print error messages and clean up
    void SoundError(const char* message);

    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // DirectSound device
    LPDIRECTSOUND8 myDSDevice;

    // DirectSound secondary buffer
    LPDIRECTSOUNDBUFFER myDSBuffer;

    // Mixer file descriptor
    int myMixerFd;

    // Original mixer volume when the sound device was opened
    int myOriginalVolume;

    // PCM buffer size
    uInt32 myBufferSize;

    // DSP sample rate
    uInt32 mySampleRate;
};

#endif
