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
// $Id: AudioDialog.cxx,v 1.14 2005-09-06 19:42:35 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "Menu.hxx"
#include "Control.hxx"
#include "Widget.hxx"
#include "PopUpWidget.hxx"
#include "Dialog.hxx"
#include "AudioDialog.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

enum {
  kAudioRowHeight = 12,
  kAudioWidth     = 200,
  kAudioHeight    = 100
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::AudioDialog(OSystem* osystem, DialogContainer* parent,
                         int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h)
{
  const GUI::Font& font = instance()->font();
  int yoff = 10,
      xoff = 30,
      woff = _w - 80,
      labelWidth = 80;

  // Volume
  myVolumeSlider = new SliderWidget(this, xoff, yoff, woff - 14, kLineHeight,
                                    "Volume: ", labelWidth, kVolumeChanged);
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  myVolumeLabel = new StaticTextWidget(this, xoff + woff - 11, yoff, 24, kLineHeight,
                                       "", kTextAlignLeft);
  myVolumeLabel->setFlags(WIDGET_CLEARBG);
  yoff += kAudioRowHeight + 4;

  // Fragment size
  myFragsizePopup = new PopUpWidget(this, xoff, yoff, woff, kLineHeight,
                                    "Fragment size: ", labelWidth);
  myFragsizePopup->appendEntry("256",  1);
  myFragsizePopup->appendEntry("512",  2);
  myFragsizePopup->appendEntry("1024", 3);
  myFragsizePopup->appendEntry("2048", 4);
  myFragsizePopup->appendEntry("4096", 5);
  yoff += kAudioRowHeight + 4;

  // Stereo sound
  mySoundTypeCheckbox = new CheckboxWidget(this, font, xoff+28, yoff,
                                           "Stereo mode", 0);
  yoff += kAudioRowHeight + 4;

  // Enable sound
  mySoundEnableCheckbox = new CheckboxWidget(this, font, xoff+28, yoff,
                                             "Enable sound", kSoundEnableChanged);
  yoff += kAudioRowHeight + 12;

  // Add Defaults, OK and Cancel buttons
  addButton( 10, _h - 24, "Defaults", kDefaultsCmd, 0);
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::~AudioDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::loadConfig()
{
  bool b;
  int i;

  // Volume
  myVolumeSlider->setValue(instance()->settings().getInt("volume"));
  myVolumeLabel->setLabel(instance()->settings().getString("volume"));

  // Fragsize
  i = instance()->settings().getInt("fragsize");
  if(i == 256)       i = 1;
  else if(i == 512)  i = 2;
  else if(i == 1024) i = 3;
  else if(i == 2048) i = 4;
  else if(i == 4096) i = 5;
  myFragsizePopup->setSelectedTag(i);

  // Stereo mode
  i = instance()->settings().getInt("channels");
  mySoundTypeCheckbox->setState(i == 2);

  // Enable sound
  b = instance()->settings().getBool("sound");
  mySoundEnableCheckbox->setState(b);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::saveConfig()
{
  string s;
  int i;
  bool b, restart = false;

  // Volume
  i = myVolumeSlider->getValue();
  instance()->sound().setVolume(i);

  // Fragsize (requires a restart to take effect)
  i = 1;
  i <<= (myFragsizePopup->getSelectedTag() + 7);
  if(instance()->settings().getInt("fragsize") != i)
  {
    instance()->settings().setInt("fragsize", i);
    restart = true;
  }

  // Enable/disable stereo sound (requires a restart to take effect)
  b = mySoundTypeCheckbox->getState();
  if((instance()->settings().getInt("channels") == 2) != b)
  {
    instance()->sound().setChannels(b ? 2 : 1);
    restart = true;
  }

  // Enable/disable sound (requires a restart to take effect)
  b = mySoundEnableCheckbox->getState();
  if(instance()->settings().getBool("sound") != b)
  {
    instance()->sound().setEnabled(b);
    restart = true;
  }

  // Only force a re-initialization when necessary, since it can
  // be a time-consuming operation
  if(restart)
  {
    instance()->sound().initialize(true);
    instance()->sound().mute(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::setDefaults()
{
  myVolumeSlider->setValue(100);
  myVolumeLabel->setLabel("100");

#ifdef WIN32
  myFragsizePopup->setSelectedTag(4);
#else
  myFragsizePopup->setSelectedTag(2);
#endif

  mySoundTypeCheckbox->setState(false);
  mySoundEnableCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(true);

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleSoundEnableChange(bool active)
{
  myVolumeSlider->setEnabled(active);
  myVolumeLabel->setEnabled(active);
  myFragsizePopup->setEnabled(active);
  mySoundTypeCheckbox->setEnabled(active);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    case kVolumeChanged:
      myVolumeLabel->setValue(myVolumeSlider->getValue());
      break;

    case kSoundEnableChanged:
      handleSoundEnableChange(data == 1);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
