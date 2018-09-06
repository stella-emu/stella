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
  const int VGAP = 4;
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth();
  int xpos, ypos;
  int lwidth = font.getStringWidth("Volume "),
    pwidth;

  WidgetArray wid;
  VariantList items;

  // Set real dimensions
  _w = 46 * fontWidth + HBORDER * 2;
  _h = 11 * (lineHeight + VGAP) + VBORDER + _th;

  xpos = HBORDER;  ypos = VBORDER + _th;

  // Enable sound
  mySoundEnableCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                             "Enable sound", kSoundEnableChanged);
  wid.push_back(mySoundEnableCheckbox);
  ypos += lineHeight + VGAP;
  xpos += INDENT;

  // Volume
  myVolumeSlider = new SliderWidget(this, font, xpos, ypos,
                                    "Volume", lwidth, 0, 4 * fontWidth, "%");
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  myVolumeSlider->setTickmarkInterval(4);
  wid.push_back(myVolumeSlider);
  ypos += lineHeight + VGAP;

  // Stereo sound
  VarList::push_back(items, "By ROM", "BYROM");
  VarList::push_back(items, "Off", "MONO");
  VarList::push_back(items, "On", "STEREO");
  pwidth = font.getStringWidth("By ROM");

  myStereoSoundPopup = new PopUpWidget(this, font, xpos, ypos,
                                       pwidth, lineHeight,
                                       items, "Stereo", lwidth);
  wid.push_back(myStereoSoundPopup);
  ypos += lineHeight + VGAP;

  // Mode
  items.clear();
  VarList::push_back(items, "Low quality, medium lag", static_cast<int>(AudioSettings::Preset::lowQualityMediumLag));
  VarList::push_back(items, "High quality, medium lag", static_cast<int>(AudioSettings::Preset::highQualityMediumLag));
  VarList::push_back(items, "High quality, low lag", static_cast<int>(AudioSettings::Preset::highQualityLowLag));
  VarList::push_back(items, "Ultra quality, minimal lag", static_cast<int>(AudioSettings::Preset::veryHighQualityVeryLowLag));
  VarList::push_back(items, "Custom", static_cast<int>(AudioSettings::Preset::custom));
  myModePopup = new PopUpWidget(this, font, xpos, ypos,
                                   font.getStringWidth("Ultry quality, minimal lag"), lineHeight,
                                   items, "Mode", lwidth, kModeChanged);
  wid.push_back(myModePopup);
  ypos += lineHeight + VGAP;
  xpos += INDENT;

  // Fragment size
  pwidth = font.getStringWidth("512 bytes");
  lwidth = font.getStringWidth("Resampling quality ");
  items.clear();
  VarList::push_back(items, "128 bytes", 128);
  VarList::push_back(items, "256 bytes", 256);
  VarList::push_back(items, "512 bytes", 512);
  VarList::push_back(items, "1 KB", 1024);
  VarList::push_back(items, "2 KB", 2048);
  VarList::push_back(items, "4 KB", 4096);
  myFragsizePopup = new PopUpWidget(this, font, xpos, ypos,
                                    pwidth, lineHeight,
                                    items, "Fragment size", lwidth);
  wid.push_back(myFragsizePopup);
  ypos += lineHeight + VGAP;

  // Output frequency
  items.clear();
  VarList::push_back(items, "44100 Hz", 44100);
  VarList::push_back(items, "48000 Hz", 48000);
  VarList::push_back(items, "96000 Hz", 96000);
  myFreqPopup = new PopUpWidget(this, font, xpos, ypos,
                                pwidth, lineHeight,
                                items, "Sample rate", lwidth);
  wid.push_back(myFreqPopup);
  ypos += lineHeight + VGAP;

  // Resampling quality
  items.clear();
  VarList::push_back(items, "Low", static_cast<int>(AudioSettings::ResamplingQuality::nearestNeightbour));
  VarList::push_back(items, "High", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_2));
  VarList::push_back(items, "Ultra", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_3));
  myResamplingPopup = new PopUpWidget(this, font, xpos, ypos,
                                pwidth, lineHeight,
                                items, "Resampling quality ", lwidth);
  wid.push_back(myResamplingPopup);
  ypos += lineHeight + VGAP;

  // Param 1
  int swidth = pwidth+23;
  myHeadroomSlider = new SliderWidget(this, font, xpos, ypos, swidth, lineHeight,
                                      "Headroom           ", 0, kHeadroomChanged, 10 * fontWidth);
  myHeadroomSlider->setMinValue(0); myHeadroomSlider->setMaxValue(AudioSettings::MAX_HEADROOM);
  myHeadroomSlider->setTickmarkInterval(5);
  wid.push_back(myHeadroomSlider);
  ypos += lineHeight + VGAP;

  // Param 2
  myBufferSizeSlider = new SliderWidget(this, font, xpos, ypos, swidth, lineHeight,
                                        "Buffer size        ", 0, kBufferSizeChanged, 10 * fontWidth);
  myBufferSizeSlider->setMinValue(0); myBufferSizeSlider->setMaxValue(AudioSettings::MAX_BUFFER_SIZE);
  myBufferSizeSlider->setTickmarkInterval(5);
  wid.push_back(myBufferSizeSlider);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::loadConfig()
{
  AudioSettings& audioSettings = instance().audioSettings();

  // Enable sound
  mySoundEnableCheckbox->setState(audioSettings.enabled());

  // Volume
  myVolumeSlider->setValue(audioSettings.volume());

  // Stereo
  myStereoSoundPopup->setSelected(audioSettings.stereo());

  // Preset / mode
  myModePopup->setSelected(static_cast<int>(audioSettings.preset()));

  updateSettingsWithPreset(instance().audioSettings());

  updateEnabledState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::updateSettingsWithPreset(AudioSettings& audioSettings)
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
  AudioSettings& audioSettings = instance().audioSettings();

  // Enabled
  audioSettings.setEnabled(mySoundEnableCheckbox->getState());
  instance().sound().setEnabled(mySoundEnableCheckbox->getState());

  // Volume
  audioSettings.setVolume(myVolumeSlider->getValue());
  instance().sound().setVolume(myVolumeSlider->getValue());

  // Stereo
  audioSettings.setStereo(myStereoSoundPopup->getSelectedTag().toString());

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
  mySoundEnableCheckbox->setState(AudioSettings::DEFAULT_ENABLED);
  myVolumeSlider->setValue(AudioSettings::DEFAULT_VOLUME);
  myStereoSoundPopup->setSelected(AudioSettings::DEFAULT_STEREO);
  myModePopup->setSelected(static_cast<int>(AudioSettings::DEFAULT_PRESET));

  if (AudioSettings::DEFAULT_PRESET == AudioSettings::Preset::custom) {
    myResamplingPopup->setSelected(static_cast<int>(AudioSettings::DEFAULT_RESAMPLING_QUALITY));
    myFragsizePopup->setSelected(AudioSettings::DEFAULT_FRAGMENT_SIZE);
    myFreqPopup->setSelected(AudioSettings::DEFAULT_SAMPLE_RATE);
    myHeadroomSlider->setValue(AudioSettings::DEFAULT_HEADROOM);
    myBufferSizeSlider->setValue(AudioSettings::DEFAULT_BUFFER_SIZE);
  }
  else updatePreset();

  updateEnabledState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::updateEnabledState()
{
  bool active = mySoundEnableCheckbox->getState();
  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());
  bool userMode = preset == AudioSettings::Preset::custom;

  myVolumeSlider->setEnabled(active);
  myStereoSoundPopup->setEnabled(active);
  myModePopup->setEnabled(active);

  myFragsizePopup->setEnabled(active && userMode);
  myFreqPopup->setEnabled(active && userMode);
  myResamplingPopup->setEnabled(active && userMode);
  myHeadroomSlider->setEnabled(active && userMode);
  myBufferSizeSlider->setEnabled(active && userMode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioDialog::updatePreset()
{
  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());

  // Make a copy that does not affect the actual settings...
  AudioSettings audioSettings = instance().audioSettings();
  audioSettings.setPersistent(false);
  // ... and set the requested preset
  audioSettings.setPreset(preset);

  updateSettingsWithPreset(audioSettings);
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
      updateEnabledState();
      break;

    case kModeChanged:
      updatePreset();
      updateEnabledState();
      break;

    case kHeadroomChanged:
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(1) << (0.5 * myHeadroomSlider->getValue()) << " frames";
      myHeadroomSlider->setValueLabel(ss.str());
      break;
    }
    case kBufferSizeChanged:
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(1) << (0.5 * myBufferSizeSlider->getValue()) << " frames";
      myBufferSizeSlider->setValueLabel(ss.str());
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
