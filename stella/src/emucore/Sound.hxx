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
// $Id: Sound.hxx,v 1.5 2003-02-25 03:12:55 stephena Exp $
//============================================================================

#ifndef SOUND_HXX
#define SOUND_HXX

#include "bspf.hxx"
#include "MediaSrc.hxx"

/**
  This class is a base class for the various sound objects.
  It has almost no functionality, but is useful if one wishes
  to compile Stella with no sound support whatsoever.

  @author  Stephen Anthony
  @version $Id: Sound.hxx,v 1.5 2003-02-25 03:12:55 stephena Exp $
*/
class Sound
{
  public:
    /**
      Create a new sound object
    */
    Sound();
 
    /**
      Destructor
    */
    virtual ~Sound();

  public: 
    /**
      Closes the sound device
    */
    virtual void closeDevice();

    /**
      Return the playback sample rate for the sound device.
    
      @return The playback sample rate
    */
    virtual uInt32 getSampleRate() const;

    /**
      Return true iff the sound device was successfully initialized.

      @return true iff the sound device was successfully initialized.
    */
    virtual bool isSuccessfullyInitialized() const;

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a percentage from 0 to 100.  A -1 indicates
      that the volume shouldn't be changed at all.

      @param percent The new volume percentage level for the sound device
    */
    virtual void setSoundVolume(Int32 percent);

    /**
      Update the sound device using the audio sample from the specified
      media source.

      @param mediaSource The media source to get audio samples from.
    */
    virtual void updateSound(MediaSource& mediaSource);
};
#endif
