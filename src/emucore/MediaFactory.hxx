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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "OSystem.hxx"
#include "Settings.hxx"

#include "FrameBufferSDL2.hxx"

#ifdef SOUND_SUPPORT
  #include "SoundSDL2.hxx"
#else
  #include "SoundNull.hxx"
#endif

/**
  This class deals with the different framebuffer/sound implementations
  for the various ports of Stella, and always returns a valid media object
  based on the specific port and restrictions on that port.

  As of SDL2, this code is greatly simplified.  However, it remains here
  in case we ever have multiple backend implementations again (should
  not be necessary since SDL2 covers this nicely).

  @author  Stephen Anthony
  @version $Id$
*/
class MediaFactory
{
  public:
    static FrameBuffer* createVideo(OSystem* osystem)
    {
      return new FrameBufferSDL2(osystem);
    }

    static Sound* createAudio(OSystem* osystem)
    {
    #ifdef SOUND_SUPPORT
      return new SoundSDL2(osystem);
    #else
      return new SoundNull(osystem);
    #endif
    }
};

#endif
