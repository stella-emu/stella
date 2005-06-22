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
// $Id: CpuWidget.hxx,v 1.3 2005-06-22 18:30:43 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef CPU_WIDGET_HXX
#define CPU_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class EditTextWidget;
class DataGridWidget;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"


class CpuWidget : public Widget, public CommandSender
{
  public:
    CpuWidget(GuiObject* boss, int x, int y, int w, int h);
    virtual ~CpuWidget();

    Widget* activeWidget() { return myActiveWidget; }

    void handleCommand(CommandSender* sender, int cmd, int data);
    void loadConfig();

  private:
    void fillGrid();

  private:
    Widget* myActiveWidget;

    DataGridWidget* myCpuGrid;
    // FIXME - add a ToggleBitWidget for processor status (ps) register

    EditTextWidget* myPCLabel;
    EditTextWidget* myCurrentIns;
    EditTextWidget* myCycleCount;
    EditTextWidget* myStatus;
};

#endif
