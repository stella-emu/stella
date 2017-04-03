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

#include "TIA.hxx"
#include "M6502.hxx"
#include "Console.hxx"
#include "Control.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "CartDebug.hxx"
#endif

enum CollisionMask: uInt32 {
  player0   = 0b0111110000000000,
  player1   = 0b0100001111000000,
  missile0  = 0b0010001000111000,
  missile1  = 0b0001000100100110,
  ball      = 0b0000100010010101,
  playfield = 0b0000010001001011
};

enum Delay: uInt8 {
  hmove = 6,
  pf = 2,
  grp = 1,
  shufflePlayer = 1,
  shuffleBall = 1,
  hmp = 2,
  hmm = 2,
  hmbl = 2,
  hmclr = 2,
  refp = 1,
  enabl = 1,
  enam = 1,
  vblank = 1
};

enum DummyRegisters: uInt8 {
  shuffleP0 = 0xF0,
  shuffleP1 = 0xF1,
  shuffleBL = 0xF2
};

enum ResxCounter: uInt8 {
  hblank = 159,
  lateHblank = 158,
  frame = 157
};

// This parameter still has room for tuning. If we go lower than 73, long005 will show
// a slight artifact (still have to crosscheck on real hardware), if we go lower than
// 70, the G.I. Joe will show an artifact (hole in roof).
static constexpr uInt8 resxLateHblankThreshold = 73;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(Console& console, Sound& sound, Settings& settings)
  : myConsole(console),
    mySound(sound),
    mySettings(settings),
    myDelayQueue(10, 20),
    myPlayfield(~CollisionMask::playfield & 0x7FFF),
    myMissile0(~CollisionMask::missile0 & 0x7FFF),
    myMissile1(~CollisionMask::missile1 & 0x7FFF),
    myPlayer0(~CollisionMask::player0 & 0x7FFF),
    myPlayer1(~CollisionMask::player1 & 0x7FFF),
    myBall(~CollisionMask::ball & 0x7FFF),
    mySpriteEnabledBits(0xFF),
    myCollisionsEnabledBits(0xFF)
{
  myFrameManager.setHandlers(
    [this] () {
      onFrameStart();
    },
    [this] () {
      onFrameComplete();
    }
  );

  myCurrentFrameBuffer  = make_ptr<uInt8[]>(160 * FrameManager::frameBufferHeight);
  myPreviousFrameBuffer = make_ptr<uInt8[]>(160 * FrameManager::frameBufferHeight);

  myTIAPinsDriven = mySettings.getBool("tiadriven");

  myPlayer0.setPlayfieldPositionProvider(this);
  myPlayer1.setPlayfieldPositionProvider(this);
  myMissile0.setPlayfieldPositionProvider(this);
  myMissile1.setPlayfieldPositionProvider(this);
  myBall.setPlayfieldPositionProvider(this);

  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::reset()
{
  myHblankCtr = 0;
  myHctr = 0;
  myMovementInProgress = false;
  myExtendedHblank = false;
  myMovementClock = 0;
  myPriority = Priority::normal;
  myHstate = HState::blank;
  myIsFreshLine = true;
  myCollisionMask = 0;
  myLinesSinceChange = 0;
  myCollisionUpdateRequired = false;
  myAutoFrameEnabled = false;
  myColorHBlank = 0;
  myLastCycle = 0;
  mySubClock = 0;
  myXDelta = 0;

  memset(myShadowRegisters, 0, 64);

  myBackground.reset();
  myPlayfield.reset();
  myMissile0.reset();
  myMissile1.reset();
  myPlayer0.reset();
  myPlayer1.reset();
  myBall.reset();

  myInput0.reset();
  myInput1.reset();

  myTimestamp = 0;
  for (PaddleReader& paddleReader : myPaddleReaders)
    paddleReader.reset(myTimestamp);

  mySound.reset();
  myDelayQueue.reset();
  myFrameManager.reset();
  toggleFixedColors(0);  // Turn off debug colours

  frameReset();  // Recalculate the size of the display
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::frameReset()
{
  clearBuffers();
  myAutoFrameEnabled = (mySettings.getInt("framerate") <= 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::systemCyclesReset()
{
  const uInt32 cycles = mySystem->cycles();

  myLastCycle -= cycles;

  mySound.adjustCycleCounter(-1 * cycles);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::install(System& system)
{
  installDelegate(system, *this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::installDelegate(System& system, Device& device)
{
  // Remember which system I'm installed in
  mySystem = &system;

  // All accesses are to the given device
  System::PageAccess access(&device, System::PA_READWRITE);

  // We're installing in a 2600 system
  for(uInt32 i = 0; i < 8192; i += (1 << System::PAGE_SHIFT))
    if((i & 0x1080) == 0x0000)
      mySystem->setPageAccess(i >> System::PAGE_SHIFT, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::clearBuffers()
{
  memset(myCurrentFrameBuffer.get(), 0, 160 * FrameManager::frameBufferHeight);
  memset(myPreviousFrameBuffer.get(), 0, 160 * FrameManager::frameBufferHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    if(!mySound.save(out)) return false;

    if(!myDelayQueue.save(out))   return false;
    if(!myFrameManager.save(out)) return false;

    if(!myBackground.save(out)) return false;
    if(!myPlayfield.save(out))  return false;
    if(!myMissile0.save(out))   return false;
    if(!myMissile1.save(out))   return false;
    if(!myPlayer0.save(out))    return false;
    if(!myPlayer1.save(out))    return false;
    if(!myBall.save(out))       return false;

    for (const PaddleReader& paddleReader : myPaddleReaders)
      if(!paddleReader.save(out)) return false;

    if(!myInput0.save(out)) return false;
    if(!myInput1.save(out)) return false;

    out.putBool(myTIAPinsDriven);

    out.putInt(int(myHstate));
    out.putBool(myIsFreshLine);

    out.putInt(myHblankCtr);
    out.putInt(myHctr);
    out.putInt(myXDelta);

    out.putBool(myCollisionUpdateRequired);
    out.putInt(myCollisionMask);

    out.putInt(myMovementClock);
    out.putBool(myMovementInProgress);
    out.putBool(myExtendedHblank);

    out.putInt(myLinesSinceChange);

    out.putInt(int(myPriority));

    out.putByte(mySubClock);
    out.putInt(myLastCycle);

    out.putByte(mySpriteEnabledBits);
    out.putByte(myCollisionsEnabledBits);

    out.putByte(myColorHBlank);

    out.putDouble(myTimestamp);

    out.putBool(myAutoFrameEnabled);

    out.putByteArray(myShadowRegisters, 64);
  }
  catch(...)
  {
    cerr << "ERROR: TIA::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    if(!mySound.load(in)) return false;

    if(!myDelayQueue.load(in))   return false;
    if(!myFrameManager.load(in)) return false;

    if(!myBackground.load(in)) return false;
    if(!myPlayfield.load(in))  return false;
    if(!myMissile0.load(in))   return false;
    if(!myMissile1.load(in))   return false;
    if(!myPlayer0.load(in))    return false;
    if(!myPlayer1.load(in))    return false;
    if(!myBall.load(in))       return false;

    for (PaddleReader& paddleReader : myPaddleReaders)
      if(!paddleReader.load(in)) return false;

    if(!myInput0.load(in)) return false;
    if(!myInput1.load(in)) return false;

    myTIAPinsDriven = in.getBool();

    myHstate = HState(in.getInt());
    myIsFreshLine = in.getBool();

    myHblankCtr = in.getInt();
    myHctr = in.getInt();
    myXDelta = in.getInt();

    myCollisionUpdateRequired = in.getBool();
    myCollisionMask = in.getInt();

    myMovementClock = in.getInt();
    myMovementInProgress = in.getBool();
    myExtendedHblank = in.getBool();

    myLinesSinceChange = in.getInt();

    myPriority = Priority(in.getInt());

    mySubClock = in.getByte();
    myLastCycle = in.getInt();

    mySpriteEnabledBits = in.getByte();
    myCollisionsEnabledBits = in.getByte();

    myColorHBlank = in.getByte();

    myTimestamp = in.getDouble();

    myAutoFrameEnabled = in.getBool();

    in.getByteArray(myShadowRegisters, 64);
  }
  catch(...)
  {
    cerr << "ERROR: TIA::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::peek(uInt16 address)
{
  updateEmulation();

  // If pins are undriven, we start with the last databus value
  // Otherwise, there is some randomness injected into the mix
  // In either case, we start out with D7 and D6 disabled (the only
  // valid bits in a TIA read), and selectively enable them
  uInt8 lastDataBusValue =
    !myTIAPinsDriven ? mySystem->getDataBusState() : mySystem->getDataBusState(0xFF);

  uInt8 result;

  switch (address & 0x0F) {
    case CXM0P:
      result = collCXM0P();
      break;

    case CXM1P:
      result = collCXM1P();
      break;

    case CXP0FB:
      result = collCXP0FB();
      break;

    case CXP1FB:
      result = collCXP1FB();
      break;

    case CXM0FB:
      result = collCXM0FB();
      break;

    case CXM1FB:
      result = collCXM1FB();
      break;

    case CXPPMM:
      result = collCXPPMM();
      break;

    case CXBLPF:
      result = collCXBLPF() | (lastDataBusValue & 0x40);
      break;

    case INPT0:
      updatePaddle(0);
      result = myPaddleReaders[0].inpt(myTimestamp) | (lastDataBusValue & 0x40);
      break;

    case INPT1:
      updatePaddle(1);
      result = myPaddleReaders[1].inpt(myTimestamp) | (lastDataBusValue & 0x40);
      break;

    case INPT2:
      updatePaddle(2);
      result = myPaddleReaders[2].inpt(myTimestamp) | (lastDataBusValue & 0x40);
      break;

    case INPT3:
      updatePaddle(3);
      result = myPaddleReaders[3].inpt(myTimestamp) | (lastDataBusValue & 0x40);
      break;

    case INPT4:
      result =
        myInput0.inpt(!myConsole.leftController().read(Controller::Six)) |
        (lastDataBusValue & 0x40);
      break;

    case INPT5:
      result =
        myInput1.inpt(!myConsole.rightController().read(Controller::Six)) |
        (lastDataBusValue & 0x40);
      break;

    default:
      result = lastDataBusValue;
  }

  return (result & 0xC0) | (lastDataBusValue & 0x3F);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::poke(uInt16 address, uInt8 value)
{
  updateEmulation();

  address &= 0x3F;

  switch (address)
  {
    case WSYNC:
      // It appears that the 6507 only halts during a read cycle so
      // we test here for follow-on writes which should be ignored as
      // far as halting the processor is concerned.
      // See issue #42 for more information.
      if (mySystem->m6502().lastAccessWasRead())
      {
        mySubClock += (228 - myHctr) % 228;
        mySystem->incrementCycles(mySubClock / 3);
        mySubClock %= 3;
      }
      myShadowRegisters[address] = value;
      break;

    case RSYNC:
      applyRsync();
      myShadowRegisters[address] = value;
      break;

    case VSYNC:
      myFrameManager.setVsync(value & 0x02);
      myShadowRegisters[address] = value;
      break;

    case VBLANK:
      myInput0.vblank(value);
      myInput1.vblank(value);

      for (PaddleReader& paddleReader : myPaddleReaders)
        paddleReader.vblank(value, myTimestamp);

      myDelayQueue.push(VBLANK, value, Delay::vblank);

      break;

    ////////////////////////////////////////////////////////////
    // FIXME - rework this when we add the new sound core
    case AUDV0:
      mySound.set(address, value, mySystem->cycles());
      myShadowRegisters[address] = value;
      break;
    case AUDV1:
      mySound.set(address, value, mySystem->cycles());
      myShadowRegisters[address] = value;
      break;
    case AUDF0:
      mySound.set(address, value, mySystem->cycles());
      myShadowRegisters[address] = value;
      break;
    case AUDF1:
      mySound.set(address, value, mySystem->cycles());
      myShadowRegisters[address] = value;
      break;
    case AUDC0:
      mySound.set(address, value, mySystem->cycles());
      myShadowRegisters[address] = value;
      break;
    case AUDC1:
      mySound.set(address, value, mySystem->cycles());
      myShadowRegisters[address] = value;
      break;
    ////////////////////////////////////////////////////////////

    case HMOVE:
      myDelayQueue.push(HMOVE, value, Delay::hmove);
      break;

    case COLUBK:
      myLinesSinceChange = 0;
      myBackground.setColor(value & 0xFE);
      myShadowRegisters[address] = value;
      break;

    case COLUP0:
      myLinesSinceChange = 0;
      value &= 0xFE;
      myPlayfield.setColorP0(value);
      myMissile0.setColor(value);
      myPlayer0.setColor(value);
      myShadowRegisters[address] = value;
      break;

    case COLUP1:
      myLinesSinceChange = 0;
      value &= 0xFE;
      myPlayfield.setColorP1(value);
      myMissile1.setColor(value);
      myPlayer1.setColor(value);
      myShadowRegisters[address] = value;
      break;

    case CTRLPF:
      myLinesSinceChange = 0;
      myPriority = (value & 0x04) ? Priority::pfp :
                   (value & 0x02) ? Priority::score : Priority::normal;
      myPlayfield.ctrlpf(value);
      myBall.ctrlpf(value);
      myShadowRegisters[address] = value;
      break;

    case COLUPF:
      myLinesSinceChange = 0;
      value &= 0xFE;
      myPlayfield.setColor(value);
      myBall.setColor(value);
      myShadowRegisters[address] = value;
      break;

    case PF0:
    {
      myDelayQueue.push(PF0, value, Delay::pf);
    #ifdef DEBUGGER_SUPPORT
      uInt16 dataAddr = mySystem->m6502().lastDataAddressForPoke();
      if(dataAddr)
        mySystem->setAccessFlags(dataAddr, CartDebug::PGFX);
    #endif
      break;
    }

    case PF1:
    {
      myDelayQueue.push(PF1, value, Delay::pf);
    #ifdef DEBUGGER_SUPPORT
      uInt16 dataAddr = mySystem->m6502().lastDataAddressForPoke();
      if(dataAddr)
        mySystem->setAccessFlags(dataAddr, CartDebug::PGFX);
    #endif
      break;
    }

    case PF2:
    {
      myDelayQueue.push(PF2, value, Delay::pf);
    #ifdef DEBUGGER_SUPPORT
      uInt16 dataAddr = mySystem->m6502().lastDataAddressForPoke();
      if(dataAddr)
        mySystem->setAccessFlags(dataAddr, CartDebug::PGFX);
    #endif
      break;
    }

    case ENAM0:
      myDelayQueue.push(ENAM0, value, Delay::enam);
      break;

    case ENAM1:
      myDelayQueue.push(ENAM1, value, Delay::enam);
      break;

    case RESM0:
      myLinesSinceChange = 0;
      myMissile0.resm(resxCounter(), myHstate == HState::blank);
      myShadowRegisters[address] = value;
      break;

    case RESM1:
      myLinesSinceChange = 0;
      myMissile1.resm(resxCounter(), myHstate == HState::blank);
      myShadowRegisters[address] = value;
      break;

    case RESMP0:
      myLinesSinceChange = 0;
      myMissile0.resmp(value, myPlayer0);
      myShadowRegisters[address] = value;
      break;

    case RESMP1:
      myLinesSinceChange = 0;
      myMissile1.resmp(value, myPlayer1);
      myShadowRegisters[address] = value;
      break;

    case NUSIZ0:
      myLinesSinceChange = 0;
      myMissile0.nusiz(value);
      myPlayer0.nusiz(value, myHstate == HState::blank);
      myShadowRegisters[address] = value;
      break;

    case NUSIZ1:
      myLinesSinceChange = 0;
      myMissile1.nusiz(value);
      myPlayer1.nusiz(value, myHstate == HState::blank);
      myShadowRegisters[address] = value;
      break;

    case HMM0:
      myDelayQueue.push(HMM0, value, Delay::hmm);
      break;

    case HMM1:
      myDelayQueue.push(HMM1, value, Delay::hmm);
      break;

    case HMCLR:
      myDelayQueue.push(HMCLR, value, Delay::hmclr);
      break;

    case GRP0:
    {
      myDelayQueue.push(GRP0, value, Delay::grp);
      myDelayQueue.push(DummyRegisters::shuffleP1, 0, Delay::shufflePlayer);
    #ifdef DEBUGGER_SUPPORT
      uInt16 dataAddr = mySystem->m6502().lastDataAddressForPoke();
      if(dataAddr)
        mySystem->setAccessFlags(dataAddr, CartDebug::GFX);
    #endif
      break;
    }

    case GRP1:
    {
      myDelayQueue.push(GRP1, value, Delay::grp);
      myDelayQueue.push(DummyRegisters::shuffleP0, 0, Delay::shufflePlayer);
      myDelayQueue.push(DummyRegisters::shuffleBL, 0, Delay::shuffleBall);
    #ifdef DEBUGGER_SUPPORT
      uInt16 dataAddr = mySystem->m6502().lastDataAddressForPoke();
      if(dataAddr)
        mySystem->setAccessFlags(dataAddr, CartDebug::GFX);
    #endif
      break;
    }

    case RESP0:
      myLinesSinceChange = 0;
      myPlayer0.resp(resxCounter());
      myShadowRegisters[address] = value;
      break;

    case RESP1:
      myLinesSinceChange = 0;
      myPlayer1.resp(resxCounter());
      myShadowRegisters[address] = value;
      break;

    case REFP0:
      myDelayQueue.push(REFP0, value, Delay::refp);
      break;

    case REFP1:
      myDelayQueue.push(REFP1, value, Delay::refp);
      break;

    case VDELP0:
      myLinesSinceChange = 0;
      myPlayer0.vdelp(value);
      myShadowRegisters[address] = value;
      break;

    case VDELP1:
      myLinesSinceChange = 0;
      myPlayer1.vdelp(value);
      myShadowRegisters[address] = value;
      break;

    case HMP0:
      myDelayQueue.push(HMP0, value, Delay::hmp);
      break;

    case HMP1:
      myDelayQueue.push(HMP1, value, Delay::hmp);
      break;

    case ENABL:
      myDelayQueue.push(ENABL, value, Delay::enabl);
      break;

    case RESBL:
      myLinesSinceChange = 0;
      myBall.resbl(resxCounter());
      myShadowRegisters[address] = value;
      break;

    case VDELBL:
      myLinesSinceChange = 0;
      myBall.vdelbl(value);
      myShadowRegisters[address] = value;
      break;

    case HMBL:
      myDelayQueue.push(HMBL, value, Delay::hmbl);
      break;

    case CXCLR:
      myLinesSinceChange = 0;
      myCollisionMask = 0;
      myShadowRegisters[address] = value;
      break;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::saveDisplay(Serializer& out) const
{
  try
  {
    out.putByteArray(myCurrentFrameBuffer.get(), 160*320);
  }
  catch(...)
  {
    cerr << "ERROR: TIA::saveDisplay" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::loadDisplay(Serializer& in)
{
  try
  {
    // Reset frame buffer pointer and data
    clearBuffers();
    in.getByteArray(myCurrentFrameBuffer.get(), 160*320);
    memcpy(myPreviousFrameBuffer.get(), myCurrentFrameBuffer.get(), 160*320);
  }
  catch(...)
  {
    cerr << "ERROR: TIA::loadDisplay" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::update()
{
  mySystem->m6502().execute(25000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::enableColorLoss(bool enabled)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::electronBeamPos(uInt32& x, uInt32& y) const
{
  uInt8 clocks = clocksThisLine();

  x = clocks < 68 ? 0 : clocks - 68;
  y = myFrameManager.getY();

  return isRendering();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::toggleBit(TIABit b, uInt8 mode)
{
  uInt8 mask;

  switch (mode) {
    case 0:
      mask = 0;
      break;

    case 1:
      mask = b;
      break;

    default:
      mask = (~mySpriteEnabledBits & b);
      break;
  }

  mySpriteEnabledBits = (mySpriteEnabledBits & ~b) | mask;

  myMissile0.toggleEnabled(mySpriteEnabledBits & TIABit::M0Bit);
  myMissile1.toggleEnabled(mySpriteEnabledBits & TIABit::M1Bit);
  myPlayer0.toggleEnabled(mySpriteEnabledBits & TIABit::P0Bit);
  myPlayer1.toggleEnabled(mySpriteEnabledBits & TIABit::P1Bit);
  myBall.toggleEnabled(mySpriteEnabledBits & TIABit::BLBit);
  myPlayfield.toggleEnabled(mySpriteEnabledBits & TIABit::PFBit);

  return mask;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::toggleBits()
{
  toggleBit(TIABit(0xFF), mySpriteEnabledBits > 0 ? 0 : 1);

  return mySpriteEnabledBits;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::toggleCollision(TIABit b, uInt8 mode)
{
  uInt8 mask;

  switch (mode) {
    case 0:
      mask = 0;
      break;

    case 1:
      mask = b;
      break;

    default:
      mask = (~myCollisionsEnabledBits & b);
      break;
  }

  myCollisionsEnabledBits = (myCollisionsEnabledBits & ~b) | mask;

  myMissile0.toggleCollisions(myCollisionsEnabledBits & TIABit::M0Bit);
  myMissile1.toggleCollisions(myCollisionsEnabledBits & TIABit::M1Bit);
  myPlayer0.toggleCollisions(myCollisionsEnabledBits & TIABit::P0Bit);
  myPlayer1.toggleCollisions(myCollisionsEnabledBits & TIABit::P1Bit);
  myBall.toggleCollisions(myCollisionsEnabledBits & TIABit::BLBit);
  myPlayfield.toggleCollisions(myCollisionsEnabledBits & TIABit::PFBit);

  return mask;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::toggleCollisions()
{
  toggleCollision(TIABit(0xFF), myCollisionsEnabledBits > 0 ? 0 : 1);

  return myCollisionsEnabledBits;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::toggleFixedColors(uInt8 mode)
{
  // If mode is 0 or 1, use it as a boolean (off or on)
  // Otherwise, flip the state
  bool on = (mode == 0 || mode == 1) ? bool(mode) : myColorHBlank == 0;

  bool pal = myFrameManager.layout() == FrameLayout::pal;
  myMissile0.setDebugColor(pal ? M0ColorPAL : M0ColorNTSC);
  myMissile1.setDebugColor(pal ? M1ColorPAL : M1ColorNTSC);
  myPlayer0.setDebugColor(pal ? P0ColorPAL : P0ColorNTSC);
  myPlayer1.setDebugColor(pal ? P1ColorPAL : P1ColorNTSC);
  myBall.setDebugColor(pal ? BLColorPAL : BLColorNTSC);
  myPlayfield.setDebugColor(pal ? PFColorPAL : PFColorNTSC);
  myBackground.setDebugColor(pal ? BKColorPAL : BKColorNTSC);

  myMissile0.enableDebugColors(on);
  myMissile1.enableDebugColors(on);
  myPlayer0.enableDebugColors(on);
  myPlayer1.enableDebugColors(on);
  myBall.enableDebugColors(on);
  myPlayfield.enableDebugColors(on);
  myBackground.enableDebugColors(on);
  myColorHBlank = on ? HBLANKColor : 0x00;

  return on;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::driveUnusedPinsRandom(uInt8 mode)
{
  // If mode is 0 or 1, use it as a boolean (off or on)
  // Otherwise, return the state
  if (mode == 0 || mode == 1)
  {
    myTIAPinsDriven = bool(mode);
    mySettings.setValue("tiadriven", myTIAPinsDriven);
  }
  return myTIAPinsDriven;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleJitter(uInt8 mode)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::setJitterRecoveryFactor(Int32 f)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanline()
{
  // Update frame by one scanline at a time
  uInt32 line = scanlines();
  while (line == scanlines())
    updateScanlineByStep();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanlineByStep()
{
  // Update frame by one CPU instruction/color clock
  mySystem->m6502().execute(1);
  updateEmulation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanlineByTrace(int target)
{
  while (mySystem->m6502().getPC() != target)
    updateScanlineByStep();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::registerValue(uInt8 reg) const
{
  return reg < 64 ? myShadowRegisters[reg] : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateEmulation()
{
  const uInt32 systemCycles = mySystem->cycles();

  if (mySubClock > 2)
    throw runtime_error("subclock exceeds range");

  const uInt32 cyclesToRun = 3 * (systemCycles - myLastCycle) + mySubClock;

  mySubClock = 0;
  myLastCycle = systemCycles;

  cycle(cyclesToRun);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::swapBuffers()
{
  myCurrentFrameBuffer.swap(myPreviousFrameBuffer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::onFrameStart()
{
  swapBuffers();

  const Int32 x = myHctr - 68;

  if (x > 0)
    memset(myCurrentFrameBuffer.get(), 0, x);

  for (uInt8 i = 0; i < 4; i++)
    updatePaddle(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::onFrameComplete()
{
  mySystem->m6502().stop();
  mySystem->resetCycles();

  // Blank out any extra lines not drawn this frame
  const uInt32 missingScanlines = myFrameManager.missingScanlines();
  if (missingScanlines > 0)
    memset(myCurrentFrameBuffer.get() + 160 * myFrameManager.getY(), 0, missingScanlines * 160);

  // Recalculate framerate, attempting to auto-correct for scanline 'jumps'
  if(myAutoFrameEnabled)
    myConsole.setFramerate(myFrameManager.frameRate());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::cycle(uInt32 colorClocks)
{
  for (uInt32 i = 0; i < colorClocks; i++)
  {
    myDelayQueue.execute(
      [this] (uInt8 address, uInt8 value) {delayedWrite(address, value);}
    );

    myCollisionUpdateRequired = false;

    tickMovement();

    if (myHstate == HState::blank)
      tickHblank();
    else
      tickHframe();

    if (++myHctr >= 228)
      nextLine();

    if (myCollisionUpdateRequired) updateCollision();

    myTimestamp++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::tickMovement()
{
  if (!myMovementInProgress) return;

  if ((myHctr & 0x03) == 0) {
    myLinesSinceChange = 0;

    const bool apply = myHstate == HState::blank;
    bool m = false;
    uInt8 movementCounter = myMovementClock > 15 ? 0 : myMovementClock;

    m = myMissile0.movementTick(movementCounter, myHctr, apply) || m;
    m = myMissile1.movementTick(movementCounter, myHctr, apply) || m;
    m = myPlayer0.movementTick(movementCounter, apply) || m;
    m = myPlayer1.movementTick(movementCounter, apply) || m;
    m = myBall.movementTick(movementCounter, apply) || m;

    myMovementInProgress = m;
    myCollisionUpdateRequired = m;

    myMovementClock++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::tickHblank()
{
  if (myIsFreshLine) {
    myHblankCtr = 0;
    myIsFreshLine = false;
  }

  if (++myHblankCtr >= 68) myHstate = HState::frame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::tickHframe()
{
  const uInt32 y = myFrameManager.getY();
  const bool lineNotCached = myLinesSinceChange < 2 || y == 0;
  const uInt32 x = myHctr - 68 - myXDelta;

  myCollisionUpdateRequired = lineNotCached;

  myPlayfield.tick(x);

  // Render sprites
  if (lineNotCached) {
    myPlayer0.render();
    myPlayer1.render();
    myMissile0.render(myHctr);
    myMissile1.render(myHctr);
    myBall.render();
  }

  // Tick sprites
  myMissile0.tick(myHctr);
  myMissile1.tick(myHctr);
  myPlayer0.tick();
  myPlayer1.tick();
  myBall.tick();

  if (myFrameManager.isRendering())
    renderPixel(x, y, lineNotCached);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::applyRsync()
{
  const uInt32 x = myHctr > 68 ? myHctr - 68 : 0;

  myXDelta = 157 - x;
  if (myFrameManager.isRendering())
    memset(myCurrentFrameBuffer.get() + myFrameManager.getY() * 160 + x, 0, 160 - x);

  myLinesSinceChange = 0;
  myHctr = 225;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::nextLine()
{
  myHctr = 0;
  myLinesSinceChange++;

  myHstate = HState::blank;
  myIsFreshLine = true;
  myExtendedHblank = false;
  myXDelta = 0;

  myFrameManager.nextLine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateCollision()
{
  myCollisionMask |= (
    myPlayer0.collision &
    myPlayer1.collision &
    myMissile0.collision &
    myMissile1.collision &
    myBall.collision &
    myPlayfield.collision
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::renderPixel(uInt32 x, uInt32 y, bool lineNotCached)
{
  if (x >= 160) return;

  if (lineNotCached) {
    uInt8 color = myBackground.getColor();

    switch (myPriority)
    {
      // Playfield has priority so ScoreBit isn't used
      // Priority from highest to lowest:
      //   BL/PF => P0/M0 => P1/M1 => BK
      case Priority::pfp:  // CTRLPF D2=1, D1=ignored
        color = myMissile1.getPixel(color);
        color = myPlayer1.getPixel(color);
        color = myMissile0.getPixel(color);
        color = myPlayer0.getPixel(color);
        color = myPlayfield.getPixel(color);
        color = myBall.getPixel(color);
        break;

      case Priority::score:  // CTRLPF D2=0, D1=1
        // Formally we have (priority from highest to lowest)
        //   PF/P0/M0 => P1/M1 => BL => BK
        // for the first half and
        //   P0/M0 => PF/P1/M1 => BL => BK
        // for the second half. However, the first ordering is equivalent
        // to the second (PF has the same color as P0/M0), so we can just
        // write
        color = myBall.getPixel(color);
        color = myMissile1.getPixel(color);
        color = myPlayer1.getPixel(color);
        color = myPlayfield.getPixel(color);
        color = myMissile0.getPixel(color);
        color = myPlayer0.getPixel(color);
        break;

      // Priority from highest to lowest:
      //   P0/M0 => P1/M1 => BL/PF => BK
      case Priority::normal:  // CTRLPF D2=0, D1=0
        color = myPlayfield.getPixel(color);
        color = myBall.getPixel(color);
        color = myMissile1.getPixel(color);
        color = myPlayer1.getPixel(color);
        color = myMissile0.getPixel(color);
        color = myPlayer0.getPixel(color);
        break;
    }

    myCurrentFrameBuffer.get()[y * 160 + x] = myFrameManager.vblank() ? 0 : color;
  } else {
    myCurrentFrameBuffer.get()[y * 160 + x] = myCurrentFrameBuffer.get()[(y-1) * 160 + x];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::clearHmoveComb()
{
  if (myFrameManager.isRendering() && myHstate == HState::blank)
    memset(myCurrentFrameBuffer.get() + myFrameManager.getY() * 160,
           myColorHBlank, 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::delayedWrite(uInt8 address, uInt8 value)
{
  if (address < 64)
    myShadowRegisters[address] = value;

  switch (address)
  {
    case VBLANK:
      myLinesSinceChange = 0;
      myFrameManager.setVblank(value & 0x02);
      break;

    case HMOVE:
      myLinesSinceChange = 0;

      myMovementClock = 0;
      myMovementInProgress = true;

      if (!myExtendedHblank) {
        myHblankCtr -= 8;
        clearHmoveComb();
        myExtendedHblank = true;
      }

      myMissile0.startMovement();
      myMissile1.startMovement();
      myPlayer0.startMovement();
      myPlayer1.startMovement();
      myBall.startMovement();
      break;

    case PF0:
      myLinesSinceChange = 0;
      myPlayfield.pf0(value);
      break;

    case PF1:
      myLinesSinceChange = 0;
      myPlayfield.pf1(value);
      break;

    case PF2:
      myLinesSinceChange = 0;
      myPlayfield.pf2(value);
      break;

    case HMM0:
      myLinesSinceChange = 0;
      myMissile0.hmm(value);
      break;

    case HMM1:
      myLinesSinceChange = 0;
      myMissile1.hmm(value);
      break;

    case HMCLR:
      myLinesSinceChange = 0;
      myMissile0.hmm(0);
      myMissile1.hmm(0);
      myPlayer0.hmp(0);
      myPlayer1.hmp(0);
      myBall.hmbl(0);
      break;

    case GRP0:
      myLinesSinceChange = 0;
      myPlayer0.grp(value);
      break;

    case GRP1:
      myLinesSinceChange = 0;
      myPlayer1.grp(value);
      break;

    case DummyRegisters::shuffleP0:
      myLinesSinceChange = 0;
      myPlayer0.shufflePatterns();
      break;

    case DummyRegisters::shuffleP1:
      myLinesSinceChange = 0;
      myPlayer1.shufflePatterns();
      break;

    case DummyRegisters::shuffleBL:
      myLinesSinceChange = 0;
      myBall.shuffleStatus();
      break;

    case HMP0:
      myLinesSinceChange = 0;
      myPlayer0.hmp(value);
      break;

    case HMP1:
      myLinesSinceChange = 0;
      myPlayer1.hmp(value);
      break;

    case HMBL:
      myLinesSinceChange = 0;
      myBall.hmbl(value);
      break;

    case REFP0:
      myLinesSinceChange = 0;
      myPlayer0.refp(value);
      break;

    case REFP1:
      myLinesSinceChange = 0;
      myPlayer1.refp(value);
      break;

    case ENABL:
      myLinesSinceChange = 0;
      myBall.enabl(value);
      break;

    case ENAM0:
      myLinesSinceChange = 0;
      myMissile0.enam(value);
      break;

    case ENAM1:
      myLinesSinceChange = 0;
      myMissile1.enam(value);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updatePaddle(uInt8 idx)
{
  static constexpr double MAX_RESISTANCE = 1400000;

  Int32 resistance;
  switch (idx) {
    case 0:
      resistance = myConsole.leftController().read(Controller::Nine);
      break;

    case 1:
      resistance = myConsole.leftController().read(Controller::Five);
      break;

    case 2:
      resistance = myConsole.rightController().read(Controller::Nine);
      break;

    case 3:
      resistance = myConsole.rightController().read(Controller::Five);
      break;

    default:
      throw runtime_error("invalid paddle index");
  }

  myPaddleReaders[idx].update(
    (resistance == Controller::maximumResistance ? -1 : double(resistance)) / MAX_RESISTANCE,
    myTimestamp,
    myFrameManager.layout()
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::resxCounter()
{
  return myHstate == HState::blank ?
    (myHctr >= resxLateHblankThreshold ? ResxCounter::lateHblank : ResxCounter::hblank) : ResxCounter::frame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXM0P() const
{
  return (
    ((myCollisionMask & CollisionMask::missile0 & CollisionMask::player0) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::missile0 & CollisionMask::player1) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXM1P() const
{
  return (
    ((myCollisionMask & CollisionMask::missile1 & CollisionMask::player1) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::missile1 & CollisionMask::player0) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXP0FB() const
{
  return (
    ((myCollisionMask & CollisionMask::player0 & CollisionMask::ball) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::player0 & CollisionMask::playfield) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXP1FB() const
{
  return (
    ((myCollisionMask & CollisionMask::player1 & CollisionMask::ball) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::player1 & CollisionMask::playfield) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXM0FB() const
{
  return (
    ((myCollisionMask & CollisionMask::missile0 & CollisionMask::ball) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::missile0 & CollisionMask::playfield) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXM1FB() const
{
  return (
    ((myCollisionMask & CollisionMask::missile1 & CollisionMask::ball) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::missile1 & CollisionMask::playfield) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXPPMM() const
{
  return (
    ((myCollisionMask & CollisionMask::missile0 & CollisionMask::missile1) ? 0x40 : 0) |
    ((myCollisionMask & CollisionMask::player0 & CollisionMask::player1) ? 0x80 : 0)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::collCXBLPF() const
{
  return (myCollisionMask & CollisionMask::ball & CollisionMask::playfield) ? 0x80 : 0;
}
