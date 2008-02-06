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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Driving.hxx,v 1.8 2008-02-06 13:45:21 stephena Exp $
//============================================================================

#ifndef DRIVING_HXX
#define DRIVING_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  The standard Atari 2600 Indy 500 driving controller.

  @author  Bradford W. Mott
  @version $Id: Driving.hxx,v 1.8 2008-02-06 13:45:21 stephena Exp $
*/
class Driving : public Controller
{
  public:
    /**
      Create a new Indy 500 driving controller plugged into 
      the specified jack

      @param jack The jack the controller is plugged into
      @param event The event object to use for events
    */
    Driving(Jack jack, const Event& event);

    /**
      Destructor
    */
    virtual ~Driving();

  public:
    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    virtual void update();

  private:
    // Counter to iterate through the gray codes
    uInt32 myCounter;

    // Pre-compute the events we care about based on given port
    // This will eliminate test for left or right port in update()
    Event::Type myCWEvent, myCCWEvent, myValueEvent, myFireEvent;
};

#endif
