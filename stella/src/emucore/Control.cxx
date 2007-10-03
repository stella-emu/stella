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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Control.cxx,v 1.7 2007-10-03 21:41:17 stephena Exp $
//============================================================================

#include <cassert>

#include "Control.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Controller(Jack jack, const Event& event, Type type)
  : myJack(jack),
    myEvent(event),
    myType(type)
{
  myDigitalPinState[One]   = 
  myDigitalPinState[Two]   = 
  myDigitalPinState[Three] = 
  myDigitalPinState[Four]  = 
  myDigitalPinState[Six]   = true;

  myAnalogPinValue[Five] = 
  myAnalogPinValue[Nine] = maximumResistance;

  switch(myType)
  {
    case Joystick:
      myName = "Joystick";
      break;
    case Paddles:
      myName = "Paddles";
      break;
    case BoosterGrip:
      myName = "BoosterGrip";
      break;
    case Driving:
      myName = "Driving";
      break;
    case Keyboard:
      myName = "Keyboard";
      break;
    case TrackBall22:
      myName = "TrackBall22";
      break;
    case AtariVox:
      myName = "AtariVox";
      break;
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::~Controller()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Controller::Type Controller::type() const
{
  return myType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::read(DigitalPin pin) const
{
  switch(pin)
  {
    case One:
    case Two:
    case Three:
    case Four:
    case Six:
      return myDigitalPinState[pin];

    default:
      return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 Controller::read(AnalogPin pin) const
{
  switch(pin)
  {
    case Five:
    case Nine:
      return myAnalogPinValue[pin];

    default:
      return maximumResistance;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::save(Serializer& out) const
{
  try
  {
    // Output the digital pins
    out.putBool(myDigitalPinState[One]);
    out.putBool(myDigitalPinState[Two]);
    out.putBool(myDigitalPinState[Three]);
    out.putBool(myDigitalPinState[Four]);
    out.putBool(myDigitalPinState[Six]);

    // Output the analog pins
    out.putInt(myAnalogPinValue[Five]);
    out.putInt(myAnalogPinValue[Nine]);
  }
  catch(...)
  {
    cerr << "Error: Controller::save() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::load(Deserializer& in)
{
  try
  {
    // Input the digital pins
    myDigitalPinState[One]   = in.getBool();
    myDigitalPinState[Two]   = in.getBool();
    myDigitalPinState[Three] = in.getBool();
    myDigitalPinState[Four]  = in.getBool();
    myDigitalPinState[Six]   = in.getBool();

    // Input the analog pins
    myAnalogPinValue[Five] = (Int32) in.getInt();
    myAnalogPinValue[Nine] = (Int32) in.getInt();
  }
  catch(...)
  {
    cerr << "Error: Controller::load() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Controller::name() const
{
  return myName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Int32 Controller::maximumResistance = 0x7FFFFFFF;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Int32 Controller::minimumResistance = 0x00000000;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Controller(const Controller& c)
  : myJack(c.myJack),
    myEvent(c.myEvent),
    myType(c.myType)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller& Controller::operator = (const Controller&)
{
  assert(false);
  return *this;
}
