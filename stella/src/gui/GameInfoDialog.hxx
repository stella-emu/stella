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
// $Id: GameInfoDialog.hxx,v 1.8 2005-09-28 19:59:24 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef GAME_INFO_DIALOG_HXX
#define GAME_INFO_DIALOG_HXX

class OSystem;
class GuiObject;
class EditTextWidget;
class PopUpWidget;
class StaticTextWidget;
class TabWidget;

#include "Dialog.hxx"
#include "Command.hxx"

class GameInfoDialog : public Dialog, public CommandSender
{
  public:
    GameInfoDialog(OSystem* osystem, DialogContainer* parent,
                   GuiObject* boss,
                   int x, int y, int w, int h);
    ~GameInfoDialog();

    virtual void loadConfig();
    virtual void saveConfig();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    TabWidget* myTab;

    // Cartridge properties
    EditTextWidget*   myName;
    StaticTextWidget* myMD5;
    EditTextWidget*   myManufacturer;
    EditTextWidget*   myModelNo;
    EditTextWidget*   myNote;
    PopupWidget*      mySound;
    PopupWidget*      myType;

    // Console/controller properties
    PopupWidget* myLeftDiff;
    PopupWidget* myRightDiff;
    PopupWidget* myTVType;
    PopupWidget* myLeftController;
    PopupWidget* myRightController;

    // Display properties
    PopupWidget*    myFormat;
    EditTextWidget* myXStart;
    EditTextWidget* myWidth;
    EditTextWidget* myYStart;
    EditTextWidget* myHeight;
    PopupWidget*    myHmoveBlanks;
};

#endif
