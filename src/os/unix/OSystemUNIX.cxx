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

#include <climits>
#include <cstdlib>
#include <cstring>

#if defined(__FreeBSD__) || defined(__DragonFly__)
  #include <sys/sysctl.h>
#elif defined(__linux__) || defined(__NetBSD__) || defined(__sun)
  #include <unistd.h>
#endif

#include "FSNode.hxx"
#include "XDGPaths.hxx"
#include "OSystemUNIX.hxx"

namespace
{
  // Returns the path to the current executable, or empty string if unsupported
  string getExecutablePath()
  {
    char resolved[PATH_MAX];

  #ifdef __linux__
    const ssize_t len = readlink("/proc/self/exe", resolved, PATH_MAX - 1);
    if(len > 0)
    {
      resolved[len] = '\0';
      return string{resolved};
    }

  #elif defined(__FreeBSD__) || defined(__DragonFly__)
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t len = PATH_MAX;
    if(sysctl(mib, 4, resolved, &len, nullptr, 0) == 0)
      return string{resolved};

  #elif defined(__NetBSD__)
    const ssize_t len = readlink("/proc/curproc/exe", resolved, PATH_MAX - 1);
    if(len > 0)
    {
      resolved[len] = '\0';
      return string{resolved};
    }

  #elif defined(__sun)
    const ssize_t len = readlink("/proc/self/path/a.out", resolved, PATH_MAX - 1);
    if(len > 0)
    {
      resolved[len] = '\0';
      return string{resolved};
    }

  #endif
    return {};  // unsupported platform (e.g. OpenBSD)
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemUNIX::getBaseDirectories(string& basedir, string& homedir,
                                     bool useappdir, string_view usedir)
{
  const auto& xdg = XDGPaths::instance();
  basedir = xdg.configDir("stella");
  homedir = xdg.home();

  // Check to see if basedir overrides are active
  if(useappdir)
  {
    const string exepath = getExecutablePath();
    if(!exepath.empty())
    {
      const FSNode parent = FSNode(exepath).getParent();
      if(parent.isDirectory())
        basedir = parent.getPath();
    }
    else
      cerr << "ERROR: base dir in app folder not supported\n";
  }
  else if(!usedir.empty())
    basedir = FSNode(usedir).getPath();
}
