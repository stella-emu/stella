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
EventHandlerSDL2::EventHandlerSDL2(OSystem* osystem)
  : EventHandler(osystem)
{
}

 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::~EventHandlerSDL2()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerSDL2::initializeJoysticks()
{
#ifdef JOYSTICK_SUPPORT
  // Initialize the joystick subsystem
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    myOSystem->logMessage("No joysticks present.", 1);
    return;
  }

  int numSticks = SDL_NumJoysticks();
  for(int i = 0; i < numSticks; ++i)
    addJoystick(new JoystickSDL2(i), i);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandlerSDL2::pollEvent()
{
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        handleKeyEvent((StellaKey)event.key.keysym.sym,
                       (StellaMod)event.key.keysym.mod,
//FIXSDL                       event.key.keysym.unicode & 0x7f,
                       event.key.keysym.scancode,
                       event.key.type == SDL_KEYDOWN);
        break;
      }

      case SDL_MOUSEMOTION:
      {
        handleMouseMotionEvent(event.motion.x, event.motion.y,
                               event.motion.xrel, event.motion.yrel, 0);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      {
        bool pressed = event.button.type == SDL_MOUSEBUTTONDOWN;
        switch(event.button.button)
        {
          case SDL_BUTTON_LEFT:
            handleMouseButtonEvent(pressed ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP,
                                   event.button.x, event.button.y);
            break;
          case SDL_BUTTON_RIGHT:
            handleMouseButtonEvent(pressed ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP,
                                   event.button.x, event.button.y);
            break;
        }
        break;
      }

      case SDL_MOUSEWHEEL:
      {
        if(event.wheel.y < 0)
          handleMouseButtonEvent(EVENT_WHEELDOWN, 0, event.wheel.y);
        else if(event.wheel.y > 0)
          handleMouseButtonEvent(EVENT_WHEELUP, 0, event.wheel.y);
        break;
      }

  #ifdef JOYSTICK_SUPPORT
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      {
        handleJoyEvent(event.jbutton.which, event.jbutton.button,
                       event.jbutton.state == SDL_PRESSED ? 1 : 0);
        break;
      }  

      case SDL_JOYAXISMOTION:
      {
        handleJoyAxisEvent(event.jaxis.which, event.jaxis.axis,
                           event.jaxis.value);
        break;
      }

      case SDL_JOYHATMOTION:
      {
        int v = event.jhat.value, value = 0;
        if(v & SDL_HAT_UP)        value |= EVENT_HATUP_M;
        if(v & SDL_HAT_DOWN)      value |= EVENT_HATDOWN_M;
        if(v & SDL_HAT_LEFT)      value |= EVENT_HATLEFT_M;
        if(v & SDL_HAT_RIGHT)     value |= EVENT_HATRIGHT_M;
        if(v == SDL_HAT_CENTERED) value  = EVENT_HATCENTER_M;

        handleJoyHatEvent(event.jhat.which, event.jhat.hat, value);
        break;  // SDL_JOYHATMOTION
      }
  #endif

      case SDL_QUIT:
        handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT

#if 0 //FIXSDL
      case SDL_ACTIVEEVENT:
        if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
          if(myState == S_EMULATE) enterMenuMode(S_MENU);
        break; // SDL_ACTIVEEVENT

      case SDL_VIDEOEXPOSE:
        myOSystem->frameBuffer().refresh();
        break;  // SDL_VIDEOEXPOSE
#endif

    }
  }
}

#ifdef JOYSTICK_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::JoystickSDL2::JoystickSDL2(int idx)
  : stick(NULL)
{
  stick = SDL_JoystickOpen(idx);
  if(stick)
  {
    initialize(SDL_JoystickName(stick), SDL_JoystickNumAxes(stick),
               SDL_JoystickNumButtons(stick), SDL_JoystickNumHats(stick));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandlerSDL2::JoystickSDL2::~JoystickSDL2()
{
  if(stick)
    SDL_JoystickClose(stick);
  stick = NULL;
}
#endif
