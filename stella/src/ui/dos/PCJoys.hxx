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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: PCJoys.hxx,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
//============================================================================

#ifndef PCJOYSTICKS_HXX
#define PCJOYSTICKS_HXX

#include "bspf.hxx"

class PCJoysticks
{
  public:
    /**
      Constructor

      @param useAxisMidpoint Indicates if a "midpoints" should be used
    */
    PCJoysticks(bool useAxisMidpoint);

    /**
      Destructor
    */
    ~PCJoysticks();

  public:
    /**
      Answers true iff a joystick is connected to the system

      @return true iff a joysticks is connected
    */
    bool present() const;

    /**
      Read the state of the joystick and update the button and axis arrays

      @param buttons Array which holds the button state upon exit
      @param axes Array which holds the axis state upon exit
    */
    void read(bool buttons[4], Int16 axes[4]);

  private:
    // Answers mask that indicates which joysticks are present
    uInt8 detect() const;

    // Calibrate axes midpoints
    void calibrate();

  private:
    uInt8 myPresent;
    const uInt16 myGamePort;
    const bool myUseAxisMidpoint;
    Int32 myMinimum[4];
    Int32 myMaximum[4];
    Int32 myMidpoint[4];
};
#endif

