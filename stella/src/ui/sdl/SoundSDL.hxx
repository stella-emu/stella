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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundSDL.hxx,v 1.2 2002-10-12 15:24:49 stephena Exp $
//============================================================================

#ifndef SOUNDSDL_HXX
#define SOUNDSDL_HXX

#include <SDL.h>

#include "bspf.hxx"
#include "MediaSrc.hxx"

/**
  This class implements the sound API for SDL.

  @author  Stephen Anthony
  @version $Id: SoundSDL.hxx,v 1.2 2002-10-12 15:24:49 stephena Exp $
*/
class SoundSDL
{
  public:
    /**
      Create a new sound object
    */
    SoundSDL(bool activate = true);
 
    /**
      Destructor
    */
    virtual ~SoundSDL();

  public: 
    /**
      Return the playback sample rate for the sound device.
    
      @return The playback sample rate
    */
    uInt32 getSampleRate() const;

    /**
      Return true iff the sound device was successfully initialized.

      @return true iff the sound device was successfully initialized
    */
    bool isSuccessfullyInitialized() const;

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a precentage from 0 to 100.

      @param volume The new volume for the sound device
    */
    void setSoundVolume(uInt32 volume);

    /**
      Notifies this class of the MediaSource object where sample data
      may be obtained.  The SDL sound api is thread-based, so the SDL
      audio callback directly queries the MediaSource when it requires
      more audio samples.

      @param mediaSource The MediaSource where sample data is obtained
    */
    void setMediaSource(MediaSource& mediaSource);

    /**
      Set the mute state of the sound object.

      @param state Mutes sound if true, unmute if false
    */
    void mute(bool state);

    /**
      Closes the sound device
    */
    void close();

  private:
    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // DSP sample rate
    uInt32 mySampleRate;

    // SDL fragment size
    uInt32 myFragSize;

    // Indicates if the sound is currently muted
    bool myIsMuted;

  private:
    /**
      The callback used by the SDL sound API.  It obtains samples
      from the MediaSource as it needs them.
    */
    static void fillAudio(void* udata, uInt8* stream, Int32 len);

};
#endif
