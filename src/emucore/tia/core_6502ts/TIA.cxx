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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "TIA.hxx"
#include "TIATypes.hxx"
#include "M6502.hxx"

enum CollisionMask: uInt32 {
  player0 =       0b0111110000000000,
  player1 =       0b0100001111000000,
  missile0 =      0b0010001000111000,
  missile1 =      0b0001000100100110,
  ball =          0b0000100010010101,
  playfield =     0b0000010001001011
};

enum Delay: uInt8 {
  hmove = 6,
  pf = 2,
  grp = 1,
  shufflePlayer = 1,
  hmp = 2,
  hmm = 2,
  hmbl = 2,
  hmclr = 2
};

enum DummyRegisters: uInt8 {
  shuffleP0 = 0xF0,
  shuffleP1 = 0xF1
};

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(Console& console, Sound& sound, Settings& settings)
  : myConsole(console),
    mySound(sound),
    mySettings(settings),
    myDelayQueue(10, 20),
    myPlayfield(CollisionMask::playfield),
    myMissile0(CollisionMask::missile0),
    myMissile1(CollisionMask::missile1),
    myPlayer0(CollisionMask::player0),
    myPlayer1(CollisionMask::player1),
    myBall(CollisionMask::ball)
{
  myFrameManager.setHandlers(
    [this] () {
      myCurrentFrameBuffer.swap(myPreviousFrameBuffer);
    },
    [this] () {
      mySystem->m6502().stop();
    }
  );

  myCurrentFrameBuffer  = make_ptr<uInt8[]>(160 * 320);
  myPreviousFrameBuffer = make_ptr<uInt8[]>(160 * 320);

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
  myColorBk = 0;

  myLastCycle = 0;

  myPlayfield.reset();
  myMissile0.reset();
  myMissile1.reset();
  myPlayer0.reset();
  myPlayer1.reset();
  myBall.reset();

  mySound.reset();
  myDelayQueue.reset();
  myFrameManager.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::systemCyclesReset()
{
  const uInt32 cycles = mySystem->cycles();

  myLastCycle -= cycles;
  mySound.adjustCycleCounter(-cycles);
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
// TODO: stub
bool TIA::save(Serializer& out) const
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::load(Serializer& in)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::peek(uInt16 address)
{
  updateEmulation();

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::poke(uInt16 address, uInt8 value)
{
  updateEmulation();

  address &= 0x3F;

  switch (address)
  {
    case WSYNC:
      // TODO: Make sure that we understand the +1... :)
      mySystem->incrementCycles((227 - myHctr) / 3 + 1);
      break;

    case VSYNC:
      myFrameManager.setVsync(value & 0x02);
      break;

    case VBLANK:
      myLinesSinceChange = 0;
      myFrameManager.setVblank(value & 0x02);
      break;

    case AUDV0:
    case AUDV1:
    case AUDF0:
    case AUDF1:
    case AUDC0:
    case AUDC1:
      mySound.set(address, value, mySystem->cycles());
      break;

    case HMOVE:
      myDelayQueue.push(HMOVE, value, Delay::hmove);
      break;

    case COLUBK:
      myLinesSinceChange = 0;
      myColorBk = value & 0xFE;
      break;

    case COLUP0:
      myLinesSinceChange = 0;
      value &= 0xFE;
      myPlayfield.setColorP0(value);
      myMissile0.setColor(value);
      myPlayer0.setColor(value);
      break;

    case COLUP1:
      myLinesSinceChange = 0;
      value &= 0xFE;
      myPlayfield.setColorP1(value);
      myMissile1.setColor(value);
      myPlayer1.setColor(value);
      break;

    case CTRLPF:
      myLinesSinceChange = 0;
      myPriority = (value & 0x04) ? Priority::inverted : Priority::normal;
      myPlayfield.ctrlpf(value);
      myBall.ctrlpf(value);
      break;

    case COLUPF:
      myLinesSinceChange = 0;
      value &= 0xFE;
      myPlayfield.setColor(value);
      myBall.setColor(value);
      break;

    case PF0:
      myDelayQueue.push(PF0, value, Delay::pf);
      break;

    case PF1:
      myDelayQueue.push(PF1, value, Delay::pf);
      break;

    case PF2:
      myDelayQueue.push(PF2, value, Delay::pf);
      break;

    case ENAM0:
      myLinesSinceChange = 0;
      myMissile0.enam(value);
      break;

    case ENAM1:
      myLinesSinceChange = 0;
      myMissile1.enam(value);
      break;

    case RESM0:
      myLinesSinceChange = 0;
      myMissile0.resm(myHstate == HState::blank);
      break;

    case RESM1:
      myLinesSinceChange = 0;
      myMissile1.resm(myHstate == HState::blank);
      break;

    case NUSIZ0:
      myLinesSinceChange = 0;
      myMissile0.nusiz(value);
      myPlayer0.nusiz(value);
      break;

    case NUSIZ1:
      myLinesSinceChange = 0;
      myMissile1.nusiz(value);
      myPlayer1.nusiz(value);
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
      myDelayQueue.push(GRP0, value, Delay::grp);
      myDelayQueue.push(DummyRegisters::shuffleP1, 0, Delay::shufflePlayer);
      break;

    case GRP1:
      myLinesSinceChange = 0;
      myDelayQueue.push(GRP1, value, Delay::grp);
      myDelayQueue.push(DummyRegisters::shuffleP0, 0, Delay::shufflePlayer);
      myBall.shuffleStatus();
      break;

    case RESP0:
      myLinesSinceChange = 0;
      myPlayer0.resp(myHstate == HState::blank);
      break;

    case RESP1:
      myLinesSinceChange = 0;
      myPlayer1.resp(myHstate == HState::blank);
      break;

    case REFP0:
      myLinesSinceChange = 0;
      myPlayer0.refp(value);
      break;

    case REFP1:
      myLinesSinceChange = 0;
      myPlayer1.refp(value);
      break;

    case VDELP0:
      myLinesSinceChange = 0;
      myPlayer0.vdelp(value);
      break;

    case VDELP1:
      myLinesSinceChange = 0;
      myPlayer1.vdelp(value);
      break;

    case HMP0:
      myDelayQueue.push(HMP0, value, Delay::hmp);
      break;

    case HMP1:
      myDelayQueue.push(HMP1, value, Delay::hmp);
      break;

    case ENABL:
      myLinesSinceChange = 0;
      myBall.enabl(value);
      break;

    case RESBL:
      myLinesSinceChange = 0;
      myBall.resbl(myHstate == HState::blank);
      break;

    case VDELBL:
      myLinesSinceChange = 0;
      myBall.vdelbl(value);
      break;

    case HMBL:
      myDelayQueue.push(HMBL, value, Delay::hmbl);
      break;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::frameReset()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::saveDisplay(Serializer& out) const
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::loadDisplay(Serializer& in)
{
  return false;
}

// TODO: stub
void TIA::update()
{
  mySystem->m6502().execute(25000);
}

uInt8* TIA::currentFrameBuffer() const
{
  return myCurrentFrameBuffer.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
uInt8* TIA::previousFrameBuffer() const
{
  return myPreviousFrameBuffer.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::height() const
{
  return myFrameManager.height();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::ystart() const
{
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::setHeight(uInt32 height)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::setYStart(uInt32 ystart)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::enableAutoFrame(bool enabled)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
void TIA::enableColorLoss(bool enabled)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::isPAL() const
{
  return myFrameManager.tvMode() == FrameManager::TvMode::pal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
uInt32 TIA::clocksThisLine() const
{
  return 0;
}

// TODO: stub
uInt32 TIA::scanlines() const
{
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::partialFrame() const
{
  return myFrameManager.isRendering();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
uInt32 TIA::startScanline() const
{
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::scanlinePos(uInt16& x, uInt16& y) const
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleBit(TIABit b, uInt8 mode)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleBits()
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleCollision(TIABit b, uInt8 mode)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleCollisions()
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleHMOVEBlank()
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::toggleFixedColors(uInt8 mode)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: stub
bool TIA::driveUnusedPinsRandom(uInt8 mode)
{
  return false;
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
void TIA::updateEmulation()
{
  const uInt32 cycles = mySystem->cycles();

  cycle(3 * (cycles - myLastCycle));

  myLastCycle = cycles;
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

    if (myCollisionUpdateRequired) updateCollision();
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

    m = myMissile0.movementTick(myMovementClock, apply) || m;
    m = myMissile1.movementTick(myMovementClock, apply) || m;
    m = myPlayer0.movementTick(myMovementClock, apply) || m;
    m = myPlayer1.movementTick(myMovementClock, apply) || m;
    m = myBall.movementTick(myMovementClock, apply) || m;

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

  myHctr++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::tickHframe()
{
  const uInt32 y = myFrameManager.currentLine();
  const bool lineNotCached = myLinesSinceChange < 2 || y == 0;
  const uInt32 x = myHctr - 68;

  myCollisionUpdateRequired = lineNotCached;

  myPlayfield.tick(x);

  if (lineNotCached)
    renderSprites();

  tickSprites();

  if (myFrameManager.isRendering()) renderPixel(x, y, lineNotCached);

  if (++myHctr >= 228) nextLine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::renderSprites()
{
  myPlayer0.render();
  myPlayer1.render();
  myMissile0.render();
  myMissile1.render();
  myBall.render();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::tickSprites()
{
  myMissile0.tick();
  myMissile1.tick();
  myPlayer0.tick();
  myPlayer1.tick();
  myBall.tick();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::nextLine()
{
  myHctr = 0;
  myLinesSinceChange++;

  myHstate = HState::blank;
  myIsFreshLine = true;
  myExtendedHblank = false;

  myFrameManager.nextLine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateCollision()
{
  // TODO: update collision mask with sprite masks
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::renderPixel(uInt32 x, uInt32 y, bool lineNotCached)
{
  if (lineNotCached) {
    uInt8 color = myColorBk;

    if (myPriority == Priority::normal) {
      color = myPlayfield.getPixel(color);
      color = myBall.getPixel(color);
      color = myMissile1.getPixel(color);
      color = myPlayer1.getPixel(color);
      color = myMissile0.getPixel(color);
      color = myPlayer0.getPixel(color);
    } else {
      color = myMissile1.getPixel(color);
      color = myPlayer1.getPixel(color);
      color = myMissile0.getPixel(color);
      color = myPlayer0.getPixel(color);
      color = myPlayfield.getPixel(color);
      color = myBall.getPixel(color);
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
    memset(myCurrentFrameBuffer.get() + myFrameManager.currentLine() * 160, 0, 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::delayedWrite(uInt8 address, uInt8 value)
{
  switch (address)
  {
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
  }
}

} // namespace TIA6502tsCore
