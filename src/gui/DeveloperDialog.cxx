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
  const int VBORDER = 4;
  //const int HBORDER = 8;
  int xpos, ypos, tabID;
  StringList actions;

  // Set real dimensions
  _w = std::min(51 * fontWidth + 10, max_w);
  _h = std::min(14 * (lineHeight + 4) + 14, max_h);

  WidgetArray wid;

  /*ypos = VBORDER;
  myDevSettings0 = new CheckboxWidget(this, font, HBORDER, ypos, "Enable developer settings", kDevSettings0);
  wid.push_back(myDevSettings0);
  addToFocusList(wid);*/

  // The tab widget
  //ypos += lineHeight + 2;
  xpos = 2; ypos = VBORDER;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2 * xpos, _h - buttonHeight - 16 - ypos);
  addTabWidget(myTab);

  addEmulationTab(font);
  //addVideoTab(font);
  //addUITab(font);
  addStatesTab(font);
  addDebuggerTab(font);
  addDefaultOKCancelButtons(font);

  // Activate the first tab
  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addEmulationTab(const GUI::Font& font)
{
  const int VGAP = 4;
  const int INDENT = 13+3;
  const int HBORDER = 10;
  const int VBORDER = 8;

  int ypos, tabID;
  int lineHeight = font.getLineHeight();
  StringList actions;
  WidgetArray wid;
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();

  tabID = myTab->addTab(" Emulation ");

  ypos = VBORDER;
  myDevSettings0 = new CheckboxWidget(myTab, font, HBORDER, ypos, "Enable developer settings", kDevSettings0);
  wid.push_back(myDevSettings0);

  ypos += lineHeight + VGAP;

  // Randomize items
  myLoadingROMLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT*1, ypos, "When loading a ROM:", kTextAlignLeft);
  wid.push_back(myLoadingROMLabel);

  ypos += lineHeight + VGAP;
  myRandomBank = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Random startup bank (TODO)");
  wid.push_back(myRandomBank);

  // Randomize RAM
  ypos += lineHeight + VGAP;
  myRandomizeRAM = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                      "Randomize zero-page and extended RAM", kRandRAMID);
  wid.push_back(myRandomizeRAM);

  // Randomize CPU
  ypos += lineHeight + VGAP;
  int lwidth = font.getStringWidth("Randomize CPU ");
  myRandomizeCPULabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Randomize CPU ");
  wid.push_back(myRandomizeCPULabel);

  int xpos = myRandomizeCPULabel->getRight() + 10;
  const char* const cpuregs[] = { "SP", "A", "X", "Y", "PS" };
  for(int i = 0; i < 5; ++i)
  {
    myRandomizeCPU[i] = new CheckboxWidget(myTab, font, xpos, ypos + 2,
                                           cpuregs[i], kRandCPUID);
    //myRandomizeCPU[i]->setID(kRandCPUID);
    //myRandomizeCPU[i]->setTarget(this);
    //addFocusWidget(myRandomizeCPU[i]);
    wid.push_back(myRandomizeCPU[i]);
    xpos += CheckboxWidget::boxSize() + font.getStringWidth("XX") + 20;
  }

  ypos += lineHeight + VGAP;
  /*myThumbException = new CheckboxWidget(myTab, font, HBORDER + INDENT, ypos + 1, "Thumb ARM emulation can throw an exception");
  wid.push_back(myThumbException);*/

  //ypos += (lineHeight + VGAP) * 2;
  myColorLoss = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "PAL color-loss");
  wid.push_back(myColorLoss);

  // TV jitter effect
  ypos += lineHeight + VGAP;
  myTVJitter = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "Jitter/Roll Effect", kTVJitter);
  wid.push_back(myTVJitter);
  myTVJitterRec = new SliderWidget(myTab, font,
                                   myTVJitter->getRight()+ 16, ypos - 1,
                                   8 * fontWidth, lineHeight, "Recovery ",
                                   font.getStringWidth("Recovery "), kTVJitterChanged);
  myTVJitterRec->setMinValue(1); myTVJitterRec->setMaxValue(20);
  wid.push_back(myTVJitterRec);

  myTVJitterRecLabel = new StaticTextWidget(myTab, font,
                                         myTVJitterRec->getRight() + 4, myTVJitterRec->getTop(),
                                         5 * fontWidth, fontHeight, "", kTextAlignLeft);
  myTVJitterRecLabel->setFlags(WIDGET_CLEARBG);
  wid.push_back(myTVJitterRecLabel);

  // debug colors
  ypos += lineHeight + VGAP;
  myDebugColors = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "Debug Colors");
  wid.push_back(myDebugColors);

  // How to handle undriven TIA pins
  ypos += lineHeight + VGAP;
  myUndrivenPins = new CheckboxWidget(myTab, font, HBORDER + INDENT*1, ypos + 1,
                                      "Drive unused TIA pins randomly on a read/peek");
  wid.push_back(myUndrivenPins);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
}

/*// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addVideoTab(const GUI::Font& font)
{
  int tabID = myTab->addTab("Video");
  WidgetArray wid;
  // PAL color-loss effect
  myColorLoss = new CheckboxWidget(myTab, font, xpos, ypos, "PAL color-loss");
  wid.push_back(myColorLoss);
  ypos += lineHeight + 4;

  addToFocusList(wid, myTab, tabID);
}*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addStatesTab(const GUI::Font& font)
{
  const int VGAP = 4;
  const int INDENT = 13 + 3;
  const int HBORDER = 10;
  const int VBORDER = 8;

  int ypos;
  int lineHeight = font.getLineHeight();
  StringList actions;
  WidgetArray wid;
  int fontWidth = font.getMaxCharWidth(), fontHeight = font.getFontHeight();

  int tabID = myTab->addTab("States");

  ypos = VBORDER;
  myDevSettings1 = new CheckboxWidget(myTab, font, HBORDER, ypos, "Enable developer settings", kDevSettings1);
  wid.push_back(myDevSettings1);
  ypos += lineHeight + VGAP;

  new StaticTextWidget(myTab, font, HBORDER + INDENT, ypos, "TODO: Rewind-States");

  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebuggerTab(const GUI::Font& font)
{
  int tabID = myTab->addTab(" Debugger UI ");

#ifdef DEBUGGER_SUPPORT
  WidgetArray wid;
  VariantList items;
  int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight(),
    buttonHeight = font.getLineHeight() + 4,
    lineHeight = font.getLineHeight();
  const int VGAP = 4;
  const int VBORDER = 8;
  const int HBORDER = 10;
  int xpos, ypos;
  int lwidth, pwidth = font.getStringWidth("Standard");
  ButtonWidget* b;
  const GUI::Size& ds = instance().frameBuffer().desktopSize();

  lwidth = font.getStringWidth("Debugger Height ");
  xpos = HBORDER;
  ypos = VBORDER;

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                           lineHeight, "Debugger Width  ",
                                           lwidth, kDWidthChanged);
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

  myDebuggerHeightSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                            lineHeight, "Debugger Height ",
                                            lwidth, kDHeightChanged);
  myDebuggerHeightSlider->setMinValue(DebuggerDialog::kSmallFontMinH);
  myDebuggerHeightSlider->setMaxValue(ds.h);
  myDebuggerHeightSlider->setStepValue(10);
  wid.push_back(myDebuggerHeightSlider);
  myDebuggerHeightLabel =
    new StaticTextWidget(myTab, font,
                         xpos + myDebuggerHeightSlider->getWidth() + 4,
                         ypos + 1, 4 * fontWidth, fontHeight, "", kTextAlignLeft);
  myDebuggerHeightLabel->setFlags(WIDGET_CLEARBG);

  // Add minimum window size buttons for different fonts
  ypos += lineHeight + VGAP * 2;
  StaticTextWidget* t = new StaticTextWidget(myTab, font, HBORDER, ypos + 3, "Reset debugger size for");

  int fbwidth = font.getStringWidth("Medium font") + 20;
  //xpos = (_w - fbwidth - 2 * VBORDER) / 2;  ypos += 2 * lineHeight + 4;
  xpos = t->getRight() + 8;
  b = new ButtonWidget(myTab, font, xpos, ypos, fbwidth, buttonHeight,
                       "Small font", kDSmallSize);
  wid.push_back(b);
  ypos += b->getHeight() + VGAP;
  b = new ButtonWidget(myTab, font, xpos, ypos, fbwidth, buttonHeight,
                       "Medium font", kDMediumSize);
  wid.push_back(b);
  ypos += b->getHeight() + VGAP;
  b = new ButtonWidget(myTab, font, xpos, ypos, fbwidth, buttonHeight,
                       "Large font", kDLargeSize);
  wid.push_back(b);
  ypos += b->getHeight() + VGAP * 4;

  // Font style (bold label vs. text, etc)
  lwidth = font.getStringWidth("Font Style ");
  pwidth = font.getStringWidth("Bold non-labels only");
  xpos = VBORDER;
  items.clear();
  VarList::push_back(items, "All Normal font", "0");
  VarList::push_back(items, "Bold labels only", "1");
  VarList::push_back(items, "Bold non-labels only", "2");
  VarList::push_back(items, "All Bold font", "3");
  myDebuggerFontStyle =
    new PopUpWidget(myTab, font, HBORDER, ypos + 1, pwidth, lineHeight, items,
                    "Font Style ", lwidth);
  wid.push_back(myDebuggerFontStyle);

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

/*// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addUITab(const GUI::Font& font)
{
  int tabID = myTab->addTab("UI");
  WidgetArray wid;


  addToFocusList(wid, myTab, tabID);
}*/

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
  myDevSettings0->setState(instance().settings().getBool("dev.settings"));

  myRandomBank->setState(instance().settings().getBool("dev.bankrandom"));
  myRandomizeRAM->setState(instance().settings().getBool("dev.ramrandom"));

  const string& cpurandom = instance().settings().getString("dev.cpurandom");
  const char* const cpuregs[] = { "S", "A", "X", "Y", "P" };
  for(int i = 0; i < 5; ++i)
    myRandomizeCPU[i]->setState(BSPF::containsIgnoreCase(cpurandom, cpuregs[i]));
  //myThumbException->setState(instance().settings().getBool("dev.thumb.trapfatal"));

  // PAL color-loss effect
  myColorLoss->setState(instance().settings().getBool("dev.colorloss"));

  myTVJitter->setState(instance().settings().getBool("dev.tv.jitter"));
  myTVJitterRec->setValue(instance().settings().getInt("dev.tv.jitter_recovery"));

  myDebugColors->setState(instance().settings().getBool("dev.debugcolors"));
  // Undriven TIA pins
  myUndrivenPins->setState(instance().settings().getBool("dev.tiadriven"));

  enableOptions();

#ifdef DEBUGGER_SUPPORT
  uInt32 w, h;

  // Debugger size
  const GUI::Size& ds = instance().settings().getSize("dbg.res");
  w = ds.w; h = ds.h;
  w = std::max(w, uInt32(DebuggerDialog::kSmallFontMinW));
  h = std::max(h, uInt32(DebuggerDialog::kSmallFontMinH));
  w = std::min(w, ds.w);
  h = std::min(h, ds.h);

  myDebuggerWidthSlider->setValue(w);
  myDebuggerWidthLabel->setValue(w);
  myDebuggerHeightSlider->setValue(h);
  myDebuggerHeightLabel->setValue(h);

  // Debugger font style
  int style = instance().settings().getInt("dbg.fontstyle");
  myDebuggerFontStyle->setSelected(style, "0");
#endif

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveConfig()
{
  //TODO
  // - bankrandom (not implemented yet)
  // - thumbexception (commandline only yet)
  // - debugcolors (no effect yet)

  bool devSettings = myDevSettings0->getState();
  instance().settings().setValue("dev.settings", devSettings);

  instance().settings().setValue("dev.bankrandom", myRandomBank->getState());
  instance().settings().setValue("dev.ramrandom", myRandomizeRAM->getState());

  string cpurandom;
  const char* const cpuregs[] = { "S", "A", "X", "Y", "P" };
  for(int i = 0; i < 5; ++i)
    if(myRandomizeCPU[i]->getState())
      cpurandom += cpuregs[i];
  instance().settings().setValue("dev.cpurandom", cpurandom);
  //instance().settings().setValue("dev.thumb.trapfatal", myThumbException->getState());

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
      instance().console().tia().toggleJitter(instance().settings().getBool("tv.jitter") ? 1 : 0);
      instance().console().tia().setJitterRecoveryFactor(instance().settings().getInt("tv.jitter_recovery"));
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
      instance().console().toggleColorLoss(instance().settings().getBool("colorloss"));
  }

  instance().settings().setValue("dev.tiadriven", myUndrivenPins->getState());

  // Finally, issue a complete framebuffer re-initialization
  instance().createFrameBuffer();

#ifdef DEBUGGER_SUPPORT
  // Debugger size
  instance().settings().setValue("dbg.res",
                                 GUI::Size(myDebuggerWidthSlider->getValue(),
                                 myDebuggerHeightSlider->getValue()));

  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
                                 myDebuggerFontStyle->getSelectedTag().toString());
#endif
}

void DeveloperDialog::setDefaults()
{
  myDevSettings0->setState(false);

  switch(myTab->getActiveTab())
  {
    case 0:
      myRandomBank->setState(true);
      myRandomizeRAM->setState(true);
      for(int i = 0; i < 5; ++i)
        myRandomizeCPU[i]->setState(true);
      //myThumbException->setState(false);

      // PAL color-loss effect
      myColorLoss->setState(true);
      // jitter
      myTVJitter->setState(true);
      myTVJitterRec->setValue(2);
      // debug colors
      myDebugColors->setState(false);
      // Undriven TIA pins
      myUndrivenPins->setState(true);

      enableOptions();
      handleTVJitterChange(false);
      handleDebugColors();
      break;

    case 1:  // States
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
      myDebuggerFontStyle->setSelected("0");
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
    case kDevSettings0:
      myDevSettings1->setState(myDevSettings0->getState());
      enableOptions();
      break;
    case kDevSettings1:
      myDevSettings0->setState(myDevSettings1->getState());
      enableOptions();
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

#ifdef DEBUGGER_SUPPORT
    case kDWidthChanged:
      myDebuggerWidthLabel->setValue(myDebuggerWidthSlider->getValue());
      break;

    case kDHeightChanged:
      myDebuggerHeightLabel->setValue(myDebuggerHeightSlider->getValue());
      break;

    case kDSmallSize:
      myDebuggerWidthSlider->setValue(DebuggerDialog::kSmallFontMinW);
      myDebuggerWidthLabel->setValue(DebuggerDialog::kSmallFontMinW);
      myDebuggerHeightSlider->setValue(DebuggerDialog::kSmallFontMinH);
      myDebuggerHeightLabel->setValue(DebuggerDialog::kSmallFontMinH);
      break;

    case kDMediumSize:
      myDebuggerWidthSlider->setValue(DebuggerDialog::kMediumFontMinW);
      myDebuggerWidthLabel->setValue(DebuggerDialog::kMediumFontMinW);
      myDebuggerHeightSlider->setValue(DebuggerDialog::kMediumFontMinH);
      myDebuggerHeightLabel->setValue(DebuggerDialog::kMediumFontMinH);
      break;

    case kDLargeSize:
      myDebuggerWidthSlider->setValue(DebuggerDialog::kLargeFontMinW);
      myDebuggerWidthLabel->setValue(DebuggerDialog::kLargeFontMinW);
      myDebuggerHeightSlider->setValue(DebuggerDialog::kLargeFontMinH);
      myDebuggerHeightLabel->setValue(DebuggerDialog::kLargeFontMinH);
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
void DeveloperDialog::enableOptions()
{
  bool enable = myDevSettings0->getState();

  // CPU
  myLoadingROMLabel->setEnabled(enable);
  myRandomBank->setEnabled(false/*enable*/);
  myRandomizeRAM->setEnabled(enable);
  myRandomizeCPULabel->setEnabled(enable);
  for(int i = 0; i < 5; ++i)
    myRandomizeCPU[i]->setEnabled(enable);
  //myThumbException->setEnabled(enable);

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
  if(instance().hasConsole() && myDevSettings0->getState())
  {
    bool fixed = instance().console().tia().usingFixedColors();
    if(fixed != myDebugColors->getState())
      instance().console().tia().toggleFixedColors();
  }
}
