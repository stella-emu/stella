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
// $Id: Sound.hxx,v 1.9 2004-04-04 02:03:15 stephena Exp $
//============================================================================

#ifndef SOUND_HXX
#define SOUND_HXX

class Console;
class MediaSource;
class Serializer;
class Deserializer;

#include "bspf.hxx"

/**
  This class is a base class for the various sound objects.
  It has almost no functionality, but is useful if one wishes
  to compile Stella with no sound support whatsoever.

  @author  Stephen Anthony
  @version $Id: Sound.hxx,v 1.9 2004-04-04 02:03:15 stephena Exp $
*/
class Sound
{
  public:
    /**
      Create a new sound object
    */
    Sound(uInt32 fragsize = 0, uInt32 queuesize = 0);
 
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
      Update the sound device with audio samples.
    */
    virtual void update();

    /**
      Sets the sound register to a given value.

      @param addr  The register address
      @param value The value to save into the register
    */
    virtual void set(uInt16 addr, uInt8 value);

    /**
      Saves the current state of this device to the given Serializer.

      @param out  The serializer device to save to.
      @return     The result of the save.  True on success, false on failure.
    */
    virtual bool save(Serializer& out);

    /**
      Loads the current state of this device from the given Deserializer.

      @param in  The deserializer device to load from.
      @return    The result of the load.  True on success, false on failure.
    */
    virtual bool load(Deserializer& in);

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
