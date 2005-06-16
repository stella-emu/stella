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
// $Id: LauncherOptionsDialog.hxx,v 1.4 2005-06-16 00:55:59 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_OPTIONS_DIALOG_HXX
#define LAUNCHER_OPTIONS_DIALOG_HXX

class OSystem;
class DialogContainer;
class BrowserDialog;
class CheckboxWidget;
class PopUpWidget;
class StaticTextWidget;

#include "Dialog.hxx"

class LauncherOptionsDialog : public Dialog
{
  public:
    LauncherOptionsDialog(OSystem* osystem, DialogContainer* parent,
                          int x, int y, int w, int h);
    ~LauncherOptionsDialog();

    virtual void loadConfig();
    virtual void saveConfig();

    virtual void handleCommand(CommandSender* sender, int cmd, int data);

  protected:
    BrowserDialog* myBrowser;

    // Rom path controls
    StaticTextWidget* myRomPath;

    // Snapshot controls
    StaticTextWidget* mySnapPath;
    PopUpWidget*      mySnapTypePopup;
    CheckboxWidget*   mySnapSingleCheckbox;

  private:
    void openRomBrowser();
    void openSnapBrowser();
};

#endif
