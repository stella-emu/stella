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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
    EventMappingWidget(GuiObject* boss, const GUI::Font& font,
                       int x, int y, int w, int h,
                       const StringList& actions, EventMode mode);
    ~EventMappingWidget();

    bool handleKeyDown(int ascii, int keycode, int modifiers);
    void handleJoyDown(int stick, int button);
    void handleJoyAxis(int stick, int axis, int value);
    bool handleJoyHat(int stick, int hat, int value);
 
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
    // Since this widget can be used for different collections of events,
    // we need to specify exactly which group of events we are remapping
    EventMode myEventMode;

    // Indicates the event that is currently selected
    int myActionSelected;

    // Indicates if we're currently in remap mode
    // In this mode, the next event received is remapped to some action
    bool myRemapStatus;

    bool myFirstTime;
};

#endif
