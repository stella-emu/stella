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
// $Id: TIADebug.cxx,v 1.9 2005-07-08 17:22:40 stephena Exp $
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
    myState.ram.push_back(mySystem->peek(i));

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::saveOldState()
{
  myOldState.ram.clear();
  for(int i = 0; i < 0x010; ++i)
    myOldState.ram.push_back(mySystem->peek(i));
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
void TIADebug::updateTIA()
{
	// not working the way I expected:
	// myTIA->updateFrame(myTIA->mySystem->cycles() * 3);
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

  // TIA::myCOLUxx are stored 4 copies to each int, we need only 1
  uInt8 COLUP0 = myTIA->myCOLUP0 & 0xff;
  uInt8 COLUP1 = myTIA->myCOLUP1 & 0xff;
  uInt8 COLUPF = myTIA->myCOLUPF & 0xff;
  uInt8 COLUBK = myTIA->myCOLUBK & 0xff;

  // TIA::myPF holds all 3 PFx regs, we shall extract
  int PF = myTIA->myPF;
  uInt8 PF0 = PF & 0x0f;
  uInt8 PF1 = (PF >>  4) & 0xff;
  uInt8 PF2 = (PF >> 12) & 0xff;

  // Hope Brad never changes this:
  uInt16 coll = myTIA->myCollision;

  // calculate sizes
  uInt8 ballSize = 1 << (myTIA->myCTRLPF & 0x18);
  uInt8 m0Size = 1 <<   (myTIA->myNUSIZ0 & 0x18);
  uInt8 m1Size = 1 <<   (myTIA->myNUSIZ1 & 0x18);

  // easier to use a table for these:
  string p0Size = nusizStrings[myTIA->myNUSIZ0 & 0x07];
  string p1Size = nusizStrings[myTIA->myNUSIZ1 & 0x07];

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
  ret += myDebugger->valueToString(COLUP0);
  ret += "/";
  ret += colorSwatch(COLUP0);

  ret += "COLUP1: ";
  ret += myDebugger->valueToString(COLUP1);
  ret += "/";
  ret += colorSwatch(COLUP1);

  ret += "COLUPF: ";
  ret += myDebugger->valueToString(COLUPF);
  ret += "/";
  ret += colorSwatch(COLUPF);

  ret += "COLUBK: ";
  ret += myDebugger->valueToString(COLUBK);
  ret += "/";
  ret += colorSwatch(COLUBK);

  ret += "\n";

  ret += "P0: GR=";
  // TODO:    ret += myDebugger->invIfChanged(myTIA->myGRP0, oldGRP0);
  ret += Debugger::to_bin_8(myTIA->myGRP0);
  ret += "/";
  ret += myDebugger->valueToString(myTIA->myGRP0);
  ret += " pos=";
  ret += myDebugger->valueToString(myTIA->myPOSP0);
  ret += " HM=";
  ret += myDebugger->valueToString(myTIA->myHMP0);
  ret += " ";
  ret += p0Size;
  ret += " ";
  ret += booleanWithLabel("reflect", (myTIA->myREFP0));
  ret += " ";
  ret += booleanWithLabel("delay", (myTIA->myVDELP0));
  ret += "\n";

  ret += "P1: GR=";
  // TODO:    ret += myDebugger->invIfChanged(myTIA->myGRP1, oldGRP1);
  ret += Debugger::to_bin_8(myTIA->myGRP1);
  ret += "/";
  ret += myDebugger->valueToString(myTIA->myGRP1);
  ret += " pos=";
  ret += myDebugger->valueToString(myTIA->myPOSP1);
  ret += " HM=";
  ret += myDebugger->valueToString(myTIA->myHMP1);
  ret += " ";
  ret += p1Size;
  ret += " ";
  ret += booleanWithLabel("reflect", (myTIA->myREFP1));
  ret += " ";
  ret += booleanWithLabel("delay", (myTIA->myVDELP1));
  ret += "\n";

  ret += "M0: ";
  ret += (myTIA->myENAM0 ? " ENABLED" : "disabled");
  ret += " pos=";
  ret += myDebugger->valueToString(myTIA->myPOSM0);
  ret += " HM=";
  ret += myDebugger->valueToString(myTIA->myHMM0);
  ret += " size=";
  ret += myDebugger->valueToString(m0Size);
  ret += " ";
  ret += booleanWithLabel("reset", (myTIA->myRESMP0));
  ret += "\n";

  ret += "M1: ";
  ret += (myTIA->myENAM1 ? " ENABLED" : "disabled");
  ret += " pos=";
  ret += myDebugger->valueToString(myTIA->myPOSM1);
  ret += " HM=";
  ret += myDebugger->valueToString(myTIA->myHMM1);
  ret += " size=";
  ret += myDebugger->valueToString(m1Size);
  ret += " ";
  ret += booleanWithLabel("reset", (myTIA->myRESMP1));
  ret += "\n";

  ret += "BL: ";
  ret += (myTIA->myENABL ? " ENABLED" : "disabled");
  ret += " pos=";
  ret += myDebugger->valueToString(myTIA->myPOSBL);
  ret += " HM=";
  ret += myDebugger->valueToString(myTIA->myHMBL);
  ret += " size=";
  ret += myDebugger->valueToString(ballSize);
  ret += " ";
  ret += booleanWithLabel("delay", (myTIA->myVDELBL));
  ret += "\n";

  ret += "PF0: ";
  ret += Debugger::to_bin_8(PF0);
  ret += "/";
  ret += myDebugger->valueToString(PF0);
  ret += " PF1: ";
  ret += Debugger::to_bin_8(PF1);
  ret += "/";
  ret += myDebugger->valueToString(PF1);
  ret += " PF2: ";
  ret += Debugger::to_bin_8(PF2);
  ret += "/";
  ret += myDebugger->valueToString(PF2);
  ret += "\n     ";
  ret += booleanWithLabel("reflect",  myTIA->myCTRLPF & 0x01);
  ret += " ";
  ret += booleanWithLabel("score",    myTIA->myCTRLPF & 0x02);
  ret += " ";
  ret += booleanWithLabel("priority", myTIA->myCTRLPF & 0x04);
  ret += "\n";

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
