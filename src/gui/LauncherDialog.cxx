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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Bankswitch.hxx"
#include "BrowserDialog.hxx"
#include "ContextMenu.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "FileListWidget.hxx"
#include "FSNode.hxx"
#include "MD5.hxx"
#include "OptionsDialog.hxx"
#include "HighScoresDialog.hxx"
#include "HighScoresManager.hxx"
#include "GlobalPropsDialog.hxx"
#include "StellaSettingsDialog.hxx"
#include "WhatsNewDialog.hxx"
#include "MessageBox.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "EventHandler.hxx"
#include "StellaKeys.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "RomInfoWidget.hxx"
#include "TIAConstants.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "StellaFont.hxx"
#include "ConsoleBFont.hxx"
#include "ConsoleMediumBFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "Stella12x24tFont.hxx"
#include "Stella14x28tFont.hxx"
#include "Stella16x32tFont.hxx"
#include "Version.hxx"
#include "LauncherDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem& osystem, DialogContainer& parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  myUseMinimalUI = instance().settings().getBool("minimal_ui");

  const GUI::Font& font = instance().frameBuffer().launcherFont();
  const int fontWidth = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight(),
            HBORDER = fontWidth * 1.25,
            VBORDER = fontHeight / 2,
            BUTTON_GAP = fontWidth,
            LBL_GAP = fontWidth,
            VGAP = fontHeight / 4,
            buttonHeight = myUseMinimalUI ? lineHeight - VGAP * 2: lineHeight * 1.25,
            buttonWidth  = (_w - 2 * HBORDER - BUTTON_GAP * (4 - 1));

  int xpos = HBORDER, ypos = VBORDER, lwidth = 0, lwidth2 = 0;
  WidgetArray wid;
  string lblRom = "Select a ROM from the list" + ELLIPSIS;
  const string& lblFilter = "Filter";
  const string& lblAllFiles = "Show all files";
  const string& lblFound = "XXXX items found";

  lwidth = font.getStringWidth(lblRom);
  lwidth2 = font.getStringWidth(lblAllFiles) + CheckboxWidget::boxSize(font);
  int lwidth3 = font.getStringWidth(lblFilter);
  int lwidth4 = font.getStringWidth(lblFound);

  if(w < HBORDER * 2 + lwidth + lwidth2 + lwidth3 + lwidth4 + fontWidth * 6 + LBL_GAP * 8)
  {
    // make sure there is space for at least 6 characters in the filter field
    lblRom = "Select a ROM" + ELLIPSIS;
    lwidth = font.getStringWidth(lblRom);
  }

  if(myUseMinimalUI)
  {
    // App information
    ostringstream ver;
    ver << "Stella " << STELLA_VERSION;
  #if defined(RETRON77)
    ver << " for RetroN 77";
  #endif
    new StaticTextWidget(this, font, 0, ypos, _w, fontHeight,
                         ver.str(), TextAlign::Center);
    ypos += lineHeight;
  }

  // Show the header
  new StaticTextWidget(this, font, xpos, ypos, lblRom);
  // Shop the files counter
  xpos = _w - HBORDER - lwidth4;
  myRomCount = new StaticTextWidget(this, font, xpos, ypos,
                                    lwidth4, fontHeight,
                                    "", TextAlign::Right);

  // Add filter that can narrow the results shown in the listing
  // It has to fit between both labels
  if(!myUseMinimalUI && w >= 640)
  {
    int fwidth = std::min(15 * fontWidth, xpos - lwidth3 - lwidth2 - lwidth - HBORDER - LBL_GAP * 8);
    // Show the filter input field
    xpos -= fwidth + LBL_GAP;
    myPattern = new EditTextWidget(this, font, xpos, ypos - 2, fwidth, lineHeight, "");
    // Show the "Filter" label
    xpos -= lwidth3 + LBL_GAP;
    new StaticTextWidget(this, font, xpos, ypos, lblFilter);
    // Show the checkbox for all files
    xpos -= lwidth2 + LBL_GAP * 3;
    myAllFiles = new CheckboxWidget(this, font, xpos, ypos, lblAllFiles, kAllfilesCmd);
    wid.push_back(myAllFiles);
    wid.push_back(myPattern);
  }

  // Add list with game titles
  // Before we add the list, we need to know the size of the RomInfoWidget
  int listHeight = _h - VBORDER * 2 - buttonHeight - lineHeight * 2 - VGAP * 6;
  float imgZoom = getRomInfoZoom(listHeight);
  int romWidth = imgZoom * TIAConstants::viewableWidth;
  if(romWidth > 0) romWidth += HBORDER;
  int listWidth = _w - (romWidth > 0 ? romWidth + fontWidth : 0) - HBORDER * 2;
  xpos = HBORDER;  ypos += lineHeight + VGAP;
  myList = new FileListWidget(this, font, xpos, ypos, listWidth, listHeight);
  myList->setEditable(false);
  myList->setListMode(FilesystemNode::ListMode::All);
  wid.push_back(myList);

  // Add ROM info area (if enabled)
  if(romWidth > 0)
  {
    xpos += myList->getWidth() + fontWidth;

    // Initial surface size is the same as the viewable area
    Common::Size imgSize(TIAConstants::viewableWidth*imgZoom,
                         TIAConstants::viewableHeight*imgZoom);

    // Calculate font area, and in the process the font that can be used
    Common::Size fontArea(romWidth - fontWidth * 2, myList->getHeight() - imgSize.h - VGAP * 3);

    setRomInfoFont(fontArea);
    myRomInfoWidget = new RomInfoWidget(this, *myROMInfoFont,
        xpos, ypos, romWidth, myList->getHeight(), imgSize);
  }

  // Add textfield to show current directory
  xpos = HBORDER;
  ypos += myList->getHeight() + VGAP * 2;
  lwidth = font.getStringWidth("Path") + LBL_GAP;
  myDirLabel = new StaticTextWidget(this, font, xpos, ypos+2, lwidth, fontHeight,
                                    "Path", TextAlign::Left);
  xpos += lwidth;
  myDir = new EditTextWidget(this, font, xpos, ypos, _w - xpos - HBORDER, lineHeight, "");
  myDir->setEditable(false, true);
  myDir->clearFlags(Widget::FLAG_RETAIN_FOCUS);

  if(!myUseMinimalUI)
  {
    // Add four buttons at the bottom
    xpos = HBORDER;  ypos = _h - VBORDER - buttonHeight;
  #ifndef BSPF_MACOS
    myStartButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 0) / 4, buttonHeight,
                                     "Select", kLoadROMCmd);
    wid.push_back(myStartButton);

    xpos += (buttonWidth + 0) / 4 + BUTTON_GAP;
    myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 1) / 4, buttonHeight,
                                       "Go Up", kPrevDirCmd);
    wid.push_back(myPrevDirButton);

    xpos += (buttonWidth + 1) / 4 + BUTTON_GAP;
    myOptionsButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 3) / 4, buttonHeight,
                                       "Options" + ELLIPSIS, kOptionsCmd);
    wid.push_back(myOptionsButton);

    xpos += (buttonWidth + 2) / 4 + BUTTON_GAP;
    myQuitButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 4) / 4, buttonHeight,
                                    "Quit", kQuitCmd);
    wid.push_back(myQuitButton);
  #else
    myQuitButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 0) / 4, buttonHeight,
                                    "Quit", kQuitCmd);
    wid.push_back(myQuitButton);

    xpos += (buttonWidth + 0) / 4 + BUTTON_GAP;
    myOptionsButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 1) / 4, buttonHeight,
                                       "Options" + ELLIPSIS, kOptionsCmd);
    wid.push_back(myOptionsButton);

    xpos += (buttonWidth + 1) / 4 + BUTTON_GAP;
    myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 2) / 4, buttonHeight,
                                       "Go Up", kPrevDirCmd);
    wid.push_back(myPrevDirButton);

    xpos += (buttonWidth + 2) / 4 + BUTTON_GAP;
    myStartButton = new ButtonWidget(this, font, xpos, ypos, (buttonWidth + 3) / 4, buttonHeight,
                                     "Select", kLoadROMCmd);
    wid.push_back(myStartButton);
  #endif
  }
  if(myUseMinimalUI) // Highlight 'Rom Listing'
    mySelectedItem = 0;
  else
    mySelectedItem = 2;

  addToFocusList(wid);

  // Create (empty) context menu for ROM list options
  myMenu = make_unique<ContextMenu>(this, osystem.frameBuffer().launcherFont(), EmptyVarList);

  // Create global props dialog, which is used to temporarily override
  // ROM properties
  myGlobalProps = make_unique<GlobalPropsDialog>(this,
    myUseMinimalUI ? osystem.frameBuffer().launcherFont() : osystem.frameBuffer().font());

  // Do we show only ROMs or all files?
  bool onlyROMs = instance().settings().getBool("launcherroms");
  showOnlyROMs(onlyROMs);
  if(myAllFiles)
    myAllFiles->setState(!onlyROMs);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& LauncherDialog::selectedRom() const
{
  return currentNode().getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& LauncherDialog::selectedRomMD5()
{
  if(currentNode().isDirectory() || !Bankswitch::isValidRomName(currentNode()))
    return EmptyString;

  // Attempt to conserve memory
  if(myMD5List.size() > 500)
    myMD5List.clear();

  // Lookup MD5, and if not present, cache it
  auto iter = myMD5List.find(currentNode().getPath());
  if(iter == myMD5List.end())
    myMD5List[currentNode().getPath()] = MD5::hash(currentNode());

  return myMD5List[currentNode().getPath()];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& LauncherDialog::currentNode() const
{
  return myList->selected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FilesystemNode& LauncherDialog::currentDir() const
{
  return myList->currentDir();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::reload()
{
  myMD5List.clear();
  myList->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadConfig()
{
  // Should we use a temporary directory specified on the commandline, or the
  // default one specified by the settings?
  const string& tmpromdir = instance().settings().getString("tmpromdir");
  const string& romdir = tmpromdir != "" ? tmpromdir :
      instance().settings().getString("romdir");
  const string& version = instance().settings().getString("stella.version");

  // Show "What's New" message when a new version of Stella is run for the first time
  if(version != STELLA_VERSION)
  {
    openWhatsNew();
    instance().settings().setValue("stella.version", STELLA_VERSION);
  }

  // Assume that if the list is empty, this is the first time that loadConfig()
  // has been called (and we should reload the list)
  if(myList->getList().empty())
  {
    FilesystemNode node(romdir == "" ? "~" : romdir);
    if(!(node.exists() && node.isDirectory()))
      node = FilesystemNode("~");

    myList->setDirectory(node, instance().settings().getString("lastrom"));
    updateUI();
  }
  Dialog::setFocus(getFocusList()[mySelectedItem]);

  if(myRomInfoWidget)
    myRomInfoWidget->reloadProperties(currentNode());

  myList->clearFlags(Widget::FLAG_WANTS_RAWDATA); // always reset this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::saveConfig()
{
  if(instance().settings().getBool("followlauncher"))
    instance().settings().setValue("romdir", myList->currentDir().getShortPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateUI()
{
  // Only hilite the 'up' button if there's a parent directory
  if(myPrevDirButton)
    myPrevDirButton->setEnabled(myList->currentDir().hasParent());

  // Show current directory
  myDir->setText(myList->currentDir().getShortPath());

  // Indicate how many files were found
  ostringstream buf;
  buf << (myList->getList().size() - 1) << " items found";
  myRomCount->setLabel(buf.str());

  // Update ROM info UI item
  loadRomInfo();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::applyFiltering()
{
  myList->setNameFilter(
    [&](const FilesystemNode& node) {
      if(!node.isDirectory())
      {
        // Do we want to show only ROMs or all files?
        if(myShowOnlyROMs && !Bankswitch::isValidRomName(node))
          return false;

        // Skip over files that don't match the pattern in the 'pattern' textbox
        if(myPattern && myPattern->getText() != "" &&
          !BSPF::containsIgnoreCase(node.getName(), myPattern->getText()))
          return false;
      }
      return true;
    }
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float LauncherDialog::getRomInfoZoom(int listHeight) const
{
  // The ROM info area is some multiple of the minimum TIA image size
  float zoom = instance().settings().getFloat("romviewer");

  if(zoom > 0.F)
  {
    const GUI::Font& font = instance().frameBuffer().launcherFont();
    const GUI::Font& smallFont = instance().frameBuffer().smallFont();
    const int fontWidth = font.getMaxCharWidth(),
              HBORDER = fontWidth * 1.25;

    // upper zoom limit - at least 24 launchers chars/line and 7 + 4 ROM info lines
    if((_w - (HBORDER * 2 + fontWidth + 30) - zoom * TIAConstants::viewableWidth)
       / font.getMaxCharWidth() < MIN_LAUNCHER_CHARS)
    {
      zoom = float(_w - (HBORDER * 2 + fontWidth + 30) - MIN_LAUNCHER_CHARS * font.getMaxCharWidth())
        / TIAConstants::viewableWidth;
    }
    if((listHeight - 12 - zoom * TIAConstants::viewableHeight) <
       MIN_ROMINFO_ROWS * smallFont.getLineHeight() +
       MIN_ROMINFO_LINES * smallFont.getFontHeight())
    {
      zoom = float(listHeight - 12 -
                   MIN_ROMINFO_ROWS * smallFont.getLineHeight() -
                   MIN_ROMINFO_LINES * smallFont.getFontHeight())
        / TIAConstants::viewableHeight;
    }

    // lower zoom limit - at least 30 ROM info chars/line
    if((zoom * TIAConstants::viewableWidth)
       / smallFont.getMaxCharWidth() < MIN_ROMINFO_CHARS + 6)
    {
      zoom = float(MIN_ROMINFO_CHARS * smallFont.getMaxCharWidth() + 6)
        / TIAConstants::viewableWidth;
    }
  }
  return zoom;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::setRomInfoFont(const Common::Size& area)
{
  // TODO: Perhaps offer a setting to override the font used?

  FontDesc FONTS[7] = {
    GUI::stella16x32tDesc, GUI::stella14x28tDesc, GUI::stella12x24tDesc,
    GUI::stellaLargeDesc, GUI::stellaMediumDesc,
    GUI::consoleMediumBDesc, GUI::consoleBDesc
  };

  // Try to pick a font that works best, based on the available area
  for(size_t i = 0; i < sizeof(FONTS) / sizeof(FontDesc); ++i)
  {
    // only use fonts <= launcher fonts
    if(instance().frameBuffer().launcherFont().getFontHeight() >= FONTS[i].height)
    {
      if(area.h >= uInt32(MIN_ROMINFO_ROWS * FONTS[i].height + 2
         + MIN_ROMINFO_LINES * FONTS[i].height)
         && area.w >= uInt32(MIN_ROMINFO_CHARS * FONTS[i].maxwidth))
      {
        myROMInfoFont = make_unique<GUI::Font>(FONTS[i]);
        return;
      }
    }
  }
  myROMInfoFont = make_unique<GUI::Font>(GUI::stellaDesc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRomInfo()
{
  if(!myRomInfoWidget)
    return;

  const string& md5 = selectedRomMD5();
  if(md5 != EmptyString)
    myRomInfoWidget->setProperties(currentNode(), md5);
  else
    myRomInfoWidget->clearProperties();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleContextMenu()
{
  const string& cmd = myMenu->getSelectedTag().toString();

  if(cmd == "override")
    myGlobalProps->open();
  else if(cmd == "reload")
    reload();
  else if(cmd == "highscores")
    openHighScores();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::showOnlyROMs(bool state)
{
  myShowOnlyROMs = state;
  instance().settings().setValue("launcherroms", state);
  applyFiltering();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  // Grab the key before passing it to the actual dialog and check for
  // Control-R (reload ROM listing)
  if(StellaModTest::isControl(mod) && key == KBDK_R)
    reload();
  else
#if defined(RETRON77)
    // handle keys used by R77
    switch(key)
    {
      case KBDK_F8: // front  ("Skill P2")
        if (!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
          myGlobalProps->open();
        break;
      case KBDK_F4: // back ("COLOR", "B/W")
        openSettings();
        break;

      case KBDK_F11: // front ("LOAD")
        // convert unused previous item key into page-up event
        _focusedWidget->handleEvent(Event::UIPgUp);
        break;

      case KBDK_F1: // front ("MODE")
        // convert unused next item key into page-down event
        _focusedWidget->handleEvent(Event::UIPgDown);
        break;

      default:
        Dialog::handleKeyDown(key, mod);
        break;
    }
#else
    Dialog::handleKeyDown(key, mod);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleJoyDown(int stick, int button, bool longPress)
{
  myEventHandled = false;
  myList->setFlags(Widget::FLAG_WANTS_RAWDATA); // allow handling long button press
  Dialog::handleJoyDown(stick, button, longPress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleJoyUp(int stick, int button)
{
  // open power-up options and settings for 2nd and 4th button if not mapped otherwise
  Event::Type e = instance().eventHandler().eventForJoyButton(EventMode::kMenuMode, stick, button);

  if (button == 1 && (e == Event::UIOK || e == Event::NoType) &&
      !currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
    myGlobalProps->open();
  if (button == 3 && (e == Event::Event::UITabPrev || e == Event::NoType))
    openSettings();
  else if (!myEventHandled)
    Dialog::handleJoyUp(stick, button);

  myList->clearFlags(Widget::FLAG_WANTS_RAWDATA); // stop allowing to handle long button press
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type LauncherDialog::getJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button)
{
  Event::Type e = instance().eventHandler().eventForJoyAxis(EventMode::kMenuMode, stick, axis, adir, button);

  if(myUseMinimalUI)
  {
    // map axis events for launcher
    switch(e)
    {
      case Event::UINavPrev:
        // convert unused previous item event into page-up event
        e = Event::UIPgUp;
        break;

      case Event::UINavNext:
        // convert unused next item event into page-down event
        e = Event::UIPgDown;
        break;

      default:
        break;
    }
  }
  return e;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  // Grab right mouse button for context menu, send left to base class
  if(b == MouseButton::RIGHT)
  {
    // Dynamically create context menu for ROM list options
    VariantList items;

    if(!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
      VarList::push_back(items, "Power-on options" + ELLIPSIS, "override");
    if(instance().highScores().enabled())
      VarList::push_back(items, "High scores" + ELLIPSIS, "highscores");
    VarList::push_back(items, "Reload listing", "reload");
    myMenu->addItems(items);

    // Add menu at current x,y mouse location
    myMenu->show(x + getAbsX(), y + getAbsY(), surface().dstRect());

  }
  else
    Dialog::handleMouseDown(x, y, b, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kAllfilesCmd:
      showOnlyROMs(myAllFiles ? !myAllFiles->getState() : true);
      reload();
      break;

    case kLoadROMCmd:
    case FileListWidget::ItemActivated:
      saveConfig();
      loadRom();
      break;

    case kOptionsCmd:
      openSettings();
      break;

    case kPrevDirCmd:
      myList->selectParent();
      break;

    case FileListWidget::ItemChanged:
      updateUI();
      break;

    case ListWidget::kLongButtonPressCmd:
      if (!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
        myGlobalProps->open();
      myEventHandled = true;
      break;

    case EditableWidget::kChangedCmd:
      applyFiltering();  // pattern matching taken care of directly in this method
      reload();
      break;

    case kQuitCmd:
      saveConfig();
      close();
      instance().eventHandler().quit();
      break;

    case kRomDirChosenCmd:
    {
      string romDir = instance().settings().getString("romdir");

      if(myList->currentDir().getPath() != romDir)
      {
        FilesystemNode node(romDir);

        if(!(node.exists() && node.isDirectory()))
          node = FilesystemNode("~");

        myList->setDirectory(node);
      }
      break;
    }

    case ContextMenu::kItemSelectedCmd:
      handleContextMenu();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRom()
{
  const string& result = instance().createConsole(currentNode(), selectedRomMD5());
  if(result == EmptyString)
  {
    instance().settings().setValue("lastrom", myList->getSelectedString());

    // If romdir has never been set, set it now based on the selected rom
    if(instance().settings().getString("romdir") == EmptyString)
      instance().settings().setValue("romdir", currentNode().getParent().getShortPath());
  }
  else
    instance().frameBuffer().showMessage(result, MessagePosition::MiddleCenter, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::setDefaultDir()
{
  instance().settings().setValue("romdir", myList->currentDir().getShortPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openSettings()
{
  saveConfig();

  // Create an options dialog, similar to the in-game one
  if (instance().settings().getBool("basic_settings"))
  {
    if (myStellaSettingsDialog == nullptr)
      myStellaSettingsDialog = make_unique<StellaSettingsDialog>(instance(), parent(),
        _w, _h, Menu::AppMode::launcher);
    myStellaSettingsDialog->open();
  }
  else
  {
    if (myOptionsDialog == nullptr)
      myOptionsDialog = make_unique<OptionsDialog>(instance(), parent(), this, _w, _h,
        Menu::AppMode::launcher);
    myOptionsDialog->open();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openHighScores()
{
  // Create an options dialog, similar to the in-game one
  if(myHighScoresDialog == nullptr)
    myHighScoresDialog = make_unique<HighScoresDialog>(instance(), parent(), _w, _h,
                                                       Menu::AppMode::launcher);

  myHighScoresDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openWhatsNew()
{
  if(myWhatsNewDialog == nullptr)
    myWhatsNewDialog = make_unique<WhatsNewDialog>(instance(), parent(), _font, _w, _h);
  myWhatsNewDialog->open();

}
