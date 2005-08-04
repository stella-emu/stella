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
// $Id: InputTextDialog.hxx,v 1.1 2005-08-04 16:31:24 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef INPUT_TEXT_DIALOG_HXX
#define INPUT_TEXT_DIALOG_HXX

class GuiObject;
class StaticTextWidget;
class EditNumWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "bspf.hxx"

class InputTextDialog : public Dialog, public CommandSender
{
  public:
    InputTextDialog(GuiObject* boss, const GUI::Font& font);

    const string& getResult() { return _input->getEditString(); }

    void setEditString(const string& str) { _input->setEditString(str); }
    void setTitle(const string& title) { _title->setLabel(title); }
    void setEmitSignal(int cmd) { _cmd = cmd; }

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    StaticTextWidget* _title;
    EditNumWidget*    _input;

    int	 _cmd;
};

#endif
