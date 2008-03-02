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
// $Id: Paddles.hxx,v 1.11 2008-03-02 19:20:50 stephena Exp $
//============================================================================

#ifndef PADDLES_HXX
#define PADDLES_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  The standard Atari 2600 pair of paddle controllers.

  @author  Bradford W. Mott
  @version $Id: Paddles.hxx,v 1.11 2008-03-02 19:20:50 stephena Exp $
*/
class Paddles : public Controller
{
  public:
    /**
      Create a new pair of paddle controllers plugged into the specified jack

      @param jack  The jack the controller is plugged into
      @param event The event object to use for events
      @param swap  Whether to swap the paddles plugged into this jack
    */
    Paddles(Jack jack, const Event& event, bool swap);

    /**
      Destructor
    */
    virtual ~Paddles();

  public:
    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    virtual void update();

  private:
    // Pre-compute the events we care about based on given port
    // This will eliminate test for left or right port in update()
    Event::Type myP0AxisValue, myP1AxisValue,
                myP0DecEvent1, myP0DecEvent2, myP0IncEvent1, myP0IncEvent2,
                myP1DecEvent1, myP1DecEvent2, myP1IncEvent1, myP1IncEvent2,
                myP0FireEvent1, myP0FireEvent2, myP1FireEvent1, myP1FireEvent2;

    int myKeyRepeat0;
    int myPaddleRepeat0;
    int myKeyRepeat1;
    int myPaddleRepeat1;

    int myCharge[2];
    int myLastCharge[2];
    int myLeftMotion[2];

    int myMouseBaseX, myMouseBaseY;
};

#endif
