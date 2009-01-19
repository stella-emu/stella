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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: AudioDialog.cxx,v 1.32 2009-01-19 21:19:59 stephena Exp $
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
#include "StringList.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "Widget.hxx"

#include "AudioDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::AudioDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font)
  : Dialog(osystem, parent, 0, 0, 0, 0)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  int lwidth = font.getStringWidth("Fragment Size: "),
      pwidth = font.getStringWidth("4096");
  WidgetArray wid;
  StringMap items;

  // Set real dimensions
  _w = 35 * fontWidth + 10;
  _h = 8 * (lineHeight + 4) + 10;

  // Volume
  xpos = 3 * fontWidth;  ypos = 10;

  myVolumeSlider = new SliderWidget(this, font, xpos, ypos, 6*fontWidth, lineHeight,
                                    "Volume: ", lwidth, kVolumeChanged);
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  wid.push_back(myVolumeSlider);
  myVolumeLabel = new StaticTextWidget(this, font,
                                       xpos + myVolumeSlider->getWidth() + 4,
                                       ypos + 1,
                                       3*fontWidth, fontHeight, "", kTextAlignLeft);

  myVolumeLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Fragment size
  items.clear();
  items.push_back("128", "128");
  items.push_back("256", "256");
  items.push_back("512", "512");
  items.push_back("1024", "1024");
  items.push_back("2048", "2048");
  items.push_back("4096", "4096");
  myFragsizePopup = new PopUpWidget(this, font, xpos, ypos,
                                    pwidth + myVolumeLabel->getWidth() - 4, lineHeight,
                                    items, "Fragment size: ", lwidth);
  wid.push_back(myFragsizePopup);
  ypos += lineHeight + 4;

  // Output frequency
  items.clear();
  items.push_back("11025", "11025");
  items.push_back("22050", "22050");
  items.push_back("31400", "31400");
  items.push_back("44100", "44100");
  items.push_back("48000", "48000");
  myFreqPopup = new PopUpWidget(this, font, xpos, ypos,
                                pwidth + myVolumeLabel->getWidth() - 4, lineHeight,
                                items, "Output freq: ", lwidth);
  wid.push_back(myFreqPopup);
  ypos += lineHeight + 4;

  // TIA frequency
  // ... use same items as above
  myTiaFreqPopup = new PopUpWidget(this, font, xpos, ypos,
                                   pwidth + myVolumeLabel->getWidth() - 4, lineHeight,
                                   items, "TIA freq: ", lwidth);
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
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::~AudioDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::loadConfig()
{
  // Volume
  myVolumeSlider->setValue(instance().settings().getInt("volume"));
  myVolumeLabel->setLabel(instance().settings().getString("volume"));

  // Fragsize
  myFragsizePopup->setSelected(instance().settings().getString("fragsize"), "512");

  // Output frequency
  myFreqPopup->setSelected(instance().settings().getString("freq"), "31400");

  // TIA frequency
  myTiaFreqPopup->setSelected(instance().settings().getString("tiafreq"), "31400");

  // Clip volume
  myClipVolumeCheckbox->setState(instance().settings().getBool("clipvol"));

  // Enable sound
  bool b = instance().settings().getBool("sound");
  mySoundEnableCheckbox->setState(b);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // Volume
  settings.setInt("volume", myVolumeSlider->getValue());
  instance().sound().setVolume(myVolumeSlider->getValue());

  // Fragsize
  settings.setString("fragsize", myFragsizePopup->getSelectedTag());

  // Output frequency
  settings.setString("freq", myFreqPopup->getSelectedTag());

  // TIA frequency
  settings.setString("tiafreq", myTiaFreqPopup->getSelectedTag());

  // Enable/disable volume clipping (requires a restart to take effect)
  settings.setBool("clipvol", myClipVolumeCheckbox->getState());

  // Enable/disable sound (requires a restart to take effect)
  instance().sound().setEnabled(mySoundEnableCheckbox->getState());

  // Only force a re-initialization when necessary, since it can
  // be a time-consuming operation
  if(&instance().console())
    instance().console().initializeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::setDefaults()
{
  myVolumeSlider->setValue(100);
  myVolumeLabel->setLabel("100");

  myFragsizePopup->setSelected("512", "");
  myFreqPopup->setSelected("31400", "");
  myTiaFreqPopup->setSelected("31400", "");

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
