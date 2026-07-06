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

#ifndef HEADLESS_RUNNER_HXX
#define HEADLESS_RUNNER_HXX

#include <ostream>
#include <streambuf>

#include "bspf.hxx"
#include "FrameLayout.hxx"
#include "ConsoleIO.hxx"
#include "Random.hxx"
#include "Event.hxx"
#include "Cart.hxx"
#include "Settings.hxx"
#include "Props.hxx"
#include "FrameManager.hxx"
#include "EmulationTiming.hxx"
#include "M6502.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "System.hxx"
#include "Switches.hxx"
#include "Joystick.hxx"

class HeadlessRunner {
  public:

    enum class CartridgeLoadResult {
      success, noFile, readFailed, badType
    };

    enum class TvStandard {
      pal, secam, ntsc, pal60, ntsc50, secam60
    };

  public:
    HeadlessRunner();

    CartridgeLoadResult loadCartridge(const FSNode& file, string_view bankingType = "");
    HeadlessRunner& setRngSeed(uInt32 seed);
    HeadlessRunner& setTvStandard(std::optional<TvStandard> tvStandard);
    HeadlessRunner& setLogToStdout(bool logToStdout);

    void init();
    bool run(uInt64 cyclesTarget);

    uInt64 getCycles() const;
    TvStandard getTvStandard() const;
    uInt32 getRngSeed() const;
    EmulationTiming& getEmulationTiming();

  protected:

    std::ostream& logStream();
    void assertCreated() const;
    void assertInitialized() const;

  private:

    struct IO: public ConsoleIO {
      Controller& leftController() const override { return *myLeftControl; }
      Controller& rightController() const override { return *myRightControl; }
      Switches& switches() const override { return *mySwitches; }

      unique_ptr<Controller> myLeftControl;
      unique_ptr<Controller> myRightControl;
      unique_ptr<Switches> mySwitches;
    };

    class StreambufNull: public std::streambuf {
        public:
        StreambufNull() = default;

      protected:
        int overflow(int c) override { return c; }
    };

    enum class State {
      created, initialized, finished
    };

  private:

    State myState{State::created};
    uInt32 myRngSeed{0};
    uInt64 myCycles{0};
    std::optional<TvStandard> myTvStandard;

    bool myLogToStdout{true};

    Settings mySettings;
    Random myRng;
    IO myConsoleIO;
    Event myEvent;
    Properties myProps;
    FrameManager myFrameManager;
    EmulationTiming myEmulationTiming;
    ConsoleTiming myConsoleTiming{ConsoleTiming::ntsc};

    unique_ptr<Cartridge> myCartridge;
    unique_ptr<M6502> myCpu;
    unique_ptr<M6532> myRiot;
    unique_ptr<TIA> myTia;
    unique_ptr<System> mySystem;

    StreambufNull myStreambufNull;
    std::ostream myNullStream;

  private:
    HeadlessRunner(const HeadlessRunner&) = delete;
    HeadlessRunner(HeadlessRunner&&) = delete;
    HeadlessRunner& operator=(const HeadlessRunner&) = delete;
    HeadlessRunner& operator=(HeadlessRunner&&) = delete;
};

#endif // HEADLESS_RUNNER_HXX
