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
// $Id: EventMappingWidget.hxx,v 1.4 2005-12-24 22:09:36 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EVENT_MAPPING_WIDGET_HXX
#define EVENT_MAPPING_WIDGET_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;
class StringListWidget;
class PopUpWidget;
class GuiObject;
class InputDialog;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"


class EventMappingWidget : public Widget, public CommandSender
{
  friend class InputDialog;

  public:
    EventMappingWidget(GuiObject* boss, int x, int y, int w, int h);
    ~EventMappingWidget();

    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleJoyDown(int stick, int button);
    virtual void handleJoyAxis(int stick, int axis, int value);
 
    bool remapMode() { return myRemapStatus; }

  protected:
    ButtonWidget*     myMapButton;
    ButtonWidget*     myCancelMapButton;
    ButtonWidget*     myEraseButton;
    ButtonWidget*     myDefaultsButton;
    StringListWidget* myActionsList;
    StaticTextWidget* myKeyMapping;

  private:
    enum {
      kStartMapCmd = 'map ',
      kEraseCmd    = 'eras',
      kStopMapCmd  = 'smap'
    };

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void startRemapping();
    void eraseRemapping();
    void stopRemapping();
    void loadConfig();
    void saveConfig();

    void drawKeyMapping();

  private:
    // Indicates the event that is currently selected
    int myActionSelected;

    // Indicates if we're currently in remap mode
    // In this mode, the next event received is remapped to some action
    bool myRemapStatus;
};

#endif
