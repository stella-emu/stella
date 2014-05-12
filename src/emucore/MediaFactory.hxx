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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
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
#if defined(BSPF_UNIX)
  #include "SettingsUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(BSPF_WINDOWS)
  #include "SettingsWINDOWS.hxx"
  #include "OSystemWINDOWS.hxx"
#elif defined(BSPF_MAC_OSX)
  #include "SettingsMACOSX.hxx"
  #include "OSystemMACOSX.hxx"
  extern "C" {
    int stellaMain(int argc, char* argv[]);
  }
#else
  #error Unsupported platform!
#endif

#include "FrameBufferSDL2.hxx"
#include "EventHandlerSDL2.hxx"
#ifdef SOUND_SUPPORT
  #include "SoundSDL2.hxx"
#else
  #include "SoundNull.hxx"
#endif

/**
  This class deals with the different framebuffer/sound/event
  implementations for the various ports of Stella, and always returns a
  valid object based on the specific port and restrictions on that port.

  As of SDL2, this code is greatly simplified.  However, it remains here
  in case we ever have multiple backend implementations again (should
  not be necessary since SDL2 covers this nicely).

  @author  Stephen Anthony
  @version $Id$
*/
class MediaFactory
{
  public:
    static OSystem* createOSystem()
    {
    #if defined(BSPF_UNIX)
      return new OSystemUNIX();
    #elif defined(BSPF_WINDOWS)
      return new OSystemWINDOWS();
    #elif defined(BSPF_MAC_OSX)
      return new OSystemMACOSX();
    #else
      #error Unsupported platform for OSystem!
    #endif
    }

    static Settings* createSettings(OSystem& osystem)
    {
    #if defined(BSPF_UNIX)
      return new SettingsUNIX(osystem);
    #elif defined(BSPF_WINDOWS)
      return new SettingsWINDOWS(osystem);
    #elif defined(BSPF_MAC_OSX)
      return new SettingsMACOSX(osystem);
    #else
      #error Unsupported platform for Settings!
    #endif
    }

    static FrameBuffer* createVideo(OSystem& osystem)
    {
      return new FrameBufferSDL2(osystem);
    }

    static Sound* createAudio(OSystem& osystem)
    {
    #ifdef SOUND_SUPPORT
      return new SoundSDL2(osystem);
    #else
      return new SoundNull(osystem);
    #endif
    }

    static EventHandler* createEventHandler(OSystem& osystem)
    {
      return new EventHandlerSDL2(osystem);
    }

};

#endif
