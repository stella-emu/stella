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
// $Id: GameInfoDialog.hxx,v 1.14 2006-01-11 01:17:11 stephena Exp $
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
class Properties;

#include "Array.hxx"
#include "Dialog.hxx"
#include "Command.hxx"

// Structure used for cartridge and controller types
struct PropType {
  string name;
  string comparitor;
};

class GameInfoDialog : public Dialog, public CommandSender
{
  public:
    GameInfoDialog(OSystem* osystem, DialogContainer* parent, GuiObject* boss,
                   int x, int y, int w, int h);
    ~GameInfoDialog();

    void setGameProfile(Properties& props) { myGameProperties = &props; }

    void loadConfig();
    void saveConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    TabWidget* myTab;

    // Cartridge properties
    EditTextWidget*   myName;
    StaticTextWidget* myMD5;
    EditTextWidget*   myManufacturer;
    EditTextWidget*   myModelNo;
    EditTextWidget*   myRarity;
    EditTextWidget*   myNote;
    PopUpWidget*      mySound;
    PopUpWidget*      myType;

    // Console properties
    PopUpWidget* myLeftDiff;
    PopUpWidget* myRightDiff;
    PopUpWidget* myTVType;
    PopUpWidget* mySwapPorts;

    // Controller properties
    PopUpWidget* myLeftController;
    PopUpWidget* myRightController;

    // Display properties
    PopUpWidget*    myFormat;
    EditTextWidget* myXStart;
    EditTextWidget* myWidth;
    EditTextWidget* myYStart;
    EditTextWidget* myHeight;
    PopUpWidget*    myPhosphor;
    PopUpWidget*    myHmoveBlanks;

    /** Game properties for currently loaded ROM */
    Properties* myGameProperties;

    /** Holds static strings for Cartridge type */
    static const PropType ourCartridgeList[21];

    /** Holds static strings for Controller type */
    static const PropType ourControllerList[5];
};

#endif
