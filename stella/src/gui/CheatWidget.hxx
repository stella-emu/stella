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
// $Id: CheatWidget.hxx,v 1.3 2005-06-14 18:55:36 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef CHEAT_WIDGET_HXX
#define CHEAT_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class StaticTextWidget;
class EditNumWidget;
class ListWidget;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"


class CheatWidget : public Widget, public CommandSender
{
  private:
    struct AddrValue {
      uInt16 addr;
      uInt8 value;
    };

    typedef GUI::Array<AddrValue> AddrValueList;

    AddrValueList mySearchArray;
    AddrValueList myCompareArray;

  public:
    CheatWidget(GuiObject *boss, int x, int y, int w, int h);
    virtual ~CheatWidget();

    Widget* activeWidget() { return myActiveWidget; }

    void handleCommand(CommandSender* sender, int cmd, int data);

  private:
    void doSearch();
    void doCompare();
    void doRestart();

    void fillResultsList();

  private:
    Widget* myActiveWidget;

    EditNumWidget* myEditBox;
    StaticTextWidget* myResult;

    ButtonWidget* mySearchButton;
    ButtonWidget* myCompareButton;
    ButtonWidget* myRestartButton;

    ListWidget* myResultsList;
};

#endif
