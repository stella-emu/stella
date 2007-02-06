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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: GameInfoDialog.hxx,v 1.22 2007-02-06 23:34:34 stephena Exp $
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
class SliderWidget;

#include "Array.hxx"
#include "Dialog.hxx"
#include "Command.hxx"

class GameInfoDialog : public Dialog, public CommandSender
{
  public:
    GameInfoDialog(OSystem* osystem, DialogContainer* parent,
                   const GUI::Font& font, GuiObject* boss,
                   int x, int y, int w, int h);
    virtual ~GameInfoDialog();

  protected:
    void loadConfig();
    void saveConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    void setDefaults();
    void loadView();

  private:
    TabWidget* myTab;
    ButtonWidget* myCancelButton;

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
    PopUpWidget* mySwapPaddles;

    // Display properties
    PopUpWidget*      myFormat;
    EditTextWidget*   myYStart;
    EditTextWidget*   myHeight;
    PopUpWidget*      myPhosphor;
    SliderWidget*     myPPBlend;
    StaticTextWidget* myPPBlendLabel;
    PopUpWidget*      myHmoveBlanks;

    // Structure used for cartridge and controller types
    struct PropType {
      const char* name;
      const char* comparitor;
    };

    enum {
      kPhosphorChanged = 'PPch',
      kPPBlendChanged  = 'PBch'
    };

    /** Game properties for currently loaded ROM */
    Properties myGameProperties;

    /** Indicates that the default properties have been loaded */
    bool myDefaultsSelected;

    /** Holds static strings for Cartridge type */
    static const char* ourCartridgeList[21][2];

    /** Holds static strings for Controller type */
    static const char* ourControllerList[5][2];
};

#endif
