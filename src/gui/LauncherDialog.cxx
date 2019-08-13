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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
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
#include "GlobalPropsDialog.hxx"
#include "StellaSettingsDialog.hxx"
#include "MessageBox.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "EventHandler.hxx"
#include "StellaKeys.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "RomInfoWidget.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "Version.hxx"
#include "LauncherDialog.hxx"

/**
  TODO:
    - show all files / only ROMs
    - connect to 'matchPattern'
    - create lambda filter to pass these to FileListWidget
*/
#if 0
  // TODO - rough contents of lambda filter
  FSList files;
  files.reserve(2048);
  myCurrentNode.getChildren(files, FilesystemNode::ListMode::All);

  // Add '[..]' to indicate previous folder
  if(myCurrentNode.hasParent())
    myGameList->appendGame(" [..]", "", "", true);

  // Now add the directory entries
  bool domatch = myPattern && myPattern->getText() != "";
  for(const auto& f: files)
  {
    bool isDir = f.isDirectory();
    const string& name = isDir ? (" [" + f.getName() + "]") : f.getName();

    // Do we want to show only ROMs or all files?
    if(!isDir && myShowOnlyROMs && !Bankswitch::isValidRomName(f))
      continue;

    // Skip over files that don't match the pattern in the 'pattern' textbox
    if(domatch && !isDir && !matchPattern(name, myPattern->getText()))
      continue;

    myGameList->appendGame(name, f.getPath(), "", isDir);
  }
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  LauncherDialog::LauncherDialog(OSystem& osystem, DialogContainer& parent,
                                 int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h),
    myStartButton(nullptr),
    myPrevDirButton(nullptr),
    myOptionsButton(nullptr),
    myQuitButton(nullptr),
    myList(nullptr),
    myPattern(nullptr),
    myAllFiles(nullptr),
    myRomInfoWidget(nullptr),
    mySelectedItem(0),
    myEventHandled(false)
{
  myUseMinimalUI = instance().settings().getBool("minimal_ui");

  const GUI::Font& font = instance().frameBuffer().launcherFont();

  const int HBORDER = 10;
  const int BUTTON_GAP = 8;
  const int fontWidth = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight(),
            bwidth  = (_w - 2 * HBORDER - BUTTON_GAP * (4 - 1)),
            bheight = myUseMinimalUI ? lineHeight - 4 : lineHeight + 4,
            LBL_GAP = fontWidth;
  int xpos = 0, ypos = 0, lwidth = 0, lwidth2 = 0;
  WidgetArray wid;

  string lblRom = "Select a ROM from the list" + ELLIPSIS;
  const string& lblFilter = "Filter";
  const string& lblAllFiles = "Show all files";
  const string& lblFound = "XXXX items found";

  lwidth = font.getStringWidth(lblRom);
  lwidth2 = font.getStringWidth(lblAllFiles) + 20;
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
    ypos += 8;
    new StaticTextWidget(this, font, xpos, ypos, _w - 20, fontHeight,
                         ver.str(), TextAlign::Center);
    ypos += fontHeight - 4;
  }

  // Show the header
  xpos += HBORDER;  ypos += 8;
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
  xpos = HBORDER;  ypos += lineHeight + 4;
  int romWidth = 0;
  int romSize = instance().settings().getInt("romviewer");
  if(romSize > 1 && w >= 1000 && h >= 720)
    romWidth = 660;
  else if(romSize > 0 && w >= 640 && h >= 480)
    romWidth = 365;

  int listWidth = _w - (romWidth > 0 ? romWidth+8 : 0) - 20;
  myList = new FileListWidget(this, font, xpos, ypos,
                              listWidth, _h - 43 - bheight - fontHeight - lineHeight);
  myList->setEditable(false);
  myList->setListMode(FilesystemNode::ListMode::All);
  wid.push_back(myList);

  // Add ROM info area (if enabled)
  if(romWidth > 0)
  {
    xpos += myList->getWidth() + 8;
    myRomInfoWidget = new RomInfoWidget(this,
        romWidth < 660 ? instance().frameBuffer().smallFont() :
                         instance().frameBuffer().infoFont(),
        xpos, ypos, romWidth, myList->getHeight());
  }

  // Add textfield to show current directory
  xpos = HBORDER;
  ypos += myList->getHeight() + 8;
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
    xpos = HBORDER;  ypos += myDir->getHeight() + 8;
  #ifndef BSPF_MACOS
    myStartButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 0) / 4, bheight,
                                     "Select", kLoadROMCmd);
    wid.push_back(myStartButton);
      xpos += (bwidth + 0) / 4 + BUTTON_GAP;
    myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 1) / 4, bheight,
                                       "Go Up", kPrevDirCmd);
    wid.push_back(myPrevDirButton);
      xpos += (bwidth + 1) / 4 + BUTTON_GAP;
      myOptionsButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 2) / 4, bheight,
                                         "Options" + ELLIPSIS, kOptionsCmd);
    wid.push_back(myOptionsButton);
      xpos += (bwidth + 2) / 4 + BUTTON_GAP;
    myQuitButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 3) / 4, bheight,
                                  "Quit", kQuitCmd);
    wid.push_back(myQuitButton);
  #else
    myQuitButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 0) / 4, bheight,
                                    "Quit", kQuitCmd);
    wid.push_back(myQuitButton);
      xpos += (bwidth + 0) / 4 + BUTTON_GAP;
    myOptionsButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 1) / 4, bheight,
                                       "Options" + ELLIPSIS, kOptionsCmd);
    wid.push_back(myOptionsButton);
      xpos += (bwidth + 1) / 4 + BUTTON_GAP;
    myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 2) / 4, bheight,
                                       "Go Up", kPrevDirCmd);
    wid.push_back(myPrevDirButton);
      xpos += (bwidth + 2) / 4 + BUTTON_GAP;
    myStartButton = new ButtonWidget(this, font, xpos, ypos, (bwidth + 3) / 4, bheight,
                                     "Select", kLoadROMCmd);
    wid.push_back(myStartButton);
  #endif
  }
  if (myUseMinimalUI) // Highlight 'Rom Listing'
    mySelectedItem = 0;
  else
    mySelectedItem = 2;

  addToFocusList(wid);

  // Create context menu for ROM list options
  VariantList l;
  VarList::push_back(l, "Power-on options" + ELLIPSIS, "override");
  VarList::push_back(l, "Reload listing", "reload");
  myMenu = make_unique<ContextMenu>(this, osystem.frameBuffer().font(), l);

  // Create global props dialog, which is used to temporarily overrride
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
void LauncherDialog::loadRomInfo()
{
  if(!myRomInfoWidget)
    return;

  const string& md5 = selectedRomMD5();
  if(md5 != EmptyString)
  {
    // Get the properties for this entry
    Properties props;
    instance().propSet().getMD5WithInsert(currentNode(), md5, props);

    myRomInfoWidget->setProperties(props, currentNode());
  }
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::showOnlyROMs(bool state)
{
  myShowOnlyROMs = state;
  instance().settings().setValue("launcherroms", state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherDialog::matchPattern(const string& s, const string& pattern) const
{
  // This method is modelled after strcasestr, which we don't use
  // because it isn't guaranteed to be available everywhere
  // The strcasestr uses the KMP algorithm when the comparisons
  // reach a certain point, but since we'll be dealing with relatively
  // short strings, I think the overhead of building a KMP table
  // each time would be slower than the brute force method used here
  const char* haystack = s.c_str();
  const char* needle = pattern.c_str();

  uInt8 b = tolower(*needle);

  needle++;
  for(;; haystack++)
  {
    if(*haystack == '\0')  /* No match */
      return false;

    /* The first character matches */
    if(tolower(*haystack) == b)
    {
      const char* rhaystack = haystack + 1;
      const char* rneedle = needle;

      for(;; rhaystack++, rneedle++)
      {
        if(*rneedle == '\0')   /* Found a match */
          return true;
        if(*rhaystack == '\0') /* No match */
          return false;

        /* Nothing in this round */
        if(tolower(*rhaystack) != tolower(*rneedle))
          break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleKeyDown(StellaKey key, StellaMod mod)
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
  // open power-up options and settings for 2nd and 4th button if not mapped otherwise
  Event::Type e = instance().eventHandler().eventForJoyButton(kMenuMode, stick, button);

  if (button == 1 && (e == Event::UIOK || e == Event::NoType))
    myGlobalProps->open();
  if (button == 3 && (e == Event::Event::UITabPrev || e == Event::NoType))
    openSettings();
  else
  {
    myEventHandled = false;
    myList->setFlags(Widget::FLAG_WANTS_RAWDATA); // allow handling long button press
    Dialog::handleJoyDown(stick, button, longPress);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleJoyUp(int stick, int button)
{
  if (!myEventHandled)
    Dialog::handleJoyUp(stick, button);
  myList->clearFlags(Widget::FLAG_WANTS_RAWDATA); // stop allowing to handle long button press
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type LauncherDialog::getJoyAxisEvent(int stick, int axis, int value, int button)
{
  Event::Type e = instance().eventHandler().eventForJoyAxis(kMenuMode, stick, axis, value, button);

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
      myGlobalProps->open();
      myEventHandled = true;
      break;

    case kQuitCmd:
      close();
      instance().eventHandler().quit();
      break;

    case kRomDirChosenCmd:
    {
      FilesystemNode node(instance().settings().getString("romdir"));
      if(!(node.exists() && node.isDirectory()))
        node = FilesystemNode("~");

      myList->setDirectory(node);
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
void LauncherDialog::openSettings()
{
  // Create an options dialog, similar to the in-game one
  if (instance().settings().getBool("basic_settings"))
  {
    if (myStellaSettingsDialog == nullptr)
      myStellaSettingsDialog = make_unique<StellaSettingsDialog>(instance(), parent(),
        instance().frameBuffer().launcherFont(), _w, _h, Menu::AppMode::launcher);
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
