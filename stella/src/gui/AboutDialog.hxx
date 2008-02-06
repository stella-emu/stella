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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: AboutDialog.hxx,v 1.8 2008-02-06 13:45:23 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef ABOUT_DIALOG_HXX
#define ABOUT_DIALOG_HXX

#define LINES_PER_PAGE 10

class OSystem;
class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;


class AboutDialog : public Dialog
{
  public:
    AboutDialog(OSystem* osystem, DialogContainer* parent,
                const GUI::Font& font, int x, int y, int w, int h);
    ~AboutDialog();

  protected:
    ButtonWidget* myNextButton;
    ButtonWidget* myPrevButton;

    StaticTextWidget* myTitle;
    StaticTextWidget* myDesc[LINES_PER_PAGE];

    int myPage;
    int myNumPages;

  private:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    virtual void updateStrings(int page, int lines, string& title, string* &dsc);
    void displayInfo();

    void loadConfig() { displayInfo(); }
};

#endif
