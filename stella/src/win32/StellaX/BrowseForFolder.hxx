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
// Copyright (c) 1998 Scott D. Killen
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: BrowseForFolder.hxx,v 1.3 2004-07-06 22:51:58 stephena Exp $
//============================================================================ 

#ifndef __BROWSE_FOR_FOLDER_
#define __BROWSE_FOR_FOLDER_

#include <shlobj.h>


class CBrowseForFolder
{
  public:
    CBrowseForFolder(const HWND hParent = NULL,
                     const LPITEMIDLIST pidl = NULL,
                     LPCTSTR strTitle = NULL );

    virtual ~CBrowseForFolder();

  public:
    // Call GetSelectedFolder to retrieve the folder selected by the user.
    LPCTSTR GetSelectedFolder() const;

    // Call SelectFolder to display the dialog and get a selection from the user.  Use
    // GetSelectedFolder and GetImage to get the results of the dialog.
    bool SelectFolder();

  protected:
    // OnSelChanged is called whenever the user selects a different directory.  pidl is
    // the LPITEMIDLIST of the new selection.  Use SHGetPathFromIDList to retrieve the
    // path of the selection.
    virtual void OnSelChanged(const LPITEMIDLIST pidl) const;

    // Call EnableOK to enable the OK button on the active dialog.  If bEnable is true
    // then the button is enabled, otherwise it is disabled.
    // NOTE -- This function should only be called within overrides of OnInit and
    // OnSelChanged.
    void EnableOK(const bool bEnable) const;

    // Call SetSelection to set the selection in the active dialog.  pidl is the
    // LPITEMIDLIST
    // of the path to be selected.  strPath is a CString containing the path to be
    // selected.
    // NOTE -- This function should only be called within overrides of OnInit and
    // OnSelChanged.
    void SetSelection(const LPITEMIDLIST pidl) const;
    void SetSelection(LPCTSTR strPath) const;

  private:
    static int __stdcall BrowseCallbackProc(HWND hwnd,
        UINT uMsg,
        LPARAM lParam,
        LPARAM lpData);

    BROWSEINFO myBrowseInfo;
    char mySelected[MAX_PATH];
    CSimpleString myPath;
    HWND myHwnd;
};

#endif
