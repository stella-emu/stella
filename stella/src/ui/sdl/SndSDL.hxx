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
// $Id: SndSDL.hxx,v 1.1 2002-08-15 00:29:40 stephena Exp $
//============================================================================

#ifndef SOUNDSDL_HXX
#define SOUNDSDL_HXX

#include <SDL.h>

#include "bspf.hxx"
#include "Sound.hxx"

/**
  This class implements the sound API for SDL.

  @author  Stephen Anthony
  @version $Id: SndSDL.hxx,v 1.1 2002-08-15 00:29:40 stephena Exp $
*/
class SoundSDL : public Sound
{
  public:
    /**
      Create a new sound object
    */
    SoundSDL(int volume, bool activate = true);
 
    /**
      Destructor
    */
    virtual ~SoundSDL();

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

    /**
      Set the current volume according to the given percentage

      @param percent Scaled value (0-100) indicating the desired volume
    */
    void setVolume(int percent);


  private:
    // Indicates if the sound system was initialized
    bool myEnabled;

    // Indicates if the sound is currently muted
    bool isMuted;
};
#endif
