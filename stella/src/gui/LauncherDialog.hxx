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
// $Id: LauncherDialog.hxx,v 1.40 2009-01-05 19:44:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

#include "bspf.hxx"

class ButtonWidget;
class CommandSender;
class ContextMenu;
class DialogContainer;
class GameList;
class OptionsDialog;
class GlobalPropsDialog;
class LauncherFilterDialog;
class OSystem;
class Properties;
class RomInfoWidget;
class StaticTextWidget;
class StringListWidget;

#include "Dialog.hxx"
#include "FSNode.hxx"
#include "StringList.hxx"

// These must be accessible from dialogs created by this class
enum {
  kRomDirChosenCmd  = 'romc',  // rom chosen
  kSnapDirChosenCmd = 'snpc',  // snap chosen
  kReloadRomDirCmd  = 'rdrl',  // reload the current listing
  kReloadFiltersCmd = 'rlfl'   // reload filtering options and current listing
};

class LauncherDialog : public Dialog
{
  public:
    LauncherDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~LauncherDialog();

    /**
      Get MD5sum for the currently selected file

      @return md5sum if a valid ROM file, else the empty string
    */
    string selectedRomMD5();

  protected:
    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void loadConfig();
    void updateListing();

  private:
    void enableButtons(bool enable);
    void loadDirListing();
    void loadRomInfo();
    void handleContextMenu();
    void setListFilters();

  private:
    ButtonWidget* myStartButton;
    ButtonWidget* myPrevDirButton;
    ButtonWidget* myOptionsButton;
    ButtonWidget* myQuitButton;

    StringListWidget* myList;
    StaticTextWidget* myDirLabel;
    StaticTextWidget* myDir;
    StaticTextWidget* myRomCount;
    GameList*         myGameList;

    OptionsDialog*    myOptions;
    RomInfoWidget*    myRomInfoWidget;

    ContextMenu*          myMenu;
    GlobalPropsDialog*    myGlobalProps;
    LauncherFilterDialog* myFilters;

    int mySelectedItem;
    int myRomInfoSize;
    FilesystemNode myCurrentNode;

    bool myShowDirs;
    StringList myRomExts;

    enum {
      kStartCmd   = 'STRT',
      kPrevDirCmd = 'PRVD',
      kOptionsCmd = 'OPTI',
      kQuitCmd    = 'QUIT'
    };
};

#endif
