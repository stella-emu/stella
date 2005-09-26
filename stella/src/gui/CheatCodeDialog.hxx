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
// $Id: CheatCodeDialog.hxx,v 1.3 2005-09-26 19:10:37 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef CHEAT_CODE_DIALOG_HXX
#define CHEAT_CODE_DIALOG_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "Cheat.hxx"
#include "EditTextWidget.hxx"
#include "Props.hxx"
#include "bspf.hxx"

class CheatCodeDialog : public Dialog
{
  public:
    CheatCodeDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~CheatCodeDialog();

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:
    Cheat* myCheat;

    ButtonWidget*     myExitButton;
    StaticTextWidget* myTitle;
    StaticTextWidget* myError;
    EditTextWidget*   myInput;
    // CheckboxWidget*   myEnableCheat;
};

#endif
