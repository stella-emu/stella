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
// $Id: Sound.hxx,v 1.2 2002-03-28 02:02:24 bwmott Exp $
//============================================================================

#ifndef SOUND_HXX
#define SOUND_HXX

#include "bspf.hxx"

/**
  Base class that defines the standard API for sound classes.  You
  should derive a new class from this one to create a new sound system 
  for a specific operating system.

  @author  Bradford W. Mott
  @version $Id: Sound.hxx,v 1.2 2002-03-28 02:02:24 bwmott Exp $
*/
class Sound
{
  public:
    /**
      Enumeration of the TIA sound registers
    */
    enum Register 
    { 
      AUDF0, AUDF1, AUDC0, AUDC1, AUDV0, AUDV1 
    };

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
      Set the value of the specified sound register.

      @param reg The sound register to set
      @param val The new value for the sound register
      @param cycles The number of elapsed CPU cycles since the last set
    */
    virtual void set(Sound::Register reg, uInt8 val, uInt32 cycles);

    /**
      Set the value of the specified sound register.  This method is being
      kept for backwards compatibility.  There's a good chance it will be
      removed in the 1.3 release of Stella as the sound system is overhauled.
    
      @param reg The sound register to set
      @param val The new value for the sound register
    */
    virtual void set(Sound::Register reg, uInt8 val);

    /**
      Set the mute state of the sound object

      @param state Mutes sound iff true
    */
    virtual void mute(bool state);
};
#endif

