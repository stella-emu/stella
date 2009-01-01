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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RiotDebug.cxx,v 1.9 2009-01-01 18:13:35 stephena Exp $
//============================================================================

#include <sstream>

#include "System.hxx"
#include "Debugger.hxx"
#include "Switches.hxx"

#include "RiotDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotDebug::RiotDebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& RiotDebug::getState()
{
  // Port A & B registers
  myState.SWCHA_R = swcha();
  myState.SWCHA_W = mySystem.m6532().myOutA;
  myState.SWACNT  = swacnt();
  myState.SWCHB   = swchb();
  Debugger::set_bits(myState.SWCHA_R, myState.swchaReadBits);
  Debugger::set_bits(myState.SWCHA_W, myState.swchaWriteBits);
  Debugger::set_bits(myState.SWACNT, myState.swacntBits);
  Debugger::set_bits(myState.SWCHB, myState.swchbBits);

  // Timer registers
  myState.TIM1T    = tim1T();
  myState.TIM8T    = tim8T();
  myState.TIM64T   = tim64T();
  myState.TIM1024T = tim1024T();
  myState.INTIM    = intim();
  myState.TIMINT   = timint();
  myState.TIMCLKS  = timClocks();

  // Controller port pins
  const Controller& port0 = myConsole.controller(Controller::Left);
  myState.P0_PIN1 = port0.myDigitalPinState[Controller::One];
  myState.P0_PIN2 = port0.myDigitalPinState[Controller::Two];
  myState.P0_PIN3 = port0.myDigitalPinState[Controller::Three];
  myState.P0_PIN4 = port0.myDigitalPinState[Controller::Four];
  myState.P0_PIN6 = port0.myDigitalPinState[Controller::Six];
  const Controller& port1 = myConsole.controller(Controller::Right);
  myState.P1_PIN1 = port1.myDigitalPinState[Controller::One];
  myState.P1_PIN2 = port1.myDigitalPinState[Controller::Two];
  myState.P1_PIN3 = port1.myDigitalPinState[Controller::Three];
  myState.P1_PIN4 = port1.myDigitalPinState[Controller::Four];
  myState.P1_PIN6 = port1.myDigitalPinState[Controller::Six];

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotDebug::saveOldState()
{
  // Port A & B registers
  myOldState.SWCHA_R = swcha();
  myOldState.SWCHA_W = mySystem.m6532().myOutA;
  myOldState.SWACNT  = swacnt();
  myOldState.SWCHB   = swchb();
  Debugger::set_bits(myOldState.SWCHA_R, myOldState.swchaReadBits);
  Debugger::set_bits(myOldState.SWCHA_W, myOldState.swchaWriteBits);
  Debugger::set_bits(myOldState.SWACNT, myOldState.swacntBits);
  Debugger::set_bits(myOldState.SWCHB, myOldState.swchbBits);

  // Timer registers
  myOldState.TIM1T    = tim1T();
  myOldState.TIM8T    = tim8T();
  myOldState.TIM64T   = tim64T();
  myOldState.TIM1024T = tim1024T();
  myOldState.INTIM    = intim();
  myOldState.TIMINT   = timint();
  myOldState.TIMCLKS  = timClocks();

  // Controller port pins
  const Controller& port0 = myConsole.controller(Controller::Left);
  myOldState.P0_PIN1 = port0.myDigitalPinState[Controller::One];
  myOldState.P0_PIN2 = port0.myDigitalPinState[Controller::Two];
  myOldState.P0_PIN3 = port0.myDigitalPinState[Controller::Three];
  myOldState.P0_PIN4 = port0.myDigitalPinState[Controller::Four];
  myOldState.P0_PIN6 = port0.myDigitalPinState[Controller::Six];
  const Controller& port1 = myConsole.controller(Controller::Right);
  myOldState.P1_PIN1 = port1.myDigitalPinState[Controller::One];
  myOldState.P1_PIN2 = port1.myDigitalPinState[Controller::Two];
  myOldState.P1_PIN3 = port1.myDigitalPinState[Controller::Three];
  myOldState.P1_PIN4 = port1.myDigitalPinState[Controller::Four];
  myOldState.P1_PIN6 = port1.myDigitalPinState[Controller::Six];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swcha(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x280, newVal);

  return mySystem.peek(0x280);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swchb(int newVal)
{
  return mySystem.peek(0x282);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swacnt(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x281, newVal);

  return mySystem.peek(0x281);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim1T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x294, newVal);

  return mySystem.m6532().myOutTimer[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim8T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x295, newVal);

  return mySystem.m6532().myOutTimer[1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim64T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x296, newVal);

  return mySystem.m6532().myOutTimer[2];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim1024T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x297, newVal);

  return mySystem.m6532().myOutTimer[3];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::intim()
{
  return mySystem.peek(0x284);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::timint()
{
  return mySystem.peek(0x285);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 RiotDebug::timClocks()
{
  return mySystem.m6532().timerClocks();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotDebug::setP0Pins(bool Pin1, bool Pin2, bool Pin3, bool Pin4, bool Pin6)
{
  Controller& port0 = myConsole.controller(Controller::Left);
  port0.myDigitalPinState[Controller::One]   = Pin1;
  port0.myDigitalPinState[Controller::Two]   = Pin2;
  port0.myDigitalPinState[Controller::Three] = Pin3;
  port0.myDigitalPinState[Controller::Four]  = Pin4;
  port0.myDigitalPinState[Controller::Six]   = Pin6;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotDebug::setP1Pins(bool Pin1, bool Pin2, bool Pin3, bool Pin4, bool Pin6)
{
  Controller& port1 = myConsole.controller(Controller::Right);
  port1.myDigitalPinState[Controller::One]   = Pin1;
  port1.myDigitalPinState[Controller::Two]   = Pin2;
  port1.myDigitalPinState[Controller::Three] = Pin3;
  port1.myDigitalPinState[Controller::Four]  = Pin4;
  port1.myDigitalPinState[Controller::Six]   = Pin6;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::diffP0(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 6, newVal > 0);

  return switches & 0x40;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::diffP1(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 7, newVal > 0);

  return switches & 0x80;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::tvType(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 3, newVal > 0);

  return switches & 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::select(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 1, newVal > 0);

  return switches & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::reset(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 0, newVal > 0);

  return switches & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::dirP0String()
{
  uInt8 reg = swcha();
  ostringstream buf;
  buf << (reg & 0x80 ? "" : "right ")
      << (reg & 0x40 ? "" : "left ")
      << (reg & 0x20 ? "" : "left ")
      << (reg & 0x10 ? "" : "left ")
      << ((reg & 0xf0) == 0xf0 ? "(no directions) " : "");
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::dirP1String()
{
  uInt8 reg = swcha();
  ostringstream buf;
  buf << (reg & 0x08 ? "" : "right ")
      << (reg & 0x04 ? "" : "left ")
      << (reg & 0x02 ? "" : "left ")
      << (reg & 0x01 ? "" : "left ")
      << ((reg & 0x0f) == 0x0f ? "(no directions) " : "");
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::diffP0String()
{
  return (swchb() & 0x40) ? "hard/A" : "easy/B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::diffP1String()
{
  return (swchb() & 0x80) ? "hard/A" : "easy/B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::tvTypeString()
{
  return (swchb() & 0x8) ? "Color" : "B&W";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::switchesString()
{
  ostringstream buf;
  buf << (swchb() & 0x2 ? "-" : "+") << "select "
      << (swchb() & 0x1 ? "-" : "+") << "reset";
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::toString()
{
  // TODO: keyboard controllers?

  const RiotState& state    = (RiotState&) getState();
  const RiotState& oldstate = (RiotState&) getOldState();
  string ret;

  ret += myDebugger.valueToString(0x280) + "/SWCHA(R)=" +
         myDebugger.invIfChanged(state.SWCHA_R, oldstate.SWCHA_R) + " ";
  ret += myDebugger.valueToString(0x280) + "/SWCHA(W)=" +
         myDebugger.invIfChanged(state.SWCHA_W, oldstate.SWCHA_W) + " ";
  ret += myDebugger.valueToString(0x281) + "/SWACNT=" +
         myDebugger.invIfChanged(state.SWACNT, oldstate.SWACNT) + " ";
  ret += myDebugger.valueToString(0x282) + "/SWCHB=" +
         myDebugger.invIfChanged(state.SWCHB, oldstate.SWCHB) + " ";
  ret += "\n";

  // These are squirrely: some symbol files will define these as
  // 0x284-0x287. Doesn't actually matter, these registers repeat
  // every 16 bytes.
  ret += myDebugger.valueToString(0x294) + "/TIM1T=" +
         myDebugger.invIfChanged(state.TIM1T, oldstate.TIM1T) + " ";
  ret += myDebugger.valueToString(0x295) + "/TIM8T=" +
         myDebugger.invIfChanged(state.TIM8T, oldstate.TIM8T) + " ";
  ret += myDebugger.valueToString(0x296) + "/TIM64T=" +
         myDebugger.invIfChanged(state.TIM64T, oldstate.TIM64T) + " ";
  ret += myDebugger.valueToString(0x297) + "/TIM1024T=" +
         myDebugger.invIfChanged(state.TIM1024T, oldstate.TIM1024T) + " ";
  ret += "\n";

  ret += myDebugger.valueToString(0x284) + "/INTIM=" +
         myDebugger.invIfChanged(state.INTIM, oldstate.INTIM) + " ";
  ret += myDebugger.valueToString(0x285) + "/TIMINT=" +
         myDebugger.invIfChanged(state.TIMINT, oldstate.TIMINT) + " ";
  ret += "Timer_Clocks=" +
         myDebugger.invIfChanged(state.TIMCLKS, oldstate.TIMCLKS) + " ";
  ret += "\n";

  ret += "Left/P0diff: " + diffP0String() + "   Right/P1diff: " + diffP0String();
  ret += "\n";

  ret += "TVType: " + tvTypeString() + "   Switches: " + switchesString();
  ret += "\n";

  // Yes, the fire buttons are in the TIA, but we might as well
  // show them here for convenience.
  ret += "Left/P0 stick:  " + dirP0String();
  ret += (mySystem.peek(0x03c) & 0x80) ? "" : "(button) ";
  ret += "\n";
  ret += "Right/P1 stick: " + dirP1String();
  ret += (mySystem.peek(0x03d) & 0x80) ? "" : "(button) ";

  return ret;
}
