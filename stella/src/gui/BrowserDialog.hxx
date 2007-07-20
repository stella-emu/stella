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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: BrowserDialog.hxx,v 1.12 2007-07-20 13:31:10 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef BROWSER_DIALOG_HXX
#define BROWSER_DIALOG_HXX

class GuiObject;
class ButtonWidget;
class StaticTextWidget;
class StringListWidget;
class GameList;

#include "Dialog.hxx"
#include "Command.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"

class BrowserDialog : public Dialog, public CommandSender
{
  public:
    BrowserDialog(GuiObject* boss, const GUI::Font& font,
                  int x, int y, int w, int h);

    virtual ~BrowserDialog();

    const FilesystemNode& getResult() { return _node; }

    void setTitle(const string& title) { _title->setLabel(title); }
    void setEmitSignal(int cmd) { _cmd = cmd; }
    void setStartPath(const string& startpath, FilesystemNode::ListMode mode);

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void updateListing();

  private:
    enum {
      kChooseCmd = 'CHOS',
      kGoUpCmd   = 'GOUP'
    };

    int	_cmd;

    StringListWidget* _fileList;
    StaticTextWidget* _currentPath;
    StaticTextWidget* _title;
    ButtonWidget*     _goUpButton;

    FilesystemNode _node;
    GameList*      _nodeList;

    FilesystemNode::ListMode _mode;
};

#endif
