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
// $Id: LauncherOptionsDialog.hxx,v 1.11 2006-11-13 00:21:41 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_OPTIONS_DIALOG_HXX
#define LAUNCHER_OPTIONS_DIALOG_HXX

class OSystem;
class GuiObject;
class DialogContainer;
class BrowserDialog;
class CheckboxWidget;
class PopUpWidget;
class StaticTextWidget;
class TabWidget;

#include "Dialog.hxx"
#include "Command.hxx"

class LauncherOptionsDialog : public Dialog, public CommandSender
{
  public:
    LauncherOptionsDialog(OSystem* osystem, DialogContainer* parent,
                          const GUI::Font& font, GuiObject* boss,
                          int x, int y, int w, int h);
    ~LauncherOptionsDialog();

    virtual void loadConfig();
    virtual void saveConfig();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    void openRomBrowser();
    void openSnapBrowser();

  private:
    enum {
      kChooseRomDirCmd  = 'LOrm', // rom select
      kChooseSnapDirCmd = 'LOsn', // snap select
      kBrowseDirCmd     = 'LObd'  // browse mode
    };

    BrowserDialog* myBrowser;
    TabWidget* myTab;

    // Rom path controls
    StaticTextWidget* myRomPath;
    CheckboxWidget*   myBrowseCheckbox;
    ButtonWidget*     myReloadButton;

    // Snapshot controls
    StaticTextWidget* mySnapPath;
    CheckboxWidget*   mySnapSingleCheckbox;
};

#endif
