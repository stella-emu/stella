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
// $Id: LauncherDialog.cxx,v 1.2 2005-05-06 22:50:15 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ListWidget.hxx"
#include "Dialog.hxx"
#include "GuiUtils.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "BrowserDialog.hxx"
#include "LauncherDialog.hxx"

#include "bspf.hxx"

enum {
  kStartCmd = 'STRT',
  kLocationCmd = 'LOCA',
  kReloadCmd = 'RELO',
  kQuitCmd = 'QUIT',
	
  kCmdGlobalGraphicsOverride = 'OGFX',
  kCmdGlobalAudioOverride = 'OSFX',
  kCmdGlobalVolumeOverride = 'OVOL',

  kCmdExtraBrowser = 'PEXT',
  kCmdGameBrowser = 'PGME',
  kCmdSaveBrowser = 'PSAV'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem* osystem, uInt16 x, uInt16 y,
                                       uInt16 w, uInt16 h)
    : Dialog(osystem, x, y, w, h)
{
  // Show game name
  new StaticTextWidget(this, 10, 8, _w - 20, kLineHeight,
                       "Select a game from the list ...", kTextAlignCenter);

  // Add three buttons at the bottom
  const int border = 10;
  const int space = 8;
  const int buttons = 4;
  const int width = (_w - 2 * border - space * (buttons - 1)) / buttons;
  int xpos = border;

  new ButtonWidget(this, xpos, _h - 24, width, 16, "Play", kStartCmd, 'S');
    xpos += space + width;
  new ButtonWidget(this, xpos, _h - 24, width, 16, "Options", kLocationCmd, 'O');
    xpos += space + width;
  new ButtonWidget(this, xpos, _h - 24, width, 16, "Reload", kReloadCmd, 'R');
    xpos += space + width;
  new ButtonWidget(this, xpos, _h - 24, width, 16, "Quit", kQuitCmd, 'Q');
    xpos += space + width;

  // Add list with game titles
  myList = new ListWidget(this, 10, 24, _w - 20, _h - 24 - 26 - 10);
  myList->setEditable(false);
  myList->setNumberingMode(kListNumberingOff);

  // Populate the list
  updateListing();

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
  // En-/Disable the buttons depending on the list selection
  updateButtons();

  // Create file browser dialog
//FIXME  myBrowser = new BrowserDialog("Select directory with game data");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::~LauncherDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::close()
{
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
void LauncherDialog::updateListing()
{
// FIXME - add bulk of KStella code here wrt loading from stella.cache
cerr << "LauncherDialog::updateListing()\n";
/*
	Common::StringList l;

	// Retrieve a list of all games defined in the config file
	_domains.clear();
	const ConfigManager::DomainMap &domains = ConfMan.getGameDomains();
	ConfigManager::DomainMap::const_iterator iter = domains.begin();
	for (iter = domains.begin(); iter != domains.end(); ++iter) {
		String name(iter->_value.get("gameid"));
		String description(iter->_value.get("description"));

		if (name.isEmpty())
			name = iter->_key;
		if (description.isEmpty()) {
			GameSettings g = GameDetector::findGame(name);
			if (g.description)
				description = g.description;
		}

		if (!name.isEmpty() && !description.isEmpty()) {
			// Insert the game into the launcher list
			int pos = 0, size = l.size();

			while (pos < size && (scumm_stricmp(description.c_str(), l[pos].c_str()) > 0))
				pos++;
			l.insert_at(pos, description);
			_domains.insert_at(pos, iter->_key);
		}
	}

	_list->setList(l);
	updateButtons();
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::reloadListing()
{
// FIXME - add bulk of KStella code here wrt loading from disk
cerr << "LauncherDialog::reloadListing()\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::addGame()
{
/*
	// Allow user to add a new game to the list.
	// 1) show a dir selection dialog which lets the user pick the directory
	//    the game data resides in.
	// 2) try to auto detect which game is in the directory, if we cannot
	//    determine it uniquely preent a list of candidates to the user 
	//    to pick from
	// 3) Display the 'Edit' dialog for that item, letting the user specify
	//    an alternate description (to distinguish multiple versions of the
	//    game, e.g. 'Monkey German' and 'Monkey English') and set default
	//    options for that game.

	if (_browser->runModal() > 0) {
		// User made his choice...
		FilesystemNode dir(_browser->getResult());
		FSList files = dir.listDir(FilesystemNode::kListAll);

		// ...so let's determine a list of candidates, games that
		// could be contained in the specified directory.
		DetectedGameList candidates(PluginManager::instance().detectGames(files));
		
		int idx;
		if (candidates.isEmpty()) {
			// No game was found in the specified directory
			MessageDialog alert("ScummVM could not find any game in the specified directory!");
			alert.runModal();
			idx = -1;
		} else if (candidates.size() == 1) {
			// Exact match
			idx = 0;
		} else {
			// Display the candidates to the user and let her/him pick one
			StringList list;
			for (idx = 0; idx < (int)candidates.size(); idx++)
				list.push_back(candidates[idx].description);
			
			ChooserDialog dialog("Pick the game:");
			dialog.setList(list);
			idx = dialog.runModal();
		}
		if (0 <= idx && idx < (int)candidates.size()) {
			DetectedGame result = candidates[idx];

			// The auto detector or the user made a choice.
			// Pick a domain name which does not yet exist (after all, we
			// are *adding* a game to the config, not replacing).
			String domain(result.name);
			if (ConfMan.hasGameDomain(domain)) {
				char suffix = 'a';
				domain += suffix;
				while (ConfMan.hasGameDomain(domain)) {
					assert(suffix < 'z');
					domain.deleteLastChar();
					suffix++;
					domain += suffix;
				}
				ConfMan.set("gameid", result.name, domain);
				ConfMan.set("description", result.description, domain);
			}
			ConfMan.set("path", dir.path(), domain);
			
			const bool customLanguage = (result.language != Common::UNK_LANG);
			const bool customPlatform = (result.platform != Common::kPlatformUnknown);

			// Set language if specified
			if (customLanguage)
				ConfMan.set("language", Common::getLanguageCode(result.language), domain);

			// Set platform if specified
			if (customPlatform)
				ConfMan.set("platform", Common::getPlatformCode(result.platform), domain);

			// Adapt the description string if custom platform/language is set
			if (customLanguage || customPlatform) {
				String desc = result.description;
				desc += " (";
				if (customLanguage)
					desc += Common::getLanguageDescription(result.language);
				if (customLanguage && customPlatform)
					desc += "/";
				if (customPlatform)
					desc += Common::getPlatformDescription(result.platform);
				desc += ")";

				ConfMan.set("description", desc, domain);
			}

			// Display edit dialog for the new entry
			EditGameDialog editDialog(domain, result);
			if (editDialog.runModal() > 0) {
				// User pressed OK, so make changes permanent

				// Write config to disk
				ConfMan.flushToDisk();

				// Update the ListWidget and force a redraw
				updateListing();
				draw();
			} else {
				// User aborted, remove the the new domain again
				ConfMan.removeGameDomain(domain);
			}
		}
	}
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
{
  Int32 item = myList->getSelected();

  switch (cmd)
  {
    case kStartCmd:
    case kListItemActivatedCmd:
    case kListItemDoubleClickedCmd:
      cerr << "Game selected: " << item << endl;
      // FIXME - start a new console based on the filename selected
      //         this is only here for testing
      instance()->createConsole("frostbite.a26");
      close();
      break;

    case kLocationCmd:
      cerr << "kLocationCmd from LauncherDialog\n";
//      instance()->launcher().addDialog(myLocationDialog);
      break;

    case kReloadCmd:
      reloadListing();
      break;

    case kListSelectionChangedCmd:
      updateButtons();
      break;

    case kQuitCmd:
      close();
      instance()->eventHandler().quit();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateButtons()
{
/*
  bool enable = (myList->getSelected() >= 0);
	if (enable != _startButton->isEnabled()) {
		_startButton->setEnabled(enable);
		_startButton->draw();
	}
*/
}
