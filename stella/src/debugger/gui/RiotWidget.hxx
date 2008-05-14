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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RiotWidget.hxx,v 1.3 2008-05-14 18:04:58 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RIOT_WIDGET_HXX
#define RIOT_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class DataGridWidget;
class ToggleBitWidget;

#include "Array.hxx"
#include "Widget.hxx"
#include "Command.hxx"

class RiotWidget : public Widget, public CommandSender
{
  public:
    RiotWidget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h);
    virtual ~RiotWidget();

    void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:

  private:
    ToggleBitWidget* mySWCHAReadBits;
    ToggleBitWidget* mySWCHAWriteBits;
    ToggleBitWidget* mySWACNTBits;
    ToggleBitWidget* mySWCHBBits;

    DataGridWidget* myTimWrite;
    DataGridWidget* myTimRead;

    CheckboxWidget* myP0Dir,  *myP1Dir;
    CheckboxWidget* myP0Diff, *myP1Diff;
    CheckboxWidget* myTVType;
    CheckboxWidget* mySwitches;

    StaticTextWidget* myP0DirText,  *myP1DirText;
    StaticTextWidget* myP0DiffText, *myP1DiffText;
    StaticTextWidget* myTVTypeText;
    StaticTextWidget* mySwitchesText;

    // ID's for the various widgets
    // We need ID's, since there are more than one of several types of widgets
    enum {
      kTim1TID = 0, kTim8TID = 1, kTim64TID = 2, kTim1024TID = 3,
      kSWCHABitsID, kSWACNTBitsID, kSWCHBBitsID, kTimWriteID
    };
};

#endif
