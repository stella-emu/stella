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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: LauncherDialog.hxx,v 1.18 2006-03-19 18:17:48 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

class DialogContainer;
class LauncherOptionsDialog;
class ProgressDialog;
class CommandSender;
class StaticTextWidget;
class StringListWidget;
class ButtonWidget;
class OSystem;

#include "Dialog.hxx"
#include "GameList.hxx"
#include "bspf.hxx"

enum {
  kChooseRomDirCmd  = 'roms',  // rom select
  kChooseSnapDirCmd = 'snps',  // snap select
  kRomDirChosenCmd  = 'romc',  // rom chosen
  kSnapDirChosenCmd = 'snpc',  // snap chosen
  kBrowseChangedCmd = 'broc'   // browse mode toggled
};

class LauncherDialog : public Dialog
{
  public:
    LauncherDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~LauncherDialog();

  protected:
    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleJoyAxis(int stick, int axis, int value);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void updateListing(bool fullReload = false);
    void loadConfig();

    virtual bool wantsEvents() { return true; }

  protected:
    ButtonWidget* myStartButton;
    ButtonWidget* myRelPrevButton;
    ButtonWidget* myOptionsButton;
    ButtonWidget* myQuitButton;

    StringListWidget* myList;
    StaticTextWidget* myNoteLabel;
    StaticTextWidget* myNote;
    StaticTextWidget* myRomCount;
    GameList*         myGameList;

    LauncherOptionsDialog* myOptions;
    ProgressDialog*        myProgressBar;

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
    string myCurrentDir;

    enum {
      kStartCmd   = 'STRT',
      kRelPrevCmd = 'REPV',
      kOptionsCmd = 'OPTI',
      kQuitCmd    = 'QUIT'
    };
};

#endif
