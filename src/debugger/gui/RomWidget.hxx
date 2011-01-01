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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
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

#ifndef ROM_WIDGET_HXX
#define ROM_WIDGET_HXX

class GuiObject;
class DataGridWidget;
class EditTextWidget;
class InputTextDialog;
class PopUpWidget;
class StringList;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "CartDebug.hxx"
#include "RomListWidget.hxx"

class RomWidget : public Widget, public CommandSender
{
  public:
    RomWidget(GuiObject* boss, const GUI::Font& font, int x, int y);
    virtual ~RomWidget();

    void invalidate(bool forcereload = true)
    { myListIsDirty = true; if(forcereload) loadConfig(); }

    void scrollTo(int line) { myRomList->setSelected(line); }
    void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:
    void setBank(uInt16 bank);
    void setBreak(int disasm_line, bool state);
    void setPC(int disasm_line);
    void runtoPC(int disasm_line);
    void patchROM(int disasm_line, const string& bytes);
    void saveROM(const string& rom);

  private:
    enum {
      kResolveDataChanged = 'ACrd',
      kRomNameEntered  = 'RWrn'
    };

    RomListWidget*   myRomList;
    DataGridWidget*  myBank;
    PopUpWidget*     myResolveData;
    InputTextDialog* mySaveRom;

    bool myListIsDirty;
    int  myCurrentBank;
};

#endif
