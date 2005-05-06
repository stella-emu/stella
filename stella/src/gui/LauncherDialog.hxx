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
// $Id: LauncherDialog.hxx,v 1.1 2005-05-06 18:39:00 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

class CommandSender;
class ButtonWidget;
class StaticTextWidget;
class BrowserDialog;
class ListWidget;

#include "Dialog.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "bspf.hxx"

class LauncherDialog : public Dialog
{
  public:
    LauncherDialog(OSystem* osystem, uInt16 x, uInt16 y, uInt16 w, uInt16 h);
    ~LauncherDialog();

    virtual void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);

  protected:
    void updateListing();
    void reloadListing();
    void updateButtons();
	
    void close();
    virtual void addGame();
    void removeGame(int item);
    void editGame(int item);
    void loadConfig();

  protected:
    ListWidget*    myList;
    BrowserDialog* myBrowser;
};

#endif
