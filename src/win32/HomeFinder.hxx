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

#ifndef __HOME_FINDER_
#define __HOME_FINDER_

#include <shlobj.h>

/*
 * Used to determine the location of the various Win32 user/system folders.
 *
 * Win98 and earlier don't have SHGetFolderPath in shell32.dll.
 * Microsoft recommend that we load shfolder.dll at run time and
 * access the function through that.
 *
 * shfolder.dll is loaded dynamically in the constructor, and unloaded in
 * the destructor
 *
 * The class makes SHGetFolderPath available through its function operator.
 * It will work on all versions of Windows >= Win95.
 *
 * This code was borrowed from the Lyx project.
 */
class HomeFinder
{
  public:
    HomeFinder() : myFolderModule(0), myFolderPathFunc(0)
    {
      myFolderModule = LoadLibrary("shfolder.dll");
      if(myFolderModule)
        myFolderPathFunc = reinterpret_cast<function_pointer>
           (::GetProcAddress(myFolderModule, "SHGetFolderPathA"));
    }

    ~HomeFinder() { if(myFolderModule) FreeLibrary(myFolderModule); }

    /** Wrapper for SHGetFolderPathA, returning the 'HOME/User' folder
        (or an empty string if the folder couldn't be determined. */
    const string& getHomePath() const
    {
      if(ourHomePath == "")
      {
        if(!myFolderPathFunc)
          ourHomePath = "";
        else
        {
          char folder_path[MAX_PATH];
          HRESULT const result = (myFolderPathFunc)
              (NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, 0, folder_path);
          ourHomePath = (result == 0) ? folder_path : "";
        }
      }
      return ourHomePath;
    }

    /** Wrapper for SHGetFolderPathA, returning the 'APPDATA' folder
        (or an empty string if the folder couldn't be determined. */
    const string& getAppDataPath() const
    {
      if(ourAppDataPath == "")
      {
        if(!myFolderPathFunc)
          ourAppDataPath = "";
        else
        {
          char folder_path[MAX_PATH];
          HRESULT const result = (myFolderPathFunc)
              (NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, folder_path);
          ourAppDataPath = (result == 0) ? folder_path : "";
        }
      }
      return ourAppDataPath;
    }

    /** Wrapper for SHGetFolderPathA, returning the 'DESKTOPDIRECTORY' folder
        (or an empty string if the folder couldn't be determined. */
    const string& getDesktopPath() const
    {
      if(ourDesktopPath == "")
      {
        if(!myFolderPathFunc)
          ourDesktopPath = "";
        else
        {
          char folder_path[MAX_PATH];
          HRESULT const result = (myFolderPathFunc)
              (NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, 0, folder_path);
          ourDesktopPath = (result == 0) ? folder_path : "";
        }
      }
      return ourDesktopPath;
    }

    private:
      typedef HRESULT (__stdcall * function_pointer)(HWND, int, HANDLE, DWORD, LPCSTR);

      HMODULE myFolderModule;
      function_pointer myFolderPathFunc;

      static string ourHomePath, ourAppDataPath, ourDesktopPath;
};

__declspec(selectany) string HomeFinder::ourHomePath = "";
__declspec(selectany) string HomeFinder::ourAppDataPath = "";
__declspec(selectany) string HomeFinder::ourDesktopPath = "";

#endif
