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
// $Id: BrowserDialog.cxx,v 1.33 2009-01-01 18:13:38 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GameList.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "StringListWidget.hxx"
#include "Widget.hxx"

#include "BrowserDialog.hxx"

/* We want to use this as a general directory selector at some point... possible uses
 * - to select the data dir for a game
 * - to select the place where save games are stored
 * - others???
 * TODO - make this dialog font sensitive
 */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h)
  : Dialog(&boss->instance(), &boss->parent(), x, y, w, h),
    CommandSender(boss),
    _fileList(NULL),
    _currentPath(NULL),
    _nodeList(NULL),
    _mode(AbstractFilesystemNode::kListDirectoriesOnly)
{
  const int lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  ButtonWidget* b;

  // Set real dimensions
  // This is one dialog that can take as much space as is available
//  _w = _DLG_MIN_SWIDTH - 30;
//  _h = _DLG_MIN_SHEIGHT - 30;

  xpos = 10;  ypos = 4;
  _title = new StaticTextWidget(this, font, xpos, ypos,
                                _w - 2 * xpos, lineHeight,
                                "", kTextAlignCenter);

  // Current path - TODO: handle long paths ?
  ypos += lineHeight + 4;
  _currentPath = new StaticTextWidget(this, font, xpos, ypos,
                                       _w - 2 * xpos, lineHeight,
                                      "", kTextAlignLeft);

  // Add file list
  ypos += lineHeight;
  _fileList = new StringListWidget(this, font, xpos, ypos,
                                   _w - 2 * xpos, _h - buttonHeight - ypos - 20);
  _fileList->setNumberingMode(kListNumberingOff);
  _fileList->setEditable(false);

  // Buttons
  _goUpButton = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                                 buttonWidth, buttonHeight, "Go up", kGoUpCmd);
  addFocusWidget(_goUpButton);

#ifndef MAC_OSX
  b = new ButtonWidget(this, font, _w - 2 * (buttonWidth + 7), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Choose", kChooseCmd);
  addFocusWidget(b);
  addOKWidget(b);
  b = new ButtonWidget(this, font, _w - (buttonWidth + 10), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Cancel", kCloseCmd);
  addFocusWidget(b);
  addCancelWidget(b);
#else
  b = new ButtonWidget(this, font, _w - 2 * (buttonWidth + 7), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Cancel", kCloseCmd);
  addFocusWidget(b);
  addCancelWidget(b);
  b = new ButtonWidget(this, font, _w - (buttonWidth + 10), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Choose", kChooseCmd);
  addFocusWidget(b);
  addOKWidget(b);
#endif

  addFocusWidget(_fileList);

  // Create a list of directory nodes
  _nodeList = new GameList();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::~BrowserDialog()
{
  delete _nodeList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::setStartPath(const string& startpath,
                                 FilesystemNode::ListMode mode)
{
  _mode = mode;

  // If no node has been set, or the last used one is now invalid,
  // go back to the root/default dir.
  _node = FilesystemNode(startpath);

  if(!_node.isValid())
    _node = FilesystemNode();

  // Generally, we always want a directory listing 
  if(!_node.isDirectory() && _node.hasParent())
    _node = _node.getParent();

  // Alway refresh file list
  updateListing();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::updateListing()
{
  if(!_node.isDirectory())
    return;

  // Start with empty list
  _nodeList->clear();

  // Update the path display
  _currentPath->setLabel(_node.path());

  // Read in the data from the file system
  FSList content = _node.listDir(_mode);

  // Add '[..]' to indicate previous folder
  if(_node.hasParent())
  {
    const string& parent = _node.getParent().path();
    _nodeList->appendGame(" [..]", parent, "", true);
  }

  // Now add the directory entries
  for(unsigned int idx = 0; idx < content.size(); idx++)
  {
    string name = content[idx].displayName();
    bool isDir = content[idx].isDirectory();
    if(isDir)
      name = " [" + name + "]";

    _nodeList->appendGame(name, content[idx].path(), "", isDir);
  }
  _nodeList->sortByName();

  // Now fill the list widget with the contents of the GameList
  StringList l;
  for (int i = 0; i < (int) _nodeList->size(); ++i)
    l.push_back(_nodeList->name(i));

  _fileList->setList(l);
  if(_fileList->getList().size() > 0)
    _fileList->setSelected(0);

  // Only hilite the 'up' button if there's a parent directory
  _goUpButton->setEnabled(_node.hasParent());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  switch (cmd)
  {
    case kChooseCmd:
      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(_cmd) sendCommand(_cmd, 0, 0);
      close();
      break;

    case kGoUpCmd:
      _node = _node.getParent();
      updateListing();
      break;

    case kListItemActivatedCmd:
    case kListItemDoubleClickedCmd:
    {
      int item = _fileList->getSelected();
      if(item >= 0)
      {
        _node = _nodeList->path(item);
        updateListing();
      }
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
