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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Settings.hxx"
#include "SerialPort.hxx"
#if defined(BSPF_UNIX)
  #include "SerialPortUNIX.hxx"
  #include "SettingsUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(BSPF_WINDOWS)
  #include "SerialPortWINDOWS.hxx"
  #include "SettingsWINDOWS.hxx"
  #include "OSystemWINDOWS.hxx"
#elif defined(BSPF_MAC_OSX)
  #include "SerialPortMACOSX.hxx"
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
    static unique_ptr<OSystem> createOSystem()
    {
    #if defined(BSPF_UNIX)
      return make_ptr<OSystemUNIX>();
    #elif defined(BSPF_WINDOWS)
      return make_ptr<OSystemWINDOWS>();
    #elif defined(BSPF_MAC_OSX)
      return make_ptr<OSystemMACOSX>();
    #else
      #error Unsupported platform for OSystem!
    #endif
    }

    static unique_ptr<Settings> createSettings(OSystem& osystem)
    {
    #if defined(BSPF_UNIX)
      return make_ptr<SettingsUNIX>(osystem);
    #elif defined(BSPF_WINDOWS)
      return make_ptr<SettingsWINDOWS>(osystem);
    #elif defined(BSPF_MAC_OSX)
      return make_ptr<SettingsMACOSX>(osystem);
    #else
      #error Unsupported platform for Settings!
    #endif
    }

    static unique_ptr<SerialPort> createSerialPort()
    {
    #if defined(BSPF_UNIX)
      return make_ptr<SerialPortUNIX>();
    #elif defined(BSPF_WINDOWS)
      return make_ptr<SerialPortWINDOWS>();
    #elif defined(BSPF_MAC_OSX)
      return make_ptr<SerialPortMACOSX>();
    #else
      return make_ptr<SerialPort>();
    #endif
    }

    static unique_ptr<FrameBuffer> createVideo(OSystem& osystem)
    {
      return make_ptr<FrameBufferSDL2>(osystem);
    }

    static unique_ptr<Sound> createAudio(OSystem& osystem)
    {
    #ifdef SOUND_SUPPORT
      return make_ptr<SoundSDL2>(osystem);
    #else
      return make_ptr<SoundNull>(osystem);
    #endif
    }

    static unique_ptr<EventHandler> createEventHandler(OSystem& osystem)
    {
      return make_ptr<EventHandlerSDL2>(osystem);
    }

  private:
    // Following constructors and assignment operators not supported
    MediaFactory() = delete;
    MediaFactory(const MediaFactory&) = delete;
    MediaFactory(MediaFactory&&) = delete;
    MediaFactory& operator=(const MediaFactory&) = delete;
    MediaFactory& operator=(MediaFactory&&) = delete;
};

#endif
