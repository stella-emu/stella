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

#include "Base.hxx"
#include "System.hxx"
#include "Debugger.hxx"
#include "TIA.hxx"
#include "DelayQueueIterator.hxx"
#include "RiotDebug.hxx"

#include "TIADebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIADebug::TIADebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console),
    myTIA{console.tia()}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& TIADebug::getState()
{
  populateState(myState);
  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::saveOldState()
{
  populateState(myOldState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::populateState(TiaState& s)
{
  // Color registers
  s.coluRegs = { coluP0(), coluP1(), coluPF(), coluBK() };

  // Debug Colors (only needed for myState, not myOldState, but harmless)
  const int timing = myConsole.timing() == ConsoleTiming::ntsc ? 0
    : myConsole.timing() == ConsoleTiming::pal ? 1 : 2;

  s.fixedCols = {
    myTIA.myFixedColorPalette[timing][TIA::P0],
    myTIA.myFixedColorPalette[timing][TIA::P1],
    myTIA.myFixedColorPalette[timing][TIA::PF],
    myTIA.myFixedColorPalette[timing][TIA::BK],
    myTIA.myFixedColorPalette[timing][TIA::M0],
    myTIA.myFixedColorPalette[timing][TIA::M1],
    myTIA.myFixedColorPalette[timing][TIA::BL],
    TIA::FixedColor::HBLANK_WHITE
  };

  // Collisions
  s.cx = {
    collP0_PF(), collP0_BL(), collM1_P0(), collM0_P0(), collP0_P1(),
    collP1_PF(), collP1_BL(), collM1_P1(), collM0_P1(), collM0_PF(),
    collM0_BL(), collM0_M1(), collM1_PF(), collM1_BL(), collBL_PF()
  };

  // Graphics registers
  s.gr = {
    myTIA.myPlayer0.getGRPNew(), myTIA.myPlayer1.getGRPNew(),
    myTIA.myPlayer0.getGRPOld(), myTIA.myPlayer1.getGRPOld(),
    myTIA.myBall.getENABLNew(),  myTIA.myBall.getENABLOld(),
    enaM0(), enaM1()
  };

  // Reflect / VDel / ResetMissile
  s.ref  = { refP0(),   refP1() };
  s.vdel = { vdelP0(),  vdelP1(), vdelBL() };
  s.resm = { resMP0(),  resMP1() };

  // Position registers
  s.pos = { posP0(), posP1(), posM0(), posM1(), posBL() };

  // Horizontal move registers
  s.hm = { hmP0(), hmP1(), hmM0(), hmM1(), hmBL() };

  // Playfield registers
  s.pf = { pf0(), pf1(), pf2(), refPF(), scorePF(), priorityPF() };

  // Size registers
  s.size = { nusizP0(), nusizP1(), nusizM0(), nusizM1(), sizeBL() };

  // VSync/VBlank
  s.vsb = { vsync(), vblank() };

  // Audio registers
  s.aud = { audF0(), audF1(), audC0(), audC1(), audV0(), audV1() };

  // Internal TIA state
  s.info = {
    frameCount(), frameCycles(),
    cyclesLo(),   cyclesHi(),
    scanlines(),  scanlinesLastFrame(),
    clocksThisLine(), frameWsyncCycles()
  };
}

/* the set methods now use mySystem.pokeOob(). This will save us the
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
    mySystem.pokeOob(VDELP0, static_cast<bool>(newVal));

  return myTIA.registerValue(VDELP0) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelP1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(VDELP1, static_cast<bool>(newVal));

  return myTIA.registerValue(VDELP1) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vdelBL(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(VDELBL, static_cast<bool>(newVal));

  return myTIA.registerValue(VDELBL) & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaM0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(ENAM0, static_cast<bool>(newVal) << 1);

  return myTIA.registerValue(ENAM0) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaM1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(ENAM1, static_cast<bool>(newVal) << 1);

  return myTIA.registerValue(ENAM1) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::enaBL(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(ENABL, static_cast<bool>(newVal) << 1);

  return myTIA.registerValue(ENABL) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::resMP0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(RESMP0, static_cast<bool>(newVal) << 1);

  return myTIA.registerValue(RESMP0) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::resMP1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(RESMP1, static_cast<bool>(newVal) << 1);

  return myTIA.registerValue(RESMP1) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refP0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(REFP0, static_cast<bool>(newVal) << 3);

  return myTIA.registerValue(REFP0) & 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::refP1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(REFP1, static_cast<bool>(newVal) << 3);

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
    mySystem.pokeOob(CTRLPF, tmp);
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
    mySystem.pokeOob(CTRLPF, tmp);
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
    mySystem.pokeOob(CTRLPF, tmp);
  }

  return myTIA.registerValue(CTRLPF) & 0x04;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::collision(CollisionBit id, bool toggle) const
{
  switch(id)
  {
    case CollisionBit::M0P1:
      if(toggle)
        myTIA.toggleCollP1M0();
      return myTIA.collCXM0P()  & 0x80;

    case CollisionBit::M0P0:
      if(toggle)
        myTIA.toggleCollP0M0();
      return myTIA.collCXM0P()  & 0x40;

    case CollisionBit::M1P0:
      if(toggle)
        myTIA.toggleCollP0M1();
      return myTIA.collCXM1P()  & 0x80;

    case CollisionBit::M1P1:
      if(toggle)
        myTIA.toggleCollP1M1();
      return myTIA.collCXM1P()  & 0x40;

    case CollisionBit::P0PF:
      if(toggle)
        myTIA.toggleCollP0PF();
      return myTIA.collCXP0FB() & 0x80;
    case CollisionBit::P0BL:
      if(toggle)
        myTIA.toggleCollP0BL();
      return myTIA.collCXP0FB() & 0x40;

    case CollisionBit::P1PF:
      if(toggle)
        myTIA.toggleCollP1PF();
      return myTIA.collCXP1FB() & 0x80;

    case CollisionBit::P1BL:
      if(toggle)
        myTIA.toggleCollP1BL();
      return myTIA.collCXP1FB() & 0x40;

    case CollisionBit::M0PF:
      if(toggle)
        myTIA.toggleCollM0PF();
      return myTIA.collCXM0FB() & 0x80;

    case CollisionBit::M0BL:
      if(toggle)
        myTIA.toggleCollM0BL();
      return myTIA.collCXM0FB() & 0x40;

    case CollisionBit::M1PF:
      if(toggle)
        myTIA.toggleCollM1PF();
      return myTIA.collCXM1FB() & 0x80;

    case CollisionBit::M1BL:
      if(toggle)
        myTIA.toggleCollM1BL();
      return myTIA.collCXM1FB() & 0x40;

    case CollisionBit::BLPF:
      if(toggle)
        myTIA.toggleCollBLPF();
      return myTIA.collCXBLPF() & 0x80;

    case CollisionBit::P0P1:
      if(toggle)
        myTIA.toggleCollP0P1();
      return myTIA.collCXPPMM() & 0x80;

    case CollisionBit::M0M1:
      if(toggle)
        myTIA.toggleCollM0M1();
      return myTIA.collCXPPMM() & 0x40;

    default:
      return false;  // Not supposed to get here
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audC0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(AUDC0, newVal);

  return myTIA.registerValue(AUDC0) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audC1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(AUDC1, newVal);

  return myTIA.registerValue(AUDC1) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audV0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(AUDV0, newVal);

  return myTIA.registerValue(AUDV0) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audV1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(AUDV1, newVal);

  return myTIA.registerValue(AUDV1) & 0x0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audF0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(AUDF0, newVal);

  return myTIA.registerValue(AUDF0) & 0x1f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::audF1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(AUDF1, newVal);

  return myTIA.registerValue(AUDF1) & 0x1f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(PF0, newVal << 4);

  return myTIA.registerValue(PF0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(PF1, newVal);

  return myTIA.registerValue(PF1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::pf2(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(PF2, newVal);

  return myTIA.registerValue(PF2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluP0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(COLUP0, newVal);

  return myTIA.registerValue(COLUP0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluP1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(COLUP1, newVal);

  return myTIA.registerValue(COLUP1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluPF(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(COLUPF, newVal);

  return myTIA.registerValue(COLUPF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::coluBK(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(COLUBK, newVal);

  return myTIA.registerValue(COLUBK);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusiz0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(NUSIZ0, newVal);

  return myTIA.registerValue(NUSIZ0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusiz1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(NUSIZ1, newVal);

  return myTIA.registerValue(NUSIZ1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::nusizP0(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(NUSIZ0) & ~0x07;
    tmp |= (newVal & 0x07);
    mySystem.pokeOob(NUSIZ0, tmp);
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
    mySystem.pokeOob(NUSIZ1, tmp);
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
    mySystem.pokeOob(NUSIZ0, tmp);
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
    mySystem.pokeOob(NUSIZ1, tmp);
  }

  return (myTIA.registerValue(NUSIZ1) & 0x30) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::grP0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(GRP0, newVal);

  return myTIA.registerValue(GRP0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::grP1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(GRP1, newVal);

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
    mySystem.pokeOob(CTRLPF, newVal);

  return myTIA.registerValue(CTRLPF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::sizeBL(int newVal)
{
  if(newVal > -1)
  {
    uInt8 tmp = myTIA.registerValue(CTRLPF) & ~0x30;
    tmp |= (newVal & 0x04) << 4;
    mySystem.pokeOob(CTRLPF, tmp);
  }

  return (myTIA.registerValue(CTRLPF) & 0x30) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmP0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(HMP0, newVal << 4);

  return myTIA.registerValue(HMP0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmP1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(HMP1, newVal << 4);

  return myTIA.registerValue(HMP1) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmM0(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(HMM0, newVal << 4);

  return myTIA.registerValue(HMM0) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmM1(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(HMM1, newVal << 4);

  return myTIA.registerValue(HMM1) >> 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIADebug::hmBL(int newVal)
{
  if(newVal > -1)
    mySystem.pokeOob(HMBL, newVal << 4);

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
void TIADebug::strobeWsync()
{
  mySystem.pokeOob(WSYNC, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeRsync()
{
  mySystem.pokeOob(RSYNC, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeResP0()
{
  mySystem.pokeOob(RESP0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeResP1()
{
  mySystem.pokeOob(RESP1, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeResM0()
{
  mySystem.pokeOob(RESM0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeResM1()
{
  mySystem.pokeOob(RESM1, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeResBL()
{
  mySystem.pokeOob(RESBL, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeHmove()
{
  mySystem.pokeOob(HMOVE, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeHmclr()
{
  mySystem.pokeOob(HMCLR, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIADebug::strobeCxclr()
{
  mySystem.pokeOob(CXCLR, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::frameCount() const
{
  return myTIA.frameCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::frameCycles() const
{
  return myTIA.frameCycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::frameWsyncCycles() const
{
  return myTIA.frameWSyncCycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::cyclesLo() const
{
  return static_cast<int>(mySystem.cycles());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::cyclesHi() const
{
  return static_cast<int>(mySystem.cycles() >> 32);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::scanlines() const
{
  return myTIA.scanlines();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::scanlinesLastFrame() const
{
  return myTIA.scanlinesLastFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::clocksThisLine() const
{
  return myTIA.clocksThisLine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TIADebug::cyclesThisLine() const
{
  return myTIA.clocksThisLine()/3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vsync(int newVal)
{
  if (newVal > -1)
    mySystem.pokeOob(VSYNC, newVal);

  return myTIA.registerValue(VSYNC) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vsync() const
{
  return myTIA.registerValue(VSYNC) & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIADebug::vblank(int newVal)
{
  if (newVal > -1)
    mySystem.pokeOob(VBLANK, newVal);

  return myTIA.registerValue(VBLANK) & 0x02;
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
string TIADebug::colorSwatch(uInt8 c)
{
  string ret;

  ret += static_cast<char>((c >> 1) | 0x80);
  ret += "\177     ";
  ret += "\177\001 ";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::audFreq0()
{
  return audFreq(audC0(), audF0());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::audFreq1()
{
  return audFreq(audC1(), audF1());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::audFreq(uInt8 dist, uInt8 div) const
{
  static constexpr array<uInt16, 16> dist_div = {
      1, 15, 465, 465, 2, 2, 31, 31,
    511, 31,  31,   1, 6, 6, 93, 93
  };

  const double hz =
    (myConsole.timing() == ConsoleTiming::ntsc ? 31440.0 : 31200.0)
    / dist_div[dist] / (div + 1);

  return std::format("{:7.1f}Hz", hz);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::stringOnly(string_view value, bool changed)
{
  if(changed)
    return static_cast<char>(kDbgColorRed & 0xff) + string{value} +
           static_cast<char>(kTextColor & 0xff);

  return string{value};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::decWithLabel(string_view label, uInt16 value,
                              bool changed, uInt16 width)
{
  string result;
  result.reserve(label.size() + width + 4);

  result += label;
  if(!label.empty())
    result += '=';
  result += std::format("#{:<{}}", value, width);

  if(changed)
    return static_cast<char>(kDbgColorRed & 0xff) + result +
           static_cast<char>(kTextColor & 0xff);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::hexWithLabel(string_view label, uInt16 value,
                              bool changed, uInt16 width)
{
  string result;
  result.reserve(label.size() + width + 3);

  result += label;
  if(!label.empty())
    result += '=';
  result += (width == 1)
    ? std::format("${:01X}", value)
    : std::format("${:02X}", value);

  if(changed)
    return static_cast<char>(kDbgColorRed & 0xff) + result +
           static_cast<char>(kTextColor & 0xff);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::binWithLabel(string_view label, uInt16 value, bool changed)
{
  string result;
  result.reserve(label.size() + 10);  // % + 8 binary digits + = + label

  result += label;
  if(!label.empty())
    result += '=';
  result += '%';
  result += Common::Base::toString(value, Common::Base::Fmt::_2_8);

  if(changed)
    return static_cast<char>(kDbgColorRed & 0xff) + result +
           static_cast<char>(kTextColor & 0xff);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::boolWithLabel(string_view label, bool value, bool changed)
{
  string result;
  result.reserve(label.size() + 4);  // +4 for the two \177 chars + 2 color chars worst case

  if(value)
  {
    string l{label};  // TODO: BSPF::toUpperCase takes a string reference
    result += '\177';  result += BSPF::toUpperCase(l);  result += '\177';
  }
  else
    result += label;

  if(changed)
    return static_cast<char>(kDbgColorRed & 0xff) + result +
           static_cast<char>(kTextColor & 0xff);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::debugColors() const
{
  const int timing = myConsole.timing() == ConsoleTiming::ntsc ? 0
    : myConsole.timing() == ConsoleTiming::pal ? 1 : 2;

  static constexpr array<std::pair<int, string_view>, 6> entries = {{
    { TIA::P0, "Player 0"  },
    { TIA::M0, "Missile 0" },
    { TIA::P1, "Player 1"  },
    { TIA::M1, "Missile 1" },
    { TIA::PF, "Playfield" },
    { TIA::BL, "Ball"      }
  }};

  string result;
  result.reserve(256);  // fits comfortably within known output size

  for(const auto& [index, label]: entries)
  {
    result += " ";  result += myTIA.myFixedColorNames[index];
    result += " ";  result += colorSwatch(myTIA.myFixedColorPalette[timing][index]);
    result += " ";  result += label;
    result += "\n";
  }

  result += " Grey   ";
  result += colorSwatch(myTIA.myFixedColorPalette[timing][TIA::BK]);
  result += " Background\n";
  result += " White  ";
  result += colorSwatch(TIA::FixedColor::HBLANK_WHITE);
  result += " HMOVE\n";

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::palette()
{
  // colorSwatch() returns 9 chars, 8 swatches per row = 72
  // row prefix " X " = 3 chars, newline = 1, total per row = 76
  // 16 rows = 1216, plus header line = 1216 + 49 = 1265
  static constexpr size_t bufSize =
    49 +                // header line incl. \n
    (3 + 72 + 1) * 16;  // rows

  static constexpr string_view header = "     0     2     4     6     8     A     C     E\n";
  static constexpr string_view hexChars = "0123456789ABCDEF";

  string result;
  result.reserve(bufSize);
  result += header;

  uInt8 c = 0;
  for(uInt16 row = 0; row < 16; ++row)
  {
    result += ' ';  result += hexChars[row];  result += ' ';
    for(uInt16 col = 0; col < 8; ++col, c += 2)
      result += colorSwatch(c);
    result += '\n';
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TIADebug::toString()
{
  std::ostringstream buf;

  // TODO: strobes? WSYNC RSYNC RESP0/1 RESM0/1 RESBL HMOVE HMCLR CXCLR

  RiotDebug& riot = myDebugger.riotDebug();
  const auto& riotState = static_cast<const RiotState&>(riot.getState());
  const auto& oldRiotState = static_cast<const RiotState&>(riot.getOldState());

  const auto& state = static_cast<const TiaState&>(getState());
  const auto& oldState = static_cast<const TiaState&>(getOldState());

  static constexpr array<string_view, 15> cxLabels = {
    "p0_pf", "p0_bl", "m1_p0", "m0_p0", "p0_p1",
    "p1_pf", "p1_bl", "m1_p1", "m0_p1", "m0_pf",
    "m0_bl", "m0_m1", "m1_pf", "m1_bl", "bl_pf"
  };
  auto writeCx = [&](int start, int end)
  {
    for(int i = start; i < end; ++i)
    {
      buf << boolWithLabel(cxLabels[i], state.cx[i], state.cx[i] != oldState.cx[i]);
      if(i + 1 < end) buf << " ";
    }
  };

  // build up output, then return it.
  buf << std::setfill(' ') << std::left
      << decWithLabel("scanline", myTIA.scanlines(),
                      std::cmp_not_equal(myTIA.scanlines(), oldState.info[4])) << " "
      << boolWithLabel("vsync",  vsync(),
                       state.vsb[0] != oldState.vsb[0]) << " "
      << boolWithLabel("vblank", vblank(),
                       state.vsb[1] != oldState.vsb[1])
      << '\n'
      << "Collisions: ";
      writeCx(0, 8);
      buf << '\n' << "            ";
      writeCx(8, 15);

  buf << '\n'
      << "COLUxx: "
      << hexWithLabel("P0", state.coluRegs[TiaState::P0],
                      state.coluRegs[TiaState::P0] != oldState.coluRegs[TiaState::P0]) << "/"
      << colorSwatch(state.coluRegs[TiaState::P0])
      << hexWithLabel("P1", state.coluRegs[TiaState::P1],
                      state.coluRegs[TiaState::P1] != oldState.coluRegs[TiaState::P1]) << "/"
      << colorSwatch(state.coluRegs[TiaState::P1])
      << hexWithLabel("PF", state.coluRegs[2],
                      state.coluRegs[2] != oldState.coluRegs[2]) << "/"
      << colorSwatch(state.coluRegs[2])
      << hexWithLabel("BK", state.coluRegs[3],
                      state.coluRegs[3] != oldState.coluRegs[3]) << "/"
      << colorSwatch(state.coluRegs[3])
      << '\n'
      << "P0: "
      << binWithLabel("GR", state.gr[TiaState::P0],
                      state.gr[TiaState::P0] != oldState.gr[TiaState::P0]) << " "
      << decWithLabel("pos", state.pos[TiaState::P0],
                      state.pos[TiaState::P0] != oldState.pos[TiaState::P0]) << " "
      << hexWithLabel("HM", state.hm[TiaState::P0],
                      state.hm[TiaState::P0] != oldState.hm[TiaState::P0], 1) << " "
      << stringOnly(nusizP0String(),
                    state.size[TiaState::P0] != oldState.size[TiaState::P0]) << " "
      << boolWithLabel("refl",  refP0(),
                       state.ref[TiaState::P0]  != oldState.ref[TiaState::P0]) << " "
      << boolWithLabel("delay", vdelP0(),
                       state.vdel[TiaState::P0] != oldState.vdel[TiaState::P0])
      << '\n'
      << "P1: "
      << binWithLabel("GR", state.gr[TiaState::P1],
                     state.gr[TiaState::P1] != oldState.gr[TiaState::P1]) << " "
      << decWithLabel("pos", state.pos[TiaState::P1],
                      state.pos[TiaState::P1] != oldState.pos[TiaState::P1]) << " "
      << hexWithLabel("HM", state.hm[TiaState::P1],
                      state.hm[TiaState::P1] != oldState.hm[TiaState::P1], 1) << " "
      << stringOnly(nusizP1String(),
                    state.size[TiaState::P1] != oldState.size[TiaState::P1]) << " "
      << boolWithLabel("refl", refP1(),
                       state.ref[TiaState::P1] != oldState.ref[TiaState::P1]) << " "
      << boolWithLabel("delay", vdelP1(),
                       state.vdel[TiaState::P1] != oldState.vdel[TiaState::P1])
      << '\n'
      << "M0: "
      << stringOnly(enaM0() ? "ENABLED " : "disabled",
                    state.gr[6] != oldState.gr[6]) << " "
      << decWithLabel("pos", state.pos[TiaState::M0],
                      state.pos[TiaState::M0] != oldState.pos[TiaState::M0]) << " "
      << hexWithLabel("HM", state.hm[TiaState::M0],
                      state.hm[TiaState::M0] != oldState.hm[TiaState::M0], 1) << " "
      << decWithLabel("size", state.size[TiaState::M0],
                      state.size[TiaState::M0] != oldState.size[TiaState::M0], 1) << " "
      << boolWithLabel("reset", resMP0(), state.resm[TiaState::P0] != oldState.resm[TiaState::P0])
      << '\n'
      << "M1: "
      << stringOnly(enaM1() ? "ENABLED " : "disabled",
                    state.gr[7] != oldState.gr[7]) << " "
      << decWithLabel("pos", state.pos[TiaState::M1],
                      state.pos[TiaState::M1] != oldState.pos[TiaState::M1]) << " "
      << hexWithLabel("HM", state.hm[TiaState::M1],
                      state.hm[TiaState::M1] != oldState.hm[TiaState::M1], 1) << " "
      << decWithLabel("size", state.size[TiaState::M1],
                      state.size[TiaState::M1] != oldState.size[TiaState::M1], 1) << " "
      << boolWithLabel("reset", resMP1(), state.resm[TiaState::P1] != oldState.resm[TiaState::P1])
      << '\n'
      << "BL: "
      << stringOnly(enaBL() ? "ENABLED " : "disabled",
                    state.gr[4] != oldState.gr[4]) << " "
      << decWithLabel("pos", state.pos[TiaState::BL],
                      state.pos[TiaState::BL] != oldState.pos[TiaState::BL]) << " "
      << hexWithLabel("HM", state.hm[TiaState::BL],
                      state.hm[TiaState::BL] != oldState.hm[TiaState::BL], 1) << " "
      << decWithLabel("size", state.size[TiaState::BL],
                      state.size[TiaState::BL] != oldState.size[TiaState::BL], 1) << " "
      << boolWithLabel("delay", vdelBL(), state.vdel[2] != oldState.vdel[2])
      << '\n'
      << "PF0: "
      << binWithLabel("", state.pf[0],
                      state.pf[0] != oldState.pf[0]) << "/"
      << hexWithLabel("", state.pf[0],
                      state.pf[0] != oldState.pf[0]) << " "
      << "PF1: "
      << binWithLabel("", state.pf[1],
                      state.pf[1] != oldState.pf[1]) << "/"
      << hexWithLabel("", state.pf[1],
                      state.pf[1] != oldState.pf[1]) << " "
      << "PF2: "
      << binWithLabel("", state.pf[2],
                      state.pf[2] != oldState.pf[2]) << "/"
      << hexWithLabel("", state.pf[2],
                      state.pf[2] != oldState.pf[2]) << " "
      << '\n' << "     "
      << boolWithLabel("reflect",  refPF(),      state.pf[3] != oldState.pf[3]) << " "
      << boolWithLabel("score",    scorePF(),    state.pf[4] != oldState.pf[4]) << " "
      << boolWithLabel("priority", priorityPF(), state.pf[5] != oldState.pf[5])
      << '\n'
      << boolWithLabel("inpt0", myTIA.peek(0x08) & 0x80,
                        (riotState.INPT0 & 0x80) != (oldRiotState.INPT0 & 0x80)) << " "
      << boolWithLabel("inpt1", myTIA.peek(0x09) & 0x80,
                        (riotState.INPT1 & 0x80) != (oldRiotState.INPT1 & 0x80)) << " "
      << boolWithLabel("inpt2", myTIA.peek(0x0a) & 0x80,
                        (riotState.INPT2 & 0x80) != (oldRiotState.INPT2 & 0x80)) << " "
      << boolWithLabel("inpt3", myTIA.peek(0x0b) & 0x80,
                        (riotState.INPT3 & 0x80) != (oldRiotState.INPT3 & 0x80)) << " "
      << boolWithLabel("inpt4", myTIA.peek(0x0c) & 0x80,
                        (riotState.INPT4 & 0x80) != (oldRiotState.INPT4 & 0x80)) << " "
      << boolWithLabel("inpt5", myTIA.peek(0x0d) & 0x80,
                        (riotState.INPT5 & 0x80) != (oldRiotState.INPT5 & 0x80)) << " "
      << boolWithLabel("dump_gnd_0123", myTIA.myAnalogReadouts[0].vblankDumped(),
                        riotState.INPTDump != oldRiotState.INPTDump)
      << '\n'
      << "AUDF0: "
      << hexWithLabel("", static_cast<int>(audF0()),
                      state.aud[0] != oldState.aud[0]) << "/"
      << std::setw(9) << std::right << stringOnly(audFreq0(),
                    state.aud[0] != oldState.aud[0]) << " "
      << "AUDC0: "
      << hexWithLabel("", static_cast<int>(audC0()),
                      state.aud[2] != oldState.aud[2], 1) << " "
      << "AUDV0: "
      << hexWithLabel("", static_cast<int>(audV0()),
                      state.aud[4] != oldState.aud[4], 1)
      << '\n'
      << "AUDF1: "
      << hexWithLabel("", static_cast<int>(audF1()),
                      state.aud[1] != oldState.aud[1]) << "/"
      << std::setw(9) << std::right << stringOnly(audFreq1(),
                    state.aud[1] != oldState.aud[1]) << " "
      << "AUDC1: "
      << hexWithLabel("", static_cast<int>(audC1()),
                      state.aud[3] != oldState.aud[3], 1) << " "
      << "AUDV1: "
      << hexWithLabel("", static_cast<int>(audV1()),
                      state.aud[5] != oldState.aud[5], 1);
  // note: last line should not contain \n, caller will add.
  return buf.str();
}
