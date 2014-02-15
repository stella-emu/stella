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

#ifndef EVENTHANDLER_SDL2_HXX
#define EVENTHANDLER_SDL2_HXX

#include <SDL.h>
#include "EventHandler.hxx"

/**
  This class handles event collection from the point of view of the specific
  backend toolkit (SDL2).  It converts from SDL2-specific events into events
  that the Stella core can understand.

  @author  Stephen Anthony
  @version $Id$
*/
class EventHandlerSDL2 : public EventHandler
{
  public:
    /**
      Create a new SDL2 event handler object
    */
    EventHandlerSDL2(OSystem* osystem);
 
    /**
      Destructor
    */
    virtual ~EventHandlerSDL2();

  private:
    /**
      Set up any joysticks on the system.
    */
    void initializeJoysticks();

    /**
      Collects and dispatches any pending SDL2 events.
    */
    void pollEvent();

  private:
    SDL_Event event;

    // A thin wrapper around a basic StellaJoystick, holding the pointer to
    // the underlying SDL stick.
    class JoystickSDL2 : public StellaJoystick
    {
      public:
        JoystickSDL2(int idx);
        virtual ~JoystickSDL2();

      private:
        SDL_Joystick* stick;
    };
};

#endif
