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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef INPUT_TEXT_DIALOG_HXX
#define INPUT_TEXT_DIALOG_HXX

class GuiObject;
class StaticTextWidget;
class EditTextWidget;

#include "Dialog.hxx"
#include "Command.hxx"

typedef Common::Array<EditTextWidget*> InputWidget;

class InputTextDialog : public Dialog, public CommandSender
{
  public:
    InputTextDialog(GuiObject* boss, const GUI::Font& font,
                    const StringList& labels);
    virtual ~InputTextDialog();

    /** Place the input dialog onscreen and center it */
    void show();

    /** Show input dialog onscreen at the specified coordinates */
    void show(uInt32 x, uInt32 y);

    const string& getResult(int idx = 0);

    void setEditString(const string& str, int idx = 0);
    void setEmitSignal(int cmd) { myCmd = cmd; }
    void setTitle(const string& title);

    void setFocus(int idx = 0);

    /** This dialog uses its own positioning, so we override Dialog::center() */
    void center();

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    InputWidget       myInput;
    StaticTextWidget* myTitle;

    bool myEnableCenter;
    bool myErrorFlag;
    int	 myCmd;

    uInt32 myXOrig, myYOrig;
};

#endif
