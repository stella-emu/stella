#include "OSystemRTStella.hxx"

#include <sstream>
#include <sched.h>
#include <sys/sysinfo.h>

#include "Logger.hxx"

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
