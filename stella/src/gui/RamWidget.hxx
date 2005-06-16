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
// $Id: RamWidget.hxx,v 1.1 2005-06-16 18:40:17 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RAM_WIDGET_HXX
#define RAM_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class StaticTextWidget;
class ByteGridWidget;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"


class RamWidget : public Widget, public CommandSender
{
  public:
    RamWidget(GuiObject* boss, int x, int y, int w, int h);
    virtual ~RamWidget();

    Widget* activeWidget() { return myActiveWidget; }

    void handleCommand(CommandSender* sender, int cmd, int data);

  public:
    void fillGrid();

  private:
    Widget* myActiveWidget;

    ByteGridWidget* myRamGrid;
};

#endif
