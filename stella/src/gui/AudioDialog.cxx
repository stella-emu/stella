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
// $Id: AudioDialog.cxx,v 1.26 2008-02-06 13:45:23 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "Console.hxx"
#include "Control.hxx"
#include "Dialog.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "Widget.hxx"

#include "AudioDialog.hxx"

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
  myFragsizePopup->appendEntry("128",  1);
  myFragsizePopup->appendEntry("256",  2);
  myFragsizePopup->appendEntry("512",  3);
  myFragsizePopup->appendEntry("1024", 4);
  myFragsizePopup->appendEntry("2048", 5);
  myFragsizePopup->appendEntry("4096", 6);
  wid.push_back(myFragsizePopup);
  ypos += lineHeight + 4;

  // Output frequency
  myFreqPopup = new PopUpWidget(this, font, xpos, ypos,
                                pwidth + myVolumeLabel->getWidth() - 4, lineHeight,
                                "Output freq: ", lwidth);
  myFreqPopup->appendEntry("11025", 1);
  myFreqPopup->appendEntry("22050", 2);
  myFreqPopup->appendEntry("31400", 3);
  myFreqPopup->appendEntry("44100", 4);
  myFreqPopup->appendEntry("48000", 5);
  wid.push_back(myFreqPopup);
  ypos += lineHeight + 4;

  // TIA frequency
  myTiaFreqPopup = new PopUpWidget(this, font, xpos, ypos,
                                   pwidth + myVolumeLabel->getWidth() - 4, lineHeight,
                                   "TIA freq: ", lwidth);
  myTiaFreqPopup->appendEntry("11025", 1);
  myTiaFreqPopup->appendEntry("22050", 2);
  myTiaFreqPopup->appendEntry("31400", 3);
  myTiaFreqPopup->appendEntry("44100", 4);
  myTiaFreqPopup->appendEntry("48000", 5);
  wid.push_back(myTiaFreqPopup);
  ypos += lineHeight + 4;

  // Clip volume
  myClipVolumeCheckbox = new CheckboxWidget(this, font, xpos+28, ypos,
                                            "Clip volume", 0);
  wid.push_back(myClipVolumeCheckbox);
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
  addOKWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
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
  if(i == 128)       i = 1;
  else if(i == 256)  i = 2;
  else if(i == 512)  i = 3;
  else if(i == 1024) i = 4;
  else if(i == 2048) i = 5;
  else if(i == 4096) i = 6;
  myFragsizePopup->setSelectedTag(i);

  // Output frequency
  i = instance()->settings().getInt("freq");
  if(i == 11025)      i = 1;
  else if(i == 22050) i = 2;
  else if(i == 31400) i = 3;
  else if(i == 44100) i = 4;
  else if(i == 48000) i = 5;
  else i = 3;  // default to '31400'
  myFreqPopup->setSelectedTag(i);

  // TIA frequency
  i = instance()->settings().getInt("tiafreq");
  if(i == 11025)      i = 1;
  else if(i == 22050) i = 2;
  else if(i == 31400) i = 3;
  else if(i == 44100) i = 4;
  else if(i == 48000) i = 5;
  else i = 3;  // default to '31400'
  myTiaFreqPopup->setSelectedTag(i);

  // Clip volume
  b = instance()->settings().getBool("clipvol");
  myClipVolumeCheckbox->setState(b);

  // Enable sound
  b = instance()->settings().getBool("sound");
  mySoundEnableCheckbox->setState(b);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::saveConfig()
{
  Settings& settings = instance()->settings();
  string s;
  bool b;
  int i;

  // Volume
  i = myVolumeSlider->getValue();
  instance()->sound().setVolume(i);

  // Fragsize
  s = myFragsizePopup->getSelectedString();
  settings.setString("fragsize", s);

  // Output frequency
  s = myFreqPopup->getSelectedString();
  settings.setString("freq", s);

  // TIA frequency
  s = myTiaFreqPopup->getSelectedString();
  settings.setString("tiafreq", s);

  // Enable/disable volume clipping (requires a restart to take effect)
  b = myClipVolumeCheckbox->getState();
  settings.setBool("clipvol", b);

  // Enable/disable sound (requires a restart to take effect)
  b = mySoundEnableCheckbox->getState();
  instance()->sound().setEnabled(b);

  // Only force a re-initialization when necessary, since it can
  // be a time-consuming operation
  if(&instance()->console())
    instance()->console().initializeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::setDefaults()
{
  myVolumeSlider->setValue(100);
  myVolumeLabel->setLabel("100");

#ifdef WIN32
  myFragsizePopup->setSelectedTag(5);
#else
  myFragsizePopup->setSelectedTag(3);
#endif
  myFreqPopup->setSelectedTag(3);
  myTiaFreqPopup->setSelectedTag(3);

  myClipVolumeCheckbox->setState(true);
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
  myFreqPopup->setEnabled(active);
  myTiaFreqPopup->setEnabled(active);
  myClipVolumeCheckbox->setEnabled(active);
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
