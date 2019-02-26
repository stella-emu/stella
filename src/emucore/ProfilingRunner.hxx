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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PROFILING_RUNNER
#define PROFILING_RUNNER

#include "bspf.hxx"
#include "Settings.hxx"

class ProfilingRunner {
  public:

    ProfilingRunner(int argc, char* argv[]);

    bool run();

  private:

    struct ProfilingRun {
      string romFile;
      uInt32 runtime;
    };

  private:

    bool runOne(const ProfilingRun run);

  private:

    vector<ProfilingRun> profilingRuns;

    Settings mySettings;
};

#endif // PROFILING_RUNNER
