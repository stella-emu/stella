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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.hxx,v 1.1 2003-09-03 20:10:58 stephena Exp $
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include "bspf.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"
#include "MediaSrc.hxx"

/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontends will send translated events here, and the handler will
  check to see what the current 'mode' is.  For now, the modes can be
  normal and remap.

  If in normal mode, events received from the frontends are remapped and
  sent to the emulation core.  If in remap mode, the events are sent
  unchanged to the remap class, where key remapping can take place.

  @author  Stephen Anthony
  @version $Id: EventHandler.hxx,v 1.1 2003-09-03 20:10:58 stephena Exp $
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler();
 
    /**
      Destructor
    */
    virtual ~EventHandler();

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    Event* event();

    /**
      Send a keyboard event to the handler.

      @param key   The StellaEvent key
      @param state The StellaEvent state (pressed or released)
    */
    void sendKeyEvent(StellaEvent::KeyCode key, StellaEvent::KeyState state);

    /**
      Set the mediasource.

      @param mediaSource   The mediasource
    */
    void setMediaSource(MediaSource& mediaSource);

  private:
    void setDefaultKeyMapping();
    void setDefaultJoyMapping();

  private:
    struct KeyEvent
    {
      Event::Type type;
      string message;
    };

    // Array of key events
    KeyEvent keyTable[StellaEvent::LastKCODE];

    // Global Event object
    Event* myEvent;

    // Global mediasource object
    MediaSource* myMediaSource;
};

#endif
