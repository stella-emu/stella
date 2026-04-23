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

#include <cassert>

#include "System.hxx"
#include "Control.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Controller(Jack jack, const Event& event, const System& system,
                       Type type)
  : myJack{jack},
    myEvent{event},
    mySystem{system},
    myType{type}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Controller::read()
{
  return (static_cast<uInt8>(read(DigitalPin::One))   << 0) |
         (static_cast<uInt8>(read(DigitalPin::Two))   << 1) |
         (static_cast<uInt8>(read(DigitalPin::Three)) << 2) |
         (static_cast<uInt8>(read(DigitalPin::Four))  << 3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::read(DigitalPin pin)
{
  return getPin(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnalogReadout::Connection Controller::read(AnalogPin pin)
{
  return getPin(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::save(Serializer& out) const
{
  try
  {
    // Output the digital pins
    out.putBool(getPin(DigitalPin::One));
    out.putBool(getPin(DigitalPin::Two));
    out.putBool(getPin(DigitalPin::Three));
    out.putBool(getPin(DigitalPin::Four));
    out.putBool(getPin(DigitalPin::Six));

    // Output the analog pins
    getPin(AnalogPin::Five).save(out);
    getPin(AnalogPin::Nine).save(out);
  }
  catch(...)
  {
    cerr << "ERROR: Controller::save() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Controller::load(Serializer& in)
{
  try
  {
    // Input the digital pins
    setPin(DigitalPin::One,   in.getBool());
    setPin(DigitalPin::Two,   in.getBool());
    setPin(DigitalPin::Three, in.getBool());
    setPin(DigitalPin::Four,  in.getBool());
    setPin(DigitalPin::Six,   in.getBool());

    // Input the analog pins
    getPin(AnalogPin::Five).load(in);
    getPin(AnalogPin::Nine).load(in);
  }
  catch(...)
  {
    cerr << "ERROR: Controller::load() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Controller::getName(const Type type)
{
  assert(static_cast<std::size_t>(type) < CONTROLLER_INFO.size());
  return string{CONTROLLER_INFO[static_cast<int>(type)].name};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Controller::getPropName(const Type type)
{
  assert(static_cast<std::size_t>(type) < CONTROLLER_INFO.size());
  return string{CONTROLLER_INFO[static_cast<int>(type)].propName};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Type Controller::getType(string_view propName)
{
  for(uInt8 i = 0; i < static_cast<uInt8>(Type::LastType); ++i)
    if(BSPF::equalsIgnoreCase(propName, CONTROLLER_INFO[i].propName))
      return Type{i};

  // special case
  if(BSPF::equalsIgnoreCase(propName, "KEYPAD"))
    return Type::Keyboard;

  return Type::Unknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// int Controller::analogDeadZoneValue(int deadZone)
// {
//   deadZone = BSPF::clamp(deadZone, MIN_ANALOG_DEADZONE, MAX_ANALOG_DEADZONE);
//
//   return deadZone * std::round(32768 / 2. / MAX_DIGITAL_DEADZONE);
// }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Controller::DIGITAL_DEAD_ZONE = 3200;
int Controller::ANALOG_DEAD_ZONE = 0;
int Controller::MOUSE_SENSITIVITY = -1;
bool Controller::AUTO_FIRE = false;
int Controller::AUTO_FIRE_RATE = 0;
