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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: LauncherDialog.hxx,v 1.23 2006-12-30 22:26:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

class DialogContainer;
class OptionsDialog;
class ProgressDialog;
class CommandSender;
class StaticTextWidget;
class StringListWidget;
class ButtonWidget;
class OSystem;

#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GameList.hxx"
#include "bspf.hxx"

// These must be accessible from LauncherOptionsDialog
enum {
  kRomDirChosenCmd  = 'romc',  // rom chosen
  kSnapDirChosenCmd = 'snpc',  // snap chosen
  kBrowseChangedCmd = 'broc',  // browse mode toggled
  kReloadRomDirCmd  = 'rdrl'   // reload the current listing
};

class LauncherDialog : public Dialog
{
  public:
    LauncherDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~LauncherDialog();

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void updateListing(bool fullReload = false);
    void loadConfig();

  protected:
    ButtonWidget* myStartButton;
    ButtonWidget* myPrevDirButton;
    ButtonWidget* myOptionsButton;
    ButtonWidget* myQuitButton;

    StringListWidget* myList;
    StaticTextWidget* myNoteLabel;
    StaticTextWidget* myNote;
    StaticTextWidget* myRomCount;
    GameList*         myGameList;

    OptionsDialog*    myOptions;
    ProgressDialog*   myProgressBar;

  private:
    void enableButtons(bool enable);
    void loadDirListing();
    void loadListFromDisk();
    void loadListFromCache();
    void createListCache();
    string MD5FromFile(const string& path);

  private:
    int mySelectedItem;
    bool myBrowseModeFlag; 
    FilesystemNode myCurrentNode;

    enum {
      kStartCmd   = 'STRT',
      kPrevDirCmd = 'PRVD',
      kOptionsCmd = 'OPTI',
      kQuitCmd    = 'QUIT'
    };
};

#endif
