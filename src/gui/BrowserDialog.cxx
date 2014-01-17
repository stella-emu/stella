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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"

#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "FSNode.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "FileListWidget.hxx"
#include "Widget.hxx"

#include "BrowserDialog.hxx"

/* We want to use this as a general directory selector at some point... possible uses
 * - to select the data dir for a game
 * - to select the place where save games are stored
 * - others???
 */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(GuiObject* boss, const GUI::Font& font,
                             int max_w, int max_h)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 0, 0),
    CommandSender(boss)
{
  // Set real dimensions
  _w = max_w;
  _h = max_h;

  const int lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4,
            selectHeight = lineHeight + 8;
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
                                      "", kTextAlignLeft);

  // Add file list
  ypos += lineHeight + 4;
  _fileList = new FileListWidget(this, font, xpos, ypos, _w - 2 * xpos,
                                 _h - selectHeight - buttonHeight - ypos - 20);
  _fileList->setEditable(false);
  addFocusWidget(_fileList);

  // Add currently selected item
  ypos += _fileList->getHeight() + 4;

  _type = new StaticTextWidget(this, font, xpos, ypos,
                               font.getStringWidth("Name: "), lineHeight,
                               "Name:", kTextAlignCenter);
  _selected = new EditTextWidget(this, font, xpos + _type->getWidth(), ypos, 
                                 _w - _type->getWidth() - 2 * xpos, lineHeight, "");
  _selected->setEditable(false);
  addFocusWidget(_selected);

  // Buttons
  _goUpButton = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                                 buttonWidth, buttonHeight, "Go up", kGoUpCmd);
  addFocusWidget(_goUpButton);

  _basedirButton =
    new ButtonWidget(this, font, 15 + buttonWidth, _h - buttonHeight - 10,
                     buttonWidth, buttonHeight, "Base Dir", kBaseDirCmd);
  addFocusWidget(_basedirButton);

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::~BrowserDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::show(const string& title, const string& startpath,
                         BrowserDialog::ListMode mode, int cmd,
                         const string& ext)
{
  _title->setLabel(title);
  _cmd = cmd;
  _mode = mode;

  switch(_mode)
  {
    case FileLoad:
      _fileList->setFileListMode(FilesystemNode::kListAll);
      _fileList->setFileExtension(ext);
      _selected->setEditable(false);
      break;

    case FileSave:
      _fileList->setFileListMode(FilesystemNode::kListAll);
      _fileList->setFileExtension(ext);
      _selected->setEditable(false);  // FIXME - disable user input for now
      break;

    case Directories:
      _fileList->setFileListMode(FilesystemNode::kListDirectoriesOnly);
      _selected->setEditable(false);
      break;
  }

  // Set start path
  _fileList->setLocation(FilesystemNode(startpath));

  updateUI();

  // Finally, open the dialog after it has been fully updated
  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& BrowserDialog::getResult() const
{
  if(_mode == FileLoad || _mode == FileSave)
    return _fileList->selected();
  else
    return _fileList->currentDir();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::updateUI()
{
  // Only hilite the 'up' button if there's a parent directory
  _goUpButton->setEnabled(_fileList->currentDir().hasParent());

  // Update the path display
  _currentPath->setLabel(_fileList->currentDir().getShortPath());

  // Enable/disable OK button based on current mode
  bool enable = _mode == Directories || !_fileList->selected().isDirectory();
  _okWidget->setEnabled(enable);

  if(!_fileList->selected().isDirectory())
    _selected->setText(_fileList->getSelectedString());
  else
    _selected->setText("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  switch (cmd)
  {
    case kChooseCmd:
    case FileListWidget::ItemActivated:
      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(_cmd) sendCommand(_cmd, -1, -1);
      close();
      break;

    case kGoUpCmd:
      _fileList->selectParent();
      break;

    case kBaseDirCmd:
      _fileList->setLocation(FilesystemNode(instance().baseDir()));
      break;

    case FileListWidget::ItemChanged:
      updateUI();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
