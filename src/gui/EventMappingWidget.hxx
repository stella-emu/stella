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

#ifndef EVENT_MAPPING_WIDGET_HXX
#define EVENT_MAPPING_WIDGET_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class EditTextWidget;
class StaticTextWidget;
class StringListWidget;
class PopUpWidget;
class GuiObject;
class ComboDialog;
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

    void setDefaults();

  private:
    enum {
      kStartMapCmd = 'map ',
      kStopMapCmd  = 'smap',
      kEraseCmd    = 'eras',
      kResetCmd    = 'rest',
      kComboCmd    = 'cmbo'
    };

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void startRemapping();
    void eraseRemapping();
    void resetRemapping();
    void stopRemapping();
    void loadConfig();
    void saveConfig();

    void drawKeyMapping();
    void enableButtons(bool state);

  private:
    ButtonWidget*     myMapButton;
    ButtonWidget*     myCancelMapButton;
    ButtonWidget*     myEraseButton;
    ButtonWidget*     myResetButton;
    ButtonWidget*     myComboButton;
    StringListWidget* myActionsList;
    EditTextWidget*   myKeyMapping;

    ComboDialog* myComboDialog;

    // Since this widget can be used for different collections of events,
    // we need to specify exactly which group of events we are remapping
    EventMode myEventMode;

    // Indicates the event that is currently selected
    int myActionSelected;

    // Indicates if we're currently in remap mode
    // In this mode, the next event received is remapped to some action
    bool myRemapStatus;

    // Joystick axes and hats can be more problematic than ordinary buttons
    // or keys, in that there can be 'drift' in the values
    // Therefore, we map these events when they've been 'released', rather
    // than on their first occurrence (aka, when they're 'pressed')
    // As a result, we need to keep track of their old values
    int myLastStick, myLastAxis, myLastHat, myLastValue;

    bool myFirstTime;
};

#endif
