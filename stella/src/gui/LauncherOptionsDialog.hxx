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
// $Id: LauncherOptionsDialog.hxx,v 1.1 2005-05-13 01:03:27 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef LAUNCHER_OPTIONS_DIALOG_HXX
#define LAUNCHER_OPTIONS_DIALOG_HXX

#include "Dialog.hxx"

class BrowserDialog;
class CheckboxWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;

class LauncherOptionsDialog : public Dialog
{
  public:
    LauncherOptionsDialog(const String &domain, int x, int y, int w, int h);

    void open();
    void close();
    void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data);

  protected:
    BrowserDialog    *myRomBrowser;
    BrowserDialog    *mySnapBrowser;
    StaticTextWidget *_savePath;
    StaticTextWidget *_extraPath;

  protected:
    /** Config domain this dialog is used to edit. */
    string _domain;
	
	int addGraphicControls(GuiObject *boss, int yoffset);
	int addMIDIControls(GuiObject *boss, int yoffset);
	int addVolumeControls(GuiObject *boss, int yoffset);

	void setGraphicSettingsState(bool enabled);
	void setAudioSettingsState(bool enabled);
	void setVolumeSettingsState(bool enabled);

  private:
    //
    // Graphics controls
    //
	bool _enableGraphicSettings;
	PopUpWidget *_gfxPopUp;
	CheckboxWidget *_fullscreenCheckbox;
	CheckboxWidget *_aspectCheckbox;

	//
	// Audio controls
	//
	bool _enableAudioSettings;
	PopUpWidget *_midiPopUp;
	CheckboxWidget *_multiMidiCheckbox;
	CheckboxWidget *_mt32Checkbox;
	CheckboxWidget *_subCheckbox;

	//
	// Volume controls
	//
	bool _enableVolumeSettings;

	SliderWidget *_masterVolumeSlider;
	StaticTextWidget *_masterVolumeLabel;

	SliderWidget *_musicVolumeSlider;
	StaticTextWidget *_musicVolumeLabel;

	SliderWidget *_sfxVolumeSlider;
	StaticTextWidget *_sfxVolumeLabel;

	SliderWidget *_speechVolumeSlider;
	StaticTextWidget *_speechVolumeLabel;
};

#endif
