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
// $Id: OptionsDialog.hxx,v 1.1 2005-03-10 22:59:40 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef OPTIONS_DIALOG_HXX
#define OPTIONS_DIALOG_HXX

class CommandSender;
class Dialog;
class VideoDialog;
class AudioDialog;
class EventMappingDialog;
class MiscDialog;
class GameInfoDialog;
class HelpDialog;

#include "OSystem.hxx"
#include "bspf.hxx"

class OptionsDialog : public Dialog
{
  public:
    OptionsDialog(OSystem* osystem);
    ~OptionsDialog();

    virtual void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);

  protected:
    VideoDialog*        myVideoDialog;
    AudioDialog*        myAudioDialog;
    EventMappingDialog* myEventMappingDialog;
    MiscDialog*         myMiscDialog;
    GameInfoDialog*     myGameInfoDialog;
    HelpDialog*         myHelpDialog;
};

#endif
