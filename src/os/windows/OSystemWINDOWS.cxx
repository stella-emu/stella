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
    wchar_t filename[MAX_PATH];
    const DWORD len = GetModuleFileNameW(nullptr, filename, MAX_PATH);

    if(len > 0 && len < MAX_PATH)
    {
      const FSNode appdir(FSNodeWINDOWS::wideToUtf8(std::wstring_view{filename, len}));
      const FSNode parent = appdir.getParent();

      if(parent.isDirectory())
        basedir = parent.getPath();
    }
  }
  else if(!usedir.empty())
    basedir = FSNodeWINDOWS(usedir).getPath();
}
