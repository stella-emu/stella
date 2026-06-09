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

#include <mach-o/dyld.h>
#include <climits>
#include <cstdlib>
#include <cstring>
#include "FSNode.hxx"
#include "MacOSUtils.hxx"
#include "OSystemMACOS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemMACOS::getBaseDirectories(string& basedir, string& homedir,
                                      bool useappdir, string_view usedir)
{
  const string appSupport = MacOSUtils::applicationSupportPath();
  basedir = appSupport.empty()
      ? "~/Library/Application Support/Stella/"
      : appSupport + "/Stella/";

  const string desktop = MacOSUtils::desktopPath();
  homedir = desktop.empty() ? "~/" : desktop + "/";

  if(useappdir)
  {
    uint32_t len = 0;
    _NSGetExecutablePath(nullptr, &len);

    string filename(len, '\0');
    if(_NSGetExecutablePath(filename.data(), &len) == 0)
    {
      filename.resize(std::strlen(filename.c_str()));

      char resolved[PATH_MAX];
      if(realpath(filename.c_str(), resolved) != nullptr)
      {
        const FSNode appdir(resolved);
        const FSNode parent = appdir.getParent();
        if(parent.isDirectory())
          basedir = parent.getPath();
      }
    }
  }
  else if(!usedir.empty())
    basedir = FSNode(usedir).getPath();
}
