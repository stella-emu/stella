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
// $Id: EventMappingDialog.hxx,v 1.10 2005-06-16 00:55:59 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EVENT_MAPPING_DIALOG_HXX
#define EVENT_MAPPING_DIALOG_HXX

class DialogContainer;
class CommandSender;
class ButtonWidget;
class StaticTextWidget;
class ListWidget;
class PopUpWidget;

#include "OSystem.hxx"
#include "bspf.hxx"

class EventMappingDialog : public Dialog
{
  public:
    EventMappingDialog(OSystem* osystem, DialogContainer* parent,
                       int x, int y, int w, int h);
    ~EventMappingDialog();

    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleJoyDown(int stick, int button);

  protected:
    ButtonWidget*     myMapButton;
    ButtonWidget*     myCancelMapButton;
    ButtonWidget*     myEraseButton;
    ButtonWidget*     myOKButton;
    ButtonWidget*     myDefaultsButton;
    ListWidget*       myActionsList;
    StaticTextWidget* myKeyMapping;
    PopUpWidget*      myPaddleModePopup;
    StaticTextWidget* myPaddleModeText;

  private:
    enum {
      kStartMapCmd = 'map ',
      kEraseCmd    = 'eras',
      kStopMapCmd  = 'smap'
    };

    virtual void handleCommand(CommandSender* sender, int cmd, int data);

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
