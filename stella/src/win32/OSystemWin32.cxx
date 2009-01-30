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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystemWin32.cxx,v 1.32 2009-01-30 23:31:41 stephena Exp $
//============================================================================

#include "bspf.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "OSystemWin32.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setConfigFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::OSystemWin32()
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
    ifstream in(basedirfile.getPath().c_str());
    if(in && in.is_open())
    {
      in >> basedir;
      in.close();
      if(basedir != "")  overrideBasedir = true;
    }
  }

  // If basedir hasn't been specified, use the 'home' directory
  if(!overrideBasedir)
  {
    FilesystemNode home("~\\");
    if(home.isDirectory())
      basedir = "~\\Stella";
    else
      basedir = ".";  // otherwise, default to current directory
  }

  setBaseDir(basedir);
  setConfigFile(basedir + "\\stella.ini");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::~OSystemWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemWin32::getTicks() const
{
  return (uInt32) SDL_GetTicks() * 1000;
}
