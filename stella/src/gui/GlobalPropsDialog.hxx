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
// $Id: GlobalPropsDialog.hxx,v 1.1 2009-01-02 01:50:03 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef GLOBAL_PROPS_DIALOG_HXX
#define GLOBAL_PROPS_DIALOG_HXX

class CommandSender;
class DialogContainer;
class CheckboxWidget;
class PopUpWidget;

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "Settings.hxx"
#include "bspf.hxx"

class GlobalPropsDialog : public Dialog
{
  public:
    GlobalPropsDialog(GuiObject* boss, const GUI::Font& font, Settings& settings,
                      int x, int y, int w, int h);
    ~GlobalPropsDialog();

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    PopUpWidget*      myBSType;
    PopUpWidget*      myLeftDiff;
    PopUpWidget*      myRightDiff;
    PopUpWidget*      myTVType;

    CheckboxWidget*   myHoldSelect;
    CheckboxWidget*   myHoldReset;
    CheckboxWidget*   myHoldButton0;

    Settings& mySettings;
};

#endif
