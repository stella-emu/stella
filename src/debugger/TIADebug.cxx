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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Base.hxx"
#include "System.hxx"
#include "Debugger.hxx"
#include "TIA.hxx"

#include "TIADebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIADebug::TIADebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console),
    myTIA(console.tia())
{
  nusizStrings[0] = "1 copy";
  nusizStrings[1] = "2 copies - close (8)";
  nusizStrings[2] = "2 copies - med (24)";
  nusizStrings[3] = "3 copies - close (8)";
  nusizStrings[4] = "2 copies - wide (56)";
  nusizStrings[5] = "2x (16) sized player";
  nusizStrings[6] = "3 copies - med (24)";
  nusizStrings[7] = "4x (32) sized player";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& TIADebug::getState()
{
  myState.ram.clear();
  for(int i = 0; i < 0x010; ++i)
    myState.ram.push_back(myTIA.peek(i));

  // Color registers
  myState.coluRegs.clear();
  myState.coluRegs.push_back(coluP0());
  myState.coluRegs.push_back(coluP1());
  myState.coluRegs.push_back(coluPF());
  myState.coluRegs.push_back(coluBK());

  // Debug Colors
  int mode = myTIA.myFrameManager.layout() == FrameLayout::ntsc ? 0 : 1;
  myState.fixedCols.clear();
  myState.fixedCols.push_back(myTIA.myFixedColorPalette[mode][TIA::P0]);
  myState.fixedCols.push_back(myTIA.myFixedColorPalette[mode][TIA::P1]);
  myState.fixedCols.push_back(myTIA.myFixedColorPalette[mode][TIA::PF]);
  myState.fixedCols.push_back(TIA::FixedColor::BK_GREY);
  myState.fixedCols.push_back(myTIA.myFixedColorPalette[mode][TIA::M0]);
  myState.fixedCols.push_back(myTIA.myFixedColorPalette[mode][TIA::M1]);
  myState.fixedCols.push_back(myTIA.myFixedColorPalette[mode][TIA::BL]);
  myState.fixedCols.push_back(TIA::FixedColor::HBLANK_WHITE);

  // Player 0 & 1 and Ball graphics registers
  myState.gr.clear();
  myState.gr.push_back(myTIA.myPlayer0.getGRPNew());
  myState.gr.push_back(myTIA.myPlayer1.getGRPNew());
  myState.gr.push_back(myTIA.myPlayer0.getGRPOld());
  myState.gr.push_back(myTIA.myPlayer1.getGRPOld());
  myState.gr.push_back(myTIA.myBall.getENABLNew());
  myState.gr.push_back(myTIA.myBall.getENABLOld());

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

  // Size registers
  myState.size.clear();
  myState.size.push_back(nusizP0());
  myState.size.push_back(nusizP1());
  myState.size.push_back(nusizM0());
  myState.size.push_back(nusizM1());
  myState.size.push_back(sizeBL());

  // Audio registers
  myState.aud.clear();
  myState.aud.push_back(audF0());
  myState.aud.push_back(audF1());
  myState.aud.push_back(audC0());
  myState.aud.push_back(audC1());
  myState.aud.push_back(audV0());
  myState.aud.push_back(audV1());

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::saveOldState()
{
  myOldState.ram.clear();
  for(int i = 0; i < 0x010; ++i)
    myOldState.ram.push_back(myTIA.peek(i));

  // Color registers
  myOldState.coluRegs.clear();
  myOldState.coluRegs.push_back(coluP0());
  myOldState.coluRegs.push_back(coluP1());
  myOldState.coluRegs.push_back(coluPF());
  myOldState.coluRegs.push_back(coluBK());

  // Player 0 & 1 graphics registers
  myOldState.gr.clear();
  myOldState.gr.push_back(myTIA.myPlayer0.getGRPNew());
  myOldState.gr.push_back(myTIA.myPlayer1.getGRPNew());
  myOldState.gr.push_back(myTIA.myPlayer0.getGRPOld());
  myOldState.gr.push_back(myTIA.myPlayer1.getGRPOld());
  myOldState.gr.push_back(myTIA.myBall.getENABLNew());
  myOldState.gr.push_back(myTIA.myBall.getENABLOld());

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

  // Size registers
  myOldState.size.clear();
  myOldState.size.push_back(nusizP0());
  myOldState.size.push_back(nusizP1());
  myOldState.size.push_back(nusizM0());
  myOldState.size.push_back(nusizM1());
  myOldState.size.push_back(sizeBL());

  // Audio registers
  myOldState.aud.clear();
  myOldState.aud.push_back(audF0());
  myOldState.aud.push_back(audF1());
  myOldState.aud.push_back(audC0());
  myOldState.aud.push_back(audC1());
  myOldState.aud.push_back(audV0());
  myOldState.aud.push_back(audV1());
}

/* the set methods now use mySystem.poke(). This will save us the
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelP0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(VDELP0, bool(newVal));

  return myTIA.registerValue(VDELP0) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelP1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(VDELP1, bool(newVal));

  return myTIA.registerValue(VDELP1) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelBL(int newVal)
{
  if(newVal > -1)
    mySystem.poke(VDELBL, bool(newVal));

  return myTIA.registerValue(VDELBL) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaM0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(ENAM0, bool(newVal) << 1);

  return myTIA.registerValue(ENAM0) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaM1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(ENAM1, bool(newVal) << 1);

  return myTIA.registerValue(ENAM1) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaBL(int newVal)
{
  if(newVal > -1)
    mySystem.poke(ENABL, bool(newVal) << 1);

  return myTIA.registerValue(ENABL) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::resMP0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(RESMP0, bool(newVal) << 1);

  return myTIA.registerValue(RESMP0) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::resMP1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(RESMP1, bool(newVal) << 1);

  return myTIA.registerValue(RESMP1) & 0x02;;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refP0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(REFP0, bool(newVal) << 3);

  return myTIA.registerValue(REFP0) & 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refP1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(REFP1, bool(newVal) << 3);

  return myTIA.registerValue(REFP1) & 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refPF(int newVal)
{
  if(newVal > -1)
  {
    int tmp = myTIA.registerValue(CTRLPF);
    if(newVal)
      tmp |= 0x01;
    else
      tmp &= ~0x01;
    mySystem.poke(CTRLPF, tmp);
  }

  return myTIA.registerValue(CTRLPF) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::scorePF(int newVal)
{
  if(newVal > -1)
  {
    int tmp = myTIA.registerValue(CTRLPF);
    if(newVal)
      tmp |= 0x02;
    else
      tmp &= ~0x02;
    mySystem.poke(CTRLPF, tmp);
  }

  return myTIA.registerValue(CTRLPF) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::priorityPF(int newVal)
{
  if(newVal > -1)
  {
    int tmp = myTIA.registerValue(CTRLPF);
    if(newVal)
      tmp |= 0x04;
    else
      tmp &= ~0x04;
    mySystem.poke(CTRLPF, tmp);
  }

  return myTIA.registerValue(CTRLPF) & 0x04;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::collision(CollisionBit id) const
{
  switch(id)
  {
    case Cx_M0P1:  return myTIA.collCXM0P()  & 0x80;
    case Cx_M0P0:  return myTIA.collCXM0P()  & 0x40;
    case Cx_M1P0:  return myTIA.collCXM1P()  & 0x80;
    case Cx_M1P1:  return myTIA.collCXM1P()  & 0x40;
    case Cx_P0PF:  return myTIA.collCXP0FB() & 0x80;
    case Cx_P0BL:  return myTIA.collCXP0FB() & 0x40;
    case Cx_P1PF:  return myTIA.collCXP1FB() & 0x80;
    case Cx_P1BL:  return myTIA.collCXP1FB() & 0x40;
    case Cx_M0PF:  return myTIA.collCXM0FB() & 0x80;
    case Cx_M0BL:  return myTIA.collCXM0FB() & 0x40;
    case Cx_M1PF:  return myTIA.collCXM1FB() & 0x80;
    case Cx_M1BL:  return myTIA.collCXM1FB() & 0x40;
    case Cx_BLPF:  return myTIA.collCXBLPF() & 0x80;
    case Cx_P0P1:  return myTIA.collCXPPMM() & 0x80;
    case Cx_M0M1:  return myTIA.collCXPPMM() & 0x40;
    default:       return false;  // make compiler happy
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audC0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(AUDC0, newVal);

  return myTIA.registerValue(AUDC0) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audC1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(AUDC1, newVal);

  return myTIA.registerValue(AUDC1) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audV0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(AUDV0, newVal);

  return myTIA.registerValue(AUDV0) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audV1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(AUDV1, newVal);

  return myTIA.registerValue(AUDV1) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audF0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(AUDF0, newVal);

  return myTIA.registerValue(AUDF0) & 0x1f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audF1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(AUDF1, newVal);

  return myTIA.registerValue(AUDF1) & 0x1f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(PF0, newVal << 4);

  return myTIA.registerValue(PF0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(PF1, newVal);

  return myTIA.registerValue(PF1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf2(int newVal)
{
  if(newVal > -1)
    mySystem.poke(PF2, newVal);

  return myTIA.registerValue(PF2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluP0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(COLUP0, newVal);

  return myTIA.registerValue(COLUP0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluP1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(COLUP1, newVal);

  return myTIA.registerValue(COLUP1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluPF(int newVal)
{
  if(newVal > -1)
    mySystem.poke(COLUPF, newVal);

  return myTIA.registerValue(COLUPF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluBK(int newVal)
{
  if(newVal > -1)
    mySystem.poke(COLUBK, newVal);

  return myTIA.registerValue(COLUBK);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusiz0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(NUSIZ0, newVal);

  return myTIA.registerValue(NUSIZ0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusiz1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(NUSIZ1, newVal);

  return myTIA.registerValue(NUSIZ1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusizP0(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(NUSIZ0) & ~0x07;
    tmp |= (newVal & 0x07);
    mySystem.poke(NUSIZ0, tmp);
  }

  return myTIA.registerValue(NUSIZ0) & 0x07;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusizP1(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(NUSIZ1) & ~0x07;
    tmp |= newVal & 0x07;
    mySystem.poke(NUSIZ1, tmp);
  }

  return myTIA.registerValue(NUSIZ1) & 0x07;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusizM0(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(NUSIZ0) & ~0x30;
    tmp |= (newVal & 0x04) << 4;
    mySystem.poke(NUSIZ0, tmp);
  }

  return (myTIA.registerValue(NUSIZ0) & 0x30) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusizM1(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(NUSIZ1) & ~0x30;
    tmp |= (newVal & 0x04) << 4;
    mySystem.poke(NUSIZ1, tmp);
  }

  return (myTIA.registerValue(NUSIZ1) & 0x30) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::grP0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(GRP0, newVal);

  return myTIA.registerValue(GRP0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::grP1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(GRP1, newVal);

  return myTIA.registerValue(GRP1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posP0(int newVal)
{
  if(newVal > -1)
    myTIA.myPlayer0.setPosition(newVal);

  return myTIA.myPlayer0.getPosition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posP1(int newVal)
{
  if(newVal > -1)
    myTIA.myPlayer1.setPosition(newVal);

  return myTIA.myPlayer1.getPosition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posM0(int newVal)
{
  if(newVal > -1)
    myTIA.myMissile0.setPosition(newVal);

  return myTIA.myMissile0.getPosition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posM1(int newVal)
{
  if(newVal > -1)
    myTIA.myMissile1.setPosition(newVal);

  return myTIA.myMissile1.getPosition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::posBL(int newVal)
{
  if(newVal > -1)
    myTIA.myBall.setPosition(newVal);

  return myTIA.myBall.getPosition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::ctrlPF(int newVal)
{
  if(newVal > -1)
    mySystem.poke(CTRLPF, newVal);

  return myTIA.registerValue(CTRLPF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::sizeBL(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(CTRLPF) & ~0x30;
    tmp |= (newVal & 0x04) << 4;
    mySystem.poke(CTRLPF, tmp);
  }

  return (myTIA.registerValue(CTRLPF) & 0x30) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmP0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(HMP0, newVal << 4);

  return myTIA.registerValue(HMP0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmP1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(HMP1, newVal << 4);

  return myTIA.registerValue(HMP1) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmM0(int newVal)
{
  if(newVal > -1)
    mySystem.poke(HMM0, newVal << 4);

  return myTIA.registerValue(HMM0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmM1(int newVal)
{
  if(newVal > -1)
    mySystem.poke(HMM1, newVal << 4);

  return myTIA.registerValue(HMM1) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmBL(int newVal)
{
  if(newVal > -1)
    mySystem.poke(HMBL, newVal << 4);

  return myTIA.registerValue(HMBL) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::setGRP0Old(uInt8 b)
{
  myTIA.myPlayer0.setGRPOld(b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::setGRP1Old(uInt8 b)
{
  myTIA.myPlayer1.setGRPOld(b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::setENABLOld(bool b)
{
  myTIA.myBall.setENABLOld(b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::frameCount() const
{
  return myTIA.myFrameManager.frameCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::scanlines() const
{
  return myTIA.scanlines();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::scanlinesLastFrame() const
{
  return myTIA.myFrameManager.scanlinesLastFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::clocksThisLine() const
{
  return myTIA.clocksThisLine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vsync() const
{
  return myTIA.registerValue(VSYNC) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vblank() const
{
  return myTIA.registerValue(VBLANK) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<DelayQueueIterator> TIADebug::delayQueueIterator() const
{
  return myTIA.delayQueueIterator();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::colorSwatch(uInt8 c) const
{
  string ret;

  ret += char((c >> 1) | 0x80);
  ret += "\177     ";
  ret += "\177\001 ";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// FIXME - how does this work; is this even needed ??
string TIADebug::audFreq(uInt8 div)
{
  string ret;
  char buf[10];

  double hz = 31400.0;
  if(div) hz /= div;
  std::snprintf(buf, 9, "%5.1f", hz);
  ret += buf;
  ret += "Hz";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::booleanWithLabel(string label, bool value)
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
string TIADebug::debugColors() const
{
  ostringstream buf;

  int mode = myTIA.myFrameManager.layout() == FrameLayout::ntsc ? 0 : 1;
  buf << " Red    " << colorSwatch(myTIA.myFixedColorPalette[mode][TIA::P0])
      << " Player 0\n"
      << " Orange " << colorSwatch(myTIA.myFixedColorPalette[mode][TIA::M0])
      << " Missile 0\n"
      << " Yellow " << colorSwatch(myTIA.myFixedColorPalette[mode][TIA::P1])
      << " Player 1\n"
      << " Green  " << colorSwatch(myTIA.myFixedColorPalette[mode][TIA::M1])
      << " Missile 1\n"
      << " Blue   " << colorSwatch(myTIA.myFixedColorPalette[mode][TIA::PF])
      << " Playfield\n"
      << " Purple " << colorSwatch(myTIA.myFixedColorPalette[mode][TIA::BL])
      << " Ball\n"
      << " Grey   " << colorSwatch(TIA::FixedColor::BK_GREY)
      << " Background\n"
      << " White  " << colorSwatch(TIA::FixedColor::HBLANK_WHITE)
      << " HMOVE\n";

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::palette() const
{
  ostringstream buf;

  buf << "     0     2     4     6     8     A     C     E\n";
  uInt8 c = 0;
  for(uInt16 row = 0; row < 16; ++row)
  {
    buf << " " << Common::Base::HEX1 << row << " ";
    for(uInt16 col = 0; col < 8; ++col, c += 2)
      buf << colorSwatch(c);

    buf << endl;
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::toString()
{
  ostringstream buf;

  buf << "00: ";
  for (uInt8 j = 0; j < 0x010; j++)
  {
    buf << Common::Base::HEX2 << int(mySystem.peek(j)) << " ";
    if(j == 0x07) buf << "- ";
  }
  buf << endl;

  // TODO: inverse video for changed regs. Core needs to track this.
  // TODO: strobes? WSYNC RSYNC RESP0/1 RESM0/1 RESBL HMOVE HMCLR CXCLR

  const TiaState& state = static_cast<const TiaState&>(getState());

  // build up output, then return it.
  buf << "scanline " << std::dec << myTIA.scanlines() << " "
      << booleanWithLabel("vsync", vsync()) << " "
      << booleanWithLabel("vblank", vblank())
      << endl
      << booleanWithLabel("inpt0", myTIA.peek(0x08) & 0x80) << " "
      << booleanWithLabel("inpt1", myTIA.peek(0x09) & 0x80) << " "
      << booleanWithLabel("inpt2", myTIA.peek(0x0a) & 0x80) << " "
      << booleanWithLabel("inpt3", myTIA.peek(0x0b) & 0x80) << " "
      << booleanWithLabel("inpt4", myTIA.peek(0x0c) & 0x80) << " "
      << booleanWithLabel("inpt5", myTIA.peek(0x0d) & 0x80) << " "
      << booleanWithLabel("dump_gnd_0123", myTIA.myPaddleReaders[0].vblankDumped())
      << endl
      << "COLUxx: "
      << "P0=$" << Common::Base::HEX2 << state.coluRegs[0] << "/"
      << colorSwatch(state.coluRegs[0])
      << " P1=$" << Common::Base::HEX2 << state.coluRegs[1] << "/"
      << colorSwatch(state.coluRegs[1])
      << " PF=$" << Common::Base::HEX2 << state.coluRegs[2] << "/"
      << colorSwatch(state.coluRegs[2])
      << " BK=$" << Common::Base::HEX2 << state.coluRegs[3] << "/"
      << colorSwatch(state.coluRegs[3])
      << endl
      << "P0: GR=%" << Common::Base::toString(state.gr[P0], Common::Base::F_2_8)
      << " pos=#" << std::dec << state.pos[P0]
      << " HM=$" << Common::Base::HEX2 << state.hm[P0] << " "
      << nusizP0String() << " "
      << booleanWithLabel("refl", refP0()) << " "
      << booleanWithLabel("delay", vdelP0())
      << endl
      << "P1: GR=%" << Common::Base::toString(state.gr[P1], Common::Base::F_2_8)
      << " pos=#" << std::dec << state.pos[P1]
      << " HM=$" << Common::Base::HEX2 << state.hm[P1] << " "
      << nusizP1String() << " "
      << booleanWithLabel("refl", refP1()) << " "
      << booleanWithLabel("delay", vdelP1())
      << endl
      << "M0: " << (enaM0() ? " ENABLED" : "disabled")
      << " pos=#" << std::dec << state.pos[M0]
      << " HM=$" << Common::Base::HEX2 << state.hm[M0]
      << " size=" << std::dec << state.size[M0] << " "
      << booleanWithLabel("reset", resMP0())
      << endl
      << "M1: " << (enaM1() ? " ENABLED" : "disabled")
      << " pos=#" << std::dec << state.pos[M1]
      << " HM=$" << Common::Base::HEX2 << state.hm[M1]
      << " size=" << std::dec << state.size[M1] << " "
      << booleanWithLabel("reset", resMP0())
      << endl
      << "BL: " << (enaBL() ? " ENABLED" : "disabled")
      << " pos=#" << std::dec << state.pos[BL]
      << " HM=$" << Common::Base::HEX2 << state.hm[BL]
      << " size=" << std::dec << state.size[BL] << " "
      << booleanWithLabel("delay", vdelBL())
      << endl
      << "PF0: %" << Common::Base::toString(state.pf[0], Common::Base::F_2_8) << "/$"
      << Common::Base::HEX2 << state.pf[0]
      << " PF1: %" << Common::Base::toString(state.pf[1], Common::Base::F_2_8) << "/$"
      << Common::Base::HEX2 << state.pf[1]
      << " PF2: %" << Common::Base::toString(state.pf[2], Common::Base::F_2_8) << "/$"
      << Common::Base::HEX2 << state.pf[2]
      << endl << "     "
      << booleanWithLabel("reflect",  refPF()) << " "
      << booleanWithLabel("score",    scorePF()) << " "
      << booleanWithLabel("priority", priorityPF())
      << endl
      << "Collisions: "
      << booleanWithLabel("m0_p1 ", collM0_P1())
      << booleanWithLabel("m0_p0 ", collM0_P0())
      << booleanWithLabel("m1_p0 ", collM1_P0())
      << booleanWithLabel("m1_p1 ", collM1_P1())
      << booleanWithLabel("p0_pf ", collP0_PF())
      << booleanWithLabel("p0_bl ", collP0_BL())
      << booleanWithLabel("p1_pf ", collP1_PF())
      << endl << "            "
      << booleanWithLabel("p1_bl ", collP1_BL())
      << booleanWithLabel("m0_pf ", collM0_PF())
      << booleanWithLabel("m0_bl ", collM0_BL())
      << booleanWithLabel("m1_pf ", collM1_PF())
      << booleanWithLabel("m1_bl ", collM1_BL())
      << booleanWithLabel("bl_pf ", collBL_PF())
      << booleanWithLabel("p0_p1 ", collP0_P1())
      << endl << "            "
      << booleanWithLabel("m0_m1 ", collM0_M1())
      << endl
      << "AUDF0: $" << Common::Base::HEX2 << int(audF0())
      << "/" << audFreq(audF0()) << " "
      << "AUDC0: $" << Common::Base::HEX2 << int(audC0()) << " "
      << "AUDV0: $" << Common::Base::HEX2 << int(audV0())
      << endl
      << "AUDF1: $" << Common::Base::HEX2 << int(audF1())
      << "/" << audFreq(audF1()) << " "
      << "AUDC1: $" << Common::Base::HEX2 << int(audC1()) << " "
      << "AUDV1: $" << Common::Base::HEX2 << int(audV1())
      ;
  // note: last line should not contain \n, caller will add.
  return buf.str();
}
