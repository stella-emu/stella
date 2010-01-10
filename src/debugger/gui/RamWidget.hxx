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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RAM_WIDGET_HXX
#define RAM_WIDGET_HXX

class GuiObject;
class InputTextDialog;
class ButtonWidget;
class EditTextWidget;
class StaticTextWidget;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "DataGridWidget.hxx"


class RamWidget : public Widget, public CommandSender
{
  public:
    RamWidget(GuiObject* boss, const GUI::Font& font, int x, int y);
    virtual ~RamWidget();

    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void loadConfig();
    void setOpsWidget(DataGridOpsWidget* w) { myRamGrid->setOpsWidget(w); }

  private:
    void fillGrid(bool updateOld);

    void showInputBox(int cmd);
    string doSearch(const string& str);
    string doCompare(const string& str);
    void doRestart();
    void showSearchResults();

  private:
    enum {
      kUndoCmd     = 'RWud',
      kRevertCmd   = 'RWrv',
      kSearchCmd   = 'RWse',
      kCmpCmd      = 'RWcp',
      kRestartCmd  = 'RWrs',
      kSValEntered = 'RWsv',
      kCValEntered = 'RWcv'
    };

    int myUndoAddress;
    int myUndoValue;
    int myCurrentRamBank;

    StaticTextWidget* myRamStart;
    StaticTextWidget* myRamLabels[8];
    DataGridWidget*   myRamGrid;

    EditTextWidget* myBinValue;
    EditTextWidget* myDecValue;
    EditTextWidget* myLabel;

    ButtonWidget* myRevertButton;
    ButtonWidget* myUndoButton;
    ButtonWidget* mySearchButton;
    ButtonWidget* myCompareButton;
    ButtonWidget* myRestartButton;

    InputTextDialog* myInputBox;

    IntArray myOldValueList;
    IntArray mySearchAddr;
    IntArray mySearchValue;
    BoolArray mySearchState;
};

#endif
