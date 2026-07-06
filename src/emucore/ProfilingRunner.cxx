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

#include <chrono>
#include <cstddef>

#include "ProfilingRunner.hxx"
#include "FSNode.hxx"
#include "HeadlessRunner.hxx"

using namespace std::chrono;

namespace {
  constexpr uInt32 RUNTIME_DEFAULT = 60;
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProfilingRunner::ProfilingRunner(int argc, char* argv[])
  : profilingRuns{std::max<size_t>(argc - 2, 0)}
{
  for (int i = 2; i < argc; i++) {
    ProfilingRun& run(profilingRuns[i-2]);

    const string arg = argv[i];
    const size_t splitPoint = arg.find_first_of(':');

    run.romFile = splitPoint == string::npos ? arg : arg.substr(0, splitPoint);

    if (splitPoint == string::npos) run.runtime = RUNTIME_DEFAULT;
    else  {
      const int runtime = BSPF::stoi(arg.substr(splitPoint+1, string::npos));
      run.runtime = runtime > 0 ? runtime : RUNTIME_DEFAULT;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProfilingRunner::run()
{
  cout << "Profiling Stella...\n";

  for (const ProfilingRun& run : profilingRuns) {
    cout << "\nrunning " << run.romFile << " for " << run.runtime
         << " seconds...\n";

    if (!runOne(run)) return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// FIXME
// Warning	C6262	Function uses '301164' bytes of stack : exceeds / analyze :
//                stacksize '16384'.  Consider moving some data to heap.
bool ProfilingRunner::runOne(const ProfilingRun& run)
{
  const FSNode imageFile(run.romFile);

  HeadlessRunner runner;
  runner.setLogToStdout(true);

  if (runner.loadCartridge(imageFile) != HeadlessRunner::CartridgeLoadResult::success) {
    cout << "ERROR: failed to load cartridge" << std::endl;
    return false;
  }

  runner.init();

  const uInt64 cyclesTarget =
    static_cast<uInt64>(run.runtime) * runner.getEmulationTiming().cyclesPerSecond();
  const time_point<high_resolution_clock> tp = high_resolution_clock::now();

  const bool runResult = runner.run(cyclesTarget);

  const double realtimeUsed = duration_cast<duration<double>>(high_resolution_clock::now () - tp).count();
  cout << "real time: " << realtimeUsed << " seconds" << std::endl;

  return runResult;
}
