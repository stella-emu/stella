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

namespace TIA6502tsCore {

// TODO: stub
TIA::TIA(Console& console, Sound& sound, Settings& settings)
  : myConsole(console),
    mySound(sound),
    mySettings(settings),
    myDelayQueue(10, 20)
{
  myFrameManager.setOnFrameCompleteHandler(
    [this] () {onFrameComplete();}
  );

  reset();
}

// TODO: stub
void TIA::reset()
{
  myDelayQueue.reset();
}

// TODO: stub
void TIA::systemCyclesReset()
{}

// TODO: stub
void TIA::install(System& system)
{}

// TODO: stub
bool TIA::save(Serializer& out) const
{
  return true;
}

// TODO: stub
bool TIA::load(Serializer& in)
{
  return true;
}

// TODO: stub
uInt8 TIA::peek(uInt16 address)
{
  return 0;
}

// TODO: stub
bool TIA::poke(uInt16 address, uInt8 value)
{
  return false;
}

// TODO: stub
void TIA::installDelegate(System& system, Device& device)
{}

// TODO: stub
void TIA::frameReset()
{}

// TODO: stub
bool TIA::saveDisplay(Serializer& out) const
{
  return true;
}

// TODO: stub
bool TIA::loadDisplay(Serializer& in)
{
  return true;
}

// TODO: stub
void TIA::update()
{}

// TODO: stub
uInt8* TIA::currentFrameBuffer() const
{
  return 0;
}

// TODO: stub
uInt8* TIA::previousFrameBuffer() const
{
  return 0;
}

// TODO: stub
uInt32 TIA::height() const
{
  return 0;
}

// TODO: stub
uInt32 TIA::ystart() const
{
  return 0;
}

// TODO: stub
void TIA::setHeight(uInt32 height)
{}

// TODO: stub
void TIA::setYStart(uInt32 ystart)
{}

// TODO: stub
void TIA::enableAutoFrame(bool enabled)
{}

// TODO: stub
void TIA::enableColorLoss(bool enabled)
{}

// TODO: stub
bool TIA::isPAL() const
{
  return false;
}

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

// TODO: stub
bool TIA::partialFrame() const
{
  return false;
}

// TODO: stub
uInt32 TIA::startScanline() const
{
  return 0;
}

// TODO: stub
bool TIA::scanlinePos(uInt16& x, uInt16& y) const
{
  return 0;
}

// TODO: stub
bool TIA::toggleBit(TIABit b, uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::toggleBits()
{
  return false;
}

// TODO: stub
bool TIA::toggleCollision(TIABit b, uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::toggleCollisions()
{
  return false;
}

// TODO: stub
bool TIA::toggleHMOVEBlank()
{
  return false;
}

// TODO: stub
bool TIA::toggleFixedColors(uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::driveUnusedPinsRandom(uInt8 mode)
{
  return false;
}

// TODO: stub
bool TIA::toggleJitter(uInt8 mode)
{
  return false;
}

// TODO: stub
void TIA::setJitterRecoveryFactor(Int32 f)
{}

// TODO: stub
void TIA::onFrameComplete()
{}

// TODO: stub
void TIA::delayedWrite(uInt8 address, uInt8 value)
{}

} // namespace TIA6502tsCore