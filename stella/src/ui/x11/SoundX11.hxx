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
// $Id: SoundX11.hxx,v 1.1 2002-10-09 04:38:12 bwmott Exp $
//============================================================================

#ifndef SOUNDX11_HXX
#define SOUNDX11_HXX

#include "bspf.hxx"
#include "MediaSrc.hxx"

/**
  This class implements a sound class for the X11 front-end.  The
  Open Sound System (OSS) API is currently supported.

  @author  Bradford W. Mott
  @version $Id: SoundX11.hxx,v 1.1 2002-10-09 04:38:12 bwmott Exp $
*/
class SoundX11
{
  public:
    /**
      Create a new sound object
    */
    SoundX11();
 
    /**
      Destructor
    */
    virtual ~SoundX11();

  public: 
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
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // DSP file descriptor
    int myDspFd;

    // Mixer file descriptor
    int myMixerFd;

    // Original mixer volume when the sound device was opened
    int myOriginalVolume;

    // DSP sample rate
    uInt32 mySampleRate;
};
#endif

