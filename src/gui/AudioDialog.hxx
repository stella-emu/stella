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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef AUDIO_DIALOG_HXX
#define AUDIO_DIALOG_HXX

class CommandSender;
class Dialog;
class DialogContainer;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class CheckboxWidget;
class OSystem;
class AudioSettings;

#include "bspf.hxx"

class AudioDialog : public Dialog
{
  public:
    AudioDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    virtual ~AudioDialog() = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void updatePreset();
    void updateEnabledState();
    void updateSettingsWithPreset(AudioSettings&);
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    enum {
      kSoundEnableChanged = 'ADse',
      kModeChanged = 'ADmc',
      kHeadroomChanged = 'ADhc',
      kBufferSizeChanged = 'ADbc'
    };

    CheckboxWidget*   mySoundEnableCheckbox;
    SliderWidget*     myVolumeSlider;
    CheckboxWidget*   myStereoSoundCheckbox;
    PopUpWidget*      myModePopup;
    PopUpWidget*      myFragsizePopup;
    PopUpWidget*      myFreqPopup;
    PopUpWidget*      myResamplingPopup;
    SliderWidget*     myHeadroomSlider;
    SliderWidget*     myBufferSizeSlider;
    SliderWidget*     myDpcPitch;

  private:
    // Following constructors and assignment operators not supported
    AudioDialog() = delete;
    AudioDialog(const AudioDialog&) = delete;
    AudioDialog(AudioDialog&&) = delete;
    AudioDialog& operator=(const AudioDialog&) = delete;
    AudioDialog& operator=(AudioDialog&&) = delete;
};

#endif
