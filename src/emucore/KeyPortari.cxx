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

#include "Console.hxx"
#include "Event.hxx"
#include "Logger.hxx"
#include "System.hxx"
#include "KeyPortari.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyPortari::KeyPortari()
  : myKeyCodeMappingArray{KeyPortari::DefaultKeyCodeMappingArray}
{}

void KeyPortari::bindController(const Controller::Jack jack, const Event& event, const System& system, unique_ptr<Controller>& bindingController, unique_ptr<Controller>& passthroughController) {
  unique_ptr<Controller> controller = make_unique<KPControl>(*this, jack, event, system, passthroughController);
  bindingController = std::move(controller);
}

KeyPortari::KeyCodeMappingArray KeyPortari::DefaultKeyCodeMappingArray = {
  {Event::KeyPortari0,             '0'},
  {Event::KeyPortari1,             '1'},
  {Event::KeyPortari2,             '2'},
  {Event::KeyPortari3,             '3'},
  {Event::KeyPortari4,             '4'},
  {Event::KeyPortari5,             '5'},
  {Event::KeyPortari6,             '6'},
  {Event::KeyPortari7,             '7'},
  {Event::KeyPortari8,             '8'},
  {Event::KeyPortari9,             '9'},
  {Event::KeyPortariA,             'A'},
  {Event::KeyPortariB,             'B'},
  {Event::KeyPortariC,             'C'},
  {Event::KeyPortariD,             'D'},
  {Event::KeyPortariE,             'E'},
  {Event::KeyPortariF,             'F'},
  {Event::KeyPortariG,             'G'},
  {Event::KeyPortariH,             'H'},
  {Event::KeyPortariI,             'I'},
  {Event::KeyPortariJ,             'J'},
  {Event::KeyPortariK,             'K'},
  {Event::KeyPortariL,             'L'},
  {Event::KeyPortariM,             'M'},
  {Event::KeyPortariN,             'N'},
  {Event::KeyPortariO,             'O'},
  {Event::KeyPortariP,             'P'},
  {Event::KeyPortariQ,             'Q'},
  {Event::KeyPortariR,             'R'},
  {Event::KeyPortariS,             'S'},
  {Event::KeyPortariT,             'T'},
  {Event::KeyPortariU,             'U'},
  {Event::KeyPortariV,             'V'},
  {Event::KeyPortariW,             'W'},
  {Event::KeyPortariX,             'X'},
  {Event::KeyPortariY,             'Y'},
  {Event::KeyPortariZ,             'Z'},
  {Event::KeyPortariComma,         ','},
  {Event::KeyPortariPeriod,        '.'},
  {Event::KeyPortariEnter,         '\n'},
  {Event::KeyPortariSpace,         ' '},
  {Event::KeyPortariQuestion,      '?'},
  {Event::KeyPortariLeftBracket,   '['},
  {Event::KeyPortariRightBracket,  ']'},
  {Event::KeyPortariMinus,         '-'},
  {Event::KeyPortariQuote,         '\''},
  {Event::KeyPortariEquals,        '='},
  {Event::KeyPortariPlus,          '+'},
  {Event::KeyPortariSlash,         '/'}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortari::KPControl::update()
{
  uInt8 c = 0xff;
  // BUGBUG: do we need to check all mappings?
  for (auto &mapping : myHandler.keyCodeMappingArray()) {
    if (myEvent.get(mapping.event)) {
      c = mapping.code;
      break;
    }
  }
  if (myJack == Controller::Jack::Left) {
    c = c >> 4;
  }
  setPin(DigitalPin::One, c & 0b0000001);
  setPin(DigitalPin::Two, c & 0b0000010);
  setPin(DigitalPin::Three, c & 0b0000100);
  setPin(DigitalPin::Four, c & 0b0001000);
  
  // passthrough if no key is pressed
  if (c == 0xff && myPassthroughController) {
    myPassthroughController->update();
  }
  
}

KeyPortari::KPControl::KPControl(class KeyPortari& handler, Controller::Jack jack, const Event& event,
                                 const System& system, unique_ptr<Controller>& passthroughController)
: Controller{jack, event, system, Controller::Type::KeyPortari},
  myHandler{handler},
  myPassthroughController{std::move(passthroughController)}
{
  if (jack == Controller::Jack::Left) {
    setPin(Controller::AnalogPin::Nine,
           AnalogReadout::connectToGround());
    setPin(Controller::AnalogPin::Five,
           AnalogReadout::connectToVcc());
  } else {
    setPin(Controller::AnalogPin::Nine,
           AnalogReadout::connectToVcc());
    setPin(Controller::AnalogPin::Five,
           AnalogReadout::connectToGround());
  }
}
