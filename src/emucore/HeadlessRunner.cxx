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

#include <iostream>

#include "HeadlessRunner.hxx"
#include "CartCreator.hxx"

#include "MD5.hxx"
#include "System.hxx"
#include "FrameLayoutDetector.hxx"
#include "DispatchResult.hxx"

namespace {
  string frameLayoutDescription(FrameLayout layout) {
    switch (layout) {
      case FrameLayout::pal:
        return "PAL";

      case FrameLayout::ntsc:
        return "NTSC";

      case FrameLayout::pal60:
        return "PAL60";

      case FrameLayout::ntsc50:
        return "NTSC50";

      default:
        std::unreachable();
    }
  }

  FrameLayout frameLayoutFromTvStandard(HeadlessRunner::TvStandard tvStandard) {
    switch (tvStandard) {
      case HeadlessRunner::TvStandard::pal:
      case HeadlessRunner::TvStandard::secam:
        return FrameLayout::pal;

      case HeadlessRunner::TvStandard::ntsc:
        return FrameLayout::ntsc;

      case HeadlessRunner::TvStandard::pal60:
      case HeadlessRunner::TvStandard::secam60:
        return FrameLayout::pal60;

      case HeadlessRunner::TvStandard::ntsc50:
        return FrameLayout::ntsc50;

      default:
        std::unreachable();
    }
  }

  HeadlessRunner::TvStandard tvStandardFromFrameLayout(FrameLayout layout) {
    switch (layout) {
      case FrameLayout::pal:
        return HeadlessRunner::TvStandard::pal;

      case FrameLayout::ntsc:
        return HeadlessRunner::TvStandard::ntsc;

      case FrameLayout::pal60:
        return HeadlessRunner::TvStandard::pal60;

      case FrameLayout::ntsc50:
        return HeadlessRunner::TvStandard::ntsc50;

      default:
        std::unreachable();
    }
  }

  ConsoleTiming consoleTimingFromTvStandard(HeadlessRunner::TvStandard tvStandard) {
    switch (tvStandard) {
      case HeadlessRunner::TvStandard::pal:
      case HeadlessRunner::TvStandard::pal60:
        return ConsoleTiming::pal;

      case HeadlessRunner::TvStandard::secam:
      case HeadlessRunner::TvStandard::secam60:
        return ConsoleTiming::secam;

      case HeadlessRunner::TvStandard::ntsc:
      case HeadlessRunner::TvStandard::ntsc50:
        return ConsoleTiming::ntsc;

      default:
        std::unreachable();
    }
  }

  void updateProgress(uInt32 from, uInt32 to) {
    while (from < to) {
      if (from % 10 == 0 && from > 0) cout << from << "%";
      else cout << ".";

      cout.flush();

      from++;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeadlessRunner::HeadlessRunner() : myNullStream(&myStreambufNull)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeadlessRunner::CartridgeLoadResult HeadlessRunner::loadCartridge(const FSNode& file,  string_view bankingType)
{
  assertCreated();
  if(myCartridge) throw std::runtime_error("cartridge already loaded");

  if (!file.isFile()) return CartridgeLoadResult::noFile;

  ByteArray image;
  file.read(image);
  if (image.size() == 0) return CartridgeLoadResult::readFailed;

  string md5 = MD5::hash(image);
  myCartridge = CartCreator::create(file, image, md5, bankingType, mySettings);

  return myCartridge ? CartridgeLoadResult::success : CartridgeLoadResult::badType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeadlessRunner& HeadlessRunner::setRngSeed(uInt32 seed)
{
  assertCreated();
  myRngSeed = seed;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeadlessRunner& HeadlessRunner::setTvStandard(std::optional<TvStandard> layout)
{
  assertCreated();
  myTvStandard = layout;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeadlessRunner& HeadlessRunner::setLogToStdout(bool logToStdout)
{
    myLogToStdout = logToStdout;

    return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeadlessRunner::init()
{
  assertCreated();

  if (!myCartridge) throw std::runtime_error("cartridge not loaded");

  mySettings.setValue("fastscbios", true);
  myRng.initSeed(myRngSeed);

  myCpu = std::make_unique<M6502>(mySettings);
  myRiot = std::make_unique<M6532>(myConsoleIO, mySettings);

  const TIA::onPhosphorCallback phosphorCallback = [] (bool enable) {};
  const TIA::ConsoleTimingProvider timingProvider = [&]() { return myConsoleTiming; };

  myTia = std::make_unique<TIA>(myConsoleIO, timingProvider, mySettings, phosphorCallback);
  mySystem = std::make_unique<System>(myRng, *myCpu, *myRiot, *myTia, *myCartridge);

  myConsoleIO.myLeftControl = std::make_unique<Joystick>(Controller::Jack::Left, myEvent, *mySystem);
  myConsoleIO.myRightControl = std::make_unique<Joystick>(Controller::Jack::Right, myEvent, *mySystem);
  myConsoleIO.mySwitches = std::make_unique<Switches>(myEvent, myProps, mySettings);

  myTia->bindToControllers();
  myTia->setFrameManager(&myFrameManager);
  myCartridge->setStartBankFromPropsFunc([]() { return -1; });
  mySystem->initialize();

  if (!myTvStandard.has_value()) {
    logStream() << "detecting frame layout... ";

    FrameLayoutDetector frameLayoutDetector;
    myTia->setFrameManager(&frameLayoutDetector);
    mySystem->reset();

    for (int i = 0; i < 60; ++i) myTia->update();

    myTvStandard = tvStandardFromFrameLayout(frameLayoutDetector.detectedLayout());
    logStream() << frameLayoutDescription(frameLayoutDetector.detectedLayout()) << std::endl;

    myTia->setFrameManager(&myFrameManager);
  }

  myFrameManager.setLayout(frameLayoutFromTvStandard(*myTvStandard));
  myConsoleTiming = consoleTimingFromTvStandard(*myTvStandard);
  myEmulationTiming
    .updateFrameLayout(frameLayoutFromTvStandard(*myTvStandard))
    .updateConsoleTiming(myConsoleTiming);

  mySystem->reset();

  myState = State::initialized;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HeadlessRunner::run(uInt64 cyclesTarget)
{
  assertInitialized();

  DispatchResult dispatchResult;
  dispatchResult.setOk(0);

  myCycles = 0;
  uInt32 percent = 0;

  logStream() << "0%" << std::flush;

  while (myCycles < cyclesTarget && dispatchResult.getStatus() == DispatchResult::Status::ok) {
    myTia->update(dispatchResult);
    myCycles += dispatchResult.getCycles();

    if (myTia->newFramePending()) myTia->renderToFrameBuffer();

    const uInt32 percentNow = static_cast<uInt32>(std::min((100 * myCycles) /
      cyclesTarget, static_cast<uInt64>(100)));

    if(myLogToStdout) updateProgress(percent, percentNow);

    percent = percentNow;
  }

  myState = State::finished;

  if (dispatchResult.getStatus() != DispatchResult::Status::ok) {
    logStream() << std::endl << "ERROR: emulation failed after " << myCycles << " cycles" << std::endl;
    return false;
  }

  logStream() << "100%" << std::endl;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 HeadlessRunner::getCycles() const
{
  return myCycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeadlessRunner::TvStandard HeadlessRunner::getTvStandard() const
{
  return myTvStandard.value_or(TvStandard::ntsc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 HeadlessRunner::getRngSeed() const
{
  return myRngSeed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming& HeadlessRunner::getEmulationTiming()
{
  return myEmulationTiming;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ostream& HeadlessRunner::logStream()
{
  return myLogToStdout ? cout : myNullStream;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeadlessRunner::assertCreated() const
{
  if(myState != State::created) throw std::runtime_error("runner already initialized");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeadlessRunner::assertInitialized() const
{
  if(myState == State::created) throw std::runtime_error("runner not initialized");
  if(myState == State::finished) throw std::runtime_error("runner already finished");
}
