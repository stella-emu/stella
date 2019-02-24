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
#if 0
  // Check if the base directory should be overridden
  // Shouldn't normally be necessary, but is useful for those people that
  // don't want to clutter their 'My Documents' folder
  bool overrideBasedir = false;
  FilesystemNode basedirfile("basedir.txt");
  if(basedirfile.exists())
  {
    ifstream in(basedirfile.getPath());
    if(in && in.is_open())
    {
      getline(in, basedir);

      // trim leading and trailing spaces
      size_t spos = basedir.find_first_not_of(" \t");
      size_t epos = basedir.find_last_not_of(" \t");
      if(spos != string::npos && epos != string::npos)
        basedir = basedir.substr(spos, epos-spos+1);

      if(basedir != "")  overrideBasedir = true;
    }
  }
#endif

  // If basedir hasn't been specified, use the 'home' directory
  if(!overrideBasedir)
  {
    HomeFinder homefinder;
    FilesystemNode appdata(homefinder.getAppDataPath());
    if(appdata.isDirectory())
    {
      basedir = appdata.getShortPath();
      if(basedir.length() > 1 && basedir[basedir.length()-1] != '\\')
        basedir += '\\';
      basedir += "Stella\\";
    }
    else
      basedir = ".\\";  // otherwise, default to current directory
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
