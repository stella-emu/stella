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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "EditTextWidget.hxx"
#include "FileListWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "BrowserDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(GuiObject* boss, const GUI::Font& font,
                             int max_w, int max_h)
  : Dialog(boss->instance(), boss->parent(), font, "Title") // dummy title value!
{
  // Set real dimensions
  _w = max_w;
  _h = max_h;
  const int lineHeight   = Dialog::lineHeight(),
            //fontHeight   = Dialog::fontHeight(),
            //fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("Base Dir"),
            BUTTON_GAP   = Dialog::buttonGap(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int selectHeight = lineHeight + VGAP * 3;
  int xpos, ypos;
  ButtonWidget* b;

  xpos = HBORDER;  ypos = VBORDER + _th;

  // Current path - TODO: handle long paths ?
  StaticTextWidget* t = new StaticTextWidget(this, font, xpos, ypos + 2, "Path ");
  _currentPath = new EditTextWidget(this, font, xpos + t->getWidth(), ypos,
                                    _w - t->getWidth() - 2 * xpos, lineHeight);
  _currentPath->setEditable(false);

  xpos = _w - (HBORDER + _font.getStringWidth("Save") + CheckboxWidget::prefixSize(_font));
  _savePathBox = new CheckboxWidget(this, font, xpos, ypos + 2, "Save");
  _savePathBox->setToolTip("Check to save current path as default.");

  // Add file list
  xpos = HBORDER; ypos += lineHeight + VGAP * 2;
  _fileList = new FileListWidget(this, font, xpos, ypos, _w - 2 * xpos,
                                 _h - selectHeight - buttonHeight - ypos - VBORDER * 2);
  _fileList->setEditable(false);
  addFocusWidget(_fileList);

  // Add currently selected item
  ypos += _fileList->getHeight() + VGAP * 2;

  _name = new StaticTextWidget(this, font, xpos, ypos + 2, "Name ");
  _selected = new EditTextWidget(this, font, xpos + _name->getWidth(), ypos,
                                 _w - _name->getWidth() - 2 * xpos, lineHeight, "");
  addFocusWidget(_selected);

  // Buttons
  _goUpButton = new ButtonWidget(this, font, xpos, _h - buttonHeight - VBORDER,
                                 buttonWidth, buttonHeight, "Go up", kGoUpCmd);
  addFocusWidget(_goUpButton);

  b = new ButtonWidget(this, font, _goUpButton->getRight() + BUTTON_GAP, _h - buttonHeight - VBORDER,
                       buttonWidth, buttonHeight, "Base Dir", kBaseDirCmd);
  b->setToolTip("Go to Stella's base directory.");
  addFocusWidget(b);

  b = new ButtonWidget(this, font, b->getRight() + BUTTON_GAP, _h - buttonHeight - VBORDER,
                       buttonWidth, buttonHeight, "Home Dir", kHomeDirCmd);
  b->setToolTip("Go to user's home directory.");
  addFocusWidget(b);

#ifndef BSPF_MACOS
  b = new ButtonWidget(this, font, _w - (2 * buttonWidth + BUTTON_GAP + HBORDER), _h - buttonHeight - VBORDER,
                       buttonWidth, buttonHeight, "OK", kChooseCmd);
  addFocusWidget(b);
  addOKWidget(b);
  b = new ButtonWidget(this, font, _w - (buttonWidth + HBORDER), _h - buttonHeight - VBORDER,
                       buttonWidth, buttonHeight, "Cancel", GuiObject::kCloseCmd);
  addFocusWidget(b);
  addCancelWidget(b);
#else
  b = new ButtonWidget(this, font, _w - (2 * buttonWidth + BUTTON_GAP + HBORDER), _h - buttonHeight - VBORDER,
                       buttonWidth, buttonHeight, "Cancel", GuiObject::kCloseCmd);
  addFocusWidget(b);
  addCancelWidget(b);
  b = new ButtonWidget(this, font, _w - (buttonWidth + HBORDER), _h - buttonHeight - VBORDER,
                       buttonWidth, buttonHeight, "OK", kChooseCmd);
  addFocusWidget(b);
  addOKWidget(b);
#endif

  // add last to avoid focus problems
  addFocusWidget(_savePathBox);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::show(GuiObject* parent, const GUI::Font& font,
                         const string& title, const string& startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FilesystemNode::NameFilter& namefilter)
{
  uInt32 w = 0, h = 0;

  static_cast<Dialog*>(parent)->getDynamicBounds(w, h);
  if(w > uInt32(font.getMaxCharWidth() * 80))
    w = font.getMaxCharWidth() * 80;

  static unique_ptr<BrowserDialog> ourBrowser{nullptr};

  if(ourBrowser == nullptr
     || ourBrowser->getWidth() != int(w) || ourBrowser->getHeight() != int(h))
    ourBrowser = make_unique<BrowserDialog>(parent, font, w, h);

  ourBrowser->setTitle(title); // has to be always updated!
  ourBrowser->show(startpath, mode, command, namefilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::show(GuiObject* parent,
                         const string& title, const string& startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FilesystemNode::NameFilter& namefilter)
{
  show(parent, parent->instance().frameBuffer().font(), title, startpath,
       mode, command, namefilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::show(const string& startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FilesystemNode::NameFilter& namefilter)
{
  const int fontWidth = Dialog::fontWidth(),
            VGAP      = Dialog::vGap();
  _mode = mode;
  _command = command;
  string directory;
  string fileName;
  bool fileSelected = true;

  // Set start path
  if(_mode != Mode::Directories)
  {
    // split startpath into path and filename
    FilesystemNode fs = FilesystemNode(startpath);
    fileName = fs.getName();
    directory = fs.isDirectory() ? "" : fs.getParent().getPath();
  }

  switch(_mode)
  {
    case Mode::FileLoad:
      _fileList->setListMode(FilesystemNode::ListMode::All);
      _fileList->setNameFilter(namefilter);
      _fileList->setHeight(_selected->getTop() - VGAP * 2 - _fileList->getTop());

      _currentPath->setWidth(_savePathBox->getLeft() - _currentPath->getLeft() - fontWidth);
      _savePathBox->setEnabled(true);
      _savePathBox->clearFlags(Widget::FLAG_INVISIBLE);
      _savePathBox->setState(instance().settings().getBool("saveuserdir"));

      _name->clearFlags(Widget::FLAG_INVISIBLE);
      _selected->clearFlags(Widget::FLAG_INVISIBLE);
      _selected->setEditable(false);
      _selected->setEnabled(false);
      _okWidget->setLabel("Load");
      break;

    case Mode::FileSave:
      _fileList->setListMode(FilesystemNode::ListMode::All);
      _fileList->setNameFilter(namefilter);
      _fileList->setHeight(_selected->getTop() - VGAP * 2 - _fileList->getTop());

      _currentPath->setWidth(_savePathBox->getLeft() - _currentPath->getLeft() - fontWidth);
      _savePathBox->setEnabled(true);
      _savePathBox->clearFlags(Widget::FLAG_INVISIBLE);
      _savePathBox->setState(instance().settings().getBool("saveuserdir"));

      _name->clearFlags(Widget::FLAG_INVISIBLE);
      _selected->clearFlags(Widget::FLAG_INVISIBLE);
      _selected->setEditable(true);
      _selected->setEnabled(true);
      _selected->setText(fileName);
      _okWidget->setLabel("Save");
      fileSelected = false;
      break;

    case Mode::Directories:
      _fileList->setListMode(FilesystemNode::ListMode::DirectoriesOnly);
      _fileList->setNameFilter([](const FilesystemNode&) { return true; });
      // TODO: scrollbar affected too!
      _fileList->setHeight(_selected->getBottom() - _fileList->getTop());

      _currentPath->setWidth(_savePathBox->getRight() - _currentPath->getLeft());
      _savePathBox->setEnabled(false);
      _savePathBox->setFlags(Widget::FLAG_INVISIBLE);

      _name->setFlags(Widget::FLAG_INVISIBLE);
      _selected->setFlags(Widget::FLAG_INVISIBLE);
      _selected->setEditable(false);
      _selected->setEnabled(false);
      _okWidget->setLabel("OK");
      break;
  }

  // Set start path
  if(_mode != Mode::Directories)
    _fileList->setDirectory(FilesystemNode(directory), fileName);
  else
    _fileList->setDirectory(FilesystemNode(startpath));

  updateUI(fileSelected);

  // Finally, open the dialog after it has been fully updated
  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& BrowserDialog::getResult() const
{
  if(_mode == Mode::FileLoad || _mode == Mode::FileSave)
  {
    static FilesystemNode node;

    return node
      = FilesystemNode(_fileList->currentDir().getPath() + _selected->getText());
  }
  else
    return _fileList->currentDir();
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
      if(_mode != Mode::Directories)
      {
        bool savePath = _savePathBox->getState();

        instance().settings().setValue("saveuserdir", savePath);
        if(savePath)
          instance().setUserDir(_fileList->currentDir().getShortPath());
      }
      _command(true, getResult());
      close();
      break;

    case kCloseCmd:
      // Send a signal to the calling class that the dialog was closed without selection
      _command(false, getResult());
      close();
      break;

    case kGoUpCmd:
      _fileList->selectParent();
      break;

    case kBaseDirCmd:
      _fileList->setDirectory(FilesystemNode(instance().baseDir()));
      break;

    case kHomeDirCmd:
      _fileList->setDirectory(FilesystemNode(instance().homeDir()));
      break;

    case EditableWidget::kChangedCmd:
      Dialog::handleCommand(sender, cmd, data, 0);
      updateUI(false);
      break;

    case FileListWidget::ItemChanged:
      updateUI(true);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::updateUI(bool fileSelected)
{
  // Only hilite the 'up' button if there's a parent directory
  _goUpButton->setEnabled(_fileList->currentDir().hasParent());

  // Update the path display
  _currentPath->setText(_fileList->currentDir().getShortPath());

  // Enable/disable OK button based on current mode and status
  bool enable = true;

  if(_mode != Mode::Directories)
    enable = !_selected->getText().empty();
  _okWidget->setEnabled(enable);

  if(fileSelected && !_fileList->selected().isDirectory())
    _selected->setText(_fileList->getSelectedString());
}
