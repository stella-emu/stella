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
KeyPortari::KeyPortari(KeyPortari::Protocol protocol, const Event &event)
 : myProtocol(protocol), myEvent(event)
{
}

void KeyPortari::bindController(const Controller::Jack jack, const Event& event, const System& system, unique_ptr<Controller>& bindingController, unique_ptr<Controller>& passthroughController)
{
  unique_ptr<KPControl> controller = make_unique<KPControl>(*this, jack, event, system);
  if (myProtocol == KeyPortari::Protocol::AlphaNumeric) {
    controller->setPassthroughController(passthroughController);
  }
  bindingController = std::move(controller);
  
}

uint8_t KeyPortari::mapKeyCode() {
  const KeyCodeMappingArray &keyCodeMappingArray = myProtocol == KeyPortari::Protocol::AlphaNumeric ? AlphaNumericKeyCodeMappingArray : AsciiKeyCodeMappingArray;
  // iterate through possible keys
  for (auto &mapping : keyCodeMappingArray) {
    if (myEvent.get(mapping.event)) {
      return mapping.code;
    }
  }
  return 0xff;
}

KeyPortari::KeyCodeMappingArray KeyPortari::AlphaNumericKeyCodeMappingArray = {
  {Event::KeyPortariTab,           ' '},
  {Event::KeyPortariExclam,        ' '},
  {Event::KeyPortariDoubleQuote,   ' '},
  {Event::KeyPortariApostrophe,    ' '},
  {Event::KeyPortariHash,          ' '},
  {Event::KeyPortariDollar,        ' '},
  {Event::KeyPortariPercent,       ' '},
  {Event::KeyPortariAmpersand,     ' '},
  {Event::KeyPortariGrave,         ' '},
  {Event::KeyPortariLeftParen,     ' '},
  {Event::KeyPortariRightParen,    ' '},
  {Event::KeyPortariMultiply,      ' '},
  {Event::KeyPortariPlus,          ' '},
  {Event::KeyPortariComma,         ' '},
  {Event::KeyPortariMinus,         ' '},
  {Event::KeyPortariPeriod,        ' '},
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
  {Event::KeyPortariColon,         ' '},
  {Event::KeyPortariSemiColon,     ' '},
  {Event::KeyPortariLessThan,      ' '},
  {Event::KeyPortariEquals,        ' '},
  {Event::KeyPortariGreaterThan,   ' '},
  {Event::KeyPortariQuestion,      ' '},
  {Event::KeyPortariAt,            ' '},
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
  {Event::KeyPortariLeftBracket,   ' '},
  {Event::KeyPortariBackslash,     ' '},
  {Event::KeyPortariRightBracket,  ' '},
  {Event::KeyPortariCaret,         ' '},
  {Event::KeyPortariUnderscore,    ' '},
  {Event::KeyPortariLowercaseA,    'A'},
  {Event::KeyPortariLowercaseB,    'B'},
  {Event::KeyPortariLowercaseC,    'C'},
  {Event::KeyPortariLowercaseD,    'D'},
  {Event::KeyPortariLowercaseE,    'E'},
  {Event::KeyPortariLowercaseF,    'F'},
  {Event::KeyPortariLowercaseG,    'G'},
  {Event::KeyPortariLowercaseH,    'H'},
  {Event::KeyPortariLowercaseI,    'I'},
  {Event::KeyPortariLowercaseJ,    'J'},
  {Event::KeyPortariLowercaseK,    'K'},
  {Event::KeyPortariLowercaseL,    'L'},
  {Event::KeyPortariLowercaseM,    'M'},
  {Event::KeyPortariLowercaseN,    'N'},
  {Event::KeyPortariLowercaseO,    'O'},
  {Event::KeyPortariLowercaseP,    'P'},
  {Event::KeyPortariLowercaseQ,    'Q'},
  {Event::KeyPortariLowercaseR,    'R'},
  {Event::KeyPortariLowercaseS,    'S'},
  {Event::KeyPortariLowercaseT,    'T'},
  {Event::KeyPortariLowercaseU,    'U'},
  {Event::KeyPortariLowercaseV,    'V'},
  {Event::KeyPortariLowercaseW,    'W'},
  {Event::KeyPortariLowercaseX,    'X'},
  {Event::KeyPortariLowercaseY,    'Y'},
  {Event::KeyPortariLowercaseZ,    'Z'},
  {Event::KeyPortariLeftBrace,     ' '},
  {Event::KeyPortariVerticalBar,   ' '},
  {Event::KeyPortariRightBrace,    ' '},
  {Event::KeyPortariTilde,         ' '},
  {Event::KeyPortariComma,         ' '},
  {Event::KeyPortariEnter,         ' '},
  {Event::KeyPortariSpace,         ' '},
  {Event::KeyPortariQuestion,      ' '},
  {Event::KeyPortariSlash,         ' '},
  {Event::KeyPortariDelete,        ' '}
};

KeyPortari::KeyCodeMappingArray KeyPortari::AsciiKeyCodeMappingArray = {
  {Event::KeyPortariTab,          '\t'},
  {Event::KeyPortariExclam,        '!'},
  {Event::KeyPortariDoubleQuote,   '"'},
  {Event::KeyPortariApostrophe,   '\''},
  {Event::KeyPortariHash,          '#'},
  {Event::KeyPortariDollar,        '$'},
  {Event::KeyPortariPercent,       '%'},
  {Event::KeyPortariAmpersand,     '&'},
  {Event::KeyPortariGrave,         '`'},
  {Event::KeyPortariLeftParen,     '('},
  {Event::KeyPortariRightParen,    ')'},
  {Event::KeyPortariMultiply,      '*'},
  {Event::KeyPortariPlus,          '+'},
  {Event::KeyPortariComma,         ','},
  {Event::KeyPortariMinus,         '-'},
  {Event::KeyPortariPeriod,        '.'},
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
  {Event::KeyPortariColon,         ':'},
  {Event::KeyPortariSemiColon,     ';'},
  {Event::KeyPortariLessThan,      '<'},
  {Event::KeyPortariEquals,        '='},
  {Event::KeyPortariGreaterThan,   '>'},
  {Event::KeyPortariQuestion,      '?'},
  {Event::KeyPortariAt,            '@'},
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
  {Event::KeyPortariLeftBracket,   '['},
  {Event::KeyPortariBackslash,     '\\'},
  {Event::KeyPortariRightBracket,  ']'},
  {Event::KeyPortariCaret,         '^'},
  {Event::KeyPortariUnderscore,    '_'},
  {Event::KeyPortariLowercaseA,    'a'},
  {Event::KeyPortariLowercaseB,    'b'},
  {Event::KeyPortariLowercaseC,    'c'},
  {Event::KeyPortariLowercaseD,    'd'},
  {Event::KeyPortariLowercaseE,    'e'},
  {Event::KeyPortariLowercaseF,    'f'},
  {Event::KeyPortariLowercaseG,    'g'},
  {Event::KeyPortariLowercaseH,    'h'},
  {Event::KeyPortariLowercaseI,    'i'},
  {Event::KeyPortariLowercaseJ,    'j'},
  {Event::KeyPortariLowercaseK,    'k'},
  {Event::KeyPortariLowercaseL,    'l'},
  {Event::KeyPortariLowercaseM,    'm'},
  {Event::KeyPortariLowercaseN,    'n'},
  {Event::KeyPortariLowercaseO,    'o'},
  {Event::KeyPortariLowercaseP,    'p'},
  {Event::KeyPortariLowercaseQ,    'q'},
  {Event::KeyPortariLowercaseR,    'r'},
  {Event::KeyPortariLowercaseS,    's'},
  {Event::KeyPortariLowercaseT,    't'},
  {Event::KeyPortariLowercaseU,    'u'},
  {Event::KeyPortariLowercaseV,    'v'},
  {Event::KeyPortariLowercaseW,    'w'},
  {Event::KeyPortariLowercaseX,    'x'},
  {Event::KeyPortariLowercaseY,    'y'},
  {Event::KeyPortariLowercaseZ,    'z'},
  {Event::KeyPortariLeftBrace,     '{'},
  {Event::KeyPortariVerticalBar,   '|'},
  {Event::KeyPortariRightBrace,    '}'},
  {Event::KeyPortariTilde,         '~'},
  {Event::KeyPortariComma,         ','},
  {Event::KeyPortariEnter,         '\n'},
  {Event::KeyPortariSpace,         ' '},
  {Event::KeyPortariQuestion,      '?'},
  {Event::KeyPortariSlash,         '/'},
  {Event::KeyPortariDelete,        127}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortari::KPControl::update()
{
  uInt8 c = myHandler.mapKeyCode();
  
  // passthrough if no key is pressed
  if (c == 0xff && myPassthroughController) {
    // TODO: analog
    myPassthroughController->update();
    c = myPassthroughController->read();
    
  } else if (myJack == Controller::Jack::Left) {
    // shift left controller codes down to match pinout
    c = c >> 4;
    
  }

  setPin(DigitalPin::One, c & 0b0000001);
  setPin(DigitalPin::Two, c & 0b0000010);
  setPin(DigitalPin::Three, c & 0b0000100);
  setPin(DigitalPin::Four, c & 0b0001000);
  
}

KeyPortari::KPControl::KPControl(class KeyPortari& handler, Controller::Jack jack, const Event& event,
                                 const System& system)
: Controller{jack, event, system, Controller::Type::KeyPortari},
  myHandler{handler}
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
