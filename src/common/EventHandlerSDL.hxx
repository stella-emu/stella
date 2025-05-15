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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef EVENTHANDLER_SDL_HXX
#define EVENTHANDLER_SDL_HXX

#include "SDL_lib.hxx"
#include "EventHandler.hxx"
#include "PhysicalJoystick.hxx"

/**
  This class handles event collection from the point of view of the specific
  backend toolkit (SDL).  It converts from SDL-specific events into events
  that the Stella core can understand.

  @author  Stephen Anthony
*/
class EventHandlerSDL : public EventHandler
{
  public:
    /**
      Create a new SDL event handler object
    */
    explicit EventHandlerSDL(OSystem& osystem);
    ~EventHandlerSDL() override;

  private:
    /**
      Enable/disable text events (distinct from single-key events).
    */
    void enableTextEvents(bool enable) override;

    /**
      Clipboard methods.
    */
    void copyText(const string& text) const override;
    string pasteText(string& text) const override;

    /**
      Collects and dispatches any pending SDL events.
    */
    void pollEvent() override;

  private:
    SDL_Event myEvent{0};

  #ifdef JOYSTICK_SUPPORT
    // A thin wrapper around a basic PhysicalJoystick, holding the pointer to
    // the underlying SDL joystick device.
    class JoystickSDL : public PhysicalJoystick
    {
      public:
        explicit JoystickSDL(int idx);
        virtual ~JoystickSDL();

      private:
        SDL_Joystick* myStick{nullptr};

      private:
        // Following constructors and assignment operators not supported
        JoystickSDL() = delete;
        JoystickSDL(const JoystickSDL&) = delete;
        JoystickSDL(JoystickSDL&&) = delete;
        JoystickSDL& operator=(const JoystickSDL&) = delete;
        JoystickSDL& operator=(JoystickSDL&&) = delete;
    };
  #endif

  private:
    // Following constructors and assignment operators not supported
    EventHandlerSDL() = delete;
    EventHandlerSDL(const EventHandlerSDL&) = delete;
    EventHandlerSDL(EventHandlerSDL&&) = delete;
    EventHandlerSDL& operator=(const EventHandlerSDL&) = delete;
    EventHandlerSDL& operator=(EventHandlerSDL&&) = delete;
};

#endif
