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
// $Id: BrowserDialog.cxx,v 1.19 2006-05-04 17:45:25 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "StringListWidget.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GuiObject.hxx"
#include "GuiUtils.hxx"
#include "BrowserDialog.hxx"

#include "bspf.hxx"

/* We want to use this as a general directory selector at some point... possible uses
 * - to select the data dir for a game
 * - to select the place where save games are stored
 * - others???
 * TODO - make this dialog font sensitive
 */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(GuiObject* boss, const GUI::Font& font,
                             int x, int y, int w, int h)
  : Dialog(boss->instance(), boss->parent(), x, y, w, h),
    CommandSender(boss),
    _fileList(NULL),
    _currentPath(NULL)
{
  const int lineHeight = font.getLineHeight(),
            bwidth     = font.getStringWidth("Cancel") + 20,
            bheight    = font.getLineHeight() + 4;
  int xpos, ypos;
  ButtonWidget* b;

  xpos = 10;  ypos = 4;
  _title = new StaticTextWidget(this, font, xpos, ypos,
                                _w - 2 * xpos, lineHeight,
                                "", kTextAlignCenter);

  // Current path - TODO: handle long paths ?
  ypos += lineHeight + 4;
  _currentPath = new StaticTextWidget(this, font, xpos, ypos,
                                       _w - 2 * xpos, lineHeight,
                                      "DUMMY", kTextAlignLeft);

  // Add file list
  ypos += lineHeight;
  _fileList = new StringListWidget(this, font, xpos, ypos,
                                   _w - 2 * xpos, _h - bheight - ypos - 15);
  _fileList->setNumberingMode(kListNumberingOff);
  _fileList->setEditable(false);

  // Buttons
  xpos = 10;  ypos = _h - bheight - 8;
  b = _goUpButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                     "Go up", kGoUpCmd);
  addFocusWidget(b);
#ifndef MAC_OSX
  xpos = _w - 2 *(bwidth + 10);  
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Choose",
                       kChooseCmd);
  addFocusWidget(b);
  xpos += bwidth + 10;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cancel",
                       kCloseCmd);
  addFocusWidget(b);
#else
  xpos = _w - 2 *(bwidth + 10);  ypos = _h - bheight - 8;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Cancel",
                       kCloseCmd);
  addFocusWidget(b);
  xpos += bwidth + 10;
  b = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Choose",
                       kChooseCmd);
  addFocusWidget(b);
#endif

  addFocusWidget(_fileList);
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
      if (selection >= 0 && selection < (int)_nodeContent.size())
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
  {
    if(_nodeContent[i].isDirectory())
      list.push_back(" [" + _nodeContent[i].displayName() + "]");
    else
      list.push_back(_nodeContent[i].displayName());
  }

  _fileList->setList(list);
  if(size > 0)
    _fileList->setSelected(0);

  // Only hilite the 'up' button if there's a parent directory
  _goUpButton->setEnabled(_node.hasParent());

  // Finally, redraw
  setDirty(); draw();
}
