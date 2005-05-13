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
// $Id: LauncherDialog.hxx,v 1.6 2005-05-13 18:28:05 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

class DialogContainer;
class LauncherOptionsDialog;
class CommandSender;
class StaticTextWidget;
class ListWidget;
class OSystem;

#include "Dialog.hxx"
#include "GameList.hxx"
#include "bspf.hxx"

class LauncherDialog : public Dialog
{
  public:
    LauncherDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~LauncherDialog();

    virtual void handleCommand(CommandSender* sender, int cmd, int data);

  protected:
    void updateListing(bool fullReload = false);
	
    void close();
    void loadConfig();

  protected:
    ListWidget*       myList;
    StaticTextWidget* myNote;
    StaticTextWidget* myRomCount;
    GameList*         myGameList;

    LauncherOptionsDialog* myOptions;

  private:
    void loadListFromDisk();
    void loadListFromCache();
    void createListCache();
    string MD5FromFile(const string& path);
};

#endif
