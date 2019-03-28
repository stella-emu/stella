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

#ifndef STELLA_OPTIONS_DIALOG_HXX
#define STELLA_OPTIONS_DIALOG_HXX

class PopUpWidget;

#include "Dialog.hxx"

namespace GUI {
  class Font;
}

class StellaOptionsDialog :
  public Dialog
{
public:
  StellaOptionsDialog(OSystem& osystem, DialogContainer& parent,
    const GUI::Font& font, int max_w, int max_h);
  virtual ~StellaOptionsDialog() = default;

private:
  void loadConfig() override;
  void saveConfig() override;
  void setDefaults() override;

  void addVideoOptions(WidgetArray wid, int& xpos, int& ypos, const GUI::Font& font);
  void addSoundOptions(WidgetArray wid, int& xpos, int& ypos, const GUI::Font& font);
  void addUIOptions(WidgetArray wid, int& xpos, int& ypos, const GUI::Font& font);

  TabWidget* myTab;

  // TV effects adjustables (custom mode)
  PopUpWidget*      myTVMode;
  SliderWidget*     myTVSharp;
  SliderWidget*     myTVHue;
  SliderWidget*     myTVRes;
  SliderWidget*     myTVArtifacts;
  SliderWidget*     myTVFringe;
  SliderWidget*     myTVBleed;
  SliderWidget*     myTVBright;
  SliderWidget*     myTVContrast;
  SliderWidget*     myTVSatur;
  SliderWidget*     myTVGamma;

  // TV phosphor effect
  CheckboxWidget*   myTVPhosphor;
  SliderWidget*     myTVPhosLevel;

  // TV scanline intensity and interpolation
  CheckboxWidget*   myTVScanlines;
  SliderWidget*     myTVScanIntense;

  // TV effects adjustables presets (custom mode)
  ButtonWidget*     myCloneComposite;
  ButtonWidget*     myCloneSvideo;
  ButtonWidget*     myCloneRGB;
  ButtonWidget*     myCloneBad;
  ButtonWidget*     myCloneCustom;

  // Audio options
  CheckboxWidget*   myStereoSoundCheckbox;

  // UI theme
  PopUpWidget*      myThemePopup;

  enum {
    kTVModeChanged      = 'VDtv',
    kCloneCompositeCmd  = 'CLcp',
    kCloneSvideoCmd     = 'CLsv',
    kCloneRGBCmd        = 'CLrb',
    kCloneBadCmd        = 'CLbd',
    kCloneCustomCmd     = 'CLcu',
    kPhosphorChanged    = 'VDph',
    kScanlinesChanged   = 'VDsc',

    kSoundEnableChanged = 'ADse',
  };

  // Following constructors and assignment operators not supported
  StellaOptionsDialog() = delete;
  StellaOptionsDialog(const StellaOptionsDialog&) = delete;
  StellaOptionsDialog(StellaOptionsDialog&&) = delete;
  StellaOptionsDialog& operator=(const StellaOptionsDialog&) = delete;
  StellaOptionsDialog& operator=(StellaOptionsDialog&&) = delete;
};

#endif

