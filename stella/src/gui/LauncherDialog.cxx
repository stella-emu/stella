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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: LauncherDialog.cxx,v 1.86 2008-03-30 15:47:10 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "BrowserDialog.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GameList.hxx"
#include "MD5.hxx"
#include "OptionsDialog.hxx"
#include "OSystem.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "RomInfoWidget.hxx"
#include "Settings.hxx"
#include "StringListWidget.hxx"
#include "Widget.hxx"

#include "LauncherDialog.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myStartButton(NULL),
    myPrevDirButton(NULL),
    myOptionsButton(NULL),
    myQuitButton(NULL),
    myList(NULL),
    myGameList(NULL),
    myProgressBar(NULL),
    myRomInfoWidget(NULL),
    mySelectedItem(0),
    myRomInfoFlag(false)
{
  const GUI::Font& font = instance()->launcherFont();

  const int fontHeight  = font.getFontHeight();
  const int bwidth  = (_w - 2 * 10 - 8 * (4 - 1)) / 4;
  const int bheight = font.getLineHeight() + 4;
  int xpos = 0, ypos = 0, lwidth = 0;
  WidgetArray wid;

  // Check if we want the ROM info viewer
  // Make sure it will fit within the current bounds
  myRomInfoFlag = instance()->settings().getBool("romviewer");
  if((w < 640 || h < 480) && myRomInfoFlag)
  {
    cerr << "Error: ROM launcher too small, deactivating ROM info viewer" << endl;
    myRomInfoFlag = false;
  }

  // Show game name
  lwidth = font.getStringWidth("Select an item from the list ...");
  xpos += 10;  ypos += 8;
  new StaticTextWidget(this, font, xpos, ypos, lwidth, fontHeight,
                       "Select an item from the list ...", kTextAlignLeft);

  lwidth = font.getStringWidth("XXXX items found");
  xpos = _w - lwidth - 10;
  myRomCount = new StaticTextWidget(this, font, xpos, ypos,
                                    lwidth, fontHeight,
                                    "", kTextAlignRight);

  // Add list with game titles
  xpos = 10;  ypos += fontHeight + 5;
  int listWidth = myRomInfoFlag ? _w - 350 : _w - 20;
  myList = new StringListWidget(this, font, xpos, ypos,
                                listWidth, _h - 28 - bheight - 2*fontHeight);
  myList->setNumberingMode(kListNumberingOff);
  myList->setEditable(false);
  wid.push_back(myList);

  // Add ROM info area (if enabled)
  if(myRomInfoFlag)
  {
    xpos += myList->getWidth() + 15;
    myRomInfoWidget = new RomInfoWidget(this, instance()->font(), xpos, ypos,
                                        326, myList->getHeight());
    wid.push_back(myRomInfoWidget);
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
  myOptions = new OptionsDialog(osystem, parent, this, true);  // not in game mode

  // Create a game list, which contains all the information about a ROM that
  // the launcher needs
  myGameList = new GameList();

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::~LauncherDialog()
{
  delete myOptions;
  delete myGameList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string LauncherDialog::selectedRomMD5()
{
  string extension;
  int item = myList->getSelected();
  if(item < 0 || myGameList->isDir(item) ||
     !instance()->isValidRomName(myGameList->name(item), extension))
    return "";

  // Make sure we have a valid md5 for this ROM
  if(myGameList->md5(item) == "")
  {
    const string& md5 = instance()->MD5FromFile(myGameList->path(item));
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
    myCurrentNode = instance()->settings().getString("romdir");

    updateListing();
  }
  Dialog::setFocus(getFocusList()[mySelectedItem]);

  if(myRomInfoFlag)
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
void LauncherDialog::updateListing(bool fullReload)
{
  // Start with empty list
  myGameList->clear();
  myDir->setLabel("");

  string romdir = instance()->settings().getString("romdir");
  loadDirListing();

  // Only hilite the 'up' button if there's a parent directory
  myPrevDirButton->setEnabled(myCurrentNode.hasParent());

  // Show current directory
  myDir->setLabel(myCurrentNode.path());

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
    string lastrom = instance()->settings().getString("lastrom");
    if(lastrom == "")
      selected = 0;
    else
    {
      unsigned int itemToSelect = 0;
      StringList::const_iterator iter;
      for(iter = myList->getList().begin(); iter != myList->getList().end();
          ++iter, ++itemToSelect)	 
      {
        if(lastrom == *iter)
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

  FSList files = myCurrentNode.listDir(FilesystemNode::kListAll);

  // Add '[..]' to indicate previous folder
  if(myCurrentNode.hasParent())
    myGameList->appendGame(" [..]", "", "", true);

  // Now add the directory entries
  for(unsigned int idx = 0; idx < files.size(); idx++)
  {
    string name = files[idx].displayName();
    bool isDir = files[idx].isDirectory();
    if(isDir)
      name = " [" + name + "]";

    myGameList->appendGame(name, files[idx].path(), "", isDir);
  }

  // Sort the list by rom name (since that's what we see in the listview)
  myGameList->sortByName();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRomInfo()
{
  if(!(myRomInfoFlag && myRomInfoWidget)) return;
  int item = myList->getSelected();
  if(item < 0) return;

  string extension;
  if(!myGameList->isDir(item) &&
     instance()->isValidRomName(myGameList->name(item), extension))
  {
    // Make sure we have a valid md5 for this ROM
    if(myGameList->md5(item) == "")
      myGameList->setMd5(item, instance()->MD5FromFile(myGameList->path(item)));

    // Get the properties for this entry
    Properties props;
    const string& md5 = myGameList->md5(item);
    instance()->propSet().getMD5(md5, props);

    myRomInfoWidget->setProperties(props);
  }
  else
    myRomInfoWidget->clearProperties();
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
          if(myGameList->name(item) == " [..]")
            myCurrentNode = myCurrentNode.getParent();
		  else
            myCurrentNode = rom;
          updateListing();
        }
        else if(!instance()->isValidRomName(rom, extension) ||
                !instance()->createConsole(rom, md5))
        {
          instance()->frameBuffer().showMessage("Not a valid ROM file", kMiddleCenter);
        }
        else
        {
        #if !defined(GP2X)   // Quick GP2X hack to spare flash-card saves
          instance()->settings().setString("lastrom", myList->getSelectedString());
        #endif
        }
      }
      break;
    }

    case kOptionsCmd:
      parent()->addDialog(myOptions);
      break;

    case kPrevDirCmd:
      myCurrentNode = myCurrentNode.getParent();
      updateListing(false);
      break;

    case kListSelectionChangedCmd:
      loadRomInfo();
      break;

    case kQuitCmd:
      close();
      instance()->eventHandler().quit();
      break;

    case kRomDirChosenCmd:
      myCurrentNode = instance()->settings().getString("romdir");
      updateListing();
      break;

    case kSnapDirChosenCmd:
      // Stub just in case we need it
      break;

    case kReloadRomDirCmd:
      updateListing(true);
      break;

    case kResizeCmd:
      // Instead of figuring out how to resize the snapshot image,
      // we just reload it
      if(myRomInfoFlag)
      {
        myRomInfoWidget->initialize();
        loadRomInfo();
      }
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
