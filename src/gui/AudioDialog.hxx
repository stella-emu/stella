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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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

#include "bspf.hxx"

class AudioDialog : public Dialog
{
  public:
    AudioDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    virtual ~AudioDialog();

  protected:
    SliderWidget*     myVolumeSlider;
    StaticTextWidget* myVolumeLabel;
    PopUpWidget*      myFragsizePopup;
    PopUpWidget*      myFreqPopup;
    CheckboxWidget*   mySoundEnableCheckbox;

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    void handleSoundEnableChange(bool active);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    enum {
      kVolumeChanged      = 'ADvc',
      kSoundEnableChanged = 'ADse'
    };

  private:
    // Following constructors and assignment operators not supported
    AudioDialog() = delete;
    AudioDialog(const AudioDialog&) = delete;
    AudioDialog(AudioDialog&&) = delete;
    AudioDialog& operator=(const AudioDialog&) = delete;
    AudioDialog& operator=(AudioDialog&&) = delete;
};

#endif
