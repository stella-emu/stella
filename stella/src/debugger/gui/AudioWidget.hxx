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
// $Id: AudioWidget.hxx,v 1.5 2008-02-06 13:45:20 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef AUDIO_WIDGET_HXX
#define AUDIO_WIDGET_HXX

class GuiObject;
class DataGridWidget;

#include "Widget.hxx"
#include "Command.hxx"


class AudioWidget : public Widget, public CommandSender
{
  public:
    AudioWidget(GuiObject* boss, const GUI::Font& font,
                int x, int y, int w, int h);
    virtual ~AudioWidget();

    void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:
    void fillGrid();

  private:
    DataGridWidget* myAudF;
    DataGridWidget* myAudC;
    DataGridWidget* myAudV;
};

#endif
