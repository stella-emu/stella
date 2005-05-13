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
// $Id: LauncherOptionsDialog.cxx,v 1.1 2005-05-13 01:03:27 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "BrowserDialog.hxx"
#include "gui/chooser.h"
//#include "gui/options.h"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"

#include "FSNode.hxx"


OptionsDialog::OptionsDialog(const string& domain, int x, int y, int w, int h)
  : Dialog(x, y, w, h),
    _domain(domain)
{

	const int vBorder = 4;
	int yoffset;

	// The tab widget
	TabWidget *tab = new TabWidget(this, 0, vBorder, _w, _h - 24 - 2 * vBorder);

	// 1) The file locations tab
	//
	tab->addTab("File Locations");
	yoffset = vBorder;

	// ROM path
	new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "ROM Path: ", kChooseSaveDirCmd, 0);
	_savePath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "Enter valid path", kTextAlignLeft);
	yoffset += 18;

	// 2) The snapshot settings tab
	//
	tab->addTab("Snapshot Settings");
	yoffset = vBorder;

	// Save game path
	new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "Save Path: ", kChooseSaveDirCmd, 0);
	_savePath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "/foo/bar", kTextAlignLeft);

	yoffset += 18;

 	new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "Extra Path:", kChooseExtraDirCmd, 0);
	_extraPath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "None", kTextAlignLeft);
	yoffset += 18;
	
/*
	//
	// 3) The miscellaneous tab
	//
	tab->addTab("Misc");
	yoffset = vBorder;

#if !( defined(__DC__) || defined(__GP32__) )
	// Save game path
	new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "Save Path: ", kChooseSaveDirCmd, 0);
	_savePath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "/foo/bar", kTextAlignLeft);

	yoffset += 18;

 	new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "Extra Path:", kChooseExtraDirCmd, 0);
	_extraPath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "None", kTextAlignLeft);
	yoffset += 18;
#endif

	// TODO: joystick setting
*/

	// Activate the first tab
	tab->setActiveTab(0);

	// Add OK & Cancel buttons
	addButton(_w - 2 *(kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
	addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);

	// Create file browser dialog
	_browser = new BrowserDialog("Select directory for savegames");

}

GlobalOptionsDialog::~GlobalOptionsDialog() {
	delete _browser;
}

void OptionsDialog::close()
{
/*
	if (getResult()) {
		if (_fullscreenCheckbox) {
			if (_enableGraphicSettings) {
				ConfMan.set("fullscreen", _fullscreenCheckbox->getState(), _domain);
				ConfMan.set("aspect_ratio", _aspectCheckbox->getState(), _domain);

				if ((int32)_gfxPopUp->getSelectedTag() >= 0)
					ConfMan.set("gfx_mode", _gfxPopUp->getSelectedString(), _domain);
			} else {
				ConfMan.removeKey("fullscreen", _domain);
				ConfMan.removeKey("aspect_ratio", _domain);
				ConfMan.removeKey("gfx_mode", _domain);
			}
		}

		if (_masterVolumeSlider) {
			if (_enableVolumeSettings) {
				ConfMan.set("master_volume", _masterVolumeSlider->getValue(), _domain);
				ConfMan.set("music_volume", _musicVolumeSlider->getValue(), _domain);
				ConfMan.set("sfx_volume", _sfxVolumeSlider->getValue(), _domain);
				ConfMan.set("speech_volume", _speechVolumeSlider->getValue(), _domain);
			} else {
				ConfMan.removeKey("master_volume", _domain);
				ConfMan.removeKey("music_volume", _domain);
				ConfMan.removeKey("sfx_volume", _domain);
				ConfMan.removeKey("speech_volume", _domain);
			}
		}

		if (_multiMidiCheckbox) {
			if (_enableAudioSettings) {
				ConfMan.set("multi_midi", _multiMidiCheckbox->getState(), _domain);
				ConfMan.set("native_mt32", _mt32Checkbox->getState(), _domain);
				ConfMan.set("subtitles", _subCheckbox->getState(), _domain); 
				const MidiDriverDescription *md = MidiDriver::getAvailableMidiDrivers();
				while (md->name && md->id != (int)_midiPopUp->getSelectedTag())
					md++;
				if (md->name)
					ConfMan.set("music_driver", md->name, _domain);
				else
					ConfMan.removeKey("music_driver", _domain);
			} else {
				ConfMan.removeKey("multi_midi", _domain);
				ConfMan.removeKey("native_mt32", _domain);
				ConfMan.removeKey("music_driver", _domain);
				ConfMan.removeKey("subtitles", _domain); 
			}
		}

		// Save config file
		ConfMan.flushToDisk();
	}
*/
	Dialog::close();
}

int OptionsDialog::addGraphicControls(GuiObject *boss, int yoffset) {
	const int x = 10;
	const int w = _w - 2 * 10;
	const OSystem::GraphicsMode *gm = g_system->getSupportedGraphicsModes();

	// The GFX mode popup
	_gfxPopUp = new PopUpWidget(boss, x-5, yoffset, w+5, kLineHeight, "Graphics mode: ", 100);
	yoffset += 16;

	_gfxPopUp->appendEntry("<default>");
	_gfxPopUp->appendEntry("");
	while (gm->name) {
		_gfxPopUp->appendEntry(gm->name, gm->id);
		gm++;
	}

	// Fullscreen checkbox
	_fullscreenCheckbox = new CheckboxWidget(boss, x, yoffset, w, 16, "Fullscreen mode");
	yoffset += 16;

	// Aspect ratio checkbox
	_aspectCheckbox = new CheckboxWidget(boss, x, yoffset, w, 16, "Aspect ratio correction");
	yoffset += 16;

#ifdef _WIN32_WCE
	_fullscreenCheckbox->setState(TRUE);
	_fullscreenCheckbox->setEnabled(FALSE);
	_aspectCheckbox->setEnabled(FALSE);	
#endif
	
	_enableGraphicSettings = true;

	return yoffset;
}

void GlobalOptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
/*
	case kChooseSaveDirCmd:
		if (_browser->runModal() > 0) {
			// User made his choice...
			FilesystemNode dir(_browser->getResult());
			_savePath->setLabel(dir.path());
			// TODO - we should check if the directory is writeable before accepting it
		}
		break;
	case kChooseExtraDirCmd:
		if (_browser->runModal() > 0) {
			// User made his choice...
			FilesystemNode dir(_browser->getResult());
			_extraPath->setLabel(dir.path());
		}
		break;
*/
	default:
		OptionsDialog::handleCommand(sender, cmd, data);
	}
}
