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
#include "FrameBuffer.hxx"
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

  // Re-render while the OS is in its modal window-resize loop (Windows/macOS),
  // during which the main event loop is blocked.  See resizeWatch().
  //
  // Only Windows and macOS have such a modal loop.  X11/Wayland do not -
  // SDL_PollEvent already delivers resize/expose events normally there, so the
  // watch is redundant.  Worse, on X11 it is actively harmful: a window-manager
  // move/resize drag floods the app with events, the watch fires (and re-renders)
  // synchronously for every one, and that unthrottled render loop locks the main
  // thread until the drag ends (showing a frozen/stretched frame meanwhile).
  // So register it only on the platforms that need it.
#if defined(BSPF_WINDOWS) || defined(BSPF_MACOS)
  SDL_AddEventWatch(resizeWatch, this);
#endif
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
bool EventHandlerSDL::hasClipboardText() const
{
  return SDL_HasClipboardText();
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
                               myEvent.motion.xrel, myEvent.motion.yrel,
                               myEvent.motion.windowID);
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
                               myEvent.button.x, myEvent.button.y,
                               myEvent.button.windowID);
        break;
      }

      case SDL_EVENT_MOUSE_WHEEL:
      {
        // SDL now uses float for mouse coords, but the core still uses int
        // throughout; this is sufficient for our current needs.  The wheel
        // event carries the window-relative mouse position, which is what we
        // need so the correct (possibly secondary) window receives it.
        const int x = static_cast<int>(myEvent.wheel.mouse_x);
        const int y = static_cast<int>(myEvent.wheel.mouse_y);
        if(myEvent.wheel.y < 0)
          handleMouseButtonEvent(MouseButton::WHEELDOWN, true, x, y,
                                 myEvent.wheel.windowID);
        else if(myEvent.wheel.y > 0)
          handleMouseButtonEvent(MouseButton::WHEELUP, true, x, y,
                                 myEvent.wheel.windowID);
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

      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        handleWindowCloseEvent(myEvent.window.windowID);
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
        // Pass the window ID so a repaint of the secondary (companion) window
        // can be routed to it rather than forcing a primary-window render
        handleSystemEvent(SystemEvent::WINDOW_EXPOSED,
                          static_cast<int>(myEvent.window.windowID));
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandlerSDL::resizeWatch(void* userdata, SDL_Event* event)
{
  // SDL invokes event watches synchronously from within its event pump,
  // including while the OS runs its own modal window-resize loop (Windows/
  // macOS).  During that loop pollEvent() above is blocked, so without this
  // hook the window shows black while being dragged.  Re-dispatching the
  // resize/expose here lets the normal handler run during the loop: it
  // re-flows the launcher live (or keeps the debugger's stretch active) and
  // presents each frame, exactly as it would from the main loop.
  auto* const self = static_cast<EventHandlerSDL*>(userdata);

  switch(event->type)
  {
    case SDL_EVENT_WINDOW_RESIZED:
      self->handleSystemEvent(SystemEvent::WINDOW_RESIZED,
                              event->window.data1, event->window.data2);
      break;
    case SDL_EVENT_WINDOW_EXPOSED:
      self->handleSystemEvent(SystemEvent::WINDOW_EXPOSED);
      break;
    default:
      break;
  }
  return true;  // return value is ignored for event watches
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
