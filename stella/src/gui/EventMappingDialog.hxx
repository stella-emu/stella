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
// $Id: EventMappingDialog.hxx,v 1.3 2005-04-06 19:50:12 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EVENT_MAPPING_DIALOG_HXX
#define EVENT_MAPPING_DIALOG_HXX

class CommandSender;
class ButtonWidget;
class StaticTextWidget;
class ListWidget;

#include "OSystem.hxx"
#include "bspf.hxx"

class EventMappingDialog : public Dialog
{
  public:
    EventMappingDialog(OSystem* osystem, uInt16 x, uInt16 y, uInt16 w, uInt16 h);
    ~EventMappingDialog();

    virtual void handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers);

  protected:
    ButtonWidget*     myMapButton;
    ButtonWidget*     myCancelMapButton;
    ButtonWidget*     myEraseButton;
    ButtonWidget*     myOKButton;
    ButtonWidget*     myDefaultsButton;
    ListWidget*       myActionsList;
    StaticTextWidget* myKeyMapping;

  private:
    enum {
      kStartMapCmd = 'map ',
      kEraseCmd    = 'eras',
      kStopMapCmd  = 'smap'
    };

    virtual void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);

    void startRemapping();
    void eraseRemapping();
    void stopRemapping();
    void loadConfig();

  private:
    // Indicates the event that is currently selected
    Int32 myActionSelected;

    // Indicates if we're currently in remap mode
    // In this mode, the next event received is remapped to some action
    bool myRemapStatus;
};

#endif
