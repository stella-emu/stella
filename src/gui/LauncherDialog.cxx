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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
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
  myList->setNumberingMode(kListNumberingOff);
  myList->setEditable(false);
  wid.push_back(myList);
  if(myPattern)  wid.push_back(myPattern);  // Add after the list for tab order

  // Add ROM info area (if enabled)
  if(romWidth > 0)
  {
    xpos += myList->getWidth() + 5;
    myRomInfoWidget =
      new RomInfoWidget(this, romWidth < 660 ? instance().smallFont() : instance().consoleFont(),
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
                                  "Select", kStartCmd);
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
                                   "Select", kStartCmd);
  wid.push_back(myStartButton);
    xpos += bwidth + 8;
#endif
  mySelectedItem = 0;  // Highlight 'Rom Listing'

  // Create an options dialog, similar to the in-game one
  myOptions = new OptionsDialog(osystem, parent, this, w, h, true);  // not in game mode

  // Create a game list, which contains all the information about a ROM that
  // the launcher needs
  myGameList = new GameList();

  addToFocusList(wid);

  // Create context menu for ROM list options
  StringMap l;
  l.push_back("Override properties", "override");
  l.push_back("Filter listing", "filter");
  l.push_back("Reload listing", "reload");
  myMenu = new ContextMenu(this, osystem->font(), l);

  // Create global props dialog, which is used to temporarily overrride
  // ROM properties
  myGlobalProps = new GlobalPropsDialog(this, osystem->font());

  // Create dialog whereby the files shown in the ROM listing can be
  // customized
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string LauncherDialog::selectedRomMD5()
{
  string extension;
  int item = myList->getSelected();
  if(item < 0 || myGameList->isDir(item) ||
     !LauncherFilterDialog::isValidRomName(myGameList->name(item), extension))
    return "";

  // Make sure we have a valid md5 for this ROM
  if(myGameList->md5(item) == "")
  {
    const string& md5 = instance().MD5FromFile(myGameList->path(item));
    myGameList->setMd5(item, md5);
  }
  return myGameList->md5(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadConfig()
{
  // Assume that if the list is empty, this is the first time that loadConfig()
  // has been called (and we should reload the list).
  if(myList->getList().isEmpty())
  {
    myPrevDirButton->setEnabled(false);
    myCurrentNode = FilesystemNode(instance().settings().getString("romdir"));
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

  string romdir = instance().settings().getString("romdir");
  loadDirListing();

  // Only hilite the 'up' button if there's a parent directory
  myPrevDirButton->setEnabled(myCurrentNode.hasParent());

  // Show current directory
  myDir->setLabel(myCurrentNode.getRelativePath());

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
  int selected = -1;
  if(!myList->getList().isEmpty())
  {
    const string& find =
      nameToSelect == "" ? instance().settings().getString("lastrom") : nameToSelect;

    if(find == "")
      selected = 0;
    else
    {
      unsigned int itemToSelect = 0;
      StringList::const_iterator iter;
      for(iter = myList->getList().begin(); iter != myList->getList().end();
          ++iter, ++itemToSelect)	 
      {
        if(find == *iter)
        {
          selected = itemToSelect;
          break;
        }
      }
      if(itemToSelect > myList->getList().size())
        selected = 0;
    }
  }
  myList->setSelected(selected);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadDirListing()
{
  if(!myCurrentNode.isDirectory())
    return;

  FSList files;
  myCurrentNode.getChildren(files, FilesystemNode::kListAll);

  // Add '[..]' to indicate previous folder
  if(myCurrentNode.hasParent())
    myGameList->appendGame(" [..]", "", "", true);

  // Now add the directory entries
  bool domatch = myPattern && myPattern->getEditString() != "";
  for(unsigned int idx = 0; idx < files.size(); idx++)
  {
    string name = files[idx].getDisplayName();
    bool isDir = files[idx].isDirectory();

    // Honour the filtering settings
    // Showing only certain ROM extensions is determined by the extension
    // that we want - if there are no extensions, it implies show all files
    // In this way, showing all files is on the 'fast code path'
    if(isDir)
      name = " [" + name + "]";
    else if(myRomExts.size() > 0)
    {
      // Skip over those names we've filtered out
      if(!LauncherFilterDialog::isValidRomName(name, myRomExts))
        continue;
    }

    // Skip over files that don't match the pattern in the 'pattern' textbox
    if(domatch && !isDir && !matchPattern(name, myPattern->getEditString()))
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
  if(!myGameList->isDir(item) &&
     LauncherFilterDialog::isValidRomName(myGameList->name(item), extension))
  {
    // Make sure we have a valid md5 for this ROM
    if(myGameList->md5(item) == "")
      myGameList->setMd5(item, instance().MD5FromFile(myGameList->path(item)));

    // Get the properties for this entry
    Properties props;
    instance().propSet().getMD5(myGameList->md5(item), props);

    myRomInfoWidget->setProperties(props);
  }
  else
    myRomInfoWidget->clearProperties();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleContextMenu()
{
  const string& cmd = myMenu->getSelectedTag();

  if(cmd == "override")
  {
    parent().addDialog(myGlobalProps);
  }
  else if(cmd == "filter")
  {
    parent().addDialog(myFilters);
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
bool LauncherDialog::matchPattern(const string& s, const string& pattern)
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
void LauncherDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Grab the key before passing it to the actual dialog and check for
  // Control-R (reload ROM listing)
  if(instance().eventHandler().kbdControl(modifiers) && keycode == 'r')
    updateListing();
  else
    Dialog::handleKeyDown(ascii, keycode, modifiers);
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
    case kStartCmd:
    case kListItemActivatedCmd:
    case kListItemDoubleClickedCmd:
    {
      int item = myList->getSelected();
      if(item >= 0)
      {
        const string& rom = myGameList->path(item);
        const string& md5 = myGameList->md5(item);
        string extension;

        // Directory's should be selected (ie, enter them and redisplay)
        if(myGameList->isDir(item))
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
            myCurrentNode = FilesystemNode(rom);
            myNodeNames.push(myGameList->name(item));
          }
          updateListing(dirname);
        }
        else
        {
          if(LauncherFilterDialog::isValidRomName(rom, extension))
          {
            if(instance().createConsole(rom, md5))
            {
            #if !defined(GP2X)   // Quick GP2X hack to spare flash-card saves
              instance().settings().setString("lastrom", myList->getSelectedString());
            #endif
            }
            else
              instance().frameBuffer().showMessage(
                  "Error creating console (screen too small)", kMiddleCenter, true);
          }
          else
            instance().frameBuffer().showMessage("Not a valid ROM file",
                                                 kMiddleCenter, true);
        }
      }
      break;
    }

    case kOptionsCmd:
      parent().addDialog(myOptions);
      break;

    case kPrevDirCmd:
    case kListPrevDirCmd:
      myCurrentNode = myCurrentNode.getParent();
      updateListing(myNodeNames.empty() ? "" : myNodeNames.pop());
      break;

    case kListSelectionChangedCmd:
      loadRomInfo();
      break;

    case kQuitCmd:
      close();
      instance().eventHandler().quit();
      break;

    case kRomDirChosenCmd:
      myCurrentNode = FilesystemNode(instance().settings().getString("romdir"));
      if(!(myCurrentNode.exists() && myCurrentNode.isDirectory()))
        myCurrentNode = FilesystemNode("~");
      updateListing();
      break;

    case kSnapDirChosenCmd:
      // Stub just in case we need it
      break;

    case kReloadRomDirCmd:
      updateListing();
      break;

    case kReloadFiltersCmd:
      setListFilters();
      updateListing();
      break;

    case kCMenuItemSelectedCmd:
      handleContextMenu();
      break;

    case kEditAcceptCmd:
    case kEditChangedCmd:
      // The updateListing() method knows what to do when the text changes
      updateListing();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
