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
// $Id: SoundALSA.hxx,v 1.2 2002-12-05 16:43:57 stephena Exp $
//============================================================================

#ifndef SOUNDALSA_HXX
#define SOUNDALSA_HXX

#include <alsa/asoundlib.h>

#include "Sound.hxx"
#include "bspf.hxx"
#include "MediaSrc.hxx"

/**
  This class implements a sound class using the 
  Advanced Linux Sound Architecture (ALSA) version 0.9.x API.

  @author  Stephen Anthony
  @version $Id: SoundALSA.hxx,v 1.2 2002-12-05 16:43:57 stephena Exp $
*/
class SoundALSA : public Sound
{
  public:
    /**
      Create a new sound object
    */
    SoundALSA();
 
    /**
      Destructor
    */
    virtual ~SoundALSA();

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
      Prints the given error message, and frees any resources used.
    */
    void alsaError(Int32 error);

  private:
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // Handle for the PCM device
    snd_pcm_t* myPcmHandle;

    // Handle for the mixer device
    snd_mixer_t* myMixerHandle;

    // Mixer elem, used to set volume
    snd_mixer_elem_t* myMixerElem;

    // Original mixer volume when the sound device was opened
    long int myOriginalVolumeLeft;
    long int myOriginalVolumeRight;

    // PCM buffer size
    uInt32 myBufferSize;

    // PCM sample rate
    uInt32 mySampleRate;
};
#endif
