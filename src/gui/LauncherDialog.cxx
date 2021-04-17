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
#include "ProgressDialog.hxx"
#include "MessageBox.hxx"
#include "ToolTip.hxx"
#include "TimerManager.hxx"
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
  : Dialog(osystem, parent, osystem.frameBuffer().launcherFont(), "",
           x, y, w, h)
{
  myUseMinimalUI = instance().settings().getBool("minimal_ui");
  const int lineHeight   = Dialog::lineHeight(),
            fontHeight   = Dialog::fontHeight(),
            fontWidth    = Dialog::fontWidth(),
            BUTTON_GAP   = Dialog::buttonGap(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int LBL_GAP      = fontWidth,
            buttonHeight = myUseMinimalUI ? lineHeight - VGAP * 2: Dialog::buttonHeight(),
            buttonWidth  = (_w - 2 * HBORDER - BUTTON_GAP * (4 - 1));

  int xpos = HBORDER, ypos = VBORDER;
  WidgetArray wid;
  string lblSelect = "Select a ROM from the list" + ELLIPSIS;
  string lblAllFiles = "Show all files";
  const string& lblFilter = "Filter";
  string lblSubDirs = "Incl. subdirectories";
  string lblFound = "12345 items found";

  tooltip().setFont(_font);

  int lwSelect = _font.getStringWidth(lblSelect);
  int cwAllFiles = _font.getStringWidth(lblAllFiles) + CheckboxWidget::prefixSize(_font);
  int cwSubDirs = _font.getStringWidth(lblSubDirs) + CheckboxWidget::prefixSize(_font);
  int lwFilter = _font.getStringWidth(lblFilter);
  int lwFound = _font.getStringWidth(lblFound);
  int wTotal = HBORDER * 2 + lwSelect + cwAllFiles + cwSubDirs + lwFilter + lwFound
    + EditTextWidget::calcWidth(_font, "123456") + LBL_GAP * 7;
  bool noSelect = false;

  if(w < wTotal)
  {
    // make sure there is space for at least 6 characters in the filter field
    lblSelect = "Select a ROM" + ELLIPSIS;
    int lwSelectShort = _font.getStringWidth(lblSelect);

    wTotal -= lwSelect - lwSelectShort;
    lwSelect = lwSelectShort;
  }
  if(w < wTotal)
  {
    // make sure there is space for at least 6 characters in the filter field
    lblSubDirs = "Subdir.";
    int cwSubDirsShort = _font.getStringWidth(lblSubDirs) + CheckboxWidget::prefixSize(_font);

    wTotal -= cwSubDirs - cwSubDirsShort;
    cwSubDirs = cwSubDirsShort;
  }
  if(w < wTotal)
  {
    // make sure there is space for at least 6 characters in the filter field
    lblAllFiles = "All files";
    int cwAllFilesShort = _font.getStringWidth(lblAllFiles) + CheckboxWidget::prefixSize(_font);

    wTotal -= cwAllFiles - cwAllFilesShort;
    cwAllFiles = cwAllFilesShort;
  }
  if(w < wTotal)
  {
    // make sure there is space for at least 6 characters in the filter field
    lblFound = "12345 found";
    int lwFoundShort = _font.getStringWidth(lblFound);

    wTotal -= lwFound - lwFoundShort;
    lwFound = lwFoundShort;
    myShortCount = true;
  }
  if(w < wTotal)
  {
    // make sure there is space for at least 6 characters in the filter field
    lblSelect = "";
    int lwSelectShort = _font.getStringWidth(lblSelect);

    // wTotal -= lwSelect - lwSelectShort; // dead code
    lwSelect = lwSelectShort;
    noSelect = true;
  }

  if(myUseMinimalUI)
  {
    // App information
    ostringstream ver;
    ver << "Stella " << STELLA_VERSION;
  #if defined(RETRON77)
    ver << " for RetroN 77";
  #endif
    new StaticTextWidget(this, _font, 0, ypos, _w, fontHeight,
                         ver.str(), TextAlign::Center);
    ypos += lineHeight;
  }

  // Show the header
  new StaticTextWidget(this, _font, xpos, ypos, lblSelect);
  // Shop the files counter
  xpos = _w - HBORDER - lwFound;
  myRomCount = new StaticTextWidget(this, _font, xpos, ypos,
                                    lwFound, fontHeight,
                                    "", TextAlign::Right);

  // Add filter that can narrow the results shown in the listing
  // It has to fit between both labels
  if(!myUseMinimalUI && w >= 640)
  {
    int fwFilter = std::min(EditTextWidget::calcWidth(_font, "123456789012345"),
                            xpos - cwSubDirs - lwFilter - cwAllFiles
                            - lwSelect - HBORDER - LBL_GAP * (noSelect ? 5 : 7));

    // Show the filter input field
    xpos -= fwFilter + LBL_GAP;
    myPattern = new EditTextWidget(this, _font, xpos, ypos - 2, fwFilter, lineHeight, "");
    myPattern->setToolTip("Enter filter text to reduce file list.\n"
                          "Use '*' and '?' as wildcards.");

    // Show the "Filter" label
    xpos -= lwFilter + LBL_GAP;
    new StaticTextWidget(this, _font, xpos, ypos, lblFilter);

    // Show the subdirectories checkbox
    xpos -= cwSubDirs + LBL_GAP * 2;
    mySubDirs = new CheckboxWidget(this, _font, xpos, ypos, lblSubDirs, kSubDirsCmd);
    ostringstream tip;
    tip << "Search files in subdirectories too.";
    mySubDirs->setToolTip(tip.str());

    // Show the checkbox for all files
    if(noSelect)
      xpos = HBORDER;
    else
      xpos -= cwAllFiles + LBL_GAP;
    myAllFiles = new CheckboxWidget(this, _font, xpos, ypos, lblAllFiles, kAllfilesCmd);
    myAllFiles->setToolTip("Uncheck to show ROM files only.");

    wid.push_back(myAllFiles);
    wid.push_back(myPattern);
    wid.push_back(mySubDirs);
  }

  // Add list with game titles
  // Before we add the list, we need to know the size of the RomInfoWidget
  int listHeight = _h - VBORDER * 2 - buttonHeight - lineHeight * 2 - VGAP * 6;
  float imgZoom = getRomInfoZoom(listHeight);
  int romWidth = imgZoom * TIAConstants::viewableWidth;
  if(romWidth > 0) romWidth += HBORDER;
  int listWidth = _w - (romWidth > 0 ? romWidth + fontWidth : 0) - HBORDER * 2;
  xpos = HBORDER;  ypos += lineHeight + VGAP;
  myList = new FileListWidget(this, _font, xpos, ypos, listWidth, listHeight);
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
  ypos += myList->getHeight() + VGAP;
  lwSelect = _font.getStringWidth("Path") + LBL_GAP;
  myDirLabel = new StaticTextWidget(this, _font, xpos, ypos+2, lwSelect, fontHeight,
                                    "Path", TextAlign::Left);
  xpos += lwSelect;
  myDir = new EditTextWidget(this, _font, xpos, ypos, _w - xpos - HBORDER, lineHeight, "");
  myDir->setEditable(false, true);
  myDir->clearFlags(Widget::FLAG_RETAIN_FOCUS);

  if(!myUseMinimalUI)
  {
    // Add four buttons at the bottom
    xpos = HBORDER;  ypos = _h - VBORDER - buttonHeight;
  #ifndef BSPF_MACOS
    myStartButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 0) / 4, buttonHeight,
                                     "Select", kLoadROMCmd);
    wid.push_back(myStartButton);

    xpos += (buttonWidth + 0) / 4 + BUTTON_GAP;
    myPrevDirButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 1) / 4, buttonHeight,
                                       "Go Up", kPrevDirCmd);
    wid.push_back(myPrevDirButton);

    xpos += (buttonWidth + 1) / 4 + BUTTON_GAP;
    myOptionsButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 3) / 4, buttonHeight,
                                       "Options" + ELLIPSIS, kOptionsCmd);
    wid.push_back(myOptionsButton);

    xpos += (buttonWidth + 2) / 4 + BUTTON_GAP;
    myQuitButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 4) / 4, buttonHeight,
                                    "Quit", kQuitCmd);
    wid.push_back(myQuitButton);
  #else
    myQuitButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 0) / 4, buttonHeight,
                                    "Quit", kQuitCmd);
    wid.push_back(myQuitButton);

    xpos += (buttonWidth + 0) / 4 + BUTTON_GAP;
    myOptionsButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 1) / 4, buttonHeight,
                                       "Options" + ELLIPSIS, kOptionsCmd);
    wid.push_back(myOptionsButton);

    xpos += (buttonWidth + 1) / 4 + BUTTON_GAP;
    myPrevDirButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 2) / 4, buttonHeight,
                                       "Go Up", kPrevDirCmd);
    wid.push_back(myPrevDirButton);

    xpos += (buttonWidth + 2) / 4 + BUTTON_GAP;
    myStartButton = new ButtonWidget(this, _font, xpos, ypos, (buttonWidth + 3) / 4, buttonHeight,
                                     "Select", kLoadROMCmd);
    wid.push_back(myStartButton);
  #endif
    myStartButton->setToolTip("Start emulation of selected ROM.");
  }
  if(myUseMinimalUI) // Highlight 'Rom Listing'
    mySelectedItem = 0;
  else
    mySelectedItem = 3;

  addToFocusList(wid);

  // since we cannot know how many files there are, use are really high value here
  myList->progress().setRange(0, 50000, 5);
  myList->progress().setMessage("        Filtering files" + ELLIPSIS + "        ");

  // Do we show only ROMs or all files?
  bool onlyROMs = instance().settings().getBool("launcherroms");
  showOnlyROMs(onlyROMs);
  if(myAllFiles)
    myAllFiles->setState(!onlyROMs);

  setHelpAnchor("ROMInfo");
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
  myPendingReload = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::resetSurfaces()
{
  if(myRomInfoWidget)
    myRomInfoWidget->resetSurfaces();

  Dialog::resetSurfaces();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::tick()
{
  if(myPendingReload && myReloadTime < TimerManager::getTicks() / 1000)
    reload();

  Dialog::tick();
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

  bool subDirs = instance().settings().getBool("launchersubdirs");
  if (mySubDirs) mySubDirs->setState(subDirs);
  myList->setIncludeSubDirs(subDirs);

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
  if (mySubDirs)
    instance().settings().setValue("launchersubdirs", mySubDirs->getState());
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
  buf << (myList->getList().size() - 1) << (myShortCount ? " found" : " items found");
  myRomCount->setLabel(buf.str());

  // Update ROM info UI item
  loadRomInfo();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t LauncherDialog::matchWithJoker(const string& str, const string& pattern)
{
  if(str.length() >= pattern.length())
  {
    // optimize a bit
    if(pattern.find('?') != string::npos)
    {
      for(size_t pos = 0; pos < str.length() - pattern.length() + 1; ++pos)
      {
        bool found = true;

        for(size_t i = 0; found && i < pattern.length(); ++i)
          if(pattern[i] != str[pos + i] && pattern[i] != '?')
            found = false;

        if(found)
          return pos;
      }
    }
    else
      return str.find(pattern);
  }
  return string::npos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherDialog::matchWithWildcards(const string& str, const string& pattern)
{
  string pat = pattern;

  // remove leading and trailing '*'
  size_t i = 0;
  while(pat[i++] == '*');
  pat = pat.substr(i - 1);

  i = pat.length();
  while(pat[--i] == '*');
  pat.erase(i + 1);

  // Search for first '*'
  size_t pos = pat.find('*');

  if(pos != string::npos)
  {
    // '*' found, split pattern into left and right part, search recursively
    const string leftPat = pat.substr(0, pos);
    const string rightPat = pat.substr(pos + 1);
    size_t posLeft = matchWithJoker(str, leftPat);

    if(posLeft != string::npos)
      return matchWithWildcards(str.substr(pos + posLeft), rightPat);
    else
      return false;
  }
  // no further '*' found
  return matchWithJoker(str, pat) != string::npos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherDialog::matchWithWildcardsIgnoreCase(const string& str, const string& pattern)
{
  string in = str;
  string pat = pattern;

  BSPF::toUpperCase(in);
  BSPF::toUpperCase(pat);

  return matchWithWildcards(in, pat);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::applyFiltering()
{
  myList->setNameFilter(
    [&](const FilesystemNode& node) {
      myList->incProgress();
      if(!node.isDirectory())
      {
        // Do we want to show only ROMs or all files?
        if(myShowOnlyROMs && !Bankswitch::isValidRomName(node))
          return false;

        // Skip over files that don't match the pattern in the 'pattern' textbox
        if(myPattern && myPattern->getText() != "" &&
           !matchWithWildcardsIgnoreCase(node.getName(), myPattern->getText()))
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
    const GUI::Font& smallFont = instance().frameBuffer().smallFont();
    const int fontWidth = Dialog::fontWidth(),
              HBORDER   = Dialog::hBorder();

    // upper zoom limit - at least 24 launchers chars/line and 7 + 4 ROM info lines
    if((_w - (HBORDER * 2 + fontWidth + 30) - zoom * TIAConstants::viewableWidth)
       / fontWidth < MIN_LAUNCHER_CHARS)
    {
      zoom = float(_w - (HBORDER * 2 + fontWidth + 30) - MIN_LAUNCHER_CHARS * fontWidth)
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
    if(Dialog::fontHeight() >= FONTS[i].height)
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
  const string& cmd = menu().getSelectedTag().toString();

  if(cmd == "override")
    openGlobalProps();
  else if(cmd == "reload")
    reload();
  else if(cmd == "highscores")
    openHighScores();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu& LauncherDialog::menu()
{
  if(myMenu == nullptr)
    // Create (empty) context menu for ROM list options
    myMenu = make_unique<ContextMenu>(this, _font, EmptyVarList);


  return *myMenu;
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
  // context menu keys
  if(StellaModTest::isControl(mod))
  {
    switch(key)
    {
      case KBDK_P:
        openGlobalProps();
        break;

      case KBDK_H:
        if(instance().highScores().enabled())
          openHighScores();
        break;

      case KBDK_R:
        reload();
        break;

      default:
        break;
    }
  }
  else
#if defined(RETRON77)
    // handle keys used by R77
    switch(key)
    {
      case KBDK_F8: // front  ("Skill P2")
        if (!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
          openGlobalProps();
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
    openGlobalProps();
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
  if(b == MouseButton::RIGHT
     && x + getAbsX() >= myList->getLeft() && x + getAbsX() <= myList->getRight()
     && y + getAbsY() >= myList->getTop() && y + getAbsY() <= myList->getBottom())
  {
    // Dynamically create context menu for ROM list options
    VariantList items;

    if(!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
      VarList::push_back(items, " Power-on options" + ELLIPSIS + "   Ctrl+P", "override");
    if(instance().highScores().enabled())
      VarList::push_back(items, " High scores" + ELLIPSIS + "        Ctrl+H", "highscores");
    VarList::push_back(items, " Reload listing      Ctrl+R ", "reload");
    menu().addItems(items);

    // Add menu at current x,y mouse location
    menu().show(x + getAbsX(), y + getAbsY(), surface().dstRect());
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

    case kSubDirsCmd:
      myList->setIncludeSubDirs(mySubDirs->getState());
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
        openGlobalProps();
      myEventHandled = true;
      break;

    case EditableWidget::kChangedCmd:
    case EditableWidget::kAcceptCmd:
    {
      bool subDirs = mySubDirs->getState();

      myList->setIncludeSubDirs(subDirs);
      applyFiltering();  // pattern matching taken care of directly in this method

      if(subDirs && cmd == EditableWidget::kChangedCmd)
      {
        // delay (potentially slow) subdirectories reloads until user stops typing
        myReloadTime = TimerManager::getTicks() / 1000 + myList->getQuickSelectDelay();
        myPendingReload = true;
      }
      else
        reload();
      break;
    }

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
    instance().frameBuffer().showTextMessage(result, MessagePosition::MiddleCenter, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::setDefaultDir()
{
  instance().settings().setValue("romdir", myList->currentDir().getShortPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openGlobalProps()
{
  // Create global props dialog, which is used to temporarily override
  // ROM properties
  myDialog = make_unique<GlobalPropsDialog>(this, myUseMinimalUI
                                            ? _font
                                            : instance().frameBuffer().font());
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openSettings()
{
  saveConfig();

  // Create an options dialog, similar to the in-game one
  if (instance().settings().getBool("basic_settings"))
    myDialog = make_unique<StellaSettingsDialog>(instance(), parent(),
                                                 _w, _h, Menu::AppMode::launcher);
  else
    myDialog = make_unique<OptionsDialog>(instance(), parent(), this, _w, _h,
                                          Menu::AppMode::launcher);
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openHighScores()
{
  // Create an high scores dialog, similar to the in-game one
  myDialog = make_unique<HighScoresDialog>(instance(), parent(), _w, _h,
                                           Menu::AppMode::launcher);
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openWhatsNew()
{
  myDialog = make_unique<WhatsNewDialog>(instance(), parent(), _w, _h);
  myDialog->open();
}
