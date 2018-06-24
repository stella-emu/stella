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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "Console.hxx"
#include "Control.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "Widget.hxx"
#include "AudioSettings.hxx"

#include "AudioDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioDialog::AudioDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font)
  : Dialog(osystem, parent, font, "Audio settings")
{
  const int VBORDER = 10;
  const int HBORDER = 10;
  const int INDENT = 20;
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight();
  int xpos, ypos;
  int lwidth = font.getStringWidth("Resampling quality "),
      pwidth = font.getStringWidth("512 bytes");
  WidgetArray wid;
  VariantList items;

  // Set real dimensions
  _w = 45 * fontWidth + HBORDER * 2;
  _h = 11 * (lineHeight + 4) + VBORDER + _th;

  xpos = HBORDER;  ypos = VBORDER + _th;

  // Enable sound
  xpos = HBORDER;
  mySoundEnableCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                             "Enable sound", kSoundEnableChanged);
  wid.push_back(mySoundEnableCheckbox);
  ypos += lineHeight + 4;
  xpos += INDENT;

  // Volume
  myVolumeSlider = new SliderWidget(this, font, xpos, ypos,
                                    "Volume   ", 0, 0, 4 * fontWidth, "%");
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  wid.push_back(myVolumeSlider);
  ypos += lineHeight + 4;

  //
  VarList::push_back(items, "Low quality, medium lag", static_cast<int>(AudioSettings::Preset::lowQualityMediumLag));
  VarList::push_back(items, "High quality, medium lag", static_cast<int>(AudioSettings::Preset::highQualityMediumLag));
  VarList::push_back(items, "High quality, low lag", static_cast<int>(AudioSettings::Preset::highQualityLowLag));
  VarList::push_back(items, "Ultra quality, minimal lag", static_cast<int>(AudioSettings::Preset::veryHighQualityVeryLowLag));
  VarList::push_back(items, "Custom", static_cast<int>(AudioSettings::Preset::custom));
  myModePopup = new PopUpWidget(this, font, xpos, ypos,
                                   font.getStringWidth("Ultry quality, minimal lag "), lineHeight,
                                   items, "Mode (*) ", 0, kModeChanged);
  wid.push_back(myModePopup);
  ypos += lineHeight + 4;
  xpos += INDENT;

  // Fragment size
  items.clear();
  VarList::push_back(items, "128 bytes", 128);
  VarList::push_back(items, "256 bytes", 256);
  VarList::push_back(items, "512 bytes", 512);
  VarList::push_back(items, "1 KB", 1024);
  VarList::push_back(items, "2 KB", 2048);
  VarList::push_back(items, "4 KB", 4096);
  myFragsizePopup = new PopUpWidget(this, font, xpos, ypos,
                                    pwidth, lineHeight,
                                    items, "Fragment size (*) ", lwidth);
  wid.push_back(myFragsizePopup);
  ypos += lineHeight + 4;

  // Output frequency
  items.clear();
  VarList::push_back(items, "44100 Hz", 44100);
  VarList::push_back(items, "48000 Hz", 48000);
  VarList::push_back(items, "96000 Hz", 96000);
  myFreqPopup = new PopUpWidget(this, font, xpos, ypos,
                                pwidth, lineHeight,
                                items, "Sample rate (*) ", lwidth);
  wid.push_back(myFreqPopup);
  ypos += lineHeight + 4;

  // Resampling quality
  items.clear();
  VarList::push_back(items, "Low", static_cast<int>(AudioSettings::ResamplingQuality::nearestNeightbour));
  VarList::push_back(items, "High", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_2));
  VarList::push_back(items, "Ultra", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_3));
  myResamplingPopup = new PopUpWidget(this, font, xpos, ypos,
                                pwidth, lineHeight,
                                items, "Resampling quality ", lwidth);
  wid.push_back(myResamplingPopup);
  ypos += lineHeight + 4;

  // Param 1
  myHeadroomSlider = new SliderWidget(this, font, xpos, ypos,
                                      "Headroom    ", 0, 0, 2 * fontWidth);
  myHeadroomSlider->setMinValue(1); myHeadroomSlider->setMaxValue(AudioSettings::MAX_HEADROOM);
  wid.push_back(myHeadroomSlider);
  ypos += lineHeight + 4;

  // Param 2
  myBufferSizeSlider = new SliderWidget(this, font, xpos, ypos,
                                      "Buffer size ", 0, 0, 2 * fontWidth);
  myBufferSizeSlider->setMinValue(1); myBufferSizeSlider->setMaxValue(AudioSettings::MAX_BUFFER_SIZE);
  wid.push_back(myBufferSizeSlider);

  // Add message concerning usage
  ypos = _h - fontHeight * 2 - 24;
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  new StaticTextWidget(this, infofont, HBORDER, ypos, "(*) Requires application restart");/* ,
        font.getStringWidth("(*) Requires application restart"), fontHeight,
        "(*) Requires application restart", TextAlign::Left);*/

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::loadConfig()
{
  AudioSettings& audioSettings = instance().audioSettings();

  // Volume
  myVolumeSlider->setValue(audioSettings.volume());

  // Enable sound
  mySoundEnableCheckbox->setState(audioSettings.enabled());

  // Preset / mode
  myModePopup->setSelected(static_cast<int>(audioSettings.preset()));

  updatePresetSettings(instance().audioSettings());

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(audioSettings.enabled());
  handleModeChange(audioSettings.enabled());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::updatePresetSettings(AudioSettings& audioSettings)
{
  // Fragsize
  myFragsizePopup->setSelected(audioSettings.fragmentSize());

  // Output frequency
  myFreqPopup->setSelected(audioSettings.sampleRate());

  // Headroom
  myHeadroomSlider->setValue(audioSettings.headroom());

  // Buffer size
  myBufferSizeSlider->setValue(audioSettings.bufferSize());

  // Resampling quality
  myResamplingPopup->setSelected(static_cast<int>(audioSettings.resamplingQuality()));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::saveConfig()
{
  AudioSettings audioSettings = instance().audioSettings();

  // Volume
  audioSettings.setVolume(myVolumeSlider->getValue());
  instance().sound().setVolume(myVolumeSlider->getValue());

  audioSettings.setEnabled(mySoundEnableCheckbox->getState());
  instance().sound().setEnabled(mySoundEnableCheckbox->getState());

  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());
  audioSettings.setPreset(preset);

  if (preset == AudioSettings::Preset::custom) {

  // Fragsize
    audioSettings.setFragmentSize(myFragsizePopup->getSelectedTag().toInt());
    audioSettings.setSampleRate(myFreqPopup->getSelectedTag().toInt());
    audioSettings.setHeadroom(myHeadroomSlider->getValue());
    audioSettings.setBufferSize(myBufferSizeSlider->getValue());
    audioSettings.setResamplingQuality(static_cast<AudioSettings::ResamplingQuality>(myResamplingPopup->getSelectedTag().toInt()));
  }

  // Only force a re-initialization when necessary, since it can
  // be a time-consuming operation
  if(instance().hasConsole())
    instance().console().initializeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::setDefaults()
{
  myVolumeSlider->setValue(100);

  myFragsizePopup->setSelected("512", "");
  myFreqPopup->setSelected("31400", "");

  mySoundEnableCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleSoundEnableChange(true);

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleSoundEnableChange(bool active)
{
  myVolumeSlider->setEnabled(active);
  myModePopup->setEnabled(active);
  handleModeChange(active);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleModeChange(bool active)
{
  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());

  AudioSettings audioSettings = instance().audioSettings();
  audioSettings.setPersistent(false);
  audioSettings.setPreset(preset);

  (cout << "Preset: " << static_cast<int>(preset) << std::endl).flush();

  updatePresetSettings(audioSettings);

  bool userMode = active && preset == AudioSettings::Preset::custom;

  myFragsizePopup->setEnabled(userMode);
  myFreqPopup->setEnabled(userMode);
  myResamplingPopup->setEnabled(userMode);
  myHeadroomSlider->setEnabled(userMode);
  myBufferSizeSlider->setEnabled(userMode);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kSoundEnableChanged:
      handleSoundEnableChange(data == 1);
      break;

    case kModeChanged:
      handleModeChange(true);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
