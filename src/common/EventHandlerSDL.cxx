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

#include "Logger.hxx"
#include "OSystem.hxx"
#include "EventHandlerSDL.hxx"

#include "ThreadDebugging.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL::EventHandlerSDL(OSystem& osystem)
  : EventHandler{osystem}
{
  ASSERT_MAIN_THREAD;

#ifdef GUI_SUPPORT
  myQwertz = int{'y'} == static_cast<int>
    (SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(StellaKey::Z),
                            static_cast<SDL_Keymod>(StellaMod::NONE), false));
  Logger::debug(std::format("Keyboard: {}", myQwertz ? "QWERTZ" : "QWERTY"));
#endif

#ifdef JOYSTICK_SUPPORT
  if(!SDL_InitSubSystem(SDL_INIT_JOYSTICK))
    Logger::error(std::format("ERROR: Couldn't initialize SDL joystick support: {}\n",
                              SDL_GetError()));
  Logger::debug("EventHandlerSDL::EventHandlerSDL SDL_INIT_JOYSTICK");
#endif

  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
  //SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1");
  //SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL::~EventHandlerSDL()
{
  ASSERT_MAIN_THREAD;

  if(SDL_WasInit(SDL_INIT_JOYSTICK))
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerSDL::copyText(const string& text) const
{
  SDL_SetClipboardText(text.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandlerSDL::pasteText(string& text) const
{
  if(SDL_HasClipboardText())
    text = SDL_GetClipboardText();
  else
    text = "";

  return text;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerSDL::pollEvent()
{
  ASSERT_MAIN_THREAD;

  while(SDL_PollEvent(&myEvent))
  {
    switch(myEvent.type)
    {
      // keyboard events
      case SDL_EVENT_KEY_UP:
      case SDL_EVENT_KEY_DOWN:
        handleKeyEvent(static_cast<StellaKey>(myEvent.key.scancode),
                       static_cast<StellaMod>(myEvent.key.mod),
                       myEvent.type == SDL_EVENT_KEY_DOWN,
                       myEvent.key.repeat);
        break;

      case SDL_EVENT_TEXT_INPUT:
        handleTextEvent(*(myEvent.text.text));
        break;

      case SDL_EVENT_MOUSE_MOTION:
        handleMouseMotionEvent(myEvent.motion.x, myEvent.motion.y,
                               myEvent.motion.xrel, myEvent.motion.yrel);
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      {
        // ToDo: check support of more buttons and double-click
        MouseButton b{MouseButton::NONE};
        switch(myEvent.button.button)
        {
          case SDL_BUTTON_LEFT:
            b = MouseButton::LEFT;
            break;
          case SDL_BUTTON_RIGHT:
            b = MouseButton::RIGHT;
            break;
          case SDL_BUTTON_MIDDLE:
            b = MouseButton::MIDDLE;
            break;
          default:
            break;
        }
        handleMouseButtonEvent(b, myEvent.button.type == SDL_EVENT_MOUSE_BUTTON_DOWN,
                               myEvent.button.x, myEvent.button.y);
        break;
      }

      case SDL_EVENT_MOUSE_WHEEL:
      {
        // TODO: SDL now uses float for mouse coords, but the core still
        //       uses int throughout; maybe this is sufficient?
        float x{0.F}, y{0.F};
        SDL_GetMouseState(&x, &y);  // we need mouse position too
        if(myEvent.wheel.y < 0)
          handleMouseButtonEvent(MouseButton::WHEELDOWN, true,
                                 static_cast<int>(x), static_cast<int>(y));
        else if(myEvent.wheel.y > 0)
          handleMouseButtonEvent(MouseButton::WHEELUP, true,
                                 static_cast<int>(x), static_cast<int>(y));
        break;
      }

  #ifdef JOYSTICK_SUPPORT
      case SDL_EVENT_JOYSTICK_BUTTON_UP:
      case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        handleJoyBtnEvent(myEvent.jbutton.which, myEvent.jbutton.button,
                          myEvent.jbutton.down);
        break;

      case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        handleJoyAxisEvent(myEvent.jaxis.which, myEvent.jaxis.axis,
                           myEvent.jaxis.value);
        break;

      case SDL_EVENT_JOYSTICK_HAT_MOTION:
      {
        JoyHatMask value = JoyHatMask::NONE;
        const int v = myEvent.jhat.value;
        if(v == SDL_HAT_CENTERED)
          value  = JoyHatMask::CENTER;
        else
        {
          if(v & SDL_HAT_UP)    value |= JoyHatMask::UP;
          if(v & SDL_HAT_DOWN)  value |= JoyHatMask::DOWN;
          if(v & SDL_HAT_LEFT)  value |= JoyHatMask::LEFT;
          if(v & SDL_HAT_RIGHT) value |= JoyHatMask::RIGHT;
        }

        handleJoyHatEvent(myEvent.jhat.which, myEvent.jhat.hat, value);
        break;
      }

      case SDL_EVENT_JOYSTICK_ADDED:
        addPhysicalJoystick(std::make_shared<JoystickSDL>(myEvent.jdevice.which));
        break;

      case SDL_EVENT_JOYSTICK_REMOVED:
        removePhysicalJoystick(myEvent.jdevice.which);
        break;
  #endif

      case SDL_EVENT_QUIT:
        handleEvent(Event::Quit);
        break;

      case SDL_EVENT_DROP_FILE:
        handleDropfileEvent(myEvent.drop.data);
        break;

      case SDL_EVENT_WINDOW_SHOWN:
        handleSystemEvent(SystemEvent::WINDOW_SHOWN);
        break;
      case SDL_EVENT_WINDOW_HIDDEN:
        handleSystemEvent(SystemEvent::WINDOW_HIDDEN);
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
        handleSystemEvent(SystemEvent::WINDOW_EXPOSED);
        break;
      case SDL_EVENT_WINDOW_MOVED:
        handleSystemEvent(SystemEvent::WINDOW_MOVED,
                          myEvent.window.data1, myEvent.window.data2);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        handleSystemEvent(SystemEvent::WINDOW_RESIZED,
                          myEvent.window.data1, myEvent.window.data2);
        break;
      case SDL_EVENT_WINDOW_MINIMIZED:
        handleSystemEvent(SystemEvent::WINDOW_MINIMIZED);
        break;
      case SDL_EVENT_WINDOW_MAXIMIZED:
        handleSystemEvent(SystemEvent::WINDOW_MAXIMIZED);
        break;
      case SDL_EVENT_WINDOW_RESTORED:
        handleSystemEvent(SystemEvent::WINDOW_RESTORED);
        break;
      case SDL_EVENT_WINDOW_MOUSE_ENTER:
        handleSystemEvent(SystemEvent::WINDOW_ENTER);
        break;
      case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        handleSystemEvent(SystemEvent::WINDOW_LEAVE);
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
        handleSystemEvent(SystemEvent::WINDOW_FOCUS_GAINED);
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        handleSystemEvent(SystemEvent::WINDOW_FOCUS_LOST);
        break;
      case SDL_EVENT_SYSTEM_THEME_CHANGED:
        handleSystemEvent(SystemEvent::THEME_CHANGED);
        break;
      default:
        break;
    }
  }
}

#ifdef JOYSTICK_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL::JoystickSDL::JoystickSDL(int idx)
{
  ASSERT_MAIN_THREAD;

  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  myStick = SDL_OpenJoystick(idx);
  if(myStick)
  {
    // In Windows, all XBox controllers using the XInput API seem to name
    // the controller as "XInput Controller".  This would be fine, except
    // it also appends " #x", where x seems to vary. Obviously this wreaks
    // havoc with the idea that a joystick will always have the same name.
    // So we truncate the number.
    const char* const sdlname = SDL_GetJoystickName(myStick);
    const string desc = BSPF::startsWithIgnoreCase(sdlname, "XInput Controller")
        ? "XInput Controller" : sdlname;

    initialize(SDL_GetJoystickID(myStick), desc,
               SDL_GetNumJoystickAxes(myStick),
               SDL_GetNumJoystickButtons(myStick),
               SDL_GetNumJoystickHats(myStick),
               SDL_GetNumJoystickBalls(myStick));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL::JoystickSDL::~JoystickSDL()
{
  ASSERT_MAIN_THREAD;

  if(SDL_WasInit(SDL_INIT_JOYSTICK) && myStick)
    SDL_CloseJoystick(myStick);
}
#endif
