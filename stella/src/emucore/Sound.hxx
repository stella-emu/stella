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
// $Id: Sound.hxx,v 1.8 2003-11-19 15:57:10 stephena Exp $
//============================================================================

#ifndef SOUND_HXX
#define SOUND_HXX

class Console;
class MediaSource;

#include "bspf.hxx"

/**
  This class is a base class for the various sound objects.
  It has almost no functionality, but is useful if one wishes
  to compile Stella with no sound support whatsoever.

  @author  Stephen Anthony
  @version $Id: Sound.hxx,v 1.8 2003-11-19 15:57:10 stephena Exp $
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
      Initializes the sound device.  This must be called before any
      calls are made to derived methods.

      @param console   The console
      @param mediasrc  The mediasource
    */
    void init(Console* console, MediaSource* mediasrc);

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
    virtual void setVolume(Int32 percent);

    /**
      Update the sound device using the audio sample from the
      media source.
    */
    virtual void update();

    /**
      Sets the pause status.  While pause is selected, update()
      should not play any sound.

      @param status  Toggle pause based on status
    */
    void pause(bool status) { myPauseStatus = status; }

  protected:
    // The Console for the system
    Console* myConsole;

    // The Mediasource for the system
    MediaSource* myMediaSource;

    // The pause status
    bool myPauseStatus;
};

#endif
