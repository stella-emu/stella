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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Control.hxx"
#include "StellaKeys.hxx"
#include "CompuMate.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompuMate::CompuMate(const Console& console, const Event& event,
                     const System& system)
  : myConsole(console),
    myColumn(0),
    myKeyTable(event.getKeys())
{
  // These controller pointers will be retrieved by the Console, which will
  // also take ownership of them
  myLeftController  = make_unique<CMControl>(*this, Controller::Jack::Left, event, system);
  myRightController = make_unique<CMControl>(*this, Controller::Jack::Right, event, system);

  myLeftController->setPin(Controller::AnalogPin::Nine, Controller::MAX_RESISTANCE);
  myLeftController->setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
  myRightController->setPin(Controller::AnalogPin::Nine, Controller::MIN_RESISTANCE);
  myRightController->setPin(Controller::AnalogPin::Five, Controller::MAX_RESISTANCE);

  enableKeyHandling(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::enableKeyHandling(bool enable)
{
  myKeyTable.enable(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::update()
{
  // Handle SWCHA changes - the following comes almost directly from z26
  Controller& lp = myConsole.leftController();
  Controller& rp = myConsole.rightController();

  lp.setPin(Controller::AnalogPin::Nine, Controller::MAX_RESISTANCE);
  lp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
  lp.setPin(Controller::DigitalPin::Six, true);
  rp.setPin(Controller::AnalogPin::Nine, Controller::MIN_RESISTANCE);
  rp.setPin(Controller::AnalogPin::Five, Controller::MAX_RESISTANCE);
  rp.setPin(Controller::DigitalPin::Six, true);

  if (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT])
    rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
  if (myKeyTable[KBDK_LCTRL] || myKeyTable[KBDK_RCTRL])
    lp.setPin(Controller::AnalogPin::Nine, Controller::MIN_RESISTANCE);

  rp.setPin(Controller::DigitalPin::Three, true);
  rp.setPin(Controller::DigitalPin::Four, true);

  switch(myColumn)  // This is updated inside CartCM class
  {
    case 0:
      if (myKeyTable[KBDK_7]) lp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_U]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_J]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_M]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 1:
      if (myKeyTable[KBDK_6]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the '?' character (Shift-6) with the actual question key
      if (myKeyTable[KBDK_SLASH] && (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_Y]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_H]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_N]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 2:
      if (myKeyTable[KBDK_8]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the '[' character (Shift-8) with the actual key
      if (myKeyTable[KBDK_LEFTBRACKET] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_I]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_K]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_COMMA]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 3:
      if (myKeyTable[KBDK_2]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the '-' character (Shift-2) with the actual minus key
      if (myKeyTable[KBDK_MINUS] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_W]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_S]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_X]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 4:
      if (myKeyTable[KBDK_3]) lp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_E]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_D]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_C]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 5:
      if (myKeyTable[KBDK_0]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the quote character (Shift-0) with the actual quote key
      if (myKeyTable[KBDK_APOSTROPHE] && (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_P]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_RETURN] || myKeyTable[KBDK_KP_ENTER])
        rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_SPACE]) rp.setPin(Controller::DigitalPin::Four, false);
      // Emulate Ctrl-space (aka backspace) with the actual Backspace key
      if (myKeyTable[KBDK_BACKSPACE])
      {
        lp.setPin(Controller::AnalogPin::Nine, Controller::MIN_RESISTANCE);
        rp.setPin(Controller::DigitalPin::Four, false);
      }
      break;
    case 6:
      if (myKeyTable[KBDK_9]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the ']' character (Shift-9) with the actual key
      if (myKeyTable[KBDK_RIGHTBRACKET] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_O]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_L]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_PERIOD]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 7:
      if (myKeyTable[KBDK_5]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the '=' character (Shift-5) with the actual equals key
      if (myKeyTable[KBDK_EQUALS] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_T]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_G]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_B]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 8:
      if (myKeyTable[KBDK_1]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the '+' character (Shift-1) with the actual plus key (Shift-=)
      if (myKeyTable[KBDK_EQUALS] && (myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_Q]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_A]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_Z]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    case 9:
      if (myKeyTable[KBDK_4]) lp.setPin(Controller::DigitalPin::Six, false);
      // Emulate the '/' character (Shift-4) with the actual slash key
      if (myKeyTable[KBDK_SLASH] && !(myKeyTable[KBDK_LSHIFT] || myKeyTable[KBDK_RSHIFT]))
      {
        rp.setPin(Controller::AnalogPin::Five, Controller::MIN_RESISTANCE);
        lp.setPin(Controller::DigitalPin::Six, false);
      }
      if (myKeyTable[KBDK_R]) rp.setPin(Controller::DigitalPin::Three, false);
      if (myKeyTable[KBDK_F]) rp.setPin(Controller::DigitalPin::Six, false);
      if (myKeyTable[KBDK_V]) rp.setPin(Controller::DigitalPin::Four, false);
      break;
    default:
      break;
  }
}
