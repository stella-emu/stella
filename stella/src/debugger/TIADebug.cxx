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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIADebug.cxx,v 1.19 2005-08-16 19:04:44 urchlay Exp $
//============================================================================

#include "System.hxx"
#include "Debugger.hxx"

#include "TIADebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIADebug::TIADebug(Debugger* dbg, Console* console)
  : DebuggerSystem(dbg, console),
    mySystem(&console->system()),
    myTIA((TIA*)&console->mediaSource())
{
  nusizStrings[0] = "size=8 copy=1";
  nusizStrings[1] = "size=8 copy=2 spac=8";
  nusizStrings[2] = "size=8 copy=2 spac=$18";
  nusizStrings[3] = "size=8 copy=3 spac=8";
  nusizStrings[4] = "size=8 copy=2 spac=$38";
  nusizStrings[5] = "size=$10 copy=1";
  nusizStrings[6] = "size=8 copy=3 spac=$18";
  nusizStrings[7] = "size=$20 copy=1";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerState& TIADebug::getState()
{
  myState.ram.clear();
  for(int i = 0; i < 0x010; ++i)
    myState.ram.push_back(myTIA->peek(i));

  // Color registers
  myState.coluRegs.clear();
  myState.coluRegs.push_back(coluP0());
  myState.coluRegs.push_back(coluP1());
  myState.coluRegs.push_back(coluPF());
  myState.coluRegs.push_back(coluBK());

  // Player 1 & 2 graphics registers
  myState.gr.clear();
  myState.gr.push_back(grP0());
  myState.gr.push_back(grP1());

  // Position registers
  myState.pos.clear();
  myState.pos.push_back(posP0());
  myState.pos.push_back(posP1());
  myState.pos.push_back(posM0());
  myState.pos.push_back(posM1());
  myState.pos.push_back(posBL());

  // Horizontal move registers
  myState.hm.clear();
  myState.hm.push_back(hmP0());
  myState.hm.push_back(hmP1());
  myState.hm.push_back(hmM0());
  myState.hm.push_back(hmM1());
  myState.hm.push_back(hmBL());

  // Playfield registers
  myState.pf.clear();
  myState.pf.push_back(pf0());
  myState.pf.push_back(pf1());
  myState.pf.push_back(pf2());

  // Reflect and delay registers
  myState.refP0 = refP0();  myState.vdelP0 = vdelP0();
  myState.refP1 = refP1();  myState.vdelP1 = vdelP1();

  // NUSIZ registers
  myState.nusiz.push_back(nusiz0());
  myState.nusiz.push_back(nusiz1());

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::saveOldState()
{
  myOldState.ram.clear();
  for(int i = 0; i < 0x010; ++i)
    myOldState.ram.push_back(myTIA->peek(i));

  // Color registers
  myOldState.coluRegs.clear();
  myOldState.coluRegs.push_back(coluP0());
  myOldState.coluRegs.push_back(coluP1());
  myOldState.coluRegs.push_back(coluPF());
  myOldState.coluRegs.push_back(coluBK());

  // Player 1 & 2 graphics registers
  myOldState.gr.clear();
  myOldState.gr.push_back(grP0());
  myOldState.gr.push_back(grP1());

  // Position registers
  myOldState.pos.clear();
  myOldState.pos.push_back(posP0());
  myOldState.pos.push_back(posP1());
  myOldState.pos.push_back(posM0());
  myOldState.pos.push_back(posM1());
  myOldState.pos.push_back(posBL());

  // Horizontal move registers
  myOldState.hm.clear();
  myOldState.hm.push_back(hmP0());
  myOldState.hm.push_back(hmP1());
  myOldState.hm.push_back(hmM0());
  myOldState.hm.push_back(hmM1());
  myOldState.hm.push_back(hmBL());

  // Playfield registers
  myOldState.pf.clear();
  myOldState.pf.push_back(pf0());
  myOldState.pf.push_back(pf1());
  myOldState.pf.push_back(pf2());

  // Reflect and delay registers
  myOldState.refP0 = refP0();  myOldState.vdelP0 = vdelP0();
  myOldState.refP1 = refP1();  myOldState.vdelP1 = vdelP1();

  // NUSIZ registers
  myOldState.nusiz.push_back(nusiz0());
  myOldState.nusiz.push_back(nusiz1());

}

/* the set methods now use mySystem->poke(). This will save us the
	trouble of masking the values here, since TIA::poke() will do it
	for us.

	This means that the GUI should *never* just display the value the
	user entered: it should always read the return value of the set
	method and display that.

	An Example:

	User enters "ff" in the AUDV0 field. GUI calls value = tiaDebug->audV0(0xff).
	The AUDV0 register is only 4 bits wide, so "value" is 0x0f. That's what
	should be displayed.

	In a perfect world, the GUI would only allow one hex digit to be entered...
	but we allow decimal or binary input in the GUI (with # or \ prefix). The
	only way to make that work would be to validate the data entry after every
	keystroke... which would be a pain for both us and the user. Using poke()
	here is a compromise that allows the TIA to do the range-checking for us,
	so the GUI and/or TIADebug don't have to duplicate logic from TIA::poke().
*/

//	 bool vdelP0(int newVal = -1);
//	 bool vdelP1(int newVal = -1);
//	 bool vdelBL(int newVal = -1);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelP0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(VDELP0, ((bool)newVal));

  return myTIA->myVDELP0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelP1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(VDELP1, ((bool)newVal));

  return myTIA->myVDELP1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelBL(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(VDELBL, ((bool)newVal));

  return myTIA->myVDELBL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaM0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(ENAM0, ((bool)newVal) << 1);

  return myTIA->myENAM0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaM1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(ENAM1, ((bool)newVal) << 1);

  return myTIA->myENAM1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaBL(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(ENABL, ((bool)newVal) << 1);

  return myTIA->myENABL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::resMP0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(RESMP0, ((bool)newVal) << 1);

  return myTIA->myRESMP0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::resMP1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(RESMP1, ((bool)newVal) << 1);

  return myTIA->myRESMP1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refP0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(REFP0, ((bool)newVal) << 3);

  return myTIA->myREFP0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refP1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(REFP1, ((bool)newVal) << 3);

  return myTIA->myREFP1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audC0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(AUDC0, newVal);

  return myTIA->myAUDC0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audC1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(AUDC1, newVal);

  return myTIA->myAUDC1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audV0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(AUDV0, newVal);

  return myTIA->myAUDV0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audV1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(AUDV1, newVal);

  return myTIA->myAUDV1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audF0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(AUDF0, newVal);

  return myTIA->myAUDF0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audF1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(AUDF1, newVal);

  return myTIA->myAUDF1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(PF0, newVal << 4);

  return myTIA->myPF & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(PF1, newVal);

  return (myTIA->myPF & 0xff0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf2(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(PF2, newVal);

  return (myTIA->myPF & 0xff000) >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluP0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(COLUP0, newVal);

  return myTIA->myCOLUP0 & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluP1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(COLUP0, newVal);

  return myTIA->myCOLUP1 & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluPF(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(COLUPF, newVal);

  return myTIA->myCOLUPF & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluBK(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(COLUBK, newVal);

  return myTIA->myCOLUBK & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusiz0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(NUSIZ0, newVal);

  return myTIA->myNUSIZ0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusiz1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(NUSIZ1, newVal);

  return myTIA->myNUSIZ1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::grP0(int newVal)
{
  if(newVal > -1)
    mySystem->poke(GRP0, newVal);

  return myTIA->myGRP0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::grP1(int newVal)
{
  if(newVal > -1)
    mySystem->poke(GRP1, newVal);

  return myTIA->myGRP1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posP0(int newVal)
{
  if(newVal > -1)
    myTIA->myPOSP0 = newVal;
  return myTIA->myPOSP0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posP1(int newVal)
{
  if(newVal > -1)
    myTIA->myPOSP1 = newVal;
  return myTIA->myPOSP1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posM0(int newVal)
{
/* FIXME
  if(newVal > -1)
    mySystem->poke(???, newVal);
*/
  return myTIA->myPOSM0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posM1(int newVal)
{
/* FIXME
  if(newVal > -1)
    mySystem->poke(???, newVal);
*/
  return myTIA->myPOSM1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posBL(int newVal)
{
/* FIXME
  if(newVal > -1)
    mySystem->poke(???, newVal);
*/
  return myTIA->myPOSBL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::ctrlPF(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(CTRLPF, newVal);

  return myTIA->myCTRLPF;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmP0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(HMP0, newVal << 4);

  return myTIA->myHMP0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmP1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(HMP1, newVal << 4);

  return myTIA->myHMP1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmM0(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(HMM0, newVal << 4);

  return myTIA->myHMM0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmM1(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(HMM1, newVal << 4);

  return myTIA->myHMM1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmBL(int newVal)
{
  if(newVal > -1)
	  mySystem->poke(HMBL, newVal << 4);

  return myTIA->myHMBL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::frameCount()
{
  return myTIA->myFrameCounter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::scanlines()
{
  return myTIA->scanlines();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::clocksThisLine()
{
  return myTIA->clocksThisLine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vsync()
{
  return (myTIA->myVSYNC & 2) == 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vblank()
{
  return (myTIA->myVBLANK & 2) == 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::colorSwatch(uInt8 c)
{
  string ret;

  ret += char((c >> 1) | 0x80);
  ret += "\177     ";
  ret += "\177\003 ";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string audFreq(uInt8 div)
{
  string ret;
  char buf[20];

  double hz = 30000.0;
  if(div) hz /= div;
  sprintf(buf, "%5.1f", hz);
  ret += buf;
  ret += "Hz";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string booleanWithLabel(string label, bool value)
{
  char buf[64];
  string ret;

  if(value)
  {
    char *p = buf;
    const char *q = label.c_str();
    while((*p++ = toupper(*q++)))
      ;
    ret += "+";
    ret += buf;
    return ret;
  }
  else
    return "-" + label;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::state()
{
  string ret;

  // TODO: inverse video for changed regs. Core needs to track this.
  // TODO: strobes? WSYNC RSYNC RESP0/1 RESM0/1 RESBL HMOVE HMCLR CXCLR

  TiaState state    = (TiaState&) getState();
// FIXME - change tracking  TiaState oldstate = (TiaState&) getOldState();

  // calculate sizes
  uInt8 ballSize = 1 << (myTIA->myCTRLPF & 0x18);
  uInt8 m0Size   = 1 << (myTIA->myNUSIZ0 & 0x18);
  uInt8 m1Size   = 1 << (myTIA->myNUSIZ1 & 0x18);

  // easier to use a table for these:
  const string& p0Size = nusiz0String();
  const string& p1Size = nusiz1String();

  // build up output, then return it.
  ret += "scanline ";

  ret += myDebugger->valueToString(myTIA->scanlines());
  ret += " ";

  ret += booleanWithLabel("vsync", vsync());
  ret += " ";

  ret += booleanWithLabel("vblank", vblank());
  ret += "\n";

  ret += booleanWithLabel("inpt0", myTIA->peek(0x08) & 0x80);
  ret += " ";
  ret += booleanWithLabel("inpt1", myTIA->peek(0x09) & 0x80);
  ret += " ";
  ret += booleanWithLabel("inpt2", myTIA->peek(0x0a) & 0x80);
  ret += " ";
  ret += booleanWithLabel("inpt3", myTIA->peek(0x0b) & 0x80);
  ret += " ";
  ret += booleanWithLabel("inpt4", myTIA->peek(0x0c) & 0x80);
  ret += " ";
  ret += booleanWithLabel("inpt5", myTIA->peek(0x0d) & 0x80);
  ret += " ";
  ret += booleanWithLabel("dump_gnd_0123", myTIA->myDumpEnabled);
  ret += "\n";

  ret += "COLUP0: ";
  ret += myDebugger->valueToString(state.coluRegs[0]);
  ret += "/";
  ret += colorSwatch(state.coluRegs[0]);

  ret += "COLUP1: ";
  ret += myDebugger->valueToString(state.coluRegs[1]);
  ret += "/";
  ret += colorSwatch(state.coluRegs[1]);

  ret += "COLUPF: ";
  ret += myDebugger->valueToString(state.coluRegs[2]);
  ret += "/";
  ret += colorSwatch(state.coluRegs[2]);

  ret += "COLUBK: ";
  ret += myDebugger->valueToString(state.coluRegs[3]);
  ret += "/";
  ret += colorSwatch(state.coluRegs[3]);

  ret += "\n";

  ret += "P0: GR=";
  ret += Debugger::to_bin_8(state.gr[P0]);
  ret += "/";
  ret += myDebugger->valueToString(state.gr[P0]);
  ret += " pos=";
  ret += myDebugger->valueToString(state.pos[P0]);
  ret += " HM=";
  ret += myDebugger->valueToString(state.hm[P0]);
  ret += " ";
  ret += p0Size;
  ret += " ";
  ret += booleanWithLabel("reflect", state.refP0);
  ret += " ";
  ret += booleanWithLabel("delay", state.vdelP0);
  ret += "\n";

  ret += "P1: GR=";
  ret += Debugger::to_bin_8(state.gr[P1]);
  ret += "/";
  ret += myDebugger->valueToString(state.gr[P1]);
  ret += " pos=";
  ret += myDebugger->valueToString(state.pos[P1]);
  ret += " HM=";
  ret += myDebugger->valueToString(state.hm[P1]);
  ret += " ";
  ret += p1Size;
  ret += " ";
  ret += booleanWithLabel("reflect", state.refP1);
  ret += " ";
  ret += booleanWithLabel("delay", state.vdelP1);
  ret += "\n";

  ret += "M0: ";
  ret += (myTIA->myENAM0 ? " ENABLED" : "disabled");
  ret += " pos=";
  ret += myDebugger->valueToString(state.pos[M0]);
  ret += " HM=";
  ret += myDebugger->valueToString(state.hm[M0]);
  ret += " size=";
  ret += myDebugger->valueToString(m0Size);
  ret += " ";
  ret += booleanWithLabel("reset", (myTIA->myRESMP0));
  ret += "\n";

  ret += "M1: ";
  ret += (myTIA->myENAM1 ? " ENABLED" : "disabled");
  ret += " pos=";
  ret += myDebugger->valueToString(state.pos[M1]);
  ret += " HM=";
  ret += myDebugger->valueToString(state.hm[M1]);
  ret += " size=";
  ret += myDebugger->valueToString(m1Size);
  ret += " ";
  ret += booleanWithLabel("reset", (myTIA->myRESMP1));
  ret += "\n";

  ret += "BL: ";
  ret += (myTIA->myENABL ? " ENABLED" : "disabled");
  ret += " pos=";
  ret += myDebugger->valueToString(state.pos[BL]);
  ret += " HM=";
  ret += myDebugger->valueToString(state.hm[BL]);
  ret += " size=";
  ret += myDebugger->valueToString(ballSize);
  ret += " ";
  ret += booleanWithLabel("delay", (myTIA->myVDELBL));
  ret += "\n";

  ret += "PF0: ";
  ret += Debugger::to_bin_8(state.pf[0]);
  ret += "/";
  ret += myDebugger->valueToString(state.pf[0]);
  ret += " PF1: ";
  ret += Debugger::to_bin_8(state.pf[1]);
  ret += "/";
  ret += myDebugger->valueToString(state.pf[1]);
  ret += " PF2: ";
  ret += Debugger::to_bin_8(state.pf[2]);
  ret += "/";
  ret += myDebugger->valueToString(state.pf[2]);
  ret += "\n     ";
  ret += booleanWithLabel("reflect",  myTIA->myCTRLPF & 0x01);
  ret += " ";
  ret += booleanWithLabel("score",    myTIA->myCTRLPF & 0x02);
  ret += " ";
  ret += booleanWithLabel("priority", myTIA->myCTRLPF & 0x04);
  ret += "\n";

  // Hope Brad never changes this:
  uInt16 coll = myTIA->myCollision;

  ret += "Collisions: ";
  ret += booleanWithLabel("m0_p1 ", bool(coll & 0x0001));
  ret += booleanWithLabel("m0_p0 ", bool(coll & 0x0002));
  ret += booleanWithLabel("m1_p0 ", bool(coll & 0x0004));
  ret += booleanWithLabel("m1_p1 ", bool(coll & 0x0008));
  ret += booleanWithLabel("p0_pf ", bool(coll & 0x0010));
  ret += booleanWithLabel("p0_bl ", bool(coll & 0x0020));
  ret += booleanWithLabel("p1_pf ", bool(coll & 0x0040));
  ret += "\n            ";
  ret += booleanWithLabel("p1_bl ", bool(coll & 0x0080));
  ret += booleanWithLabel("m0_pf ", bool(coll & 0x0100));
  ret += booleanWithLabel("m0_bl ", bool(coll & 0x0200));
  ret += booleanWithLabel("m1_pf ", bool(coll & 0x0400));
  ret += booleanWithLabel("m1_bl ", bool(coll & 0x0800));
  ret += booleanWithLabel("bl_pf ", bool(coll & 0x1000));
  ret += booleanWithLabel("p0_p1 ", bool(coll & 0x2000));
  ret += booleanWithLabel("m0_m1 ", bool(coll & 0x4000));
  ret += "\n";

  ret += "AUDF0: ";
  ret += myDebugger->valueToString(myTIA->myAUDF0);
  ret += "/";
  ret += audFreq(myTIA->myAUDF0);
  ret += " ";

  ret += "AUDC0: ";
  ret += myDebugger->valueToString(myTIA->myAUDC0);
  ret += " ";

  ret += "AUDV0: ";
  ret += myDebugger->valueToString(myTIA->myAUDV0);
  ret += "\n";

  ret += "AUDF1: ";
  ret += myDebugger->valueToString(myTIA->myAUDF1);
  ret += "/";
  ret += audFreq(myTIA->myAUDF1);
  ret += " ";

  ret += "AUDC0: ";
  ret += myDebugger->valueToString(myTIA->myAUDC1);
  ret += " ";

  ret += "AUDV0: ";
  ret += myDebugger->valueToString(myTIA->myAUDV1);
  //ret += "\n";

  // note: last "ret +=" line should not contain \n, caller will add.

  return ret;
}
