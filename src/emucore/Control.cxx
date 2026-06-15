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
#include "TIA.hxx"
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
  const Event::Type event = myDigitalPinEvent[static_cast<int>(pin)];

  // An event-bound pin reflects the input's value at the current scanline,
  // so it can change mid-frame just as the user's input did.  When no
  // input transitioned this frame (the common case) the value is constant
  // across the frame and equals the cached pin state, so skip the sub-frame
  // machinery (scanline lookup + mutex-locked transition scan) entirely.
  if(event != Event::NoType && myEvent.hasTransitions())
    return myEvent.get(event, currentInputPos()) == 0;

  return getPin(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Controller::currentInputPos() const
{
  return mySystem.tia().subFramePosition();
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
    AnalogReadout::Connection conn;
    conn.load(in);
    setPin(AnalogPin::Five, conn);
    conn.load(in);
    setPin(AnalogPin::Nine, conn);
  }
  catch(...)
  {
    cerr << "ERROR: Controller::load() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view Controller::getName(const Type type)
{
  assert(static_cast<std::size_t>(type) < CONTROLLER_INFO.size());
  return CONTROLLER_INFO[static_cast<int>(type)].name;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view Controller::getPropName(const Type type)
{
  assert(static_cast<std::size_t>(type) < CONTROLLER_INFO.size());
  return CONTROLLER_INFO[static_cast<int>(type)].propName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::Type Controller::getType(string_view propName)
{
  // NOLINTNEXTLINE(readability-qualified-auto)
  const auto it = std::ranges::find_if(CONTROLLER_INFO,
      [&](const auto& info) { return BSPF::equalsIgnoreCase(propName, info.propName); });
  if(it != CONTROLLER_INFO.end())
    return Type{static_cast<uInt8>(std::distance(CONTROLLER_INFO.begin(), it))};

  // special case
  if(BSPF::equalsIgnoreCase(propName, "KEYPAD"))
    return Type::Keyboard;

  return Type::Unknown;
}

