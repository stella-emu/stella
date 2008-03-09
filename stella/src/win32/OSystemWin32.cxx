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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystemWin32.cxx,v 1.23 2008-03-09 20:38:44 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <windows.h>
#include <shlobj.h>

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
  string basedir = ".";

  if(!FilesystemNode::fileExists("disable_profiles.txt"))
  {
    /*
       Use 'My Documents' folder for the Stella folder, which can be in many
       different places depending on the version of Windows, as follows:

        98: C:\My Documents
        XP: C:\Document and Settings\USERNAME\My Documents\
        Vista: C:\Users\USERNAME\Documents\

       This function is guaranteed to return a valid 'My Documents'
       folder (as much as Windows *can* make that guarantee) 
    */
    char configPath[256];
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE,
                                 NULL, 0, configPath)))
    {
      strcat(configPath, "\\Stella");
      basedir = configPath;
    }
  }

  setBaseDir(basedir);
  setConfigFile(basedir + "\\stella.ini");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::~OSystemWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemWin32::getTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}
