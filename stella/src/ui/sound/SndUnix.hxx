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
// $Id: SndUnix.hxx,v 1.1.1.1 2001-12-27 19:54:35 bwmott Exp $
//============================================================================

#ifndef SOUNDUNIX_HXX
#define SOUNDUNIX_HXX

#include "bspf.hxx"
#include "Sound.hxx"

/**
  This class implements the sound API for the Unix operating system.
  Under Unix the real work of the sound system is in another process
  called "stella-sound".  This process is started when an instance 
  of the SoundUnix class is created.  Communicattion with the 
  "stella-sound" process is done through a pipe.

  @author  Bradford W. Mott
  @version $Id: SndUnix.hxx,v 1.1.1.1 2001-12-27 19:54:35 bwmott Exp $
*/
class SoundUnix : public Sound
{
  public:
    /**
      Create a new sound object
    */
    SoundUnix();
 
    /**
      Destructor
    */
    virtual ~SoundUnix();

  public: 
    /**
      Set the value of the specified sound register

      @param reg The sound register to set
      @param val The new value for the sound register
    */
    virtual void set(Sound::Register reg, uInt8 val);

    /**
      Set the mute state of the sound object

      @param state Mutes sound iff true
    */
    virtual void mute(bool state);

  private:
    // Indicates if the sound system couldn't be initialized
    bool myDisabled;

    // Indicates if the sound is muted or not
    bool myMute;

    // ProcessId of the stella-sound process
    int myProcessId;

    // Write file descriptor for IPC
    int myFd;

    // Buffers for the audio registers used so we only 
    // send "changes" to the stella-sound process
    uInt8 myAUDC0;
    uInt8 myAUDC1;
    uInt8 myAUDF0;
    uInt8 myAUDF1;
    uInt8 myAUDV0;
    uInt8 myAUDV1;
};
#endif

