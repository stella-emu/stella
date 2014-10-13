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

#include "OSystem.hxx"
#include "EventHandlerSDL2.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::EventHandlerSDL2(OSystem& osystem)
  : EventHandler(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::~EventHandlerSDL2()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerSDL2::enableTextEvents(bool enable)
{
  if(enable)
    SDL_StartTextInput();
  else
    SDL_StopTextInput();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* EventHandlerSDL2::nameForKey(StellaKey key)
{
  return SDL_GetScancodeName(SDL_Scancode(key));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerSDL2::pollEvent()
{
  while(SDL_PollEvent(&myEvent))
  {
    switch(myEvent.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        if(!myEvent.key.repeat)
          handleKeyEvent((StellaKey)myEvent.key.keysym.scancode,
                         (StellaMod)myEvent.key.keysym.mod,
                          myEvent.key.type == SDL_KEYDOWN);
        break;
      }

      case SDL_TEXTINPUT:
      {
        handleTextEvent(*(myEvent.text.text));
        break;
      }

      case SDL_MOUSEMOTION:
      {
        handleMouseMotionEvent(myEvent.motion.x, myEvent.motion.y,
                               myEvent.motion.xrel, myEvent.motion.yrel, 0);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      {
        bool pressed = myEvent.button.type == SDL_MOUSEBUTTONDOWN;
        switch(myEvent.button.button)
        {
          case SDL_BUTTON_LEFT:
            handleMouseButtonEvent(pressed ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP,
                                   myEvent.button.x, myEvent.button.y);
            break;
          case SDL_BUTTON_RIGHT:
            handleMouseButtonEvent(pressed ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP,
                                   myEvent.button.x, myEvent.button.y);
            break;
        }
        break;
      }

      case SDL_MOUSEWHEEL:
      {
        int x, y;
        SDL_GetMouseState(&x, &y);  // we need mouse position too
        if(myEvent.wheel.y < 0)
          handleMouseButtonEvent(EVENT_WHEELDOWN, x, y);
        else if(myEvent.wheel.y > 0)
          handleMouseButtonEvent(EVENT_WHEELUP, x, y);
        break;
      }

  #ifdef JOYSTICK_SUPPORT
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      {
        handleJoyEvent(myEvent.jbutton.which, myEvent.jbutton.button,
                       myEvent.jbutton.state == SDL_PRESSED ? 1 : 0);
        break;
      }  

      case SDL_JOYAXISMOTION:
      {
        handleJoyAxisEvent(myEvent.jaxis.which, myEvent.jaxis.axis,
                           myEvent.jaxis.value);
        break;
      }

      case SDL_JOYHATMOTION:
      {
        int v = myEvent.jhat.value, value = 0;
        if(v & SDL_HAT_UP)        value |= EVENT_HATUP_M;
        if(v & SDL_HAT_DOWN)      value |= EVENT_HATDOWN_M;
        if(v & SDL_HAT_LEFT)      value |= EVENT_HATLEFT_M;
        if(v & SDL_HAT_RIGHT)     value |= EVENT_HATRIGHT_M;
        if(v == SDL_HAT_CENTERED) value  = EVENT_HATCENTER_M;

        handleJoyHatEvent(myEvent.jhat.which, myEvent.jhat.hat, value);
        break;  // SDL_JOYHATMOTION
      }

      case SDL_JOYDEVICEADDED:
      {
        addJoystick(new JoystickSDL2(myEvent.jdevice.which));
        break;  // SDL_JOYDEVICEADDED
      }
      case SDL_JOYDEVICEREMOVED:
      {
        removeJoystick(myEvent.jdevice.which);
        break;  // SDL_JOYDEVICEREMOVED
      }
  #endif

      case SDL_QUIT:
      {
        handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT
      }

      case SDL_WINDOWEVENT:
        switch(myEvent.window.event)
        {
          case SDL_WINDOWEVENT_SHOWN:
            handleSystemEvent(EVENT_WINDOW_SHOWN);
            break;
          case SDL_WINDOWEVENT_HIDDEN:
            handleSystemEvent(EVENT_WINDOW_HIDDEN);
            break;
          case SDL_WINDOWEVENT_EXPOSED:
            handleSystemEvent(EVENT_WINDOW_EXPOSED);
            break;
          case SDL_WINDOWEVENT_MOVED:
            handleSystemEvent(EVENT_WINDOW_MOVED,
                              myEvent.window.data1, myEvent.window.data1);
            break;
          case SDL_WINDOWEVENT_RESIZED:
            handleSystemEvent(EVENT_WINDOW_RESIZED,
                              myEvent.window.data1, myEvent.window.data1);
            break;
          case SDL_WINDOWEVENT_MINIMIZED:
            handleSystemEvent(EVENT_WINDOW_MINIMIZED);
            break;
          case SDL_WINDOWEVENT_MAXIMIZED:
            handleSystemEvent(EVENT_WINDOW_MAXIMIZED);
            break;
          case SDL_WINDOWEVENT_RESTORED:
            handleSystemEvent(EVENT_WINDOW_RESTORED);
            break;
          case SDL_WINDOWEVENT_ENTER:
            handleSystemEvent(EVENT_WINDOW_ENTER);
            break;
          case SDL_WINDOWEVENT_LEAVE:
            handleSystemEvent(EVENT_WINDOW_LEAVE);
            break;
          case SDL_WINDOWEVENT_FOCUS_GAINED:
            handleSystemEvent(EVENT_WINDOW_FOCUS_GAINED);
            break;
          case SDL_WINDOWEVENT_FOCUS_LOST:
            handleSystemEvent(EVENT_WINDOW_FOCUS_LOST);
            break;
        }
        break;  // SDL_WINDOWEVENT
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::JoystickSDL2::JoystickSDL2(int idx)
  : myStick(NULL)
{
  myStick = SDL_JoystickOpen(idx);
  if(myStick)
  {
    initialize(idx, SDL_JoystickName(myStick),
        SDL_JoystickNumAxes(myStick), SDL_JoystickNumButtons(myStick),
        SDL_JoystickNumHats(myStick), SDL_JoystickNumBalls(myStick));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::JoystickSDL2::~JoystickSDL2()
{
  if(myStick)
    SDL_JoystickClose(myStick);
  myStick = NULL;
}
