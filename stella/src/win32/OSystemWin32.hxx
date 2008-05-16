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
// $Id: OSystemWin32.hxx,v 1.14 2008-05-16 23:56:31 stephena Exp $
//============================================================================

#ifndef OSYSTEM_WIN32_HXX
#define OSYSTEM_WIN32_HXX

#include <windows.h>

#include "bspf.hxx"

/**
  This class defines Windows system specific settings.

  @author  Stephen Anthony
  @version $Id: OSystemWin32.hxx,v 1.14 2008-05-16 23:56:31 stephena Exp $
*/
class OSystemWin32 : public OSystem
{
  public:
    /**
      Create a new Win32 operating system object
    */
    OSystemWin32();

    /**
      Destructor
    */
    virtual ~OSystemWin32();

  public:
    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    virtual uInt32 getTicks() const;
};

/**
  Win98 and earlier don't have SHGetFolderPath in shell32.dll.
  Microsoft recommend that we load shfolder.dll at run time and
  access the function through that.

  shfolder.dll is loaded dynamically in the constructor. If loading
  fails or if the .dll is found not to contain SHGetFolderPathA then
  the program exits immediately. Otherwise, the .dll is unloaded in
  the destructor

  The class makes SHGetFolderPath available through its function operator.
  It will work on all versions of Windows >= Win95.

  This code was borrowed from the Lyx project.
*/
class GetFolderPathWin32
{
  public:
    enum kFolderId {
      PERSONAL,   // CSIDL_PERSONAL
      APPDATA     // CSIDL_APPDATA
    };

    GetFolderPathWin32();
    ~GetFolderPathWin32();

    /** Wrapper for SHGetFolderPathA, returning the path asscociated with id. */
    string const operator()(kFolderId id) const;

  private:
    typedef HRESULT (__stdcall * function_pointer)(HWND, int, HANDLE, DWORD, LPCSTR);

    HMODULE myFolderModule;
    function_pointer myFolderPathFunc;
};

#endif
