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
// $Id: LauncherDialog.cxx,v 1.52 2006-03-20 13:23:13 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "Props.hxx"
#include "MD5.hxx"
#include "FSNode.hxx"
#include "Widget.hxx"
#include "StringListWidget.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "GuiUtils.hxx"
#include "BrowserDialog.hxx"
#include "ProgressDialog.hxx"
#include "LauncherOptionsDialog.hxx"
#include "LauncherDialog.hxx"

#include "bspf.hxx"


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
    mySelectedItem(0),
    myBrowseModeFlag(false)
{
  const GUI::Font& font = instance()->launcherFont();

  const int fontHeight  = font.getFontHeight();
  const int bwidth  = (_w - 2 * 10 - 8 * (4 - 1)) / 4;
  const int bheight = font.getLineHeight() + 4;
  int xpos = 0, ypos = 0, lwidth = 0;
  WidgetArray wid;

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
  // The list isn't added to focus objects, but is instead made 'sticky'
  // This means it will act as if it were focused (wrt how it's drawn), but
  // won't actually be able to lose focus
  xpos = 10;  ypos += fontHeight + 5;
  myList = new StringListWidget(this, font, xpos, ypos,
                                _w - 20, _h - 28 - bheight - 2*fontHeight);
  myList->setNumberingMode(kListNumberingOff);
  myList->setEditable(false);
  myList->setFlags(WIDGET_STICKY_FOCUS);

  // Add note textwidget to show any notes for the currently selected ROM
  xpos += 5;  ypos += myList->getHeight() + 4;
  lwidth = font.getStringWidth("Note:");
  myNoteLabel = new StaticTextWidget(this, font, xpos, ypos, lwidth, fontHeight,
                                     "Note:", kTextAlignLeft);
  xpos += lwidth + 5;
  myNote = new StaticTextWidget(this, font, xpos, ypos,
                                _w - xpos - 10, fontHeight,
                                "", kTextAlignLeft);

  // Add four buttons at the bottom
  xpos = 10;  ypos += myNote->getHeight() + 4;
#ifndef MAC_OSX
  myStartButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                  "Select", kStartCmd, 0);
  myStartButton->setEditable(true);
  wid.push_back(myStartButton);
    xpos += bwidth + 8;
  myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                      "Go Up", kPrevDirCmd, 0);
  myPrevDirButton->setEditable(true);
  wid.push_back(myPrevDirButton);
    xpos += bwidth + 8;
  myOptionsButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                     "Options", kOptionsCmd, 0);
  myOptionsButton->setEditable(true);
  wid.push_back(myOptionsButton);
    xpos += bwidth + 8;
  myQuitButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                  "Quit", kQuitCmd, 0);
  myQuitButton->setEditable(true);
  wid.push_back(myQuitButton);
    xpos += bwidth + 8;
  mySelectedItem = 0;  // Highlight 'Select' button
#else
  myQuitButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                  "Quit", kQuitCmd, 0);
  myQuitButton->setEditable(true);
  wid.push_back(myQuitButton);
    xpos += bwidth + 8;
  myOptionsButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                     "Options", kOptionsCmd, 0);
  myOptionsButton->setEditable(true);
  wid.push_back(myOptionsButton);
    xpos += bwidth + 8;
  myPrevDirButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                      "Go Up", kPrevDirCmd, 0);
  myPrevDirButton->setEditable(true);
  wid.push_back(myPrevDirButton);
    xpos += bwidth + 8;
  myStartButton = new ButtonWidget(this, font, xpos, ypos, bwidth, bheight,
                                   "Select", kStartCmd, 0);
  myStartButton->setEditable(true);
  wid.push_back(myStartButton);
    xpos += bwidth + 8;
  mySelectedItem = 3;  // Highlight 'Select' button
#endif

  // Create the launcher options dialog, where you can change ROM
  // and snapshot paths
  myOptions = new LauncherOptionsDialog(osystem, parent, font, this,
                                        20, 60, _w - 40, _h - 120);

  // Create a game list, which contains all the information about a ROM that
  // the launcher needs
  myGameList = new GameList();

  addToFocusList(wid);

  // (De)activate browse mode
  myBrowseModeFlag = instance()->settings().getBool("rombrowse");
  myPrevDirButton->setEnabled(myBrowseModeFlag);
  myNoteLabel->setEnabled(!myBrowseModeFlag);

  myCurrentNode = instance()->settings().getString("romdir");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::~LauncherDialog()
{
  delete myOptions;
  delete myGameList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadConfig()
{
  // Assume that if the list is empty, this is the first time that loadConfig()
  // has been called (and we should reload the list).
  if(myList->getList().isEmpty())
    updateListing();

  Dialog::setFocus(getFocusList()[mySelectedItem]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::enableButtons(bool enable)
{
  myStartButton->setEnabled(enable);
  myPrevDirButton->setEnabled(enable && myBrowseModeFlag);
  myOptionsButton->setEnabled(enable);
  myQuitButton->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateListing(bool fullReload)
{
  // Start with empty list
  myGameList->clear();

  // If this is the first time using Stella, the romdir won't be set.
  // In that case, display the options dialog, and don't let Stella proceed
  // until the options are set.
  string romdir = instance()->settings().getString("romdir");
  if(romdir == "")
  {
    myOptionsButton->setEnabled(true);
    myQuitButton->setEnabled(true);
    parent()->addDialog(myOptions);
    return;
  }

  // If in ROM browse mode, just load the current directory and
  // don't translate by md5sum at all
  if(myBrowseModeFlag)
  {
    loadDirListing();

    // Only hilite the 'up' button if there's a parent directory
    myPrevDirButton->setEnabled(myCurrentNode.hasParent());
  }
  else
  {
    // Disable buttons, pending a reload from disk
    enableButtons(false);

    // Figure out if the ROM dir has changed since we last accessed it.
    // If so, we do a full reload from disk (takes quite some time).
    // Otherwise, we can use the cache file (which is much faster).
    string currentModTime = FilesystemNode::modTime(romdir);
    string oldModTime = instance()->settings().getString("modtime");

    if(currentModTime != oldModTime)  // romdir has changed
      loadListFromDisk();
    else if(FilesystemNode::fileExists(instance()->cacheFile()) && !fullReload)
      loadListFromCache();
    else  // we have no other choice
      loadListFromDisk();

    // Re-enable buttons
    enableButtons(true);
  }

  // Now fill the list widget with the contents of the GameList
  StringList l;
  for (int i = 0; i < (int) myGameList->size(); ++i)
    l.push_back(myGameList->name(i));

  myList->setList(l);

  // Indicate how many files were found
  ostringstream buf;
  buf << myGameList->size() << " items found";
  myRomCount->setLabel(buf.str());

  // Restore last selection
  if(!myList->getList().isEmpty())
  {
    if(myBrowseModeFlag)
      myList->setSelected(0);
    else
    {
      string lastrom = instance()->settings().getString("lastrom");
      if(lastrom == "")
        myList->setSelected(0);
      else
      {
        unsigned int itemToSelect = 0;
        StringList::const_iterator iter;
        for (iter = myList->getList().begin(); iter != myList->getList().end();
             ++iter, ++itemToSelect)
        {
          if (lastrom == *iter)
          {
            myList->setSelected(itemToSelect);
            break;
          }
        }
        if(itemToSelect > myList->getList().size())
          myList->setSelected(0);
      }
    }
  }
  else
    myList->setSelected(-1);  // redraw the empty list
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadDirListing()
{
  FSList files = myCurrentNode.listDir(FilesystemNode::kListAll);

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
void LauncherDialog::loadListFromDisk()
{
  string romdir = instance()->settings().getString("romdir");
  FilesystemNode dir(romdir);
  FSList files = dir.listDir(FilesystemNode::kListFilesOnly);

  // Create a progress dialog box to show the progress of processing
  // the ROMs, since this is usually a time-consuming operation
  ProgressDialog progress(this, instance()->launcherFont(),
                          "Loading ROM info from disk ...");
  progress.setRange(0, files.size() - 1, 10);

  // Create a entry for the GameList for each file
  Properties props;
  string md5, name, note;
  for (unsigned int idx = 0; idx < files.size(); idx++)
  {
    // Calculate the MD5 so we can get the rest of the info
    // from the PropertiesSet (stella.pro)
    md5 = MD5FromFile(files[idx].path());
    instance()->propSet().getMD5(md5, props);
    name = props.get(Cartridge_Name);
    note = props.get(Cartridge_Note);

    // Indicate that this ROM doesn't have a properties entry
    myGameList->appendGame(name, files[idx].path(), note);

    // Update the progress bar, indicating one more ROM has been processed
    progress.setProgress(idx);
  }
  progress.done();

  // Sort the list by rom name (since that's what we see in the listview)
  myGameList->sortByName();

  // And create a cache file, so that the next time Stella starts,
  // we don't have to do this time-consuming operation again
  createListCache();

  // Finally, update the modification time so we don't have to needlessly
  // call this method again
  string currentModTime = FilesystemNode::modTime(romdir);
  instance()->settings().setString("modtime", currentModTime);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadListFromCache()
{
  string cacheFile = instance()->cacheFile();
  ifstream in(cacheFile.c_str());
  if(!in)
  {
    loadListFromDisk();
    return;
  }

  // It seems terribly ugly that we need to use char arrays
  // instead of strings.  Or maybe I don't know the correct way ??
  char buf[2048];
  string line, name, path, note;
  string::size_type pos1, pos2;  // The locations of the two '|' characters

  // Keep reading until all lines have been inspected
  while(!in.eof())
  {
    in.getline(buf, 2048);
    line = buf;

    // Now split the line into three parts
    pos1 = line.find("|", 0);
    if(pos1 == string::npos) continue;
    pos2 = line.find("|", pos1+1);
    if(pos2 == string::npos) continue;

    path = line.substr(0, pos1);
    name = line.substr(pos1+1, pos2-pos1-1);
    note = line.substr(pos2+1);

    // Add this game to the list
    // We don't do sorting, since it's assumed to be done by loadListFromDisk()
    myGameList->appendGame(name, path, note);
  }    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::createListCache()
{
  string cacheFile = instance()->cacheFile();
  ofstream out(cacheFile.c_str());

  // Write the gamelist to the cachefile (sorting is already done)
  for (int i = 0; i < (int) myGameList->size(); ++i)
  {
    out << myGameList->path(i)  << "|"
        << myGameList->name(i) << "|"
        << myGameList->note(i)
        << endl;
  }
  out.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string LauncherDialog::MD5FromFile(const string& path)
{
  uInt8* image;
  int size = -1;
  string md5 = "";

  if(instance()->openROM(path, md5, &image, &size))
    if(size != -1)
      delete[] image;

  return md5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Only detect the cursor keys, otherwise pass to base class
  switch(ascii)
  {
    case kCursorLeft:
      mySelectedItem--;
      if(mySelectedItem < 0) mySelectedItem = 3;
      Dialog::setFocus(getFocusList()[mySelectedItem]);
      break;

    case kCursorRight:
      mySelectedItem++;
      if(mySelectedItem > 3) mySelectedItem = 0;
      Dialog::setFocus(getFocusList()[mySelectedItem]);
      break;

    case ' ':  // Used to activate currently focused button
    case '\n':
    case '\r':
      Dialog::handleKeyDown(ascii, keycode, modifiers);
      break;

    default:
      if(!myList->handleKeyDown(ascii, keycode, modifiers))
        Dialog::handleKeyDown(ascii, keycode, modifiers);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleJoyAxis(int stick, int axis, int value)
{
  if(!parent()->joymouse())
    return;

  // We make the (hopefully) valid assumption that all joysticks
  // treat axis the same way.  Eventually, we may need to remap
  // these actions of this assumption is invalid.
  // TODO - make this work better with analog axes ...
  int key = -1;
  if(axis % 2 == 0)  // x-direction
  {
    if(value < 0)
      key = kCursorLeft;
    else if(value > 0)
      key = kCursorRight;
  }
  else   // y-direction
  {
    if(value < 0)
      key = kCursorUp;
    else if(value > 0)
      key = kCursorDown;
  }

  if(key != -1)
    handleKeyDown(key, 0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  int item;

  switch (cmd)
  {
    case kStartCmd:
    case kListItemActivatedCmd:
    case kListItemDoubleClickedCmd:
      item = myList->getSelected();
      if(item >= 0)
      {
        string entry = myGameList->path(item);

        // Directory's should be selected (ie, enter them and redisplay)
        if(myBrowseModeFlag && myGameList->isDir(item))
        {
          myCurrentNode = entry;
          updateListing();
        }
        else if(instance()->createConsole(entry))
        {
#if !defined(GP2X)   // Quick GP2X hack to spare flash-card saves
          // Make sure the console creation actually succeeded
          instance()->settings().setString("lastrom",
              myList->getSelectedString());
#endif
          close();
        }
      }
      break;

    case kOptionsCmd:
      parent()->addDialog(myOptions);
      break;

    case kPrevDirCmd:
    {
      myCurrentNode = myCurrentNode.getParent();
      updateListing(!myBrowseModeFlag);  // Force full update in non-browse mode
      break;
    }

    case kListSelectionChangedCmd:
      item = myList->getSelected();
      if(item >= 0)
        myNote->setLabel(myGameList->note(item));
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

    case kBrowseChangedCmd:
      myCurrentNode = instance()->settings().getString("romdir");
      myBrowseModeFlag = instance()->settings().getBool("rombrowse");
      myPrevDirButton->setEnabled(myBrowseModeFlag);
      myNoteLabel->setEnabled(!myBrowseModeFlag);
      break;

    case kReloadRomDirCmd:
      updateListing(true);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
