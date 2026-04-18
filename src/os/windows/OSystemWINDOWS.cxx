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

#include "bspf.hxx"
#include "FSNodeWINDOWS.hxx"
#include "HomeFinder.hxx"

#include "OSystemWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWINDOWS::getBaseDirectories(string& basedir, string& homedir,
                                        bool useappdir, string_view usedir)
{
  const FSNodeWINDOWS appdata(HomeFinder::getAppDataPathW());

  if(appdata.isDirectory())
  {
    basedir = appdata.getShortPath();
    if(basedir.length() > 1 && basedir[basedir.length() - 1] != '\\')
      basedir += '\\';
    basedir += "Stella\\";
  }

  const FSNodeWINDOWS defaultHomeDir(HomeFinder::getDesktopPathW());
  homedir = defaultHomeDir.getShortPath();

  // Check to see if basedir overrides are active
  if(useappdir)
  {
    std::wstring filename;
    DWORD len = GetModuleFileNameW(nullptr, filename.data(),
      static_cast<DWORD>(filename.size()));
    filename.resize(len);

    FSNode appdir(FSNodeWINDOWS::wideToUtf8(filename));

    appdir = appdir.getParent();
    if(appdir.isDirectory())
      basedir = appdir.getPath();
  }
  else if(usedir != "")
    basedir = FSNode(usedir).getPath();
}
