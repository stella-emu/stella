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

#include <sstream>

#include "bspf.hxx"

#include "BrowserDialog.hxx"
#include "ContextMenu.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "GameList.hxx"
#include "MD5.hxx"
#include "OptionsDialog.hxx"
#include "GlobalPropsDialog.hxx"
#include "LauncherFilterDialog.hxx"
#include "MessageBox.hxx"
#include "OSystem.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "RomInfoWidget.hxx"
#include "Settings.hxx"
#include "StringList.hxx"
#include "StringListWidget.hxx"
#include "Widget.hxx"

#include "LauncherDialog.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h, true),  // use base surface
    myStartButton(NULL),
    myPrevDirButton(NULL),
    myOptionsButton(NULL),
    myQuitButton(NULL),
    myList(NULL),
    myGameList(NULL),
    myRomInfoWidget(NULL),
    myMenu(NULL),
    myGlobalProps(NULL),
    myFilters(NULL),
    myFirstRunMsg(NULL),
    myRomDir(NULL),
    mySelectedItem(0)
{
  const GUI::Font& font = instance().launcherFont();

  const int fontWidth = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            bwidth  = (_w - 2 * 10 - 8 * (4 - 1)) / 4,
            bheight = font.getLineHeight() + 4;
  int xpos = 0, ypos = 0, lwidth = 0, lwidth2 = 0, fwidth = 0;
  WidgetArray wid;

  // Show game name
  lwidth = font.getStringWidth("Select an item from the list ...");
  xpos += 10;  ypos += 8;
  new StaticTextWidget(this, font, xpos, ypos, lwidth, fontHeight,
                       "Select an item from the list ...", kTextAlignLeft);

  lwidth2 = font.getStringWidth("XXXX items found");
  xpos = _w - lwidth2 - 10;
  myRomCount = new StaticTextWidget(this, font, xpos, ypos,
                                    lwidth2, fontHeight,
                                    "", kTextAlignRight);

  // Add filter that can narrow the results shown in the listing
  // It has to fit between both labels
  if(w >= 640)
  {
    fwidth = BSPF_min(15 * fontWidth, xpos - 20 - lwidth);
    xpos -= fwidth + 5;
    myPattern = new EditTextWidget(this, font, xpos, ypos,
                                   fwidth, fontHeight, "");
  }

  // Add list with game titles
  // Before we add the list, we need to know the size of the RomInfoWidget
  xpos = 10;  ypos += fontHeight + 5;
  int romWidth = 0;
  int romSize = instance().settings().getInt("romviewer");
  if(romSize > 1 && w >= 1000 && h >= 760)
    romWidth = 660;
  else if(romSize > 0 && w >= 640 && h >= 480)
    romWidth = 365;

  int listWidth = _w - (romWidth > 0 ? romWidth+5 : 0) - 20;
  myList = new StringListWidget(this, font, xpos, ypos,
                                listWidth, _h - 28 - bheight - 2*fontHeight);
  myList->setEditable(false);
  wid.push_back(myList);
  if(myPattern)  wid.push_back(myPattern);  // Add after the list for tab order

  // Add ROM info area (if enabled)
  if(romWidth > 0)
  {
    xpos += myList->getWidth() + 5;
    myRomInfoWidget =
      new RomInfoWidget(this, romWidth < 660 ? instance().smallFont() : instance().infoFont(),
                        xpos, ypos, romWidth, myList->getHeight());
  }

  // Add note textwidget to show any notes for the currently selected ROM
  xpos = 10;
  xpos += 5;  ypos += myList->getHeight() + 4;
  lwidth = font.getStringWidth("Note:");
  myDirLabel = new StaticTextWidget(this, font, xpos, ypos, lwidth, fontHeight,
                                    "Dir:", kTextAlignLeft);
  xpos += lwidth + 5;
  myDir = new StaticTextWidget(this, font, xpos, ypos,
                                _w - xpos - 10, fontHeight,
                                "", kTextAlignLeft);

  // Add four buttons at the bottom
  xpos = 10;  ypos += myDir->getHeight() + 4;
#ifndef MAC_OSX
  myStartButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                  "Select", kLoadROMCmd);
  wid.push_back(myStartButton);
    xpos += bwidth + 8;
  myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                      "Go Up", kPrevDirCmd);
  wid.push_back(myPrevDirButton);
    xpos += bwidth + 8;
  myOptionsButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                     "Options", kOptionsCmd);
  wid.push_back(myOptionsButton);
    xpos += bwidth + 8;
  myQuitButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                  "Quit", kQuitCmd);
  wid.push_back(myQuitButton);
    xpos += bwidth + 8;
#else
  myQuitButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                  "Quit", kQuitCmd);
  wid.push_back(myQuitButton);
    xpos += bwidth + 8;
  myOptionsButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                     "Options", kOptionsCmd);
  wid.push_back(myOptionsButton);
    xpos += bwidth + 8;
  myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                      "Go Up", kPrevDirCmd);
  wid.push_back(myPrevDirButton);
    xpos += bwidth + 8;
  myStartButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                   "Select", kLoadROMCmd);
  wid.push_back(myStartButton);
    xpos += bwidth + 8;
#endif
  mySelectedItem = 0;  // Highlight 'Rom Listing'

  // Create an options dialog, similar to the in-game one
  myOptions = new OptionsDialog(osystem, parent, this, int(w * 0.8), int(h * 0.8), true);

  // Create a game list, which contains all the information about a ROM that
  // the launcher needs
  myGameList = new GameList();

  addToFocusList(wid);

  // Create context menu for ROM list options
  VariantList l;
  l.push_back("Power-on options", "override");
  l.push_back("Filter listing", "filter");
  l.push_back("Reload listing", "reload");
  myMenu = new ContextMenu(this, osystem->font(), l);

  // Create global props dialog, which is used to temporarily overrride
  // ROM properties
  myGlobalProps = new GlobalPropsDialog(this, osystem->font());

  // Create dialog whereby the files shown in the ROM listing can be customized
  myFilters = new LauncherFilterDialog(this, osystem->font());

  // Figure out which filters are needed for the ROM listing
  setListFilters();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::~LauncherDialog()
{
  delete myOptions;
  delete myGameList;
  delete myMenu;
  delete myGlobalProps;
  delete myFilters;
  delete myRomDir;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& LauncherDialog::selectedRomMD5()
{
  int item = myList->getSelected();
  if(item < 0)
    return EmptyString;

  string extension;
  const FilesystemNode node(myGameList->path(item));
  if(node.isDirectory() || !LauncherFilterDialog::isValidRomName(node, extension))
    return EmptyString;

  // Make sure we have a valid md5 for this ROM
  if(myGameList->md5(item) == "")
    myGameList->setMd5(item, MD5(node));

  return myGameList->md5(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadConfig()
{
  const string& tmpromdir = instance().settings().getString("tmpromdir");
  const string& romdir = tmpromdir != "" ? tmpromdir :
      instance().settings().getString("romdir");

  // When romdir hasn't been set, it probably indicates that this is the first
  // time running Stella; in this case, we should prompt the user
  if(romdir == "")
  {
    if(!myFirstRunMsg)
    {
      StringList msg;
      msg.push_back("This seems to be your first time running Stella.");
      msg.push_back("Before you can start a game, you need to");
      msg.push_back("specify where your ROMs are located.");
      msg.push_back("");
      msg.push_back("Click 'OK' to select a default ROM directory,");
      msg.push_back("or 'Cancel' to browse the filesystem manually.");
      myFirstRunMsg = new GUI::MessageBox(this, instance().font(), msg,
                                          _w, _h, kFirstRunMsgChosenCmd);
    }
    myFirstRunMsg->show();
  }

  // Assume that if the list is empty, this is the first time that loadConfig()
  // has been called (and we should reload the list)
  if(myList->getList().isEmpty())
  {
    myPrevDirButton->setEnabled(false);
    myCurrentNode = FilesystemNode(romdir == "" ? "~" : romdir);
    if(!(myCurrentNode.exists() && myCurrentNode.isDirectory()))
      myCurrentNode = FilesystemNode("~");

    updateListing();
  }
  Dialog::setFocus(getFocusList()[mySelectedItem]);

  if(myRomInfoWidget)
    myRomInfoWidget->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::enableButtons(bool enable)
{
  myStartButton->setEnabled(enable);
  myPrevDirButton->setEnabled(enable);
  myOptionsButton->setEnabled(enable);
  myQuitButton->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateListing(const string& nameToSelect)
{
  // Start with empty list
  myGameList->clear();
  myDir->setLabel("");

  loadDirListing();

  // Only hilite the 'up' button if there's a parent directory
  myPrevDirButton->setEnabled(myCurrentNode.hasParent());

  // Show current directory
  myDir->setLabel(myCurrentNode.getShortPath());

  // Now fill the list widget with the contents of the GameList
  StringList l;
  for (int i = 0; i < (int) myGameList->size(); ++i)
    l.push_back(myGameList->name(i));

  myList->setList(l);

  // Indicate how many files were found
  ostringstream buf;
  buf << (myGameList->size() - 1) << " items found";
  myRomCount->setLabel(buf.str());

  // Restore last selection
  const string& find =
    nameToSelect == "" ? instance().settings().getString("lastrom") : nameToSelect;
  myList->setSelected(find);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadDirListing()
{
  if(!myCurrentNode.isDirectory())
    return;

  FSList files;
  files.reserve(2048);
  myCurrentNode.getChildren(files, FilesystemNode::kListAll);

  // Add '[..]' to indicate previous folder
  if(myCurrentNode.hasParent())
    myGameList->appendGame(" [..]", "", "", true);

  // Now add the directory entries
  bool domatch = myPattern && myPattern->getText() != "";
  for(unsigned int idx = 0; idx < files.size(); idx++)
  {
    bool isDir = files[idx].isDirectory();
    const string& name = isDir ? (" [" + files[idx].getName() + "]")
                               : files[idx].getName();

    // Honour the filtering settings
    // Showing only certain ROM extensions is determined by the extension
    // that we want - if there are no extensions, it implies show all files
    // In this way, showing all files is on the 'fast code path'
    if(!isDir && myRomExts.size() > 0)
    {
      // Skip over those names we've filtered out
      if(!LauncherFilterDialog::isValidRomName(name, myRomExts))
        continue;
    }

    // Skip over files that don't match the pattern in the 'pattern' textbox
    if(domatch && !isDir && !matchPattern(name, myPattern->getText()))
      continue;

    myGameList->appendGame(name, files[idx].getPath(), "", isDir);
  }

  // Sort the list by rom name (since that's what we see in the listview)
  myGameList->sortByName();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRomInfo()
{
  if(!myRomInfoWidget) return;
  int item = myList->getSelected();
  if(item < 0) return;

  string extension;
  const FilesystemNode node(myGameList->path(item));
  if(!node.isDirectory() && LauncherFilterDialog::isValidRomName(node, extension))
  {
    // Make sure we have a valid md5 for this ROM
    if(myGameList->md5(item) == "")
      myGameList->setMd5(item, MD5(node));

    // Get the properties for this entry
    Properties props;
    instance().propSet().getMD5WithInsert(node, myGameList->md5(item), props);

    myRomInfoWidget->setProperties(props);
  }
  else
    myRomInfoWidget->clearProperties();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleContextMenu()
{
  const string& cmd = myMenu->getSelectedTag().toString();

  if(cmd == "override")
  {
    myGlobalProps->open();
  }
  else if(cmd == "filter")
  {
    myFilters->open();
  }
  else if(cmd == "reload")
  {
    updateListing();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::setListFilters()
{
  const string& exts = instance().settings().getString("launcherexts");
  myRomExts.clear();
  LauncherFilterDialog::parseExts(myRomExts, exts);
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

  unsigned char b = tolower((unsigned char) *needle);

  needle++;
  for (;; haystack++)
  {
    if (*haystack == '\0')  /* No match */
      return false;

    /* The first character matches */
    if (tolower ((unsigned char) *haystack) == b)
    {
      const char* rhaystack = haystack + 1;
      const char* rneedle = needle;

      for (;; rhaystack++, rneedle++)
      {
        if (*rneedle == '\0')   /* Found a match */
          return true;
        if (*rhaystack == '\0') /* No match */
          return false;

        /* Nothing in this round */
        if (tolower ((unsigned char) *rhaystack)
            != tolower ((unsigned char) *rneedle))
          break;
      }
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleKeyDown(StellaKey key, StellaMod mod, char ascii)
{
  // Grab the key before passing it to the actual dialog and check for
  // Control-R (reload ROM listing)
  if(instance().eventHandler().kbdControl(mod) && key == KBDK_r)
    updateListing();
  else
    Dialog::handleKeyDown(key, mod, ascii);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Grab right mouse button for context menu, send left to base class
  if(button == 2)
  {
    // Add menu at current x,y mouse location
    myMenu->show(x + getAbsX(), y + getAbsY());
  }
  else
    Dialog::handleMouseDown(x, y, button, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kLoadROMCmd:
    case ListWidget::kActivatedCmd:
    case ListWidget::kDoubleClickedCmd:
    {
      int item = myList->getSelected();
      if(item >= 0)
      {
        const FilesystemNode romnode(myGameList->path(item));

        // Directory's should be selected (ie, enter them and redisplay)
        if(romnode.isDirectory())
        {
          string dirname = "";
          if(myGameList->name(item) == " [..]")
          {
            myCurrentNode = myCurrentNode.getParent();
            if(!myNodeNames.empty())
              dirname = myNodeNames.pop();
          }
          else
          {
            myCurrentNode = romnode;
            myNodeNames.push(myGameList->name(item));
          }
          updateListing(dirname);
        }
        else
        {
          const string& result =
            instance().createConsole(romnode, myGameList->md5(item));
          if(result == EmptyString)
            instance().settings().setValue("lastrom", myList->getSelectedString());
          else
            instance().frameBuffer().showMessage(result, kMiddleCenter, true);
        }
      }
      break;
    }

    case kOptionsCmd:
      myOptions->open();
      break;

    case kPrevDirCmd:
    case ListWidget::kPrevDirCmd:
      myCurrentNode = myCurrentNode.getParent();
      updateListing(myNodeNames.empty() ? "" : myNodeNames.pop());
      break;

    case ListWidget::kSelectionChangedCmd:
      loadRomInfo();
      break;

    case kQuitCmd:
      close();
      instance().eventHandler().quit();
      break;

    case kFirstRunMsgChosenCmd:
      // Show a file browser, starting from the users' home directory
      if(!myRomDir)
        myRomDir = new BrowserDialog(this, instance().font(), _w, _h);

      myRomDir->show("Select ROM directory:", "~",
                     BrowserDialog::Directories, kStartupRomDirChosenCmd);
      break;

    case kStartupRomDirChosenCmd:
    {
      FilesystemNode dir(myRomDir->getResult());
      instance().settings().setValue("romdir", dir.getShortPath());
      // fall through to the next case
    }
    case kRomDirChosenCmd:
      myCurrentNode = FilesystemNode(instance().settings().getString("romdir"));
      if(!(myCurrentNode.exists() && myCurrentNode.isDirectory()))
        myCurrentNode = FilesystemNode("~");
      updateListing();
      break;

    case kReloadRomDirCmd:
      updateListing();
      break;

    case kReloadFiltersCmd:
      setListFilters();
      updateListing();
      break;

    case ContextMenu::kItemSelectedCmd:
      handleContextMenu();
      break;

    case EditableWidget::kAcceptCmd:
    case EditableWidget::kChangedCmd:
      // The updateListing() method knows what to do when the text changes
      updateListing();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
