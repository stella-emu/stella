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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
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
#include "OverlayMenu.hxx"
#include "EditTextWidget.hxx"
#include "FileListWidget.hxx"
#include "NavigationWidget.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "Font.hxx"
#include "BrowserDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(GuiObject* boss, const GUI::Font& font,
                             int max_w, int max_h)
  : Dialog(boss->instance(), boss->parent(), font, "Title") // dummy title value!
{
  initialize(max_w, max_h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrowserDialog::BrowserDialog(OSystem& osystem, DialogContainer& parent,
                             const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Title") // dummy title value!
{
  initialize(max_w, max_h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::initialize(int max_w, int max_h)
{
  // This dialog takes as much space as is made available to it
  _w = max_w;
  _h = max_h;

  const int lineHeight   = Dialog::lineHeight(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("Base Dir"),
            HBORDER      = Dialog::hBorder();

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all positions and sizes from the current font and dialog size.  The
  // composite widgets (navigation bar and file list) are created at a real base
  // size to avoid degenerate tiny-size initialization.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Current path (navigation bar) and the "save path" checkbox beside it
  _navigationBar = new NavigationWidget(this, _font, 0, 0, _w - HBORDER * 2,
                                        buttonHeight);
  _savePathBox = new CheckboxWidget(this, _font, 0, 0, "Save");
  _savePathBox->setToolTip("Check to save current path as default.");

  // File listing
  _fileList = new FileListWidget(this, _font, 0, 0, _w - HBORDER * 2,
                                 buttonHeight * 4);
  _fileList->setEditable(false);
  addFocusWidget(_fileList);
  _navigationBar->setList(_fileList);

  // Currently selected item
  _name = new StaticTextWidget(this, _font, 0, 0, "Name ");
  _selected = new EditTextWidget(this, _font, 0, 0, _w, lineHeight, "");
  addFocusWidget(_selected);

  // Directory-navigation buttons
  _goUpButton = new ButtonWidget(this, _font, 0, 0, buttonWidth, buttonHeight,
                                 "Go up", kGoUpCmd);
  addFocusWidget(_goUpButton);
  _baseDirButton = new ButtonWidget(this, _font, 0, 0, buttonWidth, buttonHeight,
                                    "Base Dir", kBaseDirCmd);
  _baseDirButton->setToolTip("Go to Stella's base directory.");
  addFocusWidget(_baseDirButton);
  _homeDirButton = new ButtonWidget(this, _font, 0, 0, buttonWidth, buttonHeight,
                                    "Home Dir", kHomeDirCmd);
  _homeDirButton->setToolTip("Go to user's home directory.");
  addFocusWidget(_homeDirButton);

  // OK and Cancel; the platform-specific left/right ordering is handled by
  // Dialog::layoutButtonGroup()
  auto* okButton = new ButtonWidget(this, _font, 0, 0, buttonWidth, buttonHeight,
                                    "OK", kChooseCmd);
  addFocusWidget(okButton);
  addOKWidget(okButton);
  auto* cancelButton = new ButtonWidget(this, _font, 0, 0, buttonWidth,
                                        buttonHeight, "Cancel",
                                        GuiObject::kCloseCmd);
  addFocusWidget(cancelButton);
  addCancelWidget(cancelButton);

  // add last to avoid focus problems
  addFocusWidget(_savePathBox);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::vCentered;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("Base Dir"),
            BUTTON_GAP   = Dialog::buttonGap(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const bool fileMode  = _mode != Mode::Directories;      // has selected-item row
  const bool hasNavBar = _mode != Mode::FileLoadNoDirs;   // has navigation bar

  // Vertical stack: navigation bar, file listing (fills the available space),
  // an optional selected-item row, then a reserved band for the bottom buttons.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);

  // Navigation-bar row (absent in FileLoadNoDirs, whose directory is fixed, so
  // the file listing fills that space instead).  In the file-selection modes
  // the "save path" checkbox sits at the right; in Directories mode the bar
  // spans the full width.
  if(hasNavBar)
  {
    if(fileMode)
    {
      auto navRow = std::make_unique<BoxLayout>(Dir::Horizontal);
      navRow->addStretch(widgetItem(_navigationBar));
      navRow->addSpace(fontWidth);
      navRow->addFixed(vCentered(_savePathBox, _savePathBox->getHeight()),
                       _savePathBox->getWidth());
      root->addFixed(std::move(navRow), buttonHeight);
    }
    else
      root->addFixed(widgetItem(_navigationBar), buttonHeight);

    root->addSpace(VGAP);
  }

  root->addStretch(widgetItem(_fileList));

  // Currently-selected item (label + editable name), only in file modes
  if(fileMode)
  {
    auto nameRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    nameRow->addFixed(vCentered(_name, _name->getHeight()), _name->getWidth());
    nameRow->addStretch(vCentered(_selected, _selected->getHeight()));

    root->addSpace(VGAP * 2);
    root->addFixed(std::move(nameRow), _selected->getHeight());
  }

  // Gap down to the bottom button row, then the reserved button band (the
  // buttons themselves are positioned below)
  root->addSpace(VBORDER + VGAP - 2);
  root->addSpace(buttonHeight);

  root->doLayout(0, _th, _w, _h - _th);

  // Bottom-left directory-navigation buttons (Go up / Base Dir / Home Dir)
  auto navButtons = std::make_unique<BoxLayout>(Dir::Horizontal, BUTTON_GAP);
  navButtons->addFixed(widgetItem(_goUpButton), buttonWidth);
  navButtons->addFixed(widgetItem(_baseDirButton), buttonWidth);
  navButtons->addFixed(widgetItem(_homeDirButton), buttonWidth);
  navButtons->doLayout(HBORDER, _h - buttonHeight - VBORDER,
                       buttonWidth * 3 + BUTTON_GAP * 2, buttonHeight);

  // OK/Cancel along the bottom-right (platform order)
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
void BrowserDialog::show(Dialog* parent, const GUI::Font& font,
                         string_view title, string_view startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FSNode::NameFilter& namefilter)
{
  uInt32 w = 0, h = 0;

  if (parent) {
    parent->getDynamicBounds(w, h);
  } else {
    w = FBMinimum::Width;
    h = FBMinimum::Height;
  }

  if(std::cmp_greater(w, font.getMaxCharWidth() * 80))
    w = font.getMaxCharWidth() * 80;

  if(ourBrowser == nullptr || &ourBrowser->parent() != &parent->parent() ||
     std::cmp_greater(ourBrowser->_w, w) || std::cmp_greater(ourBrowser->_h, h))
  {
    ourBrowser = std::make_unique<BrowserDialog>(parent, font, w, h);
  }
  ourBrowser->setTitle(title); // has to be always updated!
  ourBrowser->show(startpath, mode, command, namefilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
void BrowserDialog::show(Dialog* parent,
                         string_view title, string_view startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FSNode::NameFilter& namefilter)
{
  show(parent, parent->instance().frameBuffer().font(), title, startpath,
       mode, command, namefilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
void BrowserDialog::show(OSystem& osystem,
                         string_view title, string_view startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FSNode::NameFilter& namefilter)
{
  const GUI::Font& font = osystem.frameBuffer().font();
  const Common::Rect& r = osystem.frameBuffer().imageRect();
  const uInt32 scale = osystem.frameBuffer().hidpiScaleFactor();
  const auto w = std::min(static_cast<uInt32>(0.95 * r.w() / scale),
                          static_cast<uInt32>(font.getMaxCharWidth() * 80));
  const auto h = static_cast<uInt32>(0.95 * r.h() / scale);

  auto& overlay = osystem.overlayMenu();
  if(ourBrowser == nullptr || &ourBrowser->parent() != &overlay ||
     std::cmp_greater(ourBrowser->_w, w) || std::cmp_greater(ourBrowser->_h, h))
  {
    ourBrowser = std::make_unique<BrowserDialog>(osystem, overlay, font, w, h);
  }
  ourBrowser->setTitle(title); // has to be always updated!
  ourBrowser->show(startpath, mode, command, namefilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
void BrowserDialog::hide()
{
  ourBrowser.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::show(string_view startpath,
                         BrowserDialog::Mode mode,
                         const Command& command,
                         const FSNode::NameFilter& namefilter)
{
  if(startpath.empty())
    startpath = "~";

  _mode = mode;
  _command = command;
  bool fileSelected = true;
  string directory;
  string fileName;

  // Set start path
  if(_mode != Mode::Directories)
  {
    // split startpath into path and filename
    const FSNode fs{startpath};
    fileName = fs.getName();
    directory = fs.isDirectory() ? string{} : fs.getParent().getPath();
  }

  // Default navigation settings:
  _navigationBar->setVisible(true);
  _fileList->setListMode(FSNode::ListMode::All);
  _fileList->setShowFileExtensions(true);
  _goUpButton->clearFlags(Widget::FLAG_INVISIBLE);
  _goUpButton->setEnabled(true);
  _baseDirButton->clearFlags(Widget::FLAG_INVISIBLE);
  _baseDirButton->setEnabled(true);
  _homeDirButton->clearFlags(Widget::FLAG_INVISIBLE);
  _homeDirButton->setEnabled(true);

  // Common setup for all file-selection modes
  if(_mode != Mode::Directories)
  {
    _fileList->setNameFilter(namefilter);
    _name->clearFlags(Widget::FLAG_INVISIBLE);
    _selected->clearFlags(Widget::FLAG_INVISIBLE);
    _selected->setEditable(false);
    _selected->setEnabled(false);
  }

  switch(_mode)
  {
    case Mode::FileLoad:
      _fileList->setListMode(FSNode::ListMode::All);
      // Show "save" checkbox
      _savePathBox->setEnabled(true);
      _savePathBox->clearFlags(Widget::FLAG_INVISIBLE);
      _savePathBox->setState(instance().settings().getBool("saveuserdir"));
      _okWidget->setLabel("Load");
      break;

    case Mode::FileLoadNoDirs:
      _fileList->setListMode(FSNode::ListMode::FilesOnly);
      _fileList->setShowFileExtensions(false);

      _navigationBar->setVisible(false);
      _navigationBar->setEnabled(false);
      // Hide "save" checkbox
      _savePathBox->setEnabled(false);
      _savePathBox->setFlags(Widget::FLAG_INVISIBLE);

      _goUpButton->setFlags(Widget::FLAG_INVISIBLE);
      _goUpButton->setEnabled(false);
      _baseDirButton->setFlags(Widget::FLAG_INVISIBLE);
      _baseDirButton->setEnabled(false);
      _homeDirButton->setFlags(Widget::FLAG_INVISIBLE);
      _homeDirButton->setEnabled(false);
      _okWidget->setLabel("Select");
      break;

    case Mode::FileSave:
      _fileList->setListMode(FSNode::ListMode::All);
      // Show "save" checkbox
      _savePathBox->setEnabled(true);
      _savePathBox->clearFlags(Widget::FLAG_INVISIBLE);
      _savePathBox->setState(instance().settings().getBool("saveuserdir"));

      _selected->setEditable(true);
      _selected->setEnabled(true);
      _selected->setText(fileName);
      _okWidget->setLabel("Save");
      fileSelected = false;
      break;

    case Mode::Directories:
      _fileList->setListMode(FSNode::ListMode::DirectoriesOnly);
      _fileList->setNameFilter([](const FSNode&) { return true; });
      // Hide "save" checkbox
      _savePathBox->setEnabled(false);
      _savePathBox->setFlags(Widget::FLAG_INVISIBLE);

      _name->setFlags(Widget::FLAG_INVISIBLE);
      _selected->setFlags(Widget::FLAG_INVISIBLE);
      _selected->setEditable(false);
      _selected->setEnabled(false);
      _okWidget->setLabel("OK");
      break;

    default:
      break;  // Not supposed to get here
  }

  // Set start path
  if(_mode != Mode::Directories && !directory.empty())
    _fileList->setInitialDirectory(FSNode(directory), fileName);
  else
    _fileList->setInitialDirectory(FSNode(startpath));

  updateUI(fileSelected);

  // Finally, open the dialog after it has been fully updated
  open();
  setFocus(_fileList);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode BrowserDialog::getResult() const
{
  if(_mode != Mode::Directories)
    return FSNode(_fileList->currentDir().getPath() + _selected->getText());
  else
    return _fileList->currentDir();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrowserDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  // Grab the key before passing it to the actual dialog and check for
  // file list navigation keys
  // Required because BrowserDialog does not want raw input
  if(repeated || !_fileList->handleKeyDown(key, mod))
    Dialog::handleKeyDown(key, mod, repeated);
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
        const bool savePath = _savePathBox->getState();

        instance().settings().setValue("saveuserdir", savePath);
        if(savePath)
          instance().setUserDir(_fileList->currentDir().getShortPath());
      }
      if(_command) _command(true, getResult());
      close();
      break;

    case kCloseCmd:
      // Send a signal to the calling class that the dialog was closed without selection
      if(_command) _command(false, getResult());
      close();
      break;

    case kGoUpCmd:
      _fileList->selectParent();
      break;

    case kBaseDirCmd:
      _fileList->selectDirectory(FSNode(instance().baseDir()));
      break;

    case kHomeDirCmd:
      _fileList->selectDirectory(FSNode(instance().homeDir()));
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
  _goUpButton->setEnabled(_goUpButton->isVisible()
                          && _fileList->currentDir().hasParent());

  // Update the path display
  _navigationBar->updateUI();

  // Enable/disable OK button based on current mode and status
  bool enable = true;

  if(_mode != Mode::Directories)
    enable = !_selected->getText().empty();
  _okWidget->setEnabled(enable);

  if(fileSelected && !_fileList->getList().empty() &&
     !_fileList->selected().isDirectory())
    _selected->setText(_fileList->getSelectedString());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<BrowserDialog> BrowserDialog::ourBrowser;
