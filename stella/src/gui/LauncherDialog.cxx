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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: LauncherDialog.cxx,v 1.12 2005-05-13 18:28:05 stephena Exp $
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
#include "ListWidget.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "GuiUtils.hxx"
#include "BrowserDialog.hxx"
#include "LauncherOptionsDialog.hxx"
#include "LauncherDialog.hxx"

#include "bspf.hxx"

enum {
  kStartCmd   = 'STRT',
  kOptionsCmd = 'OPTI',
  kReloadCmd  = 'RELO',
  kQuitCmd    = 'QUIT',
	
  kCmdGlobalGraphicsOverride = 'OGFX',
  kCmdGlobalAudioOverride = 'OSFX',
  kCmdGlobalVolumeOverride = 'OVOL',

  kCmdExtraBrowser = 'PEXT',
  kCmdGameBrowser = 'PGME',
  kCmdSaveBrowser = 'PSAV'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myList(NULL),
    myGameList(NULL)
{
  // Show game name
  new StaticTextWidget(this, 10, 8, 200, kLineHeight,
                       "Select a game from the list ...", kTextAlignLeft);

  myRomCount = new StaticTextWidget(this, _w - 100, 8, 90, kLineHeight,
                                    "", kTextAlignRight);

  // Add three buttons at the bottom
  const int border = 10;
  const int space = 8;
  const int buttons = 4;
  const int width = (_w - 2 * border - space * (buttons - 1)) / buttons;
  int xpos = border;

  new ButtonWidget(this, xpos, _h - 24, width, 16, "Play", kStartCmd, 'S');
    xpos += space + width;
  new ButtonWidget(this, xpos, _h - 24, width, 16, "Options", kOptionsCmd, 'O');
    xpos += space + width;
  new ButtonWidget(this, xpos, _h - 24, width, 16, "Reload", kReloadCmd, 'R');
    xpos += space + width;
  new ButtonWidget(this, xpos, _h - 24, width, 16, "Quit", kQuitCmd, 'Q');
    xpos += space + width;

  // Add list with game titles
  myList = new ListWidget(this, 10, 24, _w - 20, _h - 24 - 26 - 10 - 10);
  myList->setEditable(false);
  myList->setNumberingMode(kListNumberingOff);

  // Add note textwidget to show any notes for the currently selected ROM
  new StaticTextWidget(this, 20, _h - 43, 30, 16, "Note:", kTextAlignLeft);
  myNote = new StaticTextWidget(this, 50, _h - 43, w - 70, 16,
                                       "", kTextAlignLeft);

  // Restore last selection
/*
  string last = ConfMan.get(String("lastselectedgame"), ConfigManager::kApplicationDomain);
  if (!last.isEmpty())
  {
    int itemToSelect = 0;
    StringList::const_iterator iter;
    for (iter = _domains.begin(); iter != _domains.end(); ++iter, ++itemToSelect)
    {
      if (last == *iter)
      {
        myList->setSelected(itemToSelect);
        break;
      }
    }
  }
*/

  // Create the launcher options dialog, where you can change ROM
  // and snapshot paths
  myOptions = new LauncherOptionsDialog(osystem, parent, 20, 40, _w - 40, _h - 80);

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
void LauncherDialog::close()
{
  // Save romdir specified by the browser
/*
  FilesystemNode dir(myBrowser->getResult());
  instance()->settings().setString("romdir", dir.path());
*/
/*
  // Save last selection
  const int sel = _list->getSelected();
  if (sel >= 0)
    ConfMan.set(String("lastselectedgame"), _domains[sel], ConfigManager::kApplicationDomain);	
  else
    ConfMan.removeKey(String("lastselectedgame"), ConfigManager::kApplicationDomain);
 
  ConfMan.flushToDisk();	
  Dialog::close();
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateListing(bool fullReload)
{
  // Figure out if the ROM dir has changed since we last accessed it.
  // If so, we do a full reload from disk (takes quite some time).
  // Otherwise, we can use the cache file (which is much faster).
// FIXME - actually implement the following code, where we use
//         a function to detect last modification time, and compare
//         it to the current modification time
/*
  if(ROM_DIR_CHANGED)
    loadListFromDisk();
  else if(CACHE_FILE_EXISTS)
    loadListFromCache();
  else  // we have no other choice
    loadListFromDisk();
*/
  // Start with empty list
  myGameList->clear();

  string cacheFile = instance()->cacheFile();
  if(instance()->fileExists(cacheFile) && !fullReload)
    loadListFromCache();
  else
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadListFromDisk()
{
  Properties props;

  string romdir = instance()->settings().getString("romdir");
  FilesystemNode dir(romdir);
  FSList files = dir.listDir(FilesystemNode::kListAll);

  // Create a entry for the GameList for each file
  string path = dir.path(), rom, md5, name, note;
  for (int idx = 0; idx < (int)files.size(); idx++)
  {
    rom = path + files[idx].displayName();

    // Calculate the MD5 so we can get the rest of the info
    // from the PropertiesSet (stella.pro)
    md5 = MD5FromFile(rom);
    instance()->propSet().getMD5(md5, props);
    name = props.get("Cartridge.Name");
    note = props.get("Cartridge.Note");

    // Some games may not have a name, since there may not
    // be an entry in stella.pro.  In that case, we use the rom name
    if(name == "Untitled")
      name = files[idx].displayName();

    myGameList->appendGame(rom, name, note);
  }

  // Sort the list by rom name (since that's what we see in the listview)
  myGameList->sortByName();

  // And create a cache file, so that the next time Stella starts,
  // we don't have to do this time-consuming operation again
  createListCache();
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
  ifstream in(path.c_str(), ios_base::binary);
  if(!in)
    return "";

  uInt8* image = new uInt8[512 * 1024];
  in.read((char*)image, 512 * 1024);
  int size = in.gcount();
  in.close();

  string md5 = MD5(image, size);
  delete[] image;

  return md5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleCommand(CommandSender* sender, int cmd, int data)
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
        instance()->createConsole(myGameList->rom(item));
        close();
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

    default:
      Dialog::handleCommand(sender, cmd, data);
  }
}
