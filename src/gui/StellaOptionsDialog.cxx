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

#include "NTSCFilter.hxx"
#include "PopUpWidget.hxx"

#include "StellaOptionsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaOptionsDialog::StellaOptionsDialog(OSystem& osystem, DialogContainer& parent,
  const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Stella options")
{
  const int VGAP = 4;
  const int VBORDER = 8*2;
  const int HBORDER = 10*2;
  const int INDENT = 20;
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth(),
    buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
    
  WidgetArray wid;
  VariantList items;

  // Set real dimensions
  setSize(33 * fontWidth + HBORDER * 2, 11 * (lineHeight + VGAP) + _th, max_w, max_h);

  xpos = HBORDER;
  ypos = VBORDER + _th;

  addUIOptions(wid, xpos, ypos, font);
  ypos += VGAP * 5;
  addVideoOptions(wid, xpos, ypos, font);
  ypos += VGAP * 4;
  addSoundOptions(wid, xpos, ypos, font);

  addToFocusList(wid);

  wid.clear();
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaOptionsDialog::addUIOptions(WidgetArray wid, int& xpos, int& ypos, const GUI::Font& font)
{
  const int VGAP = 4;
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth();
  VariantList items;
  int pwidth = font.getStringWidth("Bad adjust");

  ypos += 1;
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  myThemePopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight, items, "UI Theme ");
  wid.push_back(myThemePopup);
  ypos += lineHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaOptionsDialog::addVideoOptions(WidgetArray wid, int& xpos, int& ypos, const GUI::Font& font)
{
  const int VGAP = 4;
  const int INDENT = 20;
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth();
  VariantList items;

  // TV effects options
  int swidth = font.getMaxCharWidth() * 8 - 4;

  // TV Mode
  items.clear();
  VarList::push_back(items, "Disabled", NTSCFilter::PRESET_OFF);
  VarList::push_back(items, "RGB", NTSCFilter::PRESET_RGB);
  VarList::push_back(items, "S-Video", NTSCFilter::PRESET_SVIDEO);
  VarList::push_back(items, "Composite", NTSCFilter::PRESET_COMPOSITE);
  VarList::push_back(items, "Bad adjust", NTSCFilter::PRESET_BAD);
  int lwidth = font.getStringWidth("TV Mode  ");
  int pwidth = font.getStringWidth("Bad adjust");

  myTVMode = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
    items, "TV mode ", lwidth, kTVModeChanged);
  wid.push_back(myTVMode);
  ypos += lineHeight + VGAP * 2;

#define CREATE_CUSTOM_SLIDERS(obj, desc)                                 \
  myTV ## obj =                                                          \
    new SliderWidget(this, font, xpos, ypos-1, swidth, lineHeight,       \
                     desc, lwidth, 0, fontWidth*4, "%");                 \
  myTV ## obj->setMinValue(0); myTV ## obj->setMaxValue(100);            \
  myTV ## obj->setTickmarkInterval(2);                                   \
  myTV ## obj->setStepValue(10);                                         \
  wid.push_back(myTV ## obj);                                            \
  ypos += lineHeight + VGAP;

  lwidth = font.getStringWidth("Intensity ");
  swidth = font.getMaxCharWidth() * 10;

  // Scanline intensity
  myTVScanlines = new CheckboxWidget(this, font, xpos, ypos + 1, "Scanlines", kScanlinesChanged);
  ypos += lineHeight;
  xpos += INDENT;
  CREATE_CUSTOM_SLIDERS(ScanIntense, "Intensity ")
  xpos -= INDENT;

  // TV Phosphor effect
  myTVPhosphor = new CheckboxWidget(this, font, xpos, ypos + 1, "Phosphor effect", kPhosphorChanged);
  wid.push_back(myTVPhosphor);
  ypos += lineHeight;
  // TV Phosphor blend level
  xpos += INDENT;
  CREATE_CUSTOM_SLIDERS(PhosLevel, "Blend     ")
  xpos -= INDENT;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaOptionsDialog::addSoundOptions(WidgetArray wid, int& xpos, int& ypos, const GUI::Font& font)
{
  const int VGAP = 4;
  const int lineHeight = font.getLineHeight();

  // Stereo sound
  myStereoSoundCheckbox = new CheckboxWidget(this, font, xpos, ypos + 1, 
    "Stereo sound");
  wid.push_back(myStereoSoundCheckbox);
  ypos += lineHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaOptionsDialog::loadConfig()
{

  //myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaOptionsDialog::saveConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaOptionsDialog::setDefaults()
{

}
