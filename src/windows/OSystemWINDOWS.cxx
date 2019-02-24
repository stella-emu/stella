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

#include "bspf.hxx"
#include "FSNode.hxx"
#include "HomeFinder.hxx"

#include "OSystemWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWINDOWS::getBaseDirAndConfig(string& basedir, string& cfgfile,
        string& savedir, string& loaddir,
        bool useappdir, const string& usedir)
{
  HomeFinder homefinder;
  FilesystemNode appdata(homefinder.getAppDataPath());
  if(appdata.isDirectory())
  {
    basedir = appdata.getShortPath();
    if(basedir.length() > 1 && basedir[basedir.length() - 1] != '\\')
      basedir += '\\';
    basedir += "Stella\\";
  }
  savedir = loaddir = "~/"; // FIXME - change this to 'Desktop'

  // Check to see if basedir overrides are active
  if(useappdir)
    cout << "ERROR: base dir in app folder not supported" << endl;
  else if(usedir != "")
  {
    basedir = FilesystemNode(usedir).getPath();
    savedir = loaddir = basedir;
  }

  cfgfile = basedir + "stella.ini";
}
#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystemWINDOWS::defaultSaveDir() const
{
  HomeFinder homefinder;
  FilesystemNode documents(homefinder.getDocumentsPath());
  return documents.isDirectory() ? documents.getShortPath() + "\\Stella\\" : "~\\";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystemWINDOWS::defaultLoadDir() const
{
  return defaultSaveDir();
}
#endif
