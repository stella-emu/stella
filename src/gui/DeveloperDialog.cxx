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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
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
#include "EventMappingWidget.hxx"
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
#include "DeveloperDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DeveloperDialog::DeveloperDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent),
  myMaxWidth(max_w),
  myMaxHeight(max_h)
{
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth(),
    buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos, tabID;

  // Set real dimensions
  _w = std::min(53 * fontWidth + 10, max_w);
  _h = std::min(15 * (lineHeight + 4) + 14, max_h);

  // The tab widget
  xpos = 2; ypos = 4;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2 * xpos, _h - buttonHeight - 16 - ypos);
  addTabWidget(myTab);

  addEmulationTab(font);
  addStatesTab(font);
  addDebugColorsTab(font);
  addDebuggerTab(font);
  addDefaultOKCancelButtons(font);

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
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab("Emulation");

  // settings set
  mySettingsGroup0 = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1, "Player settings", mySettingsGroup0, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1, "Developer settings", mySettingsGroup0, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  myFrameStatsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1, "Frame statistics");
  wid.push_back(myFrameStatsWidget);
  ypos += lineHeight + VGAP;

  // 2600/7800 mode
  items.clear();
  VarList::push_back(items, "Atari 2600", "2600");
  VarList::push_back(items, "Atari 7800", "7800");
  int lwidth = font.getStringWidth("Console ");
  int pwidth = font.getStringWidth("Atari 2600");

  myConsoleWidget = new PopUpWidget(myTab, font, HBORDER + INDENT * 1, ypos, pwidth, lineHeight, items, "Console ", lwidth, kConsole);
  wid.push_back(myConsoleWidget);
  ypos += lineHeight + VGAP;

  // Randomize items
  myLoadingROMLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "When loading a ROM:", kTextAlignLeft);
  wid.push_back(myLoadingROMLabel);
  ypos += lineHeight + VGAP;

  myRandomBankWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Random startup bank");
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

  // debug colors
  myDebugColorsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1, "Debug colors");
  wid.push_back(myDebugColorsWidget);
  ypos += lineHeight + VGAP;

  myColorLossWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1, "PAL color-loss");
  wid.push_back(myColorLossWidget);
  ypos += lineHeight + VGAP;

  // TV jitter effect
  myTVJitterWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1, "Jitter/roll effect", kTVJitter);
  wid.push_back(myTVJitterWidget);
  myTVJitterRecWidget = new SliderWidget(myTab, font,
                                   myTVJitterWidget->getRight() + fontWidth * 3, ypos - 1,
                                   8 * fontWidth, lineHeight, "Recovery ",
                                   font.getStringWidth("Recovery "), kTVJitterChanged);
  myTVJitterRecWidget->setMinValue(1); myTVJitterRecWidget->setMaxValue(20);
  wid.push_back(myTVJitterRecWidget);
  myTVJitterRecLabelWidget = new StaticTextWidget(myTab, font,
                                         myTVJitterRecWidget->getRight() + 4, myTVJitterRecWidget->getTop() + 2,
                                         5 * fontWidth, fontHeight, "", kTextAlignLeft);
  myTVJitterRecLabelWidget->setFlags(WIDGET_CLEARBG);
  wid.push_back(myTVJitterRecLabelWidget);
  ypos += lineHeight + VGAP;

  // How to handle undriven TIA pins
  myUndrivenPinsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                      "Drive unused TIA pins randomly on a read/peek");
  wid.push_back(myUndrivenPinsWidget);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addStatesTab(const GUI::Font& font)
{
  const int HBORDER = 10;
  const int INDENT = 16+4;
  const int VBORDER = 8;
  const int VGAP = 4;
  int ypos = VBORDER;
  int lineHeight = font.getLineHeight();
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();
  WidgetArray wid;
  int tabID = myTab->addTab("States");

  // settings set
  mySettingsGroup1 = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1, "Player settings", mySettingsGroup1, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1, "Developer settings", mySettingsGroup1, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 2;

  myContinuousRewindWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT, ypos + 1, "Continuous rewind", kRewind);
  wid.push_back(myContinuousRewindWidget);
  ypos += lineHeight + VGAP;

  int sWidth = font.getMaxCharWidth() * 8;
  myStateSizeWidget = new SliderWidget(myTab, font, HBORDER + INDENT * 2, ypos - 1, sWidth, lineHeight,
                                 "Buffer size (*) ", 0, kSizeChanged);
  myStateSizeWidget->setMinValue(100);
  myStateSizeWidget->setMaxValue(1000);
  myStateSizeWidget->setStepValue(100);
  wid.push_back(myStateSizeWidget);
  myStateSizeLabelWidget = new StaticTextWidget(myTab, font, myStateSizeWidget->getRight() + 4, myStateSizeWidget->getTop() + 2, "100 ");

  ypos += lineHeight + VGAP;
  myStateIntervalWidget = new SliderWidget(myTab, font, HBORDER + INDENT * 2, ypos - 1, sWidth, lineHeight,
                                     "Interval        ", 0, kIntervalChanged);

  myStateIntervalWidget->setMinValue(0);
  myStateIntervalWidget->setMaxValue(NUM_INTERVALS - 1);
  wid.push_back(myStateIntervalWidget);
  myStateIntervalLabelWidget = new StaticTextWidget(myTab, font, myStateIntervalWidget->getRight() + 4, myStateIntervalWidget->getTop() + 2, "50 scanlines");

  ypos += lineHeight + VGAP;
  myStateHorizonWidget = new SliderWidget(myTab, font, HBORDER + INDENT * 2, ypos - 1, sWidth, lineHeight,
                                    "Horizon         ", 0, kHorizonChanged);
  myStateHorizonWidget->setMinValue(0);
  myStateHorizonWidget->setMaxValue(NUM_HORIZONS - 1);
  wid.push_back(myStateHorizonWidget);
  myStateHorizonLabelWidget = new StaticTextWidget(myTab, font, myStateHorizonWidget->getRight() + 4, myStateHorizonWidget->getTop() + 2, "~60 minutes");

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, HBORDER, ypos, "(*) Requires application restart");

  addToFocusList(wid, myTab, tabID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebugColorsTab(const GUI::Font& font)
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
  int tabID = myTab->addTab("Debug Colors");

  wid.clear();
  ypos = VBORDER;

  items.clear();
  VarList::push_back(items, "Red", "r");
  VarList::push_back(items, "Orange", "o");
  VarList::push_back(items, "Yellow", "y");
  VarList::push_back(items, "Green", "g");
  VarList::push_back(items, "Blue", "b");
  VarList::push_back(items, "Purple", "p");

  static constexpr int dbg_cmds[6] = {
    kP0ColourChangedCmd,  kM0ColourChangedCmd,  kP1ColourChangedCmd,
    kM1ColourChangedCmd,  kPFColourChangedCmd,  kBLColourChangedCmd
  };

  auto createDebugColourWidgets = [&](int idx, const string& desc)
  {
    int x = HBORDER;
    myDbgColour[idx] = new PopUpWidget(myTab, font, x, ypos,
                                       pwidth, lineHeight, items, desc, lwidth, dbg_cmds[idx]);
    wid.push_back(myDbgColour[idx]);
    x += myDbgColour[idx]->getWidth() + 10;
    myDbgColourSwatch[idx] = new ColorWidget(myTab, font, x, ypos,
                                             uInt32(2 * lineHeight), lineHeight);
    ypos += lineHeight + VGAP * 1;
  };

  createDebugColourWidgets(0, "Player 0 ");
  createDebugColourWidgets(1, "Missile 0 ");
  createDebugColourWidgets(2, "Player 1 ");
  createDebugColourWidgets(3, "Missile 1 ");
  createDebugColourWidgets(4, "Playfield ");
  createDebugColourWidgets(5, "Ball ");

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, 10, ypos, "(*) Colors must be different for each object");

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebuggerTab(const GUI::Font& font)
{
  int tabID = myTab->addTab("Debugger");

#ifdef DEBUGGER_SUPPORT
  const int HBORDER = 10;
  const int VBORDER = 8;
  const int VGAP = 4;

  WidgetArray wid;
  VariantList items;
  int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight(),
    lineHeight = font.getLineHeight();
  int xpos, ypos, pwidth;
  ButtonWidget* b;
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
                    "Font size  ", 0, kDFontSizeChanged);
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
                    "Font style ", 0);
  wid.push_back(myDebuggerFontStyle);

  ypos += lineHeight + VGAP * 4;

  pwidth = font.getMaxCharWidth() * 8;
  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, xpos, ypos-1, pwidth,
                                           lineHeight, "Debugger width  ",
                                           0, kDWidthChanged);
  myDebuggerWidthSlider->setMinValue(DebuggerDialog::kSmallFontMinW);
  myDebuggerWidthSlider->setMaxValue(ds.w);
  myDebuggerWidthSlider->setStepValue(10);
  wid.push_back(myDebuggerWidthSlider);
  myDebuggerWidthLabel =
    new StaticTextWidget(myTab, font,
                         xpos + myDebuggerWidthSlider->getWidth() + 4,
                         ypos + 1, 4 * fontWidth, fontHeight, "", kTextAlignLeft);
  myDebuggerWidthLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + VGAP;

  myDebuggerHeightSlider = new SliderWidget(myTab, font, xpos, ypos-1, pwidth,
                                            lineHeight, "Debugger height ",
                                            0, kDHeightChanged);
  myDebuggerHeightSlider->setMinValue(DebuggerDialog::kSmallFontMinH);
  myDebuggerHeightSlider->setMaxValue(ds.h);
  myDebuggerHeightSlider->setStepValue(10);
  wid.push_back(myDebuggerHeightSlider);
  myDebuggerHeightLabel =
    new StaticTextWidget(myTab, font,
                         xpos + myDebuggerHeightSlider->getWidth() + 4,
                         ypos + 1, 4 * fontWidth, fontHeight, "", kTextAlignLeft);
  myDebuggerHeightLabel->setFlags(WIDGET_CLEARBG);

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, HBORDER, ypos, "(*) Changes require application restart");

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
    myDebuggerWidthLabel->clearFlags(WIDGET_ENABLED);
    myDebuggerHeightSlider->clearFlags(WIDGET_ENABLED);
    myDebuggerHeightLabel->clearFlags(WIDGET_ENABLED);
  }

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);
#else
  new StaticTextWidget(myTab, font, 0, 20, _w - 20, fontHeight,
                       "Debugger support not included", kTextAlignCenter);
#endif

  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDefaultOKCancelButtons(const GUI::Font& font)
{
  const int buttonWidth = font.getStringWidth("Defaults") + 20,
    buttonHeight = font.getLineHeight() + 4;
  WidgetArray wid;

  wid.clear();
  ButtonWidget* btn = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                                       buttonWidth, buttonHeight, "Defaults", GuiObject::kDefaultsCmd);
  wid.push_back(btn);
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
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
  // Debug colors
  myDebugColors[set] = instance().settings().getBool(prefix + "debugcolors");
  // PAL color-loss effect
  myColorLoss[set] = instance().settings().getBool(prefix + "colorloss");
  // Jitter
  myTVJitter[set] = instance().settings().getBool(prefix + "tv.jitter");
  myTVJitterRec[set] = instance().settings().getInt(prefix + "tv.jitter_recovery");
  // Undriven TIA pins
  myUndrivenPins[set] = instance().settings().getBool(prefix + "tiadriven");

  // States
  myContinuousRewind[set] = instance().settings().getBool(prefix + "rewind");
  myStateSize[set] = instance().settings().getInt(prefix + "rewind.size");
  myStateInterval[set] = instance().settings().getInt(prefix + "rewind.interval");
  myStateHorizon[set] = instance().settings().getInt(prefix + "rewind.horizon");
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
  // Debug colors
  instance().settings().setValue(prefix + "debugcolors", myDebugColors[set]);
  // PAL color loss
  instance().settings().setValue(prefix + "colorloss", myColorLoss[set]);
  // Jitter
  instance().settings().setValue(prefix + "tv.jitter", myTVJitter[set]);
  instance().settings().setValue(prefix + "tv.jitter_recovery", myTVJitterRec[set]);
  // Undriven TIA pins
  instance().settings().setValue(prefix + "tiadriven", myUndrivenPins[set]);

  // States
  instance().settings().setValue(prefix + "rewind", myContinuousRewind[set]);
  instance().settings().setValue(prefix + "rewind.size", myStateSize[set]);
  instance().settings().setValue(prefix + "rewind.interval", myStateInterval[set]);
  instance().settings().setValue(prefix + "rewind.horizon", myStateHorizon[set]);
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
  // Debug colors
  myDebugColors[set] = myDebugColorsWidget->getState();
  // PAL color-loss effect
  myColorLoss[set] = myColorLossWidget->getState();
  // Jitter
  myTVJitter[set] = myTVJitterWidget->getState();
  myTVJitterRec[set] = myTVJitterRecWidget->getValue();
  // Undriven TIA pins
  myUndrivenPins[set] = myUndrivenPinsWidget->getState();

  // States
  myContinuousRewind[set] = myContinuousRewindWidget->getState();
  myStateSize[set] = myStateSizeWidget->getValue();
  myStateInterval[set] = myStateIntervalWidget->getValue();
  myStateHorizon[set] = myStateHorizonWidget->getValue();
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
  // Debug colors
  myDebugColorsWidget->setState(myDebugColors[set]);
  // PAL color-loss effect
  myColorLossWidget->setState(myColorLoss[set]);
  // Jitter
  myTVJitterWidget->setState(myTVJitter[set]);
  myTVJitterRecWidget->setValue(myTVJitterRec[set]);
  // Undriven TIA pins
  myUndrivenPinsWidget->setState(myUndrivenPins[set]);

  handleConsole();
  handleTVJitterChange(myTVJitterWidget->getState());
  handleEnableDebugColors();

  // States
  myContinuousRewindWidget->setState(myContinuousRewind[set]);
  myStateSizeWidget->setValue(myStateSize[set]);
  myStateIntervalWidget->setValue(myStateInterval[set]);
  myStateHorizonWidget->setValue(myStateHorizon[set]);

  handleRewind();
  handleSize();
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

  // load both setting sets...
  loadSettings(SettingsSet::player);
  loadSettings(SettingsSet::developer);
  // ...and select the current one
  setWidgetStates((SettingsSet)mySettingsGroup0->getSelected());

  // Debug colours
  handleDebugColours(instance().settings().getString("tia.dbgcolors"));

#ifdef DEBUGGER_SUPPORT
  uInt32 w, h;

  // Debugger size
  const GUI::Size& ds = instance().settings().getSize("dbg.res");
  w = ds.w; h = ds.h;

  myDebuggerWidthSlider->setValue(w);
  myDebuggerWidthLabel->setValue(w);
  myDebuggerHeightSlider->setValue(h);
  myDebuggerHeightLabel->setValue(h);

  // Debugger font size
  string size = instance().settings().getString("dbg.fontsize");
  myDebuggerFontSize->setSelected(size, "medium");

  // Debugger font style
  int style = instance().settings().getInt("dbg.fontstyle");
  myDebuggerFontStyle->setSelected(style, "0");

  handleFontSize();
#endif

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveConfig()
{
  instance().settings().setValue("dev.settings", mySettingsGroup0->getSelected() == SettingsSet::developer);
  // copy current widget status into set...
  getWidgetStates((SettingsSet)mySettingsGroup0->getSelected());
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
  for(int i = 0; i < 6; ++i)
    dbgcolors += myDbgColour[i]->getSelectedTag().toString();
  if(instance().hasConsole() &&
     instance().console().tia().setFixedColorPalette(dbgcolors))
    instance().settings().setValue("tia.dbgcolors", dbgcolors);

  // Finally, issue a complete framebuffer re-initialization
  //instance().createFrameBuffer();

  // TODO: update RewindManager
  instance().state().setRewindMode(myContinuousRewindWidget->getState() ?
                                   StateManager::Mode::Rewind : StateManager::Mode::Off);

  // define interval growth factor
  uInt32 size = myStateSizeWidget->getValue();
  uInt64 horizon = HORIZON_CYCLES[myStateHorizonWidget->getValue()];
  double factor, minFactor = 1, maxFactor = 2;

  while(true)
  {
    double interval = INTERVAL_CYCLES[myStateIntervalWidget->getValue()];
    double cycleSum = 0.0;
    // calculate next factor
    factor = (minFactor + maxFactor) / 2;
    // sum up interval cycles
    for(uInt32 i = 0; i < size; ++i)
    {
      cycleSum += interval;
      interval *= factor;
    }
    double diff = cycleSum - horizon;
//cerr << "factor " << factor << ", diff " << diff << endl;
    // exit loop if result is close enough
    if(abs(diff) < horizon * 1E-5)
      break;
    // define new boundary
    if(cycleSum < horizon)
      minFactor = factor;
    else
      maxFactor = factor;
  }
  // TODO factor calculation code above into RewindManager
  //instance().settings().setValue("dev.rewind.factor", factor);

  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
                                 myDebuggerFontStyle->getSelectedTag().toString());

#ifdef DEBUGGER_SUPPORT
  // Debugger size
  instance().settings().setValue("dbg.res",
                                 GUI::Size(myDebuggerWidthSlider->getValue(),
                                 myDebuggerHeightSlider->getValue()));

  // Debugger font size
  instance().settings().setValue("dbg.fontsize", myDebuggerFontSize->getSelectedTag().toString());
#endif
}

void DeveloperDialog::setDefaults()
{
  bool devSettings = mySettingsGroup0->getSelected() == 1;
  SettingsSet set = (SettingsSet)mySettingsGroup0->getSelected();

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation
      myFrameStats[set] = devSettings ? true : false;
      myConsole[set] = 0;
      // Randomization
      myRandomBank[set] = devSettings ? true : false;
      myRandomizeRAM[set] = devSettings ? true : false;
      myRandomizeCPU[set] = devSettings ? "SAXYP" : "";
      // Debug colors
      myDebugColors[set] = false;
      // PAL color-loss effect
      myColorLoss[set] = devSettings ? true : false;
      // Jitter
      myTVJitter[set] = true;
      myTVJitterRec[set] = devSettings ? 2 : 10;
      // Undriven TIA pins
      myUndrivenPins[set] = devSettings ? true : false;

      setWidgetStates(set);
      break;

    case 1: // States
      myContinuousRewind[set] = devSettings ? true : false;
      myStateSize[set] = 100;
      myStateInterval[set] = devSettings ? 2 : 4;
      myStateHorizon[set] = devSettings ? 3 : 5;

      setWidgetStates(set);
      break;

    case 2:  // Debug colours
      handleDebugColours("roygpb");
      break;

    case 3: // Debugger options
    {
#ifdef DEBUGGER_SUPPORT
      uInt32 w = std::min(instance().frameBuffer().desktopSize().w, uInt32(DebuggerDialog::kMediumFontMinW));
      uInt32 h = std::min(instance().frameBuffer().desktopSize().h, uInt32(DebuggerDialog::kMediumFontMinH));
      myDebuggerWidthSlider->setValue(w);
      myDebuggerWidthLabel->setValue(w);
      myDebuggerHeightSlider->setValue(h);
      myDebuggerHeightLabel->setValue(h);
      myDebuggerFontSize->setSelected("medium");
      myDebuggerFontStyle->setSelected("0");
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

    case kRewind:
      handleRewind();
      break;

    case kSizeChanged:
      handleSize();
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
    case kDWidthChanged:
      myDebuggerWidthLabel->setValue(myDebuggerWidthSlider->getValue());
      break;

    case kDHeightChanged:
      myDebuggerHeightLabel->setValue(myDebuggerHeightSlider->getValue());
      break;

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
void DeveloperDialog::handleRewind()
{
  bool enable = myContinuousRewindWidget->getState();

  myStateSizeWidget->setEnabled(enable);
  myStateSizeLabelWidget->setEnabled(enable);

  myStateIntervalWidget->setEnabled(enable);
  myStateIntervalLabelWidget->setEnabled(enable);

  myStateHorizonWidget->setEnabled(enable);
  myStateHorizonLabelWidget->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSize()
{
  bool found = false;
  uInt64 size = myStateSizeWidget->getValue();
  uInt64 interval = myStateIntervalWidget->getValue();
  uInt64 horizon = myStateHorizonWidget->getValue();
  Int32 i;

  myStateSizeLabelWidget->setValue(size);
  // adapt horizon and interval
  do
  {
    for(i = horizon; i < NUM_HORIZONS; ++i)
    {
      if(size * INTERVAL_CYCLES[interval] <= HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      interval--;
  } while(!found);

  myStateHorizonWidget->setValue(i);
  myStateHorizonLabelWidget->setLabel(HORIZONS[i]);
  myStateIntervalWidget->setValue(interval);
  myStateIntervalLabelWidget->setLabel(INTERVALS[interval]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleInterval()
{
  bool found = false;
  uInt64 size = myStateSizeWidget->getValue();
  uInt64 interval = myStateIntervalWidget->getValue();
  uInt64 horizon = myStateHorizonWidget->getValue();
  Int32 i;

  myStateIntervalLabelWidget->setLabel(INTERVALS[interval]);
  // adapt horizon and size
  do
  {
    for(i = horizon; i < NUM_HORIZONS; ++i)
    {
      if(size * INTERVAL_CYCLES[interval] <= HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      size -= myStateSizeWidget->getStepValue();
  } while(!found);

  myStateHorizonWidget->setValue(i);
  myStateHorizonLabelWidget->setLabel(HORIZONS[i]);
  myStateSizeWidget->setValue(size);
  myStateSizeLabelWidget->setValue(myStateSizeWidget->getValue());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleHorizon()
{
  bool found = false;
  uInt64 size = myStateSizeWidget->getValue();
  uInt64 interval = myStateIntervalWidget->getValue();
  uInt64 horizon = myStateHorizonWidget->getValue();
  Int32 i;

  myStateHorizonLabelWidget->setLabel(HORIZONS[horizon]);
  // adapt interval and size
  do
  {
    for(i = interval; i >= 0; --i)
    {
      if(size * INTERVAL_CYCLES[i] <= HORIZON_CYCLES[horizon])
      {
        found = true;
        break;
      }
    }
    if(!found)
      size -= myStateSizeWidget->getStepValue();
  } while(!found);

  myStateIntervalWidget->setValue(i);
  myStateIntervalLabelWidget->setLabel(INTERVALS[i]);
  myStateSizeWidget->setValue(size);
  myStateSizeLabelWidget->setValue(myStateSizeWidget->getValue());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(int idx, int color)
{
  if(idx < 0 || idx > 5)
    return;

  if(!instance().hasConsole())
  {
    myDbgColour[idx]->clearFlags(WIDGET_ENABLED);
    myDbgColourSwatch[idx]->clearFlags(WIDGET_ENABLED);
    return;
  }

  static constexpr int dbg_color[2][6] = {
    { TIA::FixedColor::NTSC_RED,
    TIA::FixedColor::NTSC_ORANGE,
    TIA::FixedColor::NTSC_YELLOW,
    TIA::FixedColor::NTSC_GREEN,
    TIA::FixedColor::NTSC_BLUE,
    TIA::FixedColor::NTSC_PURPLE
    },
    { TIA::FixedColor::PAL_RED,
    TIA::FixedColor::PAL_ORANGE,
    TIA::FixedColor::PAL_YELLOW,
    TIA::FixedColor::PAL_GREEN,
    TIA::FixedColor::PAL_BLUE,
    TIA::FixedColor::PAL_PURPLE
    }
  };
  int mode = instance().console().tia().frameLayout() == FrameLayout::ntsc ? 0 : 1;
  myDbgColourSwatch[idx]->setColor(dbg_color[mode][color]);
  myDbgColour[idx]->setSelectedIndex(color);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(const string& colors)
{
  for(int i = 0; i < 6; ++i)
  {
    switch(colors[i])
    {
      case 'r':  handleDebugColours(i, 0);  break;
      case 'o':  handleDebugColours(i, 1);  break;
      case 'y':  handleDebugColours(i, 2);  break;
      case 'g':  handleDebugColours(i, 3);  break;
      case 'b':  handleDebugColours(i, 4);  break;
      case 'p':  handleDebugColours(i, 5);  break;
      default:                              break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleFontSize()
{
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
  if(minW > myDebuggerWidthSlider->getValue())
  {
    myDebuggerWidthSlider->setValue(minW);
    myDebuggerWidthLabel->setValue(minW);
  }

  myDebuggerHeightSlider->setMinValue(minH);
  if(minH > myDebuggerHeightSlider->getValue())
  {
    myDebuggerHeightSlider->setValue(minH);
    myDebuggerHeightLabel->setValue(minH);
  }
}
