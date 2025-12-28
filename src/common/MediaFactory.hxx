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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "bspf.hxx"
#ifdef SDL_SUPPORT
  #include "SDL_lib.hxx"
#endif

#include "OSystem.hxx"
#include "Settings.hxx"
#include "SerialPort.hxx"
#ifdef BSPF_UNIX
  #include "SerialPortUNIX.hxx"
  #include "OSystemUNIX.hxx"
#elif defined(BSPF_WINDOWS)
  #include "SerialPortWINDOWS.hxx"
  #include "OSystemWINDOWS.hxx"
#elif defined(BSPF_MACOS)
  #include "SerialPortMACOS.hxx"
  #include "OSystemMACOS.hxx"
#elif defined(__LIB_RETRO__)
  #include "OSystemLIBRETRO.hxx"
#else
  #error Unsupported platform!
#endif

#ifdef __LIB_RETRO__
  #include "EventHandlerLIBRETRO.hxx"
  #include "FBBackendLIBRETRO.hxx"
#elif defined(SDL_SUPPORT)
  #include "EventHandlerSDL.hxx"
  #include "FBBackendSDL.hxx"
#else
  #error Unsupported backend!
#endif

#ifdef SOUND_SUPPORT
  #ifdef __LIB_RETRO__
    #include "SoundLIBRETRO.hxx"
  #elif defined(SDL_SUPPORT)
    #include "SoundSDL.hxx"
  #else
    #include "SoundNull.hxx"
  #endif
#else
  #include "SoundNull.hxx"
#endif

class AudioSettings;

/**
  This class deals with the different framebuffer/sound/event
  implementations for the various ports of Stella, and always returns a
  valid object based on the specific port and restrictions on that port.

  @author  Stephen Anthony
*/
class MediaFactory
{
  public:
    static unique_ptr<OSystem> createOSystem()
    {
    #ifdef BSPF_UNIX
      return make_unique<OSystemUNIX>();
    #elif defined(BSPF_WINDOWS)
      return make_unique<OSystemWINDOWS>();
    #elif defined(BSPF_MACOS)
      return make_unique<OSystemMACOS>();
    #elif defined(__LIB_RETRO__)
      return make_unique<OSystemLIBRETRO>();
    #else
      #error Unsupported platform for OSystem!
    #endif
    }

    static unique_ptr<Settings> createSettings()
    {
      return make_unique<Settings>();
    }

    static unique_ptr<SerialPort> createSerialPort()
    {
    #ifdef BSPF_UNIX
      return make_unique<SerialPortUNIX>();
    #elif defined(BSPF_WINDOWS)
      return make_unique<SerialPortWINDOWS>();
    #elif defined(BSPF_MACOS)
      return make_unique<SerialPortMACOS>();
    #else
      return make_unique<SerialPort>();
    #endif
    }

    static unique_ptr<FBBackend> createVideoBackend(OSystem& osystem)
    {
    #ifdef __LIB_RETRO__
      return make_unique<FBBackendLIBRETRO>(osystem);
    #elif defined(SDL_SUPPORT)
      return make_unique<FBBackendSDL>(osystem);
    #else
      #error Unsupported platform for FrameBuffer!
    #endif
    }

    static unique_ptr<Sound> createAudio(OSystem& osystem, AudioSettings& audioSettings)
    {
    #ifdef SOUND_SUPPORT
      #ifdef __LIB_RETRO__
        return make_unique<SoundLIBRETRO>(osystem, audioSettings);
      #elif defined(SOUND_SUPPORT) && defined(SDL_SUPPORT)
        return make_unique<SoundSDL>(osystem, audioSettings);
      #else
        return make_unique<SoundNull>(osystem);
      #endif
    #else
      return make_unique<SoundNull>(osystem);
    #endif
    }

    static unique_ptr<EventHandler> createEventHandler(OSystem& osystem)
    {
    #ifdef __LIB_RETRO__
      return make_unique<EventHandlerLIBRETRO>(osystem);
    #elif defined(SDL_SUPPORT)
      return make_unique<EventHandlerSDL>(osystem);
    #else
      #error Unsupported platform for EventHandler!
    #endif
    }

    static void cleanUp()
    {
    #ifdef SDL_SUPPORT
      SDL_Quit();
    #endif
    }

    static string backendName()
    {
    #ifdef SDL_SUPPORT
      return SDLVersion();
    #else
      return "Custom backend";
    #endif
    }

    static bool openURL(const string& url)
    {
    #ifdef SDL_SUPPORT
      return SDLOpenURL(url);
    #else
      return false;
    #endif
    }

  private:
    // Following constructors and assignment operators not supported
    MediaFactory() = delete;
    ~MediaFactory() = delete;
    MediaFactory(const MediaFactory&) = delete;
    MediaFactory(MediaFactory&&) = delete;
    MediaFactory& operator=(const MediaFactory&) = delete;
    MediaFactory& operator=(MediaFactory&&) = delete;
};

#endif
