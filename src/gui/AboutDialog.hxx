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
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef ABOUT_DIALOG_HXX
#define ABOUT_DIALOG_HXX

class OSystem;
class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;


class AboutDialog : public Dialog
{
  public:
    AboutDialog(OSystem* osystem, DialogContainer* parent,
                const GUI::Font& font);
    ~AboutDialog();

  protected:
    enum { kLINES_PER_PAGE = 9 };
    ButtonWidget* myNextButton;
    ButtonWidget* myPrevButton;

    StaticTextWidget* myTitle;
    StaticTextWidget* myDesc[kLINES_PER_PAGE];
    string myDescStr[kLINES_PER_PAGE];

    int myPage;
    int myNumPages;

  private:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    virtual void updateStrings(int page, int lines, string& title);
    void displayInfo();

    void loadConfig() { displayInfo(); }
};

#endif
