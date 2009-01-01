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
// $Id: OSystemWin32.cxx,v 1.26 2009-01-01 18:13:39 stephena Exp $
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
    try
    {
      GetFolderPathWin32 win32FolderPath;
      string path = win32FolderPath(GetFolderPathWin32::PERSONAL) + "\\Stella";
      basedir = path;
    }
    catch(char* msg)
    {
      cerr << msg << endl;
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
uInt32 OSystemWin32::getTicks() const
{
  return (uInt32) SDL_GetTicks() * 1000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GetFolderPathWin32::GetFolderPathWin32()
  : myFolderModule(0),
    myFolderPathFunc(0)
{
  myFolderModule = LoadLibrary("shfolder.dll");
  if(!myFolderModule)
    throw "ERROR: GetFolderPathWin32() failed; cannot determine \'My Documents\' folder";

  myFolderPathFunc = reinterpret_cast<function_pointer>
    (::GetProcAddress(myFolderModule, "SHGetFolderPathA"));
  if(myFolderPathFunc == 0)
    throw "ERROR: GetFolderPathWin32() failed; cannot determine \'My Documents\' folder";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GetFolderPathWin32::~GetFolderPathWin32()
{
  if(myFolderModule)
    FreeLibrary(myFolderModule);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Given a folder ID, returns the folder name.
string const GetFolderPathWin32::operator()(kFolderId _id) const
{
  char folder_path[MAX_PATH];
  int id = 0;

  switch(_id)
  {
    case PERSONAL:
      id = CSIDL_PERSONAL;
      break;
    case APPDATA:
      id = CSIDL_APPDATA;
      break;
    default:
      return "";
  }

  HRESULT const result =
    (myFolderPathFunc)(NULL, id | CSIDL_FLAG_CREATE, NULL, 0, folder_path);

  return (result == 0) ? folder_path : "";
}
