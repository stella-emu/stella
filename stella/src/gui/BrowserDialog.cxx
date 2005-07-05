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
// $Id: BrowserDialog.cxx,v 1.7 2005-07-05 15:25:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ListWidget.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GuiObject.hxx"
#include "GuiUtils.hxx"
#include "BrowserDialog.hxx"

#include "bspf.hxx"

enum {
  kChooseCmd = 'CHOS',
  kGoUpCmd   = 'GOUP'
};

/* We want to use this as a general directory selector at some point... possible uses
 * - to select the data dir for a game
 * - to select the place where save games are stored
 * - others???
 */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(GuiObject* boss, int x, int y, int w, int h)
  : Dialog(boss->instance(), boss->parent(), x, y, w, h),
    CommandSender(boss),
    _fileList(NULL),
    _currentPath(NULL)
{
  _title = new StaticTextWidget(this, 10, 8, _w - 2 * 10, kLineHeight, "", kTextAlignCenter);

  // Current path - TODO: handle long paths ?
  _currentPath = new StaticTextWidget(this, 10, 20, _w - 2 * 10, kLineHeight,
                                      "DUMMY", kTextAlignLeft);

  // Add file list
  _fileList = new ListWidget(this, 10, 34, _w - 2 * 10, _h - 34 - 24 - 10);
  _fileList->setNumberingMode(kListNumberingOff);
  _fileList->setEditable(false);
  _fileList->clearFlags(WIDGET_TAB_NAVIGATE);

  // Buttons
  addButton(10, _h - 24, "Go up", kGoUpCmd, 0);
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "Choose", kChooseCmd, 0);
  addButton(_w - (kButtonWidth+10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth+10), _h - 24, "Choose", kChooseCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::setStartPath(const string& startpath)
{
  // If no node has been set, or the last used one is now invalid,
  // go back to the root/default dir.
  _choice = FilesystemNode(startpath);

  if (_choice.isValid())
    _node = _choice;
  else if (!_node.isValid())
    _node = FilesystemNode();

  // Alway refresh file list
  updateListing();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  switch (cmd)
  {
    case kChooseCmd:
    {
      // If nothing is selected in the list widget, choose the current dir.
      // Else, choose the dir that is selected.
      int selection = _fileList->getSelected();
      if (selection >= 0)
        _choice = _nodeContent[selection];
      else
        _choice = _node;

      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(_cmd)
        sendCommand(_cmd, 0, 0);

      close();
      break;
    }

    case kGoUpCmd:
      _node = _node.getParent();
      updateListing();
      break;

    case kListItemActivatedCmd:
    case kListItemDoubleClickedCmd:
      _node = _nodeContent[data];
      updateListing();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::updateListing()
{
  // Update the path display
  _currentPath->setLabel(_node.path());

  // Read in the data from the file system
  _nodeContent = _node.listDir();
  _nodeContent.sort();

  // Populate the ListWidget
  StringList list;
  int size = _nodeContent.size();
  for (int i = 0; i < size; i++)
    list.push_back(_nodeContent[i].displayName());

  _fileList->setList(list);
  _fileList->scrollTo(0);

  // Finally, redraw
  draw();
}
