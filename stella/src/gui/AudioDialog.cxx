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
// $Id: AudioDialog.cxx,v 1.5 2005-05-12 18:45:21 stephena Exp $
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
                         uInt16 x, uInt16 y, uInt16 w, uInt16 h)
    : Dialog(osystem, parent, x, y, w, h)
{
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

  // Enable sound
  new StaticTextWidget(this, xoff+8, yoff+3, 20, kLineHeight, "", kTextAlignLeft);
  mySoundEnableCheckbox = new CheckboxWidget(this, xoff+28, yoff, woff - 14, kLineHeight,
                                             "Enable sound", kSoundEnableChanged);
  yoff += kAudioRowHeight + 12;

  // Add a short message about options that need a restart
//  new StaticTextWidget(this, xoff+30, yoff, 170, kLineHeight,
//                       "* Note that these options take effect", kTextAlignLeft);
//  yoff += kAudioRowHeight;
//  new StaticTextWidget(this, xoff+30, yoff, 170, kLineHeight,
//                       "the next time you restart Stella.", kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  addButton( 10, _h - 24, "Defaults", kDefaultsCmd, 0);
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
#endif

  setDefaults();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::~AudioDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::loadConfig()
{
  bool b;
  uInt32 i;

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
  uInt32 i;
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

// FIXME - get defaults from OSystem or Settings
#ifdef WIN32
  myFragsizePopup->setSelectedTag(4);
#else
  myFragsizePopup->setSelectedTag(2);
#endif

  mySoundEnableCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(true);

  instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleSoundEnableChange(bool active)
{
  myVolumeSlider->setEnabled(active);
  myVolumeLabel->setEnabled(active);
  myFragsizePopup->setEnabled(active);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
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
      Dialog::handleCommand(sender, cmd, data);
      break;
  }
}
