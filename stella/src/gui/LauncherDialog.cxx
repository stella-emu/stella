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
// $Id: LauncherDialog.cxx,v 1.32 2005-10-19 00:59:51 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <algorithm>
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

/////////////////////////////////////////
// TODO - make this dialog font sensitive
/////////////////////////////////////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myStartButton(NULL),
    myOptionsButton(NULL),
    myReloadButton(NULL),
    myQuitButton(NULL),
    myList(NULL),
    myGameList(NULL),
    myProgressBar(NULL)
{
  const GUI::Font& font = instance()->font();
  const int fontHeight = font.getFontHeight();

  // Show game name
  new StaticTextWidget(this, 10, 8, 200, fontHeight,
                       "Select a game from the list ...", kTextAlignLeft);

  myRomCount = new StaticTextWidget(this, _w - 100, 8, 90, fontHeight,
                                    "", kTextAlignRight);

  // Add four buttons at the bottom
  const int border = 10;
  const int space = 8;
  const int buttons = 4;
  const int width = (_w - 2 * border - space * (buttons - 1)) / buttons;
  int xpos = border;

#ifndef MAC_OSX
  myStartButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Play", kStartCmd, 'S');
    xpos += space + width;
  myOptionsButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Options", kOptionsCmd, 'O');
    xpos += space + width;
  myReloadButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Reload", kReloadCmd, 'R');
    xpos += space + width;
  myQuitButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Quit", kQuitCmd, 'Q');
    xpos += space + width;
#else
  myQuitButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Quit", kQuitCmd, 'Q');
    xpos += space + width;
  myOptionsButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Options", kOptionsCmd, 'O');
    xpos += space + width;
  myReloadButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Reload", kReloadCmd, 'R');
    xpos += space + width;
  myStartButton = new ButtonWidget(this, xpos, _h - 24, width, 16, "Start", kStartCmd, 'Q');
    xpos += space + width;
#endif

  // Add list with game titles
  myList = new StringListWidget(this, instance()->font(),
                                10, 24, _w - 20, _h - 24 - 26 - 10 - 10);
  myList->setNumberingMode(kListNumberingOff);
  myList->setEditable(false);
  myList->setFlags(WIDGET_NODRAW_FOCUS);
  addFocusWidget(myList);

  // Add note textwidget to show any notes for the currently selected ROM
  new StaticTextWidget(this, 20, _h - 43, 30, fontHeight, "Note:", kTextAlignLeft);
  myNote = new StaticTextWidget(this, 50, _h - 43, w - 70, fontHeight,
                                "", kTextAlignLeft);

  // Create the launcher options dialog, where you can change ROM
  // and snapshot paths
  myOptions = new LauncherOptionsDialog(osystem, parent, this,
                                        20, 60, _w - 40, _h - 120);

  // Create a game list, which contains all the information about a ROM that
  // the launcher needs
  myGameList = new GameList();
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::enableButtons(bool enable)
{
  myStartButton->setEnabled(enable);
  myOptionsButton->setEnabled(enable);
  myReloadButton->setEnabled(enable);
  myQuitButton->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateListing(bool fullReload)
{
  enableButtons(false);

  // Start with empty list
  myGameList->clear();

  string romdir = instance()->settings().getString("romdir");
  string cacheFile = instance()->cacheFile();

  // If this is the first time using Stella, the romdir won't be set.
  // In that case, display the options dialog, and don't let Stella proceed
  // until the options are set.
  if(romdir == "")
  {
    myOptionsButton->setEnabled(true);
    parent()->addDialog(myOptions);
    return;
  }

  // Figure out if the ROM dir has changed since we last accessed it.
  // If so, we do a full reload from disk (takes quite some time).
  // Otherwise, we can use the cache file (which is much faster).
  string currentModTime = FilesystemNode::modTime(romdir);
  string oldModTime = instance()->settings().getString("modtime");
/*
cerr << "old:     \'" << oldModTime     << "\'\n"
     << "current: \'" << currentModTime << "\'\n"
     << endl;
*/
  if(currentModTime != oldModTime)  // romdir has changed
    loadListFromDisk();
  else if(FilesystemNode::fileExists(cacheFile) && !fullReload)
    loadListFromCache();
  else  // we have no other choice
    loadListFromDisk();

  // Now fill the list widget with the contents of the GameList
  StringList l;
  for (int i = 0; i < (int) myGameList->size(); ++i)
    l.push_back(myGameList->name(i));

  myList->setList(l);

  // Indicate how many files were found
  ostringstream buf;
  buf << myGameList->size() << " files found";
  myRomCount->setLabel(buf.str());

  enableButtons(true);

  // Restore last selection
  if(!myList->getList().isEmpty())
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadListFromDisk()
{
  string romdir = instance()->settings().getString("romdir");
  FilesystemNode dir(romdir);
  FSList files = dir.listDir(FilesystemNode::kListAll);

  // Create a progress dialog box to show the progress of processing
  // the ROMs, since this is usually a time-consuming operation
  ProgressDialog progress(this, instance()->font(),
                          "Loading ROM's from disk ...");
  progress.setRange(0, files.size() - 1, 10);

  // Create a entry for the GameList for each file
  Properties props;
  string path = dir.path(), rom, md5, name, note;
  for (unsigned int idx = 0; idx < files.size(); idx++)
  {
    rom = path + files[idx].displayName();

    // Calculate the MD5 so we can get the rest of the info
    // from the PropertiesSet (stella.pro)
    md5 = MD5FromFile(rom);
    instance()->propSet().getMD5(md5, props);
    name = props.get("Cartridge.Name");
    note = props.get("Cartridge.Note");

    // Indicate that this ROM doesn't have a properties entry
    myGameList->appendGame(rom, name, note);

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
  string line, rom, name, note;
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

    rom  = line.substr(0, pos1);
    name = line.substr(pos1+1, pos2-pos1-1);
    note = line.substr(pos2+1);

    // Add this game to the list
    // We don't do sorting, since it's assumed to be done by loadListFromDisk()
    myGameList->appendGame(rom, name, note);
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
    out << myGameList->rom(i)  << "|"
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
        string s = myList->getSelectedString();

        // Make sure the console creation actually succeeded
        if(instance()->createConsole(myGameList->rom(item)))
        {
          instance()->settings().setString("lastrom", s);
          close();
        }
      }
      break;

    case kOptionsCmd:
      parent()->addDialog(myOptions);
      break;

    case kReloadCmd:
      updateListing(true);  // force a reload from disk
      break;

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
      updateListing();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
