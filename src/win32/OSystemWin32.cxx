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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <SDL_syswm.h>
#include <fstream>

#include "bspf.hxx"
#include "FSNode.hxx"
#include "HomeFinder.hxx"
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
      getline(in, basedir);
      in.close();

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
      basedir = appdata.getRelativePath();
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
OSystemWin32::~OSystemWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWin32::setAppWindowPos(int x, int y, int w, int h)
{
  SDL_SysWMinfo sdl_info;
  memset(&sdl_info, 0, sizeof(sdl_info));
  SDL_VERSION (&sdl_info.version);
  if(SDL_GetWMInfo(&sdl_info) <= 0)
    return;

  // The following mostly comes from SDL_dx5video.c
  HWND window = sdl_info.window;
  RECT bounds;

  bounds.left   = x;
  bounds.top    = y;
  bounds.right  = x + w;
  bounds.bottom = y + h;
  AdjustWindowRectEx(&bounds, GetWindowLong(window, GWL_STYLE), (GetMenu(window) != NULL), 0);

  SetWindowPos(window, HWND_NOTOPMOST, x, y, bounds.right-bounds.left,
    bounds.bottom-bounds.top, SWP_NOCOPYBITS | SWP_SHOWWINDOW);
}
