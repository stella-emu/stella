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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DebuggerDialog.hxx,v 1.1 2005-06-03 17:52:06 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DEBUGGER_DIALOG_HXX
#define DEBUGGER_DIALOG_HXX

class OSystem;
class DialogContainer;
class BrowserDialog;
class CheckboxWidget;
class PopUpWidget;
class StaticTextWidget;

#include "Dialog.hxx"

class DebuggerDialog : public Dialog
{
  public:
    DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                          int x, int y, int w, int h);
    ~DebuggerDialog();

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
