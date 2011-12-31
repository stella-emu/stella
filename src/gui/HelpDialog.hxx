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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
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

#ifndef HELP_DIALOG_HXX
#define HELP_DIALOG_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

class HelpDialog : public Dialog
{
  public:
    HelpDialog(OSystem* osystem, DialogContainer* parent, const GUI::Font& font);
    ~HelpDialog();

  protected:
    enum { kLINES_PER_PAGE = 10 };
    ButtonWidget* myNextButton;
    ButtonWidget* myPrevButton;

    StaticTextWidget* myTitle;
    StaticTextWidget* myKey[kLINES_PER_PAGE];
    StaticTextWidget* myDesc[kLINES_PER_PAGE];
    string myKeyStr[kLINES_PER_PAGE];
    string myDescStr[kLINES_PER_PAGE];

    uInt8 myPage;
    uInt8 myNumPages;

  private:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    virtual void updateStrings(uInt8 page, uInt8 lines, string& title);
    void displayInfo();
    void loadConfig() { displayInfo(); }
};

#endif
