#include "OSystemRTStella.hxx"

#include <sstream>
#include <sched.h>
#include <sys/sysinfo.h>

#include "Logger.hxx"
#include "RTEmulationWorker.hxx"
#include "EventHandler.hxx"
#include "TimerManager.hxx"
#include "DispatchResult.hxx"
#include "Console.hxx"
#include "Debugger.hxx"
#include "TIA.hxx"

namespace {
  void configureScheduler() {
    const int cores = get_nprocs();
    if (cores < 2) {
      Logger::error("failed to set scheduling affinity on main thread - not enough cores");
      return;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < cores - 1; i++) CPU_SET(i, &cpuset);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) < 0){
      ostringstream ss;
      ss << "failed to pin main and auxiliary thread: " << errno;

      Logger::error(ss.str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystemRTStella::initialize(const Settings::Options& options)
{
  configureScheduler();

  return OSystemStandalone::initialize(options);
}

void OSystemRTStella::mainLoop()
{
  RTEmulationWorker worker;
  DispatchResult dispatchResult;

  cout << "starting rtstella main loop..." << endl << std::flush;

  for (;;) {
    TIA& tia = myConsole->tia();

    if (worker.isRunning()) worker.suspend();

    const EventHandlerState oldState = myEventHandler->state();
    const bool workerWasRunning = worker.isRunning();

    if (oldState == EventHandlerState::EMULATION && worker.isRunning()) {
      tia.renderToFrameBuffer();

      switch (dispatchResult.getStatus()) {
        case DispatchResult::Status::ok:
        break;

        case DispatchResult::Status::debugger:
          #ifdef DEBUGGER_SUPPORT
           myDebugger->start(
              dispatchResult.getMessage(),
              dispatchResult.getAddress(),
              dispatchResult.wasReadTrap(),
              dispatchResult.getToolTip()
            );
          #endif

          break;

        case DispatchResult::Status::fatal:
          #ifdef DEBUGGER_SUPPORT
            myDebugger->startWithFatalError(dispatchResult.getMessage());
          #else
            cerr << dispatchResult.getMessage() << endl;
          #endif
            break;

        default:
          cout << (int)dispatchResult.getStatus() << endl << std::flush;
          throw runtime_error("invalid emulation dispatch result");
      }
    }

    myEventHandler->poll(TimerManager::getTicks());
    if (myQuitLoop) break;

    if (dispatchResult.getStatus() == DispatchResult::Status::ok && myEventHandler->frying() && worker.isRunning())
      myConsole->fry();

    const EventHandlerState newState = myEventHandler->state();

    if (newState == EventHandlerState::EMULATION && !worker.isRunning()) {
      worker.start(myConsole->emulationTiming().cyclesPerSecond(), &dispatchResult, &tia);
      myFpsMeter.reset();
    } else if (newState != EventHandlerState::EMULATION && worker.isRunning()) {
      worker.stop();
    } else if (newState == EventHandlerState::EMULATION && worker.isRunning()) {
      worker.resume();
    }

    if (oldState == EventHandlerState::EMULATION && workerWasRunning)
      myFrameBuffer->updateInEmulationMode(myFpsMeter.fps());
    else
      myFrameBuffer->update();
  }
}
