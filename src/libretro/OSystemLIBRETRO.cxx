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

#include "FSNode.hxx"
#include "OSystemLIBRETRO.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemLIBRETRO::getBaseDirAndConfig(string& basedir, string& cfgfile,
        string& savedir, string& loaddir,
        bool useappdir, const string& usedir)
{
  basedir = ".";
  cfgfile = ".";

#if 0
  // Check to see if basedir overrides are active
  if(useappdir)
    cout << "ERROR: base dir in app folder not supported" << endl;
  else if(usedir != "")
  {
    basedir = FilesystemNode(usedir).getPath();
    savedir = loaddir = basedir;
  }
#endif

  FilesystemNode desktop(".");
  savedir = ".";
}
