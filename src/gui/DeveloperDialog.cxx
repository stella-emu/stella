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
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#ifdef DEBUGGER_SUPPORT
#include "DebuggerDialog.hxx"
#endif
#include "Console.hxx"
#include "TIA.hxx"
#include "OSystem.hxx"
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
  _w = std::min(51 * fontWidth + 10, max_w);
  _h = std::min(15 * (lineHeight + 4) + 14, max_h);

  // The tab widget
  xpos = 2; ypos = 4;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2 * xpos, _h - buttonHeight - 16 - ypos);
  addTabWidget(myTab);

  addEmulationTab(font);
  addStatesTab(font);
  addDebuggerTab(font);
  addDefaultOKCancelButtons(font);

  // Activate the first tab
  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addEmulationTab(const GUI::Font& font)
{
  const int HBORDER = 10;
  const int INDENT = 16;
  const int VBORDER = 8;
  const int VGAP = 4;
  int ypos = VBORDER;
  int lineHeight = font.getLineHeight();
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab(" Emulation ");

  myDevSettings = new CheckboxWidget(myTab, font, HBORDER, ypos, "Enable developer settings", kDevSettings);
  wid.push_back(myDevSettings);
  ypos += lineHeight + VGAP;

  myFrameStats = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos, "Show frame statistics");
  wid.push_back(myFrameStats);
  ypos += lineHeight + VGAP;

  // 2600/7800 mode
  items.clear();
  VarList::push_back(items, "Atari 2600", "2600");
  VarList::push_back(items, "Atari 7800", "7800");
  int lwidth = font.getStringWidth("Console ");
  int pwidth = font.getStringWidth("Atari 2600");

  myConsole = new PopUpWidget(myTab, font, HBORDER + INDENT * 1, ypos, pwidth, lineHeight, items, "Console ", lwidth, kConsole);
  wid.push_back(myConsole);
  ypos += lineHeight + VGAP;

  // Randomize items
  myLoadingROMLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT*1, ypos, "When loading a ROM:", kTextAlignLeft);
  wid.push_back(myLoadingROMLabel);
  ypos += lineHeight + VGAP;

  myRandomBank = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Random startup bank");
  wid.push_back(myRandomBank);
  ypos += lineHeight + VGAP;

  // Randomize RAM
  myRandomizeRAM = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                      "Randomize zero-page and extended RAM", kRandRAMID);
  wid.push_back(myRandomizeRAM);
  ypos += lineHeight + VGAP;

  // Randomize CPU
  lwidth = font.getStringWidth("Randomize CPU ");
  myRandomizeCPULabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Randomize CPU ");
  wid.push_back(myRandomizeCPULabel);

  int xpos = myRandomizeCPULabel->getRight() + 10;
  const char* const cpuregs[] = { "SP", "A", "X", "Y", "PS" };
  for(int i = 0; i < 5; ++i)
  {
    myRandomizeCPU[i] = new CheckboxWidget(myTab, font, xpos, ypos + 2,
                                           cpuregs[i], kRandCPUID);
    wid.push_back(myRandomizeCPU[i]);
    xpos += CheckboxWidget::boxSize() + font.getStringWidth("XX") + 20;
  }
  ypos += lineHeight + VGAP;

  // debug colors
  myDebugColors = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1, "Debug colors");
  wid.push_back(myDebugColors);
  ypos += lineHeight + VGAP;

  myColorLoss = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "PAL color-loss");
  wid.push_back(myColorLoss);
  ypos += lineHeight + VGAP;

  // TV jitter effect
  myTVJitter = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "Jitter/Roll effect", kTVJitter);
  wid.push_back(myTVJitter);
  myTVJitterRec = new SliderWidget(myTab, font,
                                   myTVJitter->getRight()+ 16, ypos - 1,
                                   8 * fontWidth, lineHeight, "recovery ",
                                   font.getStringWidth("recovery "), kTVJitterChanged);
  myTVJitterRec->setMinValue(1); myTVJitterRec->setMaxValue(20);
  wid.push_back(myTVJitterRec);

  myTVJitterRecLabel = new StaticTextWidget(myTab, font,
                                         myTVJitterRec->getRight() + 4, myTVJitterRec->getTop(),
                                         5 * fontWidth, fontHeight, "", kTextAlignLeft);
  myTVJitterRecLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myTVJitterRecLabel);
  ypos += lineHeight + VGAP;

  // How to handle undriven TIA pins
  myUndrivenPins = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1,
                                      "Drive unused TIA pins randomly on a read/peek");
  wid.push_back(myUndrivenPins);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addStatesTab(const GUI::Font& font)
{
  const int HBORDER = 10;
  const int INDENT = 16;
  const int VBORDER = 8;
  const int VGAP = 4;
  int ypos = VBORDER;
  int lineHeight = font.getLineHeight();
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();
  WidgetArray wid;
  int tabID = myTab->addTab("States");

  myContinuousRewind = new CheckboxWidget(myTab, font, HBORDER, ypos + 1, "Continuous rewind", kRewind);
  wid.push_back(myContinuousRewind);
  ypos += lineHeight + VGAP;

  int sWidth = font.getMaxCharWidth() * 8;
  myStateSize = new SliderWidget(myTab, font, HBORDER + INDENT, ypos - 1, sWidth, lineHeight,
                                 "Buffer size (*) ", 0, kSizeChanged);
  myStateSize->setMinValue(100);
  myStateSize->setMaxValue(1000);
  myStateSize->setStepValue(100);
  wid.push_back(myStateSize);
  myStateSizeLabel = new StaticTextWidget(myTab, font, myStateSize->getRight() + 4, myStateSize->getTop() + 2, "100 ");

  ypos += lineHeight + VGAP;
  myStateInterval = new SliderWidget(myTab, font, HBORDER + INDENT, ypos - 1, sWidth, lineHeight,
                                     "Interval        ", 0, kIntervalChanged);

  myStateInterval->setMinValue(0);
  myStateInterval->setMaxValue(NUM_INTERVALS - 1);
  wid.push_back(myStateInterval);
  myStateIntervalLabel = new StaticTextWidget(myTab, font, myStateInterval->getRight() + 4, myStateInterval->getTop() + 2, "50 scanlines");

  ypos += lineHeight + VGAP;
  myStateHorizon = new SliderWidget(myTab, font, HBORDER + INDENT, ypos - 1, sWidth, lineHeight,
                                    "Horizon         ", 0, kHorizonChanged);
  myStateHorizon->setMinValue(0);
  myStateHorizon->setMaxValue(NUM_HORIZONS - 1);
  wid.push_back(myStateHorizon);
  myStateHorizonLabel = new StaticTextWidget(myTab, font, myStateHorizon->getRight() + 4, myStateHorizon->getTop() + 2, "~60 minutes");

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  StaticTextWidget* t = new StaticTextWidget(myTab, infofont, HBORDER, _h - lineHeight * 4 - 10, "(*) Requires application restart");

  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebuggerTab(const GUI::Font& font)
{
  int tabID = myTab->addTab(" Debugger ");

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
  new StaticTextWidget(myTab, infofont, HBORDER, _h - lineHeight * 4 - 10, "(*) Changes require application restart");

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
void DeveloperDialog::loadConfig()
{
  myDevSettings->setState(instance().settings().getBool("dev.settings"));

  myFrameStats->setState(instance().settings().getBool("dev.stats"));
  myConsole->setSelectedIndex(instance().settings().getString("dev.console") == "7800" ? 1 : 0);
  myRandomBank->setState(instance().settings().getBool("dev.bankrandom"));
  myRandomizeRAM->setState(instance().settings().getBool("dev.ramrandom"));

  const string& cpurandom = instance().settings().getString("dev.cpurandom");
  const char* const cpuregs[] = { "S", "A", "X", "Y", "P" };
  for(int i = 0; i < 5; ++i)
    myRandomizeCPU[i]->setState(BSPF::containsIgnoreCase(cpurandom, cpuregs[i]));

  // PAL color-loss effect
  myColorLoss->setState(instance().settings().getBool("dev.colorloss"));

  myTVJitter->setState(instance().settings().getBool("dev.tv.jitter"));
  myTVJitterRec->setValue(instance().settings().getInt("dev.tv.jitter_recovery"));

  myDebugColors->setState(instance().settings().getBool("dev.debugcolors"));
  // Undriven TIA pins
  myUndrivenPins->setState(instance().settings().getBool("dev.tiadriven"));

  handleDeveloperOptions();

  myContinuousRewind->setState(instance().settings().getBool("dev.rewind"));
  myStateSize->setValue(instance().settings().getInt("dev.rewind.size"));
  myStateInterval->setValue(instance().settings().getInt("dev.rewind.interval"));
  myStateHorizon->setValue(instance().settings().getInt("dev.rewind.horizon"));

  handleRewind();
  handleSize();
  handleInterval();
  handleHorizon();

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
  handleFontSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveConfig()
{
  bool devSettings = myDevSettings->getState();
  instance().settings().setValue("dev.settings", devSettings);

  instance().settings().setValue("dev.stats", myFrameStats->getState());
  instance().frameBuffer().showFrameStats(devSettings && myFrameStats->getState());

  bool is7800 = myConsole->getSelected() == 1;
  instance().settings().setValue("dev.console", is7800 ? "7800" : "2600");

  instance().settings().setValue("dev.bankrandom", myRandomBank->getState());
  instance().settings().setValue("dev.ramrandom", myRandomizeRAM->getState());

  string cpurandom;
  const char* const cpuregs[] = { "S", "A", "X", "Y", "P" };
  for(int i = 0; i < 5; ++i)
    if(myRandomizeCPU[i]->getState())
      cpurandom += cpuregs[i];
  instance().settings().setValue("dev.cpurandom", cpurandom);

  // jitter
  instance().settings().setValue("dev.tv.jitter", myTVJitter->getState());
  instance().settings().setValue("dev.tv.jitter_recovery", myTVJitterRecLabel->getLabel());
  if(instance().hasConsole())
  {
    if (devSettings)
    {
      instance().console().tia().toggleJitter(myTVJitter->getState() ? 1 : 0);
      instance().console().tia().setJitterRecoveryFactor(myTVJitterRec->getValue());
    }
    else
    {
      instance().console().tia().toggleJitter(0);
    }
  }

  instance().settings().setValue("dev.debugcolors", myDebugColors->getState());
  handleDebugColors();

  // PAL color loss
  instance().settings().setValue("dev.colorloss", myColorLoss->getState());
  if(instance().hasConsole())
  {
    if(devSettings)
      instance().console().toggleColorLoss(myColorLoss->getState());
    else
      instance().console().toggleColorLoss(false);
  }

  instance().settings().setValue("dev.tiadriven", myUndrivenPins->getState());

  // Finally, issue a complete framebuffer re-initialization
  //instance().createFrameBuffer();

  instance().settings().setValue("dev.rewind", myContinuousRewind->getState());
  instance().settings().setValue("dev.rewind.size", myStateSize->getValue());
  instance().settings().setValue("dev.rewind.interval", myStateInterval->getValue());
  instance().settings().setValue("dev.rewind.horizon", myStateHorizon->getValue());
  // TODO: update RewindManager

  // define interval growth factor
  uInt32 size = myStateSize->getValue();
  uInt64 horizon = HORIZON_CYCLES[myStateHorizon->getValue()];
  double factor, minFactor = 1, maxFactor = 2;

  while(true)
  {
    double interval = INTERVAL_CYCLES[myStateInterval->getValue()];
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

#ifdef DEBUGGER_SUPPORT
  // Debugger size
  instance().settings().setValue("dbg.res",
                                 GUI::Size(myDebuggerWidthSlider->getValue(),
                                 myDebuggerHeightSlider->getValue()));

  // Debugger font size
  instance().settings().setValue("dbg.fontsize", myDebuggerFontSize->getSelectedTag().toString());

  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
                                 myDebuggerFontStyle->getSelectedTag().toString());
#endif
}

void DeveloperDialog::setDefaults()
{
  myDevSettings->setState(false);

  switch(myTab->getActiveTab())
  {
    case 0:
      myFrameStats->setState(true);
      myConsole->setSelectedIndex(0);
      myRandomBank->setState(true);
      myRandomizeRAM->setState(true);
      for(int i = 0; i < 5; ++i)
        myRandomizeCPU[i]->setState(true);

      // PAL color-loss effect
      myColorLoss->setState(true);
      // jitter
      myTVJitter->setState(true);
      myTVJitterRec->setValue(2);
      // debug colors
      myDebugColors->setState(false);
      // Undriven TIA pins
      myUndrivenPins->setState(true);

      handleDeveloperOptions();
      handleTVJitterChange(false);
      handleDebugColors();
      break;

    case 1:  // States
      myContinuousRewind->setState(false);
      myStateSize->setValue(100);
      myStateInterval->setValue(2);
      myStateHorizon->setValue(3);
      handleRewind();
      handleSize();
      handleInterval();
      handleHorizon();
      break;

    case 2:  // Debugger options
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
    case kDevSettings:
      handleDeveloperOptions();
      break;

    case kConsole:
      handleConsole();
        break;

    case kTVJitter:
      handleTVJitterChange(myTVJitter->getState());
      break;

    case kTVJitterChanged:
      myTVJitterRecLabel->setValue(myTVJitterRec->getValue());
      break;

    case kPPinCmd:
      instance().console().tia().driveUnusedPinsRandom(myUndrivenPins->getState());
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
void DeveloperDialog::handleDeveloperOptions()
{
  bool enable = myDevSettings->getState();

  myFrameStats->setEnabled(enable);
  myConsole->setEnabled(enable);
  // CPU
  myLoadingROMLabel->setEnabled(enable);
  myRandomBank->setEnabled(enable);
  myRandomizeRAM->setEnabled(enable);
  myRandomizeCPULabel->setEnabled(enable);
  for(int i = 0; i < 5; ++i)
    myRandomizeCPU[i]->setEnabled(enable);
  handleConsole();

  // TIA
  myColorLoss->setEnabled(enable);
  myTVJitter->setEnabled(enable);
  handleTVJitterChange(enable && myTVJitter->getState());
  myDebugColors->setEnabled(enable);
  myUndrivenPins->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTVJitterChange(bool enable)
{
  myTVJitterRec->setEnabled(enable);
  myTVJitterRecLabel->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColors()
{
  if(instance().hasConsole() && myDevSettings->getState())
  {
    bool fixed = instance().console().tia().usingFixedColors();
    if(fixed != myDebugColors->getState())
      instance().console().tia().toggleFixedColors();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleConsole()
{
  bool is7800 = myConsole->getSelected() == 1;
  bool enable = myDevSettings->getState();

  myRandomizeRAM->setEnabled(enable && !is7800);
  if(is7800)
    myRandomizeRAM->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleRewind()
{
  bool enable = myContinuousRewind->getState();

  myStateSize->setEnabled(enable);
  myStateSizeLabel->setEnabled(enable);

  myStateInterval->setEnabled(enable);
  myStateIntervalLabel->setEnabled(enable);

  myStateHorizon->setEnabled(enable);
  myStateHorizonLabel->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSize()
{
  bool found = false;
  uInt64 size = myStateSize->getValue();
  uInt64 interval = myStateInterval->getValue();
  uInt64 horizon = myStateHorizon->getValue();
  Int32 i;

  myStateSizeLabel->setValue(size);
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

  myStateHorizon->setValue(i);
  myStateHorizonLabel->setLabel(HORIZONS[i]);
  myStateInterval->setValue(interval);
  myStateIntervalLabel->setLabel(INTERVALS[interval]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleInterval()
{
  bool found = false;
  uInt64 size = myStateSize->getValue();
  uInt64 interval = myStateInterval->getValue();
  uInt64 horizon = myStateHorizon->getValue();
  Int32 i;

  myStateIntervalLabel->setLabel(INTERVALS[interval]);
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
      size -= myStateSize->getStepValue();
  } while(!found);

  myStateHorizon->setValue(i);
  myStateHorizonLabel->setLabel(HORIZONS[i]);
  myStateSize->setValue(size);
  myStateSizeLabel->setValue(myStateSize->getValue());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleHorizon()
{
  bool found = false;
  uInt64 size = myStateSize->getValue();
  uInt64 interval = myStateInterval->getValue();
  uInt64 horizon = myStateHorizon->getValue();
  Int32 i;

  myStateHorizonLabel->setLabel(HORIZONS[horizon]);
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
      size -= myStateSize->getStepValue();
  } while(!found);

  myStateInterval->setValue(i);
  myStateIntervalLabel->setLabel(INTERVALS[i]);
  myStateSize->setValue(size);
  myStateSizeLabel->setValue(myStateSize->getValue());
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
