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

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PointingDevice.hxx"
#include "SaveKey.hxx"
#include "AtariVox.hxx"
#include "Settings.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "ColorWidget.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#ifdef DEBUGGER_SUPPORT
#include "DebuggerDialog.hxx"
#endif
#include "Console.hxx"
#include "TIA.hxx"
#include "OSystem.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "M6502.hxx"
#include "DeveloperDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DeveloperDialog::DeveloperDialog(OSystem& osystem, DialogContainer& parent,
                                 const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Developer settings")
{
  const int VGAP = 4;
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth(),
    buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;

  // Set real dimensions
  _w = std::min(53 * fontWidth + 10, max_w);
  _h = std::min(15 * (lineHeight + VGAP) + 14 + _th, max_h);

  // The tab widget
  xpos = 2; ypos = 4;
  myTab = new TabWidget(this, font, xpos, ypos + _th, _w - 2 * xpos, _h - _th - buttonHeight - 16 - ypos);
  addTabWidget(myTab);

  addEmulationTab(font);
  addVideoTab(font);
  addTimeMachineTab(font);
  addDebuggerTab(font);

  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  // Activate the first tab
  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addEmulationTab(const GUI::Font& font)
{
  const int HBORDER = 10;
  const int INDENT = 16+4;
  const int VBORDER = 8;
  const int VGAP = 4;
  int ypos = VBORDER;
  int lineHeight = font.getLineHeight();
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab("Emulation");

  // settings set
  mySettingsGroup0 = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                                               "Player settings", mySettingsGroup0, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                            "Developer settings", mySettingsGroup0, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 1;

  myFrameStatsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1, "Frame statistics");
  wid.push_back(myFrameStatsWidget);
  ypos += lineHeight + VGAP;

  // 2600/7800 mode
  items.clear();
  VarList::push_back(items, "Atari 2600", "2600");
  VarList::push_back(items, "Atari 7800", "7800");
  int lwidth = font.getStringWidth("Console ");
  int pwidth = font.getStringWidth("Atari 2600");

  myConsoleWidget = new PopUpWidget(myTab, font, HBORDER + INDENT * 1, ypos, pwidth, lineHeight, items,
                                    "Console ", lwidth, kConsole);
  wid.push_back(myConsoleWidget);
  ypos += lineHeight + VGAP;

  // Randomize items
  myLoadingROMLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "When loading a ROM:");
  wid.push_back(myLoadingROMLabel);
  ypos += lineHeight + VGAP;

  myRandomBankWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                          "Random startup bank");
  wid.push_back(myRandomBankWidget);
  ypos += lineHeight + VGAP;

  // Randomize RAM
  myRandomizeRAMWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                            "Randomize zero-page and extended RAM", kRandRAMID);
  wid.push_back(myRandomizeRAMWidget);
  ypos += lineHeight + VGAP;

  // Randomize CPU
  lwidth = font.getStringWidth("Randomize CPU ");
  myRandomizeCPULabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Randomize CPU ");
  wid.push_back(myRandomizeCPULabel);

  int xpos = myRandomizeCPULabel->getRight() + 10;
  const char* const cpuregs[] = { "SP", "A", "X", "Y", "PS" };
  for(int i = 0; i < 5; ++i)
  {
    myRandomizeCPUWidget[i] = new CheckboxWidget(myTab, font, xpos, ypos + 1,
                                           cpuregs[i], kRandCPUID);
    wid.push_back(myRandomizeCPUWidget[i]);
    xpos += CheckboxWidget::boxSize() + font.getStringWidth("XX") + 20;
  }
  ypos += lineHeight + VGAP;

  // How to handle undriven TIA pins
  myUndrivenPinsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                            "Drive unused TIA pins randomly on a read/peek");
  wid.push_back(myUndrivenPinsWidget);
  ypos += lineHeight + VGAP;

  // Thumb ARM emulation exception
  myThumbExceptionWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                              "Fatal ARM emulation error throws exception");
  wid.push_back(myThumbExceptionWidget);
  ypos += lineHeight + VGAP;

  // AtariVox/SaveKey EEPROM access
  myEEPROMAccessWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                            "Display AtariVox/SaveKey EEPROM R/W access");
  wid.push_back(myEEPROMAccessWidget);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addVideoTab(const GUI::Font& font)
{
  const int HBORDER = 10;
  const int INDENT = 16 + 4;
  const int VBORDER = 8;
  const int VGAP = 4;
  int ypos = VBORDER;
  int lineHeight = font.getLineHeight();
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();
  int lwidth = font.getStringWidth("Intensity ");
  int pwidth = font.getMaxCharWidth() * 6;
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab("Video");

  wid.clear();
  ypos = VBORDER;

  // settings set
  mySettingsGroup1 = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                                               "Player settings", mySettingsGroup1, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                            "Developer settings", mySettingsGroup1, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 1;

  // TV jitter effect
  myTVJitterWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                        "Jitter/roll effect", kTVJitter);
  wid.push_back(myTVJitterWidget);
  myTVJitterRecWidget = new SliderWidget(myTab, font,
                                         myTVJitterWidget->getRight() + fontWidth * 3, ypos - 1,
                                         "Recovery ", 0, kTVJitterChanged);
  myTVJitterRecWidget->setMinValue(1); myTVJitterRecWidget->setMaxValue(20);
  myTVJitterRecWidget->setTickmarkInterval(5);
  wid.push_back(myTVJitterRecWidget);
  myTVJitterRecLabelWidget = new StaticTextWidget(myTab, font,
                                                  myTVJitterRecWidget->getRight() + 4,
                                                  myTVJitterRecWidget->getTop() + 2,
                                                  5 * fontWidth, fontHeight, "");
  ypos += lineHeight + VGAP;

  myColorLossWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                         "PAL color-loss");
  wid.push_back(myColorLossWidget);
  ypos += lineHeight + VGAP;

  // debug colors
  myDebugColorsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                           "Debug colors (*)");
  wid.push_back(myDebugColorsWidget);
  ypos += lineHeight + VGAP + 2;

  items.clear();
  VarList::push_back(items, "Red", "r");
  VarList::push_back(items, "Orange", "o");
  VarList::push_back(items, "Yellow", "y");
  VarList::push_back(items, "Green", "g");
  VarList::push_back(items, "Purple", "p");
  VarList::push_back(items, "Blue", "b");

  static constexpr int dbg_cmds[DEBUG_COLORS] = {
    kP0ColourChangedCmd,  kM0ColourChangedCmd,  kP1ColourChangedCmd,
    kM1ColourChangedCmd,  kPFColourChangedCmd,  kBLColourChangedCmd
  };

  auto createDebugColourWidgets = [&](int idx, const string& desc)
  {
    int x = HBORDER + INDENT * 1;
    myDbgColour[idx] = new PopUpWidget(myTab, font, x, ypos - 1,
                                       pwidth, lineHeight, items, desc, lwidth, dbg_cmds[idx]);
    wid.push_back(myDbgColour[idx]);
    x += myDbgColour[idx]->getWidth() + 10;
    myDbgColourSwatch[idx] = new ColorWidget(myTab, font, x, ypos - 1,
                                             uInt32(2 * lineHeight), lineHeight);
    ypos += lineHeight + VGAP * 1;
  };

  createDebugColourWidgets(0, "Player 0  ");
  createDebugColourWidgets(1, "Missile 0 ");
  createDebugColourWidgets(2, "Player 1  ");
  createDebugColourWidgets(3, "Missile 1 ");
  createDebugColourWidgets(4, "Playfield ");
  createDebugColourWidgets(5, "Ball      ");

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, HBORDER, ypos, "(*) colors identical for player and developer settings");

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addTimeMachineTab(const GUI::Font& font)
{
  const string INTERVALS[NUM_INTERVALS] = {
    " 1 frame",
    " 3 frames",
    "10 frames",
    "30 frames",
    " 1 second",
    " 3 seconds",
    "10 seconds"
  };
  const string INT_SETTINGS[NUM_INTERVALS] = {
    "1f",
    "3f",
    "10f",
    "30f",
    "1s",
    "3s",
    "10s"
  };
  const string HORIZONS[NUM_HORIZONS] = {
    " 3 seconds",
    "10 seconds",
    "30 seconds",
    " 1 minute",
    " 3 minutes",
    "10 minutes",
    "30 minutes",
    "60 minutes"
  };
  const string HOR_SETTINGS[NUM_HORIZONS] = {
    "3s",
    "10s",
    "30s",
    "1m",
    "3m",
    "10m",
    "30m",
    "60m"
  };
  const int HBORDER = 10;
  const int INDENT = 16+4;
  const int VBORDER = 8;
  const int VGAP = 4;
  int ypos = VBORDER;
  int lineHeight = font.getLineHeight(),
    fontHeight = font.getFontHeight(),
    fontWidth = font.getMaxCharWidth(),
    lwidth = fontWidth * 11;
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab("Time Machine");

  // settings set
  mySettingsGroup2 = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                                               "Player settings", mySettingsGroup2, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                            "Developer settings", mySettingsGroup2, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 1;

  myTimeMachineWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT, ypos + 1,
                                           "Time Machine", kTimeMachine);
  wid.push_back(myTimeMachineWidget);
  ypos += lineHeight + VGAP;

  int swidth = fontWidth * 12 + 5; // width of PopUpWidgets below
  myStateSizeWidget = new SliderWidget(myTab, font, HBORDER + INDENT * 2, ypos - 1, swidth, lineHeight,
                                       "Buffer size (*)   ", 0, kSizeChanged, lwidth, " states");
  myStateSizeWidget->setMinValue(20);
  myStateSizeWidget->setMaxValue(1000);
  myStateSizeWidget->setStepValue(20);
  wid.push_back(myStateSizeWidget);
  ypos += lineHeight + VGAP;

  myUncompressedWidget = new SliderWidget(myTab, font, HBORDER + INDENT * 2, ypos - 1, swidth, lineHeight,
                                          "Uncompressed size ", 0, kUncompressedChanged, lwidth, " states");
  myUncompressedWidget->setMinValue(0);
  myUncompressedWidget->setMaxValue(1000);
  myUncompressedWidget->setStepValue(20);
  wid.push_back(myUncompressedWidget);
  ypos += lineHeight + VGAP;

  items.clear();
  for(int i = 0; i < NUM_INTERVALS; ++i)
    VarList::push_back(items, INTERVALS[i], INT_SETTINGS[i]);
  int pwidth = font.getStringWidth("10 seconds");
  myStateIntervalWidget = new PopUpWidget(myTab, font, HBORDER + INDENT * 2, ypos, pwidth,
                                          lineHeight, items, "Interval          ", 0, kIntervalChanged);
  wid.push_back(myStateIntervalWidget);
  ypos += lineHeight + VGAP;

  items.clear();
  for(int i = 0; i < NUM_HORIZONS; ++i)
    VarList::push_back(items, HORIZONS[i], HOR_SETTINGS[i]);
  myStateHorizonWidget = new PopUpWidget(myTab, font, HBORDER + INDENT * 2, ypos, pwidth,
                                         lineHeight, items, "Horizon         ~ ", 0, kHorizonChanged);
  wid.push_back(myStateHorizonWidget);

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, HBORDER, ypos, "(*) Any size change clears the buffer");

  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebuggerTab(const GUI::Font& font)
{
  int tabID = myTab->addTab("Debugger");
  WidgetArray wid;

#ifdef DEBUGGER_SUPPORT
  const int HBORDER = 10;
  const int VBORDER = 8;
  const int VGAP = 4;

  VariantList items;
  int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight(),
    lineHeight = font.getLineHeight();
  int xpos, ypos, pwidth;
  const GUI::Size& ds = instance().frameBuffer().desktopSize();

  xpos = HBORDER;
  ypos = VBORDER;

  // font size
  items.clear();
  VarList::push_back(items, "Small", "small");
  VarList::push_back(items, "Medium", "medium");
  VarList::push_back(items, "Large", "large");
  pwidth = font.getStringWidth("Medium");
  myDebuggerFontSize =
    new PopUpWidget(myTab, font, HBORDER, ypos + 1, pwidth, lineHeight, items,
                    "Font size (*)  ", 0, kDFontSizeChanged);
  wid.push_back(myDebuggerFontSize);
  ypos += lineHeight + 4;

  // Font style (bold label vs. text, etc)
  items.clear();
  VarList::push_back(items, "All Normal font", "0");
  VarList::push_back(items, "Bold labels only", "1");
  VarList::push_back(items, "Bold non-labels only", "2");
  VarList::push_back(items, "All Bold font", "3");
  pwidth = font.getStringWidth("Bold non-labels only");
  myDebuggerFontStyle =
    new PopUpWidget(myTab, font, HBORDER, ypos + 1, pwidth, lineHeight, items,
                    "Font style (*) ", 0);
  wid.push_back(myDebuggerFontStyle);

  ypos += lineHeight + VGAP * 4;

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, xpos, ypos-1, "Debugger width (*)  ",
                                           0, 0, 6 * fontWidth, "px");
  myDebuggerWidthSlider->setMinValue(DebuggerDialog::kSmallFontMinW);
  myDebuggerWidthSlider->setMaxValue(ds.w);
  myDebuggerWidthSlider->setStepValue(10);
  wid.push_back(myDebuggerWidthSlider);
  ypos += lineHeight + VGAP;

  myDebuggerHeightSlider = new SliderWidget(myTab, font, xpos, ypos-1, "Debugger height (*) ",
                                            0, 0, 6 * fontWidth, "px");
  myDebuggerHeightSlider->setMinValue(DebuggerDialog::kSmallFontMinH);
  myDebuggerHeightSlider->setMaxValue(ds.h);
  myDebuggerHeightSlider->setStepValue(10);
  wid.push_back(myDebuggerHeightSlider);
  ypos += lineHeight + VGAP * 4;

  myGhostReadsTrapWidget = new CheckboxWidget(myTab, font, HBORDER, ypos + 1,
                                             "Trap on 'ghost' reads", kGhostReads);
  wid.push_back(myGhostReadsTrapWidget);

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, HBORDER, ypos, "(*) Changes require a ROM reload");

  // Debugger is only realistically available in windowed modes 800x600 or greater
  // (and when it's actually been compiled into the app)
  bool debuggerAvailable =
#if defined(DEBUGGER_SUPPORT) && defined(WINDOWED_SUPPORT)
    (ds.w >= 800 && ds.h >= 600);  // TODO - maybe this logic can disappear?
#else
    false;
#endif
  if(!debuggerAvailable)
  {
    myDebuggerWidthSlider->clearFlags(WIDGET_ENABLED);
    myDebuggerHeightSlider->clearFlags(WIDGET_ENABLED);
  }
#else
  new StaticTextWidget(myTab, font, 0, 20, _w - 20, font.getFontHeight(),
                       "Debugger support not included", TextAlign::Center);
#endif

  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::loadSettings(SettingsSet set)
{
  string prefix = set == SettingsSet::player ? "plr." : "dev.";

  myFrameStats[set] = instance().settings().getBool(prefix + "stats");
  myConsole[set] = instance().settings().getString(prefix + "console") == "7800" ? 1 : 0;
  // Randomization
  myRandomBank[set] = instance().settings().getBool(prefix + "bankrandom");
  myRandomizeRAM[set] = instance().settings().getBool(prefix + "ramrandom");
  myRandomizeCPU[set] = instance().settings().getString(prefix + "cpurandom");
  // Undriven TIA pins
  myUndrivenPins[set] = instance().settings().getBool(prefix + "tiadriven");
  // Thumb ARM emulation exception
  myThumbException[set] = instance().settings().getBool(prefix + "thumb.trapfatal");
  // AtariVox/SaveKey EEPROM access
  myEEPROMAccess[set] = instance().settings().getBool(prefix + "eepromaccess");

  // Debug colors
  myDebugColors[set] = instance().settings().getBool(prefix + "debugcolors");
  // PAL color-loss effect
  myColorLoss[set] = instance().settings().getBool(prefix + "colorloss");
  // Jitter
  myTVJitter[set] = instance().settings().getBool(prefix + "tv.jitter");
  myTVJitterRec[set] = instance().settings().getInt(prefix + "tv.jitter_recovery");

  // States
  myTimeMachine[set] = instance().settings().getBool(prefix + "timemachine");
  myStateSize[set] = instance().settings().getInt(prefix + "tm.size");
  myUncompressed[set] = instance().settings().getInt(prefix + "tm.uncompressed");
  myStateInterval[set] = instance().settings().getString(prefix + "tm.interval");
  myStateHorizon[set] = instance().settings().getString(prefix + "tm.horizon");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveSettings(SettingsSet set)
{
  string prefix = set == SettingsSet::player ? "plr." : "dev.";

  instance().settings().setValue(prefix + "stats", myFrameStats[set]);
  instance().settings().setValue(prefix + "console", myConsole[set] == 1 ? "7800" : "2600");
  // Randomization
  instance().settings().setValue(prefix + "bankrandom", myRandomBank[set]);
  instance().settings().setValue(prefix + "ramrandom", myRandomizeRAM[set]);
  instance().settings().setValue(prefix + "cpurandom", myRandomizeCPU[set]);
  // Undriven TIA pins
  instance().settings().setValue(prefix + "tiadriven", myUndrivenPins[set]);
  // Thumb ARM emulation exception
  instance().settings().setValue(prefix + "thumb.trapfatal", myThumbException[set]);
  // AtariVox/SaveKey EEPROM access
  instance().settings().setValue(prefix + "eepromaccess", myEEPROMAccess[set]);

  // Debug colors
  instance().settings().setValue(prefix + "debugcolors", myDebugColors[set]);
  // PAL color loss
  instance().settings().setValue(prefix + "colorloss", myColorLoss[set]);
  // Jitter
  instance().settings().setValue(prefix + "tv.jitter", myTVJitter[set]);
  instance().settings().setValue(prefix + "tv.jitter_recovery", myTVJitterRec[set]);

  // States
  instance().settings().setValue(prefix + "timemachine", myTimeMachine[set]);
  instance().settings().setValue(prefix + "tm.size", myStateSize[set]);
  instance().settings().setValue(prefix + "tm.uncompressed", myUncompressed[set]);
  instance().settings().setValue(prefix + "tm.interval", myStateInterval[set]);
  instance().settings().setValue(prefix + "tm.horizon", myStateHorizon[set]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::getWidgetStates(SettingsSet set)
{
  myFrameStats[set] = myFrameStatsWidget->getState();
  myConsole[set] = myConsoleWidget->getSelected() == 1;
  // Randomization
  myRandomBank[set] = myRandomBankWidget->getState();
  myRandomizeRAM[set] = myRandomizeRAMWidget->getState();
  string cpurandom;
  const char* const cpuregs[] = { "S", "A", "X", "Y", "P" };
  for(int i = 0; i < 5; ++i)
    if(myRandomizeCPUWidget[i]->getState())
      cpurandom += cpuregs[i];
  myRandomizeCPU[set] = cpurandom;
  // Undriven TIA pins
  myUndrivenPins[set] = myUndrivenPinsWidget->getState();
  // Thumb ARM emulation exception
  myThumbException[set] = myThumbExceptionWidget->getState();
  // AtariVox/SaveKey EEPROM access
  myEEPROMAccess[set] = myEEPROMAccessWidget->getState();

  // Debug colors
  myDebugColors[set] = myDebugColorsWidget->getState();
  // PAL color-loss effect
  myColorLoss[set] = myColorLossWidget->getState();
  // Jitter
  myTVJitter[set] = myTVJitterWidget->getState();
  myTVJitterRec[set] = myTVJitterRecWidget->getValue();

  // States
  myTimeMachine[set] = myTimeMachineWidget->getState();
  myStateSize[set] = myStateSizeWidget->getValue();
  myUncompressed[set] = myUncompressedWidget->getValue();
  myStateInterval[set] = myStateIntervalWidget->getSelected();
  myStateInterval[set] = myStateIntervalWidget->getSelectedTag().toString();
  myStateHorizon[set] = myStateHorizonWidget->getSelectedTag().toString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::setWidgetStates(SettingsSet set)
{
  myFrameStatsWidget->setState(myFrameStats[set]);
  myConsoleWidget->setSelectedIndex(myConsole[set]);
  // Randomization
  myRandomBankWidget->setState(myRandomBank[set]);
  myRandomizeRAMWidget->setState(myRandomizeRAM[set]);

  const string& cpurandom = myRandomizeCPU[set];
  const char* const cpuregs[] = { "S", "A", "X", "Y", "P" };
  for(int i = 0; i < 5; ++i)
    myRandomizeCPUWidget[i]->setState(BSPF::containsIgnoreCase(cpurandom, cpuregs[i]));
  // Undriven TIA pins
  myUndrivenPinsWidget->setState(myUndrivenPins[set]);
  // Thumb ARM emulation exception
  myThumbExceptionWidget->setState(myThumbException[set]);
  // AtariVox/SaveKey EEPROM access
  myEEPROMAccessWidget->setState(myEEPROMAccess[set]);

  handleConsole();

  // Debug colors
  myDebugColorsWidget->setState(myDebugColors[set]);
  // PAL color-loss effect
  myColorLossWidget->setState(myColorLoss[set]);
  // Jitter
  myTVJitterWidget->setState(myTVJitter[set]);
  myTVJitterRecWidget->setValue(myTVJitterRec[set]);

  handleTVJitterChange(myTVJitterWidget->getState());
  handleEnableDebugColors();

  // States
  myTimeMachineWidget->setState(myTimeMachine[set]);
  myStateSizeWidget->setValue(myStateSize[set]);
  myUncompressedWidget->setValue(myUncompressed[set]);
  myStateIntervalWidget->setSelected(myStateInterval[set]);
  myStateHorizonWidget->setSelected(myStateHorizon[set]);

  handleTimeMachine();
  handleSize();
  handleUncompressed();
  handleInterval();
  handleHorizon();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::loadConfig()
{
  bool devSettings = instance().settings().getBool("dev.settings");
  mySettings = devSettings;
  mySettingsGroup0->setSelected(devSettings ? 1 : 0);
  mySettingsGroup1->setSelected(devSettings ? 1 : 0);
  mySettingsGroup2->setSelected(devSettings ? 1 : 0);

  // load both setting sets...
  loadSettings(SettingsSet::player);
  loadSettings(SettingsSet::developer);
  // ...and select the current one
  setWidgetStates(SettingsSet(mySettingsGroup0->getSelected()));

  // Debug colours
  handleDebugColours(instance().settings().getString("tia.dbgcolors"));

#ifdef DEBUGGER_SUPPORT
  uInt32 w, h;

  // Debugger size
  const GUI::Size& ds = instance().settings().getSize("dbg.res");
  w = ds.w; h = ds.h;

  myDebuggerWidthSlider->setValue(w);
  myDebuggerHeightSlider->setValue(h);

  // Debugger font size
  string size = instance().settings().getString("dbg.fontsize");
  myDebuggerFontSize->setSelected(size, "medium");

  // Debugger font style
  int style = instance().settings().getInt("dbg.fontstyle");
  myDebuggerFontStyle->setSelected(style, "0");

  // Ghost reads trap
  myGhostReadsTrapWidget->setState(instance().settings().getBool("dbg.ghostreadstrap"));

  handleFontSize();
#endif

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveConfig()
{
  instance().settings().setValue("dev.settings", mySettingsGroup0->getSelected() == SettingsSet::developer);
  // copy current widget status into set...
  getWidgetStates(SettingsSet(mySettingsGroup0->getSelected()));
  // ...and save both sets
  saveSettings(SettingsSet::player);
  saveSettings(SettingsSet::developer);

  // activate the current settings
  instance().frameBuffer().showFrameStats(myFrameStatsWidget->getState());
  // jitter
  if(instance().hasConsole())
  {
    instance().console().tia().toggleJitter(myTVJitterWidget->getState() ? 1 : 0);
    instance().console().tia().setJitterRecoveryFactor(myTVJitterRecWidget->getValue());
  }
  handleEnableDebugColors();
  // PAL color loss
  if(instance().hasConsole())
    instance().console().enableColorLoss(myColorLossWidget->getState());

  // Debug colours
  string dbgcolors;
  for(int i = 0; i < DEBUG_COLORS; ++i)
    dbgcolors += myDbgColour[i]->getSelectedTag().toString();
  if(instance().hasConsole() &&
     instance().console().tia().setFixedColorPalette(dbgcolors))
    instance().settings().setValue("tia.dbgcolors", dbgcolors);

  // update RewindManager
  instance().state().rewindManager().setup();
  instance().state().setRewindMode(myTimeMachineWidget->getState() ?
                                   StateManager::Mode::TimeMachine : StateManager::Mode::Off);

#ifdef DEBUGGER_SUPPORT
  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
                                 myDebuggerFontStyle->getSelectedTag().toString());
  // Debugger size
  instance().settings().setValue("dbg.res",
                                 GUI::Size(myDebuggerWidthSlider->getValue(),
                                 myDebuggerHeightSlider->getValue()));
  // Debugger font size
  instance().settings().setValue("dbg.fontsize", myDebuggerFontSize->getSelectedTag().toString());

  // Ghost reads trap
  instance().settings().setValue("dbg.ghostreadstrap", myGhostReadsTrapWidget->getState());
  if(instance().hasConsole())
    instance().console().system().m6502().setGhostReadsTrap(myGhostReadsTrapWidget->getState());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::setDefaults()
{
  bool devSettings = mySettingsGroup0->getSelected() == 1;
  SettingsSet set = SettingsSet(mySettingsGroup0->getSelected());

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation
      myFrameStats[set] = devSettings ? true : false;
      myConsole[set] = 0;
      // Randomization
      myRandomBank[set] = devSettings ? true : false;
      myRandomizeRAM[set] = devSettings ? true : false;
      myRandomizeCPU[set] = devSettings ? "SAXYP" : "";
      // Undriven TIA pins
      myUndrivenPins[set] = devSettings ? true : false;
      // Thumb ARM emulation exception
      myThumbException[set] = devSettings ? true : false;
      // AtariVox/SaveKey EEPROM access
      myEEPROMAccess[set] = devSettings ? true : false;

      setWidgetStates(set);
      break;

    case 1: // Video
      // Jitter
      myTVJitter[set] = true;
      myTVJitterRec[set] = devSettings ? 2 : 10;
      // PAL color-loss effect
      myColorLoss[set] = devSettings ? true : false;
      // Debug colors
      myDebugColors[set] = false;
      handleDebugColours("roygpb");

      setWidgetStates(set);
      break;

    case 2: // States
      myTimeMachine[set] = devSettings ? true : false;
      myStateSize[set] = 100;
      myUncompressed[set] = devSettings ? 60 : 30;
      myStateInterval[set] = devSettings ? "1f" : "30f";
      myStateHorizon[set] = devSettings ? "10s" : "10m";

      setWidgetStates(set);
      break;

    case 3: // Debugger options
    {
#ifdef DEBUGGER_SUPPORT
      uInt32 w = std::min(instance().frameBuffer().desktopSize().w, uInt32(DebuggerDialog::kMediumFontMinW));
      uInt32 h = std::min(instance().frameBuffer().desktopSize().h, uInt32(DebuggerDialog::kMediumFontMinH));
      myDebuggerWidthSlider->setValue(w);
      myDebuggerHeightSlider->setValue(h);
      myDebuggerFontSize->setSelected("medium");
      myDebuggerFontStyle->setSelected("0");

      myGhostReadsTrapWidget->setState(true);

      handleFontSize();
#endif
      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kPlrSettings:
      handleSettings(false);
      break;

    case kDevSettings:
      handleSettings(true);
      break;

    case kConsole:
      handleConsole();
        break;

    case kTVJitter:
      handleTVJitterChange(myTVJitterWidget->getState());
      break;

    case kTVJitterChanged:
      myTVJitterRecLabelWidget->setValue(myTVJitterRecWidget->getValue());
      break;

    case kPPinCmd:
      instance().console().tia().driveUnusedPinsRandom(myUndrivenPinsWidget->getState());
      break;

    case kTimeMachine:
      handleTimeMachine();
      break;

    case kSizeChanged:
      handleSize();
      break;

    case kUncompressedChanged:
      handleUncompressed();
      break;

    case kIntervalChanged:
      handleInterval();
      break;

    case kHorizonChanged:
      handleHorizon();
      break;

    case kP0ColourChangedCmd:
      handleDebugColours(0, myDbgColour[0]->getSelected());
      break;

    case kM0ColourChangedCmd:
      handleDebugColours(1, myDbgColour[1]->getSelected());
      break;

    case kP1ColourChangedCmd:
      handleDebugColours(2, myDbgColour[2]->getSelected());
      break;

    case kM1ColourChangedCmd:
      handleDebugColours(3, myDbgColour[3]->getSelected());
      break;

    case kPFColourChangedCmd:
      handleDebugColours(4, myDbgColour[4]->getSelected());
      break;

    case kBLColourChangedCmd:
      handleDebugColours(5, myDbgColour[5]->getSelected());
      break;

#ifdef DEBUGGER_SUPPORT
    case kDFontSizeChanged:
      handleFontSize();
      break;
#endif

    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kCloseCmd:
      // Revert changes made to event mapping
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSettings(bool devSettings)
{
  if (mySettings != devSettings)
  {
    mySettings = devSettings; // block redundant events first!
    mySettingsGroup0->setSelected(devSettings ? 1 : 0);
    mySettingsGroup1->setSelected(devSettings ? 1 : 0);
    mySettingsGroup2->setSelected(devSettings ? 1 : 0);
    getWidgetStates(devSettings ? SettingsSet::player : SettingsSet::developer);
    setWidgetStates(devSettings ? SettingsSet::developer : SettingsSet::player);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTVJitterChange(bool enable)
{
  myTVJitterRecWidget->setEnabled(enable);
  myTVJitterRecLabelWidget->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleEnableDebugColors()
{
  if(instance().hasConsole())
  {
    bool fixed = instance().console().tia().usingFixedColors();
    if(fixed != myDebugColorsWidget->getState())
      instance().console().tia().toggleFixedColors();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleConsole()
{
  bool is7800 = myConsoleWidget->getSelected() == 1;

  myRandomizeRAMWidget->setEnabled(!is7800);
  if(is7800)
    myRandomizeRAMWidget->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTimeMachine()
{
  bool enable = myTimeMachineWidget->getState();

  myStateSizeWidget->setEnabled(enable);
  myUncompressedWidget->setEnabled(enable);
  myStateIntervalWidget->setEnabled(enable);

  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();

  myStateHorizonWidget->setEnabled(enable && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSize()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt horizon and interval
  do
  {
    for(i = horizon; i < NUM_HORIZONS; ++i)
    {
      if(uInt64(size) * instance().state().rewindManager().INTERVAL_CYCLES[interval]
         <= instance().state().rewindManager().HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      interval--;
  } while(!found);

  if(size < uncompressed)
    myUncompressedWidget->setValue(size);
  myStateIntervalWidget->setSelectedIndex(interval);
  myStateHorizonWidget->setSelectedIndex(i);
  myStateHorizonWidget->setEnabled(myTimeMachineWidget->getState() && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleUncompressed()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();

  if(size < uncompressed)
    myStateSizeWidget->setValue(uncompressed);
  myStateHorizonWidget->setEnabled(myTimeMachineWidget->getState() && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleInterval()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt horizon and size
  do
  {
    for(i = horizon; i < NUM_HORIZONS; ++i)
    {
      if(uInt64(size) * instance().state().rewindManager().INTERVAL_CYCLES[interval]
         <= instance().state().rewindManager().HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      size -= myStateSizeWidget->getStepValue();
  } while(!found);

  myStateHorizonWidget->setSelectedIndex(i);
  myStateSizeWidget->setValue(size);
  if(size < uncompressed)
    myUncompressedWidget->setValue(size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleHorizon()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt interval and size
  do
  {
    for(i = interval; i >= 0; --i)
    {
      if(uInt64(size) * instance().state().rewindManager().INTERVAL_CYCLES[i]
         <= instance().state().rewindManager().HORIZON_CYCLES[horizon])
      {
        found = true;
        break;
      }
    }
    if(!found)
      size -= myStateSizeWidget->getStepValue();
  } while(!found);

  myStateIntervalWidget->setSelectedIndex(i);
  myStateSizeWidget->setValue(size);
  if(size < uncompressed)
    myUncompressedWidget->setValue(size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(int idx, int color)
{
  if(idx < 0 || idx >= DEBUG_COLORS)
    return;

  if(!instance().hasConsole())
  {
    myDbgColour[idx]->clearFlags(WIDGET_ENABLED);
    myDbgColourSwatch[idx]->clearFlags(WIDGET_ENABLED);
    return;
  }

  static constexpr int dbg_color[2][DEBUG_COLORS] = {
    {
      TIA::FixedColor::NTSC_RED,
      TIA::FixedColor::NTSC_ORANGE,
      TIA::FixedColor::NTSC_YELLOW,
      TIA::FixedColor::NTSC_GREEN,
      TIA::FixedColor::NTSC_PURPLE,
      TIA::FixedColor::NTSC_BLUE
    },
    {
      TIA::FixedColor::PAL_RED,
      TIA::FixedColor::PAL_ORANGE,
      TIA::FixedColor::PAL_YELLOW,
      TIA::FixedColor::PAL_GREEN,
      TIA::FixedColor::PAL_PURPLE,
      TIA::FixedColor::PAL_BLUE
    }
  };

  int mode = instance().console().tia().frameLayout() == FrameLayout::ntsc ? 0 : 1;
  myDbgColourSwatch[idx]->setColor(dbg_color[mode][color]);
  myDbgColour[idx]->setSelectedIndex(color);

  // make sure the selected debug colors are all different
  bool usedCol[DEBUG_COLORS];

  // identify used colors
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    usedCol[i] = false;
    for(int j = 0; j < DEBUG_COLORS; ++j)
    {
      if(myDbgColourSwatch[j]->getColor() == dbg_color[mode][i])
      {
        usedCol[i] = true;
        break;
      }
    }
  }
  // check if currently changed color was used somewhere else
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    if (i != idx && myDbgColourSwatch[i]->getColor() == dbg_color[mode][color])
    {
      // if already used, change the other color to an unused one
      for(int j = 0; j < DEBUG_COLORS; ++j)
      {
        if(!usedCol[j])
        {
          myDbgColourSwatch[i]->setColor(dbg_color[mode][j]);
          myDbgColour[i]->setSelectedIndex(j);
          break;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(const string& colors)
{
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    switch(colors[i])
    {
      case 'r':  handleDebugColours(i, 0);  break;
      case 'o':  handleDebugColours(i, 1);  break;
      case 'y':  handleDebugColours(i, 2);  break;
      case 'g':  handleDebugColours(i, 3);  break;
      case 'p':  handleDebugColours(i, 4);  break;
      case 'b':  handleDebugColours(i, 5);  break;
      default:                              break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleFontSize()
{
#ifdef DEBUGGER_SUPPORT
  uInt32 minW, minH;
  int fontSize = myDebuggerFontSize->getSelected();

  if(fontSize == 0)
  {
    minW = DebuggerDialog::kSmallFontMinW;
    minH = DebuggerDialog::kSmallFontMinH;
  }
  else if(fontSize == 1)
  {
    minW = DebuggerDialog::kMediumFontMinW;
    minH = DebuggerDialog::kMediumFontMinH;
  }
  else // large
  {
    minW = DebuggerDialog::kLargeFontMinW;
    minH = DebuggerDialog::kLargeFontMinH;
  }
  minW = std::min(instance().frameBuffer().desktopSize().w, minW);
  minH = std::min(instance().frameBuffer().desktopSize().h, minH);

  myDebuggerWidthSlider->setMinValue(minW);
  if(minW > uInt32(myDebuggerWidthSlider->getValue()))
    myDebuggerWidthSlider->setValue(minW);

  myDebuggerHeightSlider->setMinValue(minH);
  if(minH > uInt32(myDebuggerHeightSlider->getValue()))
    myDebuggerHeightSlider->setValue(minH);
#endif
}
