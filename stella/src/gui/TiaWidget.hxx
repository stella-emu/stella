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
// $Id: TiaWidget.hxx,v 1.2 2005-07-05 15:25:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef TIA_WIDGET_HXX
#define TIA_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class StaticTextWidget;
class EditTextWidget;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "DataGridWidget.hxx"


class TiaWidget : public Widget, public CommandSender
{
  public:
    TiaWidget(GuiObject* boss, int x, int y, int w, int h);
    virtual ~TiaWidget();

    Widget* activeWidget() { return myActiveWidget; }

    void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:

  private:
    Widget* myActiveWidget;

    DataGridWidget* myRamGrid;
    EditTextWidget* myBinValue;
    EditTextWidget* myDecValue;
    EditTextWidget* myLabel;
};

#endif
