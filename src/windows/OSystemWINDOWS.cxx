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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <fstream>

#include "bspf.hxx"
#include "FSNode.hxx"
#include "HomeFinder.hxx"

#include "OSystemWINDOWS.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setConfigFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWINDOWS::OSystemWINDOWS()
  : OSystem()
{
  string basedir = "";

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
      basedir += "Stella";
    }
    else
      basedir = ".";  // otherwise, default to current directory
  }

  setBaseDir(basedir);
  setConfigFile(basedir + "\\stella.ini");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWINDOWS::~OSystemWINDOWS()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystemWINDOWS::defaultSnapSaveDir()
{
  HomeFinder homefinder;
  FilesystemNode desktop(homefinder.getDesktopPath());
  return desktop.isDirectory() ? desktop.getShortPath() : "~";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystemWINDOWS::defaultSnapLoadDir()
{
  return defaultSnapSaveDir();
}
