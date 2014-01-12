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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef GLOBAL_PROPS_DIALOG_HXX
#define GLOBAL_PROPS_DIALOG_HXX

class CommandSender;
class DialogContainer;
class CheckboxWidget;
class PopUpWidget;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

class GlobalPropsDialog : public Dialog, public CommandSender
{
  public:
    GlobalPropsDialog(GuiObject* boss, const GUI::Font& font);
    virtual ~GlobalPropsDialog();

  private:
    int addHoldWidgets(const GUI::Font& font, int x, int y, WidgetArray& wid);

    void loadConfig();
    void saveConfig();
    void setDefaults();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    enum {
      kJ0Up, kJ0Down, kJ0Left, kJ0Right, kJ0Fire,
      kJ1Up, kJ1Down, kJ1Left, kJ1Right, kJ1Fire
    };

    PopUpWidget* myBSType;
    PopUpWidget* myLeftDiff;
    PopUpWidget* myRightDiff;
    PopUpWidget* myTVType;
    PopUpWidget* myDebug;

    CheckboxWidget* myJoy[10];
    CheckboxWidget* myHoldSelect;
    CheckboxWidget* myHoldReset;

    static const char* ourJoyState[10];
};

#endif
