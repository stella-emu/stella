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
// $Id: HelpDialog.hxx,v 1.1 2005-03-27 03:07:34 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef HELP_DIALOG_HXX
#define HELP_DIALOG_HXX

class CommandSender;
class ButtonWidget;
class StaticTextWidget;

#define ADD_BIND(k,d) do { key[i] = k; dsc[i] = d; i++; } while(0)
#define ADD_TEXT(d) ADD_BIND("",d)
#define ADD_LINE ADD_BIND("","")

#define LINES_PER_PAGE 10

#include "OSystem.hxx"
#include "Props.hxx"
#include "bspf.hxx"

class HelpDialog : public Dialog
{
  public:
    HelpDialog(OSystem* osystem, uInt16 x, uInt16 y, uInt16 w, uInt16 h);
    ~HelpDialog();

  protected:
    ButtonWidget* myNextButton;
    ButtonWidget* myPrevButton;

    StaticTextWidget* myTitle;
    StaticTextWidget* myKey[LINES_PER_PAGE];
    StaticTextWidget* myDesc[LINES_PER_PAGE];

    uInt8 myPage;
    uInt8 myNumPages;

  private:
    virtual void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);
    virtual void updateStrings(uInt8 page, uInt8 lines,
                               string& title, string*& key, string* &dsc);
	void displayInfo();
    void loadConfig() { displayInfo(); }
};

#endif
