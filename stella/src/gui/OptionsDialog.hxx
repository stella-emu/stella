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
// $Id: OptionsDialog.hxx,v 1.18 2006-11-18 13:29:11 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef OPTIONS_DIALOG_HXX
#define OPTIONS_DIALOG_HXX

class Properties;
class CommandSender;
class DialogContainer;
class AudioDialog;
class InputDialog;
class GameInfoDialog;
class CheatCodeDialog;
class HelpDialog;
class AboutDialog;
class OSystem;

#include "Dialog.hxx"
#include "GameInfoDialog.hxx"
#include "bspf.hxx"

class OptionsDialog : public Dialog
{
  public:
    OptionsDialog(OSystem* osystem, DialogContainer* parent);
    virtual ~OptionsDialog();

    void setGameProfile(Properties& props) { myGameInfoDialog->setGameProfile(props); }

  private:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void checkBounds(int width, int height, int* x, int* y, int* w, int* h);

  private:
    VideoDialog*     myVideoDialog;
    AudioDialog*     myAudioDialog;
    InputDialog*     myInputDialog;
    GameInfoDialog*  myGameInfoDialog;
    CheatCodeDialog* myCheatCodeDialog;
    HelpDialog*      myHelpDialog;
    AboutDialog*     myAboutDialog;
};

#endif
