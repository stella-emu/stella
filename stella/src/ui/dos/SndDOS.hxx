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
// $Id: SndDOS.hxx,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
//============================================================================

#ifndef SOUNDDOS_HXX
#define SOUNDDOS_HXX

#include "bspf.hxx"
#include "Sound.hxx"

/**
  This class implements the sound API for the DOS operating system
  using a sound-blaster card.

  @author  Bradford W. Mott
  @version $Id: SndDOS.hxx,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
*/
class SoundDOS : public Sound
{
  public:
    /**
      Create a new sound object
    */
    SoundDOS();
 
    /**
      Destructor
    */
    virtual ~SoundDOS();

  public: 
    /**
      Set the value of the specified sound register

      @param reg The sound register to set
      @param val The new value for the sound registers
    */
    virtual void set(Sound::Register reg, uInt8 val);

    /**
      Set the mute state of the sound object

      @param state Mutes sound iff true
    */
    virtual void mute(bool state);

  private:
    // Indicates if the sound system was initialized
    bool myEnabled;
};
#endif

