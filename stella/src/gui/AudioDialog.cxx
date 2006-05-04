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
// $Id: AudioDialog.cxx,v 1.19 2006-05-04 17:45:25 stephena Exp $
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::AudioDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font, int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h)
{
  const int lineHeight = font.getLineHeight(),
            fontHeight = font.getFontHeight();
  int xpos, ypos;
  int lwidth = font.getStringWidth("Fragment Size: "),
      pwidth = font.getStringWidth("4096");
  WidgetArray wid;

  // Volume
  xpos = (w - lwidth - pwidth - 40) / 2;  ypos = 10;

  myVolumeSlider = new SliderWidget(this, font, xpos, ypos, 30, lineHeight,
                                    "Volume: ", lwidth, kVolumeChanged);
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  wid.push_back(myVolumeSlider);
  myVolumeLabel = new StaticTextWidget(this, font,
                                       xpos + myVolumeSlider->getWidth() + 4,
                                       ypos + 1,
                                       15, fontHeight, "", kTextAlignLeft);

  myVolumeLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Fragment size
  myFragsizePopup = new PopUpWidget(this, font, xpos, ypos,
                                    pwidth + myVolumeLabel->getWidth() - 4, lineHeight,
                                    "Fragment size: ", lwidth);
  myFragsizePopup->appendEntry("256",  1);
  myFragsizePopup->appendEntry("512",  2);
  myFragsizePopup->appendEntry("1024", 3);
  myFragsizePopup->appendEntry("2048", 4);
  myFragsizePopup->appendEntry("4096", 5);
  wid.push_back(myFragsizePopup);
  ypos += lineHeight + 4;

  // Stereo sound
  mySoundTypeCheckbox = new CheckboxWidget(this, font, xpos+28, ypos,
                                           "Stereo mode", 0);
  wid.push_back(mySoundTypeCheckbox);
  ypos += lineHeight + 4;

  // Enable sound
  mySoundEnableCheckbox = new CheckboxWidget(this, font, xpos+28, ypos,
                                             "Enable sound", kSoundEnableChanged);
  wid.push_back(mySoundEnableCheckbox);
  ypos += lineHeight + 12;

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = addButton(font, 10, _h - 24, "Defaults", kDefaultsCmd);
  wid.push_back(b);
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
#endif

  addToFocusList(wid);
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
    instance()->console().setChannels(b ? 2 : 1);
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
    instance()->sound().close();
    instance()->sound().initialize();
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
