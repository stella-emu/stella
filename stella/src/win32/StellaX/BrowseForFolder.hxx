//============================================================================
//
//   SSSS    tt          lll  lll          XX     XX
//  SS  SS   tt           ll   ll           XX   XX
//  SS     tttttt  eeee   ll   ll   aaaa     XX XX
//   SSSS    tt   ee  ee  ll   ll      aa     XXX
//      SS   tt   eeeeee  ll   ll   aaaaa    XX XX
//  SS  SS   tt   ee      ll   ll  aa  aa   XX   XX
//   SSSS     ttt  eeeee llll llll  aaaaa  XX     XX
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: BrowseForFolder.hxx,v 1.4 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef BROWSE_FOR_FOLDER_HXX
#define BROWSE_FOR_FOLDER_HXX

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
    HWND myHwnd;
};

#endif
