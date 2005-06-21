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
// $Id: LauncherDialog.hxx,v 1.10 2005-06-21 18:46:33 stephena Exp $
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
class ListWidget;
class ButtonWidget;
class OSystem;

#include "Dialog.hxx"
#include "GameList.hxx"
#include "bspf.hxx"

enum {
  kStartCmd   = 'STRT',
  kOptionsCmd = 'OPTI',
  kReloadCmd  = 'RELO',
  kQuitCmd    = 'QUIT',
  kChooseRomDirCmd  = 'roms',  // rom select
  kChooseSnapDirCmd = 'snps',  // snap select
  kRomDirChosenCmd  = 'romc',  // rom chosen
  kSnapDirChosenCmd = 'snpc'   // snap chosen
};

class LauncherDialog : public Dialog
{
  public:
    LauncherDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~LauncherDialog();

    virtual void handleCommand(CommandSender* sender, int cmd, int data);

  protected:
    void updateListing(bool fullReload = false);
	
    void reset();
    void loadConfig();

  protected:
    ButtonWidget* myStartButton;
    ButtonWidget* myOptionsButton;
    ButtonWidget* myReloadButton;
    ButtonWidget* myQuitButton;

    ListWidget*       myList;
    StaticTextWidget* myNote;
    StaticTextWidget* myRomCount;
    GameList*         myGameList;

    LauncherOptionsDialog* myOptions;
    ProgressDialog*        myProgressBar;

  private:
    void enableButtons(bool enable);
    void loadListFromDisk();
    void loadListFromCache();
    void createListCache();
    string MD5FromFile(const string& path);
};

#endif
