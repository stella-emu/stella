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

#include "ProfilingRunner.hxx"
#include "FSNode.hxx"
#include "CartDetector.hxx"
#include "Cart.hxx"
#include "MD5.hxx"

static constexpr uInt32 RUNTIME_DEFAULT = 60;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProfilingRunner::ProfilingRunner(int argc, char* argv[])
  : profilingRuns(std::max(argc - 2, 0))
{
  for (int i = 2; i < argc; i++) {
    ProfilingRun& run(profilingRuns[i-2]);

    string arg = argv[i];
    size_t splitPoint = arg.find_first_of(":");

    run.romFile = splitPoint == string::npos ? arg : arg.substr(0, splitPoint);

    if (splitPoint == string::npos) run.runtime = RUNTIME_DEFAULT;
    else  {
      int runtime = atoi(arg.substr(splitPoint+1, string::npos).c_str());
      run.runtime = runtime > 0 ? runtime : RUNTIME_DEFAULT;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProfilingRunner::run()
{
  cout << "Profiling Stella..." << endl;

  for (ProfilingRun& run : profilingRuns) {
    cout << endl << "running " << run.romFile << " for " << run.runtime << " seconds..." << endl;

    if (!runOne(run)) return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ProfilingRunner::runOne(const ProfilingRun run)
{
  FilesystemNode imageFile(run.romFile);

  if (!imageFile.isFile()) {
    cout << "ERROR: " << run.romFile << " is not a ROM image" << endl;
    return false;
  }

  BytePtr image;
  uInt32 size = imageFile.read(image);
  if (size == 0) {
    cout << "ERROR: unable to read " << run.romFile << endl;
    return false;
  }

  string md5 = MD5::hash(image, size);
  string type = "";
  unique_ptr<Cartridge> cartridge = CartDetector::create(imageFile, image, size, md5, type, mySettings);

  if (!cartridge) {
    cout << "ERROR: unable to determine cartridge type" << endl;
    return false;
  }

  return true;
}
