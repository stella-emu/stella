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

#ifndef ROM_LIST_SETTINGS_HXX
#define ROM_LIST_SETTINGS_HXX

class CheckboxWidget;
class EditTextWidget;

#include "Command.hxx"
#include "Dialog.hxx"

/**
 * A dialog which controls the settings for the RomListWidget.
 * Currently, all Distella disassembler options are located here as well.
 */
class RomListSettings : public Dialog, public CommandSender
{
  public:
    RomListSettings(GuiObject* boss, const GUI::Font& font);
    virtual ~RomListSettings();

    /** Show dialog onscreen at the specified coordinates
        ('data' will be the currently selected line number in RomListWidget) */
    void show(uInt32 x, uInt32 y, int data = -1);

    /** This dialog uses its own positioning, so we override Dialog::center() */
    void center();

  private:
    void loadConfig();
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    uInt32 _xorig, _yorig;
    int _item; // currently selected line number in the disassembly list

    CheckboxWidget* myShowTentative;
    CheckboxWidget* myShowAddresses;
    CheckboxWidget* myShowGFXBinary;
    CheckboxWidget* myUseRelocation;
};

#endif
