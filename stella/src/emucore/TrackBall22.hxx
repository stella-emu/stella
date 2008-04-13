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
// $Id: TrackBall22.hxx,v 1.3 2008-04-13 23:43:14 stephena Exp $
//============================================================================

#ifndef TRACKBALL22_HXX
#define TRACKBALL22_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  The standard Atari 2600 CX-22 Trakball controller.

  @author  Stephen Anthony
  @version $Id: TrackBall22.hxx,v 1.3 2008-04-13 23:43:14 stephena Exp $
*/
class TrackBall22 : public Controller
{
  public:
    /**
      Create a new CX-22 TrackBall controller plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
    */
    TrackBall22(Jack jack, const Event& event, const System& system);

    /**
      Destructor
    */
    virtual ~TrackBall22();

  public:
    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    virtual void update();

  private:
    // Counter to iterate through the gray codes
    uInt32 myHCounter, myVCounter;

    // Pre-compute the events we care about based on given port
    // This will eliminate test for left or right port in update()
    Event::Type myUpEvent, myDownEvent, myLeftEvent, myRightEvent, myFireEvent;
};

#endif
