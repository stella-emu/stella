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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "PointingDevice.hxx"
#include "SaveKey.hxx"
#include "AtariVox.hxx"
#include "Settings.hxx"
#include "DevSettingsHandler.hxx"
#include "PopUpWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "ColorWidget.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "Font.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "JitterEmulation.hxx"
#include "EventHandler.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "M6502.hxx"
#include "CartELF.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "DebuggerDialog.hxx"
#endif
#include "DeveloperDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DeveloperDialog::DeveloperDialog(OSystem& osystem, DialogContainer& parent,
                                 const GUI::Font& font)
  : Dialog(osystem, parent, font, "Developer settings"),
    DevSettingsHandler(osystem)
{
  // Widgets are only created here (at placeholder geometry); layout() sizes the
  // dialog and positions everything from the current font.  The tab bar geometry
  // is recomputed in layout() via TabWidget::updateTabSizes().
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTab = new TabWidget(this, font, 0, 0, 1, 1);
  addTabWidget(myTab);

  addEmulationTab(font);
  addTiaTab(font);
  addVideoTab(font);
  addTimeMachineTab(font);
  addDebuggerTab(font);

  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  // Activate the first tab
  myTab->setActiveTab(0);

  setHelpAnchor("Debugger");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DeveloperDialog::~DeveloperDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::layout()
{
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Size the dialog from the current font
  _w = 61 * fontWidth + HBORDER * 2;
  _h = _th + VGAP * 3 + lineHeight + 15 * (lineHeight + VGAP)
       + buttonHeight + VBORDER * 3;

  // Position/size the tab widget below the title bar, then recompute its tab-bar
  // geometry for the current font/width
  constexpr int xpos = 2;
  myTab->setPos(xpos, VGAP + _th);
  myTab->setWidth(_w - 2 * xpos);
  myTab->setHeight(_h - _th - VGAP - buttonHeight - VBORDER * 2);
  myTab->updateTabSizes();

  layoutEmulationTab();
  layoutTiaTab();
  layoutVideoTab();
  layoutTimeMachineTab();
  layoutDebuggerTab();

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addEmulationTab(const GUI::Font& font)
{
  const int lineHeight = Dialog::lineHeight();
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  WidgetArray wid;
  VariantList items;

  // Widgets are created here at placeholder positions; layoutEmulationTab()
  // assigns geometry from the current font.
  const int tabID = myTab->addTab(" Emulation ", TabWidget::AUTO_WIDTH);

  // settings set
  mySettingsGroupEmulation = std::make_unique<RadioButtonGroup>();
  myEmuSettings[0] = new RadioButtonWidget(myTab, font, 0, 0, "Player settings",
                                           mySettingsGroupEmulation.get(), kPlrSettings);
  myEmuSettings[0]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myEmuSettings[0]);
  myEmuSettings[1] = new RadioButtonWidget(myTab, font, 0, 0, "Developer settings",
                                           mySettingsGroupEmulation.get(), kDevSettings);
  myEmuSettings[1]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myEmuSettings[1]);

  myFrameStatsWidget = new CheckboxWidget(myTab, font, 0, 0, "Console info overlay");
  myFrameStatsWidget->setToolTip(Event::ToggleFrameStats);
  wid.push_back(myFrameStatsWidget);

  myDetectedInfoWidget = new CheckboxWidget(myTab, font, 0, 0, "Detected settings info");
  myDetectedInfoWidget->setToolTip("Display detected controllers, bankswitching\n"
                                   "and TV types at ROM start.");
  wid.push_back(myDetectedInfoWidget);

  // AtariVox/SaveKey/PlusROM access
  myExternAccessWidget = new CheckboxWidget(myTab, font, 0, 0, "External access messages");
  myExternAccessWidget->setToolTip("Display a message for any external access\n"
                                   "AtariVox/SaveKey EEPROM, PlusROM, Supercharger...).");
  wid.push_back(myExternAccessWidget);

  // 2600/7800 mode
  items.clear();
  VarList::push_back(items, "Atari 2600", "2600");
  VarList::push_back(items, "Atari 7800", "7800");
  myConsoleWidget = new PopUpWidget(myTab, font, 0, 0, font.getStringWidth("Atari 2600"),
                                    lineHeight, items, "Console ",
                                    font.getStringWidth("Console "), kConsole);
  myConsoleWidget->setToolTip("Emulate Color/B&W/Pause key and zero\n"
                              "page RAM initialization differently.");
  wid.push_back(myConsoleWidget);

  // PlusROM functionality
  myPlusRomWidget = new CheckboxWidget(myTab, font, 0, 0, "PlusROM support");
  myPlusRomWidget->setToolTip("Enable PlusROM support");
  wid.push_back(myPlusRomWidget);

  // Randomize items
  myLoadingROMLabel = new StaticTextWidget(myTab, font, 0, 0, "When loading a ROM:");
  wid.push_back(myLoadingROMLabel);

  myRandomBankWidget = new CheckboxWidget(myTab, font, 0, 0, "Random startup bank");
  myRandomBankWidget->setToolTip("Randomize the startup bank for\n"
                                 "most classic bankswitching types.");
  wid.push_back(myRandomBankWidget);

  myRandomizeTIAWidget = new CheckboxWidget(myTab, font, 0, 0, "Randomize TIA");
  wid.push_back(myRandomizeTIAWidget);

  // Randomize RAM
  myRandomizeRAMWidget = new CheckboxWidget(myTab, font, 0, 0,
                                            "Randomize zero-page and extended RAM");
  wid.push_back(myRandomizeRAMWidget);

  // Randomize CPU
  myRandomizeCPULabel = new StaticTextWidget(myTab, font, 0, 0, "Randomize CPU ");
  wid.push_back(myRandomizeCPULabel);

  const std::array<string, 5> cpuregsLabels = {"SP", "A", "X", "Y", "PS"};
  for(int i = 0; i < 5; ++i)
  {
    myRandomizeCPUWidget[i] = new CheckboxWidget(myTab, font, 0, 0, cpuregsLabels[i]);
    wid.push_back(myRandomizeCPUWidget[i]);
  }

  myRandomHotspotsWidget = new CheckboxWidget(myTab, font, 0, 0,
                                              "Random hotspot peek values");
  wid.push_back(myRandomHotspotsWidget);

  // How to handle undriven TIA pins
  myUndrivenPinsWidget = new CheckboxWidget(myTab, font, 0, 0,
                                            "Drive unused TIA pins randomly on a read/peek");
  myUndrivenPinsWidget->setToolTip("Read TIA pins random instead of last databus values.\n"
                                   "Helps detecting missing '#' for immediate loads.");
  wid.push_back(myUndrivenPinsWidget);

#ifdef DEBUGGER_SUPPORT
  myPortBreakLabel = new StaticTextWidget(myTab, font, 0, 0, "Break on:");
  myRWPortBreakWidget = new CheckboxWidget(myTab, font, 0, 0, "Reads from write ports");
  myRWPortBreakWidget->setToolTip("Cause reads from write ports to interrupt\n"
                                  "emulation and enter debugger.");
  wid.push_back(myRWPortBreakWidget);

  myWRPortBreakWidget = new CheckboxWidget(myTab, font, 0, 0, "Writes to read ports");
  myWRPortBreakWidget->setToolTip("Cause writes to read ports to interrupt\n"
                                  "emulation and enter debugger.");
  wid.push_back(myWRPortBreakWidget);
#endif
  // Thumb ARM/ELF emulation exception
  myThumbExceptionWidget = new CheckboxWidget(myTab, font, 0, 0, "Strict ARM emulation (*)");
  myThumbExceptionWidget->setToolTip("Strict checking for exceptions and suspicious program\n"
                                     "behaviour in ARM emulation.\n"
                                     "Interrupts emulation and enters debugger in such cases.");
  wid.push_back(myThumbExceptionWidget);

  myArmSpeedWidget = new SliderWidget(myTab, font, 0, 0,
                                      Dialog::fontWidth() * 10, lineHeight, "Limit ARM speed (*) ",
                                      0, kArmSpeedChanged, Dialog::fontWidth() * 9, " MIPS");
  myArmSpeedWidget->setMinValue(CartridgeELF::MIPS_MIN);
  myArmSpeedWidget->setMaxValue(CartridgeELF::MIPS_MAX);
  myArmSpeedWidget->setTickmarkIntervals((CartridgeELF::MIPS_MAX - CartridgeELF::MIPS_MIN) / 50);
  myArmSpeedWidget->setStepValue(2);
  myArmSpeedWidget->setToolTip("Limit emulation speed to simulate ARM CPU used for ELF.");
  wid.push_back(myArmSpeedWidget);

  myEmuInfo = new StaticTextWidget(myTab, infofont, 0, 0,
                                   "(*) Change requires a reload for ELF ROMs");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperEmulator");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::layoutEmulationTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth(),
            fontHeight = Dialog::fontHeight(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();

  // Vertical column: the primary widget of each row, indented to its level
  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  col->addFixed(anchoredItem(myEmuSettings[0]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myEmuSettings[1]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myFrameStatsWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myExternAccessWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myConsoleWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myLoadingROMLabel, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myRandomBankWidget, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myRandomizeRAMWidget, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myRandomizeCPULabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myRandomHotspotsWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myUndrivenPinsWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
#ifdef DEBUGGER_SUPPORT
  col->addFixed(indentedItem(myPortBreakLabel, INDENT), lineHeight);
  col->addSpace(VGAP);
#endif
  col->addFixed(indentedItem(myThumbExceptionWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myArmSpeedWidget, INDENT), lineHeight);
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // Secondary widgets sharing a row with the primary above
  myDetectedInfoWidget->setPos(myFrameStatsWidget->getRight() + fontWidth * 3,
                               myFrameStatsWidget->getTop());
  myPlusRomWidget->setPos(myDetectedInfoWidget->getLeft(), myConsoleWidget->getTop());
  myRandomizeTIAWidget->setPos(myDetectedInfoWidget->getLeft(), myRandomBankWidget->getTop());

  int xpos = myRandomizeCPULabel->getRight() + fontWidth * 1.25;
  for(int i = 0; i < 5; ++i)
  {
    myRandomizeCPUWidget[i]->setPos(xpos, myRandomizeCPULabel->getTop());
    xpos += CheckboxWidget::boxSize(_font) + _font.getStringWidth("XX") + fontWidth * 2.5;
  }
#ifdef DEBUGGER_SUPPORT
  myRWPortBreakWidget->setPos(myPortBreakLabel->getRight() + fontWidth,
                              myPortBreakLabel->getTop());
  myWRPortBreakWidget->setPos(myRWPortBreakWidget->getRight() + fontWidth * 2,
                              myPortBreakLabel->getTop());
#endif

  // Usage note along the bottom of the tab
  myEmuInfo->setPos(HBORDER,
      myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER);
  myEmuInfo->setWidth(infofont.getStringWidth(myEmuInfo->getLabel()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addTiaTab(const GUI::Font& font)
{
  const int lineHeight = Dialog::lineHeight();
  const int pwidth = font.getStringWidth("Faulty Cosmic Ark stars");
  WidgetArray wid;
  VariantList items;

  // Widgets are created here at placeholder positions; layoutTiaTab() assigns
  // geometry from the current font.
  const int tabID = myTab->addTab("  TIA  ", TabWidget::AUTO_WIDTH);

  // settings set
  mySettingsGroupTia = std::make_unique<RadioButtonGroup>();
  myTiaSettings[0] = new RadioButtonWidget(myTab, font, 0, 0, "Player settings",
                                           mySettingsGroupTia.get(), kPlrSettings);
  myTiaSettings[0]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myTiaSettings[0]);
  myTiaSettings[1] = new RadioButtonWidget(myTab, font, 0, 0, "Developer settings",
                                           mySettingsGroupTia.get(), kDevSettings);
  myTiaSettings[1]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myTiaSettings[1]);

  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Faulty Kool-Aid Man", "koolaidman");
  VarList::push_back(items, "Faulty Cosmic Ark stars", "cosmicark");
  VarList::push_back(items, "Glitched Pesco", "pesco");
  VarList::push_back(items, "Glitched Quick Step!", "quickstep");
  VarList::push_back(items, "Glitched Matchie line", "matchie");
  VarList::push_back(items, "Glitched Indy 500 menu", "indy500");
  VarList::push_back(items, "Glitched He-Man title", "heman");
  VarList::push_back(items, "Shifted flashcart menu", "flashmenu");
  VarList::push_back(items, "Glitched Light Sixer", "lightsixer");
  VarList::push_back(items, "Glitched Jr. missiles", "juniorbug");
  VarList::push_back(items, "Custom", "custom");
  myTIATypeWidget = new PopUpWidget(myTab, font, 0, 0,
                                    pwidth, lineHeight, items, "Chip type ", 0, kTIAType);
  myTIATypeWidget->setToolTip("Select which TIA chip type to emulate.\n"
                              "Some types cause defined glitches.");
  wid.push_back(myTIATypeWidget);

  myInvPhaseLabel = new StaticTextWidget(myTab, font, 0, 0,
                                         "Inverted HMOVE clock phase for");
  myInvPhaseLabel->setToolTip("Objects react different to too\n"
                              "early HM" + ELLIPSIS + " after HMOVE changes.");
  wid.push_back(myInvPhaseLabel);
  myPlInvPhaseWidget = new CheckboxWidget(myTab, font, 0, 0, "Players");
  wid.push_back(myPlInvPhaseWidget);
  myMsInvPhaseWidget = new CheckboxWidget(myTab, font, 0, 0, "Missiles");
  wid.push_back(myMsInvPhaseWidget);
  myBlInvPhaseWidget = new CheckboxWidget(myTab, font, 0, 0, "Ball");
  wid.push_back(myBlInvPhaseWidget);

  myLateHMoveLabel = new StaticTextWidget(myTab, font, 0, 0, "Short late HMOVE for");
  myLateHMoveLabel->setToolTip("Objects react different to late HMOVEs");
  wid.push_back(myLateHMoveLabel);
  myPlLateHMoveWidget = new CheckboxWidget(myTab, font, 0, 0, "Players");
  wid.push_back(myPlLateHMoveWidget);
  myMsLateHMoveWidget = new CheckboxWidget(myTab, font, 0, 0, "Missiles");
  wid.push_back(myMsLateHMoveWidget);
  myBlLateHMoveWidget = new CheckboxWidget(myTab, font, 0, 0, "Ball");
  wid.push_back(myBlLateHMoveWidget);

  myLateRespxLabel = new StaticTextWidget(myTab, font, 0, 0, "Late RESPx for");
  myLateRespxLabel->setToolTip("RESP/RESM/RESBL strobed during HBLANK at HMOVE start shifts object 1 pixel right");
  wid.push_back(myLateRespxLabel);
  myPlLateRespxWidget = new CheckboxWidget(myTab, font, 0, 0, "Players");
  wid.push_back(myPlLateRespxWidget);
  myMsLateRespxWidget = new CheckboxWidget(myTab, font, 0, 0, "Missiles");
  wid.push_back(myMsLateRespxWidget);
  myBlLateRespxWidget = new CheckboxWidget(myTab, font, 0, 0, "Ball");
  wid.push_back(myBlLateRespxWidget);

  myPlayfieldLabel = new StaticTextWidget(myTab, font, 0, 0, "Delayed playfield");
  myPlayfieldLabel->setToolTip("Playfield reacts one color clock slower to updates.");
  wid.push_back(myPlayfieldLabel);
  myPFBitsWidget = new CheckboxWidget(myTab, font, 0, 0, "Bits");
  wid.push_back(myPFBitsWidget);
  myPFColorWidget = new CheckboxWidget(myTab, font, 0, 0, "Color");
  wid.push_back(myPFColorWidget);
  myPFScoreWidget = new CheckboxWidget(myTab, font, 0, 0, "Score color");
  myPFScoreWidget->setToolTip("In score mode, playfield color gets updated one pixel early.");
  wid.push_back(myPFScoreWidget);

  myBackgroundLabel = new StaticTextWidget(myTab, font, 0, 0, "Delayed background");
  myBackgroundLabel->setToolTip("Background color reacts one color clock slower to updates.");
  wid.push_back(myBackgroundLabel);
  myBKColorWidget = new CheckboxWidget(myTab, font, 0, 0, "Color");
  wid.push_back(myBKColorWidget);

  mySwapLabel = new StaticTextWidget(myTab, font, 0, 0,
    std::format("Delayed VDEL{} swap for", ELLIPSIS));
  mySwapLabel->setToolTip("VDELed objects react one color clock slower to updates.");
  wid.push_back(mySwapLabel);
  myPlSwapWidget = new CheckboxWidget(myTab, font, 0, 0, "Players");
  wid.push_back(myPlSwapWidget);
  myBlSwapWidget = new CheckboxWidget(myTab, font, 0, 0, "Ball");
  wid.push_back(myBlSwapWidget);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperTIA");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::layoutTiaTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth(),
            HBORDER    = Dialog::hBorder(),
            VBORDER    = Dialog::vBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  const int gap = fontWidth * 2.5;

  // Positions the two extra checkboxes of a label + Players/Missiles/Ball row
  const auto tripleTail = [gap](CheckboxWidget* first, CheckboxWidget* second,
                                CheckboxWidget* third) {
    second->setPos(first->getRight() + gap, first->getTop());
    if(third != nullptr)
      third->setPos(second->getRight() + gap, second->getTop());
  };

  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  col->addFixed(anchoredItem(myTiaSettings[0]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myTiaSettings[1]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myTIATypeWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myInvPhaseLabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myPlInvPhaseWidget, INDENT * 3), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myLateHMoveLabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myPlLateHMoveWidget, INDENT * 3), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myLateRespxLabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myPlLateRespxWidget, INDENT * 3), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myPlayfieldLabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myPFBitsWidget, INDENT * 3), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myBackgroundLabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myBKColorWidget, INDENT * 3), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(mySwapLabel, INDENT * 2), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myPlSwapWidget, INDENT * 3), lineHeight);
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // The remaining checkboxes on each object row
  tripleTail(myPlInvPhaseWidget, myMsInvPhaseWidget, myBlInvPhaseWidget);
  tripleTail(myPlLateHMoveWidget, myMsLateHMoveWidget, myBlLateHMoveWidget);
  tripleTail(myPlLateRespxWidget, myMsLateRespxWidget, myBlLateRespxWidget);
  tripleTail(myPFBitsWidget, myPFColorWidget, myPFScoreWidget);
  tripleTail(myPlSwapWidget, myBlSwapWidget, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addVideoTab(const GUI::Font& font)
{
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth();
  const int lwidth = font.getStringWidth("Intensity ");
  const int pwidth = fontWidth * 6;
  WidgetArray wid;
  VariantList items;

  // Widgets are created here at placeholder positions; layoutVideoTab() assigns
  // geometry from the current font.
  const int tabID = myTab->addTab(" Video ", TabWidget::AUTO_WIDTH);

  // settings set
  mySettingsGroupVideo = std::make_unique<RadioButtonGroup>();
  myVideoSettings[0] = new RadioButtonWidget(myTab, font, 0, 0, "Player settings",
                                             mySettingsGroupVideo.get(), kPlrSettings);
  myVideoSettings[0]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myVideoSettings[0]);
  myVideoSettings[1] = new RadioButtonWidget(myTab, font, 0, 0, "Developer settings",
                                             mySettingsGroupVideo.get(), kDevSettings);
  myVideoSettings[1]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myVideoSettings[1]);

  // TV jitter effect
  myTVJitterWidget = new CheckboxWidget(myTab, font, 0, 0, "Jitter/roll effect", kTVJitter);
  myTVJitterWidget->setToolTip("Enable to emulate TV loss of sync.", Event::ToggleJitter);
  wid.push_back(myTVJitterWidget);

  myTVJitterSenseWidget = new SliderWidget(myTab, font, 0, 0, fontWidth * 10, lineHeight,
                                           "Sensitivity ", 0, 0, fontWidth * 2);
  myTVJitterSenseWidget->setMinValue(JitterEmulation::MIN_SENSITIVITY);
  myTVJitterSenseWidget->setMaxValue(JitterEmulation::MAX_SENSITIVITY);
  myTVJitterSenseWidget->setTickmarkIntervals(3);
  myTVJitterSenseWidget->setToolTip("Define sensitivity to unstable frames.",
    Event::JitterSenseDecrease, Event::JitterSenseIncrease);
  wid.push_back(myTVJitterSenseWidget);

  myTVJitterRecWidget = new SliderWidget(myTab, font, 0, 0, fontWidth * 10, lineHeight,
                                         "Recovery ", 0, 0, fontWidth * 2);
  myTVJitterRecWidget->setMinValue(JitterEmulation::MIN_RECOVERY);
  myTVJitterRecWidget->setMaxValue(JitterEmulation::MAX_RECOVERY);
  myTVJitterRecWidget->setTickmarkIntervals(5);
  myTVJitterRecWidget->setToolTip("Define speed of sync recovery.",
    Event::JitterRecDecrease, Event::JitterRecIncrease);
  wid.push_back(myTVJitterRecWidget);

  myColorLossWidget = new CheckboxWidget(myTab, font, 0, 0, "PAL color-loss");
  myColorLossWidget->setToolTip("PAL games with odd scanline count\n"
                                "will be displayed without color.", Event::ToggleColorLoss);
  wid.push_back(myColorLossWidget);

  // debug colors
  myDebugColorsWidget = new CheckboxWidget(myTab, font, 0, 0, "Debug colors (*)");
  myDebugColorsWidget->setToolTip("Enable fixed debug colors", Event::ToggleFixedColors);
  wid.push_back(myDebugColorsWidget);

  items.clear();
  VarList::push_back(items, "Red", "r");
  VarList::push_back(items, "Orange", "o");
  VarList::push_back(items, "Yellow", "y");
  VarList::push_back(items, "Green", "g");
  VarList::push_back(items, "Purple", "p");
  VarList::push_back(items, "Blue", "b");

  static constexpr std::array<int, DEBUG_COLORS> dbg_cmds = {
    kP0ColourChangedCmd,  kM0ColourChangedCmd,  kP1ColourChangedCmd,
    kM1ColourChangedCmd,  kPFColourChangedCmd,  kBLColourChangedCmd
  };

  const auto createDebugColourWidgets = [&](int idx, string_view desc)
  {
    myDbgColour[idx] = new PopUpWidget(myTab, font, 0, 0,
                                       pwidth, lineHeight, items, desc, lwidth, dbg_cmds[idx]);
    wid.push_back(myDbgColour[idx]);
    myDbgColourSwatch[idx] = new ColorWidget(
      myTab, font, 0, 0, static_cast<uInt32>(2 * lineHeight), lineHeight);
  };

  createDebugColourWidgets(0, "Player 0  ");
  createDebugColourWidgets(1, "Missile 0 ");
  createDebugColourWidgets(2, "Player 1  ");
  createDebugColourWidgets(3, "Missile 1 ");
  createDebugColourWidgets(4, "Playfield ");
  createDebugColourWidgets(5, "Ball      ");

  myVideoInfo = new StaticTextWidget(myTab, instance().frameBuffer().infoFont(), 0, 0,
                                     "(*) Colors identical for player and developer settings");

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperVideo");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::layoutVideoTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth(),
            fontHeight = Dialog::fontHeight(),
            HBORDER    = Dialog::hBorder(),
            VBORDER    = Dialog::vBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  const int prefix = CheckboxWidget::prefixSize(_font);

  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  col->addFixed(anchoredItem(myVideoSettings[0]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myVideoSettings[1]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myTVJitterWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  // Sensitivity slider is indented under the jitter checkbox text
  col->addFixed(indentedItem(myTVJitterSenseWidget, INDENT + prefix), lineHeight);
  col->addSpace(VGAP * 2);
  col->addFixed(indentedItem(myColorLossWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myDebugColorsWidget, INDENT), lineHeight);
  col->addSpace(VGAP + 2);
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    col->addFixed(indentedItem(myDbgColour[i], INDENT), lineHeight);
    if(i < DEBUG_COLORS - 1)
      col->addSpace(VGAP);
  }
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // Recovery slider sits to the right of the sensitivity slider
  myTVJitterRecWidget->setPos(myTVJitterSenseWidget->getRight() + fontWidth * 2,
                              myTVJitterSenseWidget->getTop());
  // Colour swatches sit to the right of each debug-colour popup
  for(int i = 0; i < DEBUG_COLORS; ++i)
    myDbgColourSwatch[i]->setPos(myDbgColour[i]->getRight() + fontWidth * 1.25,
                                 myDbgColour[i]->getTop());

  // Usage note along the bottom of the tab
  myVideoInfo->setPos(HBORDER,
      myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER);
  myVideoInfo->setWidth(std::min(infofont.getStringWidth(myVideoInfo->getLabel()),
                                 _w - HBORDER * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addTimeMachineTab(const GUI::Font& font)
{
  static constexpr std::array<string_view, RewindManager::NUM_INTERVALS> INTERVALS = {
    " 1 frame",
    " 3 frames",
    "10 frames",
    "30 frames",
    " 1 second",
    " 3 seconds",
    "10 seconds"
  };
  static constexpr std::array<string_view, RewindManager::NUM_HORIZONS> HORIZONS = {
    " 3 seconds",
    "10 seconds",
    "30 seconds",
    " 1 minute",
    " 3 minutes",
    "10 minutes",
    "30 minutes",
    "60 minutes"
  };
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth();
  const int lwidth = fontWidth * 11;
  WidgetArray wid;
  VariantList items;

  // Widgets are created here at placeholder positions; layoutTimeMachineTab()
  // assigns geometry from the current font.
  const int tabID = myTab->addTab(" Time Machine ", TabWidget::AUTO_WIDTH);

  // settings set
  mySettingsGroupTM = std::make_unique<RadioButtonGroup>();
  myTMSettings[0] = new RadioButtonWidget(myTab, font, 0, 0, "Player settings",
                                          mySettingsGroupTM.get(), kPlrSettings);
  myTMSettings[0]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myTMSettings[0]);
  myTMSettings[1] = new RadioButtonWidget(myTab, font, 0, 0, "Developer settings",
                                          mySettingsGroupTM.get(), kDevSettings);
  myTMSettings[1]->setToolTip(Event::ToggleDeveloperSet);
  wid.push_back(myTMSettings[1]);

  myTimeMachineWidget = new CheckboxWidget(myTab, font, 0, 0, "Time Machine", kTimeMachine);
  myTimeMachineWidget->setToolTip(Event::ToggleTimeMachine);
  wid.push_back(myTimeMachineWidget);

  const int swidth = fontWidth * 12 + 5; // width of PopUpWidgets below
  myStateSizeWidget = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                       "Buffer size (*)   ", 0, kSizeChanged, lwidth, " states");
  myStateSizeWidget->setMinValue(RewindManager::MIN_BUF_SIZE);
  myStateSizeWidget->setMaxValue(RewindManager::MAX_BUF_SIZE);
  myStateSizeWidget->setStepValue(20);
  myStateSizeWidget->setTickmarkIntervals(5);
  myStateSizeWidget->setToolTip("Define the total Time Machine buffer size.");
  wid.push_back(myStateSizeWidget);

  myUncompressedWidget = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                          "Uncompressed size ", 0, kUncompressedChanged, lwidth, " states");
  myUncompressedWidget->setMinValue(0);
  myUncompressedWidget->setMaxValue(RewindManager::MAX_BUF_SIZE);
  myUncompressedWidget->setStepValue(20);
  myUncompressedWidget->setTickmarkIntervals(5);
  myUncompressedWidget->setToolTip("Define the number of completely kept states.\n"
                                   "States beyond this number will be slowly removed\n"
                                   "to fit the requested horizon into the buffer.");
  wid.push_back(myUncompressedWidget);

  items.clear();
  for(int i = 0; i < RewindManager::NUM_INTERVALS; ++i)
    VarList::push_back(items, INTERVALS[i], RewindManager::INT_SETTINGS[i]);
  const int pwidth = font.getStringWidth("10 seconds");
  myStateIntervalWidget = new PopUpWidget(myTab, font, 0, 0, pwidth,
                                          lineHeight, items, "Interval          ", 0, kIntervalChanged);
  myStateIntervalWidget->setToolTip("Define the interval between each saved state.");
  wid.push_back(myStateIntervalWidget);

  items.clear();
  for(int i = 0; i < RewindManager::NUM_HORIZONS; ++i)
    VarList::push_back(items, HORIZONS[i], RewindManager::HOR_SETTINGS[i]);
  myStateHorizonWidget = new PopUpWidget(myTab, font, 0, 0, pwidth,
                                         lineHeight, items, "Horizon         ~ ", 0, kHorizonChanged);
  myStateHorizonWidget->setToolTip("Define how far the Time Machine\n"
                                   "will allow moving back in time.");
  wid.push_back(myStateHorizonWidget);

  myTMInfo = new StaticTextWidget(myTab, instance().frameBuffer().infoFont(), 0, 0,
                                  "(*) Any size change clears the buffer");

  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperTimeMachine");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::layoutTimeMachineTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            HBORDER    = Dialog::hBorder(),
            VBORDER    = Dialog::vBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  const int prefix = CheckboxWidget::prefixSize(_font);

  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  col->addFixed(anchoredItem(myTMSettings[0]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myTMSettings[1]), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myTimeMachineWidget, INDENT), lineHeight);
  col->addSpace(VGAP);
  // The sliders/popups line up under the Time Machine checkbox text
  col->addFixed(indentedItem(myStateSizeWidget, INDENT + prefix), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myUncompressedWidget, INDENT + prefix), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myStateIntervalWidget, INDENT + prefix), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(indentedItem(myStateHorizonWidget, INDENT + prefix), lineHeight);
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // Usage note along the bottom of the tab
  myTMInfo->setPos(HBORDER,
      myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER);
  myTMInfo->setWidth(std::min(infofont.getStringWidth(myTMInfo->getLabel()),
                              _w - HBORDER * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebuggerTab(const GUI::Font& font)
{
  const int tabID = myTab->addTab(" Debugger ", TabWidget::AUTO_WIDTH);
  WidgetArray wid;

  // Widgets are created here at placeholder positions; layoutDebuggerTab()
  // assigns geometry from the current font.
#ifdef DEBUGGER_SUPPORT
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth();
  VariantList items;
  const Common::Size& ds = instance().frameBuffer().desktopSize();

  // font size
  items.clear();
  VarList::push_back(items, "Small", "small");
  VarList::push_back(items, "Medium", "medium");
  VarList::push_back(items, "Large", "large");
  myDebuggerFontSize =
    new PopUpWidget(myTab, font, 0, 0, font.getStringWidth("Medium"), lineHeight, items,
                    "Font size (*)  ", 0, kDFontSizeChanged);
  wid.push_back(myDebuggerFontSize);

  // Font style (bold label vs. text, etc)
  items.clear();
  VarList::push_back(items, "All normal font", "0");
  VarList::push_back(items, "Bold labels only", "1");
  VarList::push_back(items, "Bold non-labels only", "2");
  VarList::push_back(items, "All bold font", "3");
  myDebuggerFontStyle =
    new PopUpWidget(myTab, font, 0, 0, font.getStringWidth("Bold non-labels only"),
                    lineHeight, items, "Font style (*) ", 0);
  wid.push_back(myDebuggerFontStyle);

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, 0, 0, "Debugger width (*)  ",
                                           0, 0, 6 * fontWidth, "px");
  myDebuggerWidthSlider->setMinValue(DebuggerDialog::kSmallFontMinW);
  myDebuggerWidthSlider->setMaxValue(ds.w);
  myDebuggerWidthSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myDebuggerWidthSlider->setTickmarkIntervals((ds.w - DebuggerDialog::kSmallFontMinW + 50) / 100);
  wid.push_back(myDebuggerWidthSlider);

  myDebuggerHeightSlider = new SliderWidget(myTab, font, 0, 0, "Debugger height (*) ",
                                            0, 0, 6 * fontWidth, "px");
  myDebuggerHeightSlider->setMinValue(DebuggerDialog::kSmallFontMinH);
  myDebuggerHeightSlider->setMaxValue(ds.h);
  myDebuggerHeightSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myDebuggerHeightSlider->setTickmarkIntervals((ds.h - DebuggerDialog::kSmallFontMinH + 50) / 100);
  wid.push_back(myDebuggerHeightSlider);

  myGhostReadsTrapWidget = new CheckboxWidget(myTab, font, 0, 0, "Trap on 'ghost' reads");
  myGhostReadsTrapWidget->setToolTip("Traps will consider CPU 'ghost' reads too.");
  wid.push_back(myGhostReadsTrapWidget);

  myDebuggerInfo = new StaticTextWidget(myTab, instance().frameBuffer().infoFont(), 0, 0,
                                        "(*) Change requires a ROM reload");

#if defined(DEBUGGER_SUPPORT) && defined(WINDOWED_SUPPORT)
  // Debugger is only realistically available in windowed modes 800x600 or greater
  // (and when it's actually been compiled into the app)
  if(ds.w < 800 || ds.h < 600)  // TODO - maybe this logic can disappear?
  {
    myDebuggerWidthSlider->clearFlags(Widget::FLAG_ENABLED);
    myDebuggerHeightSlider->clearFlags(Widget::FLAG_ENABLED);
  }
#endif
#else
  myDebuggerInfo = new StaticTextWidget(myTab, font, 0, 0, 1, font.getFontHeight(),
                                        "Debugger support not included", TextAlign::Center);
#endif

  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperDebugger");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::layoutDebuggerTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

#ifdef DEBUGGER_SUPPORT
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            HBORDER    = Dialog::hBorder(),
            VBORDER    = Dialog::vBorder(),
            VGAP       = Dialog::vGap();

  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  col->addFixed(anchoredItem(myDebuggerFontSize), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myDebuggerFontStyle), lineHeight);
  col->addSpace(VGAP * 4);
  col->addFixed(anchoredItem(myDebuggerWidthSlider), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myDebuggerHeightSlider), lineHeight);
  col->addSpace(VGAP * 4);
  col->addFixed(anchoredItem(myGhostReadsTrapWidget), lineHeight);
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // Usage note along the bottom of the tab
  myDebuggerInfo->setPos(HBORDER,
      myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER);
#else
  // Single centered "not included" message
  myDebuggerInfo->setPos(0, 20);
  myDebuggerInfo->setWidth(_w - 20);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::getWidgetStates(SettingsSet set)
{
  myFrameStats[set] = myFrameStatsWidget->getState();
  myDetectedInfo[set] = myDetectedInfoWidget->getState();
  // AtariVox/SaveKey/PlusROM access
  myExternAccess[set] = myExternAccessWidget->getState();
  myConsole[set] = myConsoleWidget->getSelected() == 1;
  myPlusROM[set] = myPlusRomWidget->getState() == 1;
  // Randomization
  myRandomBank[set] = myRandomBankWidget->getState();
  myRandomizeTIA[set] = myRandomizeTIAWidget->getState();
  myRandomizeRAM[set] = myRandomizeRAMWidget->getState();
  string cpurandom;

  for(int i = 0; i < 5; ++i)
    if(myRandomizeCPUWidget[i]->getState())
      cpurandom += ourCPURegs[i];
  myRandomizeCPU[set] = cpurandom;
  // Random hotspot peeks
  myRandomHotspots[set] = myRandomHotspotsWidget->getState();
  // Undriven TIA pins
  myUndrivenPins[set] = myUndrivenPinsWidget->getState();
#ifdef DEBUGGER_SUPPORT
  // Read from write ports break
  myRWPortBreak[set] = myRWPortBreakWidget->getState();
  myWRPortBreak[set] = myWRPortBreakWidget->getState();
#endif
  // Thumb ARM emulation exception
  myThumbException[set] = myThumbExceptionWidget->getState();
  myArmSpeed[set] = myArmSpeedWidget->getValue();

  // TIA tab
  myTIAType[set] = myTIATypeWidget->getSelectedTag().toString();
  myPlInvPhase[set] = myPlInvPhaseWidget->getState();
  myMsInvPhase[set] = myMsInvPhaseWidget->getState();
  myBlInvPhase[set] = myBlInvPhaseWidget->getState();
  myPlLateHMove[set] = myPlLateHMoveWidget->getState();
  myMsLateHMove[set] = myMsLateHMoveWidget->getState();
  myBlLateHMove[set] = myBlLateHMoveWidget->getState();
  myPlLateRespx[set] = myPlLateRespxWidget->getState();
  myMsLateRespx[set] = myMsLateRespxWidget->getState();
  myBlLateRespx[set] = myBlLateRespxWidget->getState();
  myPFBits[set] = myPFBitsWidget->getState();
  myPFColor[set] = myPFColorWidget->getState();
  myPFScore[set] = myPFScoreWidget->getState();
  myBKColor[set] = myBKColorWidget->getState();
  myPlSwap[set] = myPlSwapWidget->getState();
  myBlSwap[set] = myBlSwapWidget->getState();

  // Debug colors
  myDebugColors[set] = myDebugColorsWidget->getState();
  // PAL color-loss effect
  myColorLoss[set] = myColorLossWidget->getState();
  // Jitter
  myTVJitter[set] = myTVJitterWidget->getState();
  myTVJitterSense[set] = myTVJitterSenseWidget->getValue();
  myTVJitterRec[set] = myTVJitterRecWidget->getValue();

  // States
  myTimeMachine[set] = myTimeMachineWidget->getState();
  myStateSize[set] = myStateSizeWidget->getValue();
  myUncompressed[set] = myUncompressedWidget->getValue();
  myStateInterval[set] = myStateIntervalWidget->getSelectedTag().toString();
  myStateHorizon[set] = myStateHorizonWidget->getSelectedTag().toString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::setWidgetStates(SettingsSet set)
{
  myFrameStatsWidget->setState(myFrameStats[set]);
  myDetectedInfoWidget->setState(myDetectedInfo[set]);
  // AtariVox/SaveKey/PlusROM access
  myExternAccessWidget->setState(myExternAccess[set]);
  myConsoleWidget->setSelectedIndex(myConsole[set]);
  myPlusRomWidget->setState(myPlusROM[set]);
  // Randomization
  myRandomBankWidget->setState(myRandomBank[set]);
  myRandomizeTIAWidget->setState(myRandomizeTIA[set]);
  myRandomizeRAMWidget->setState(myRandomizeRAM[set]);

  const string_view cpurandom = myRandomizeCPU[set];

  for(int i = 0; i < 5; ++i)
    myRandomizeCPUWidget[i]->setState(BSPF::containsIgnoreCase(cpurandom, ourCPURegs[i]));
  // Random hotspot peeks
  myRandomHotspotsWidget->setState(myRandomHotspots[set]);
  // Undriven TIA pins
  myUndrivenPinsWidget->setState(myUndrivenPins[set]);
#ifdef DEBUGGER_SUPPORT
  // Read from write ports break
  myRWPortBreakWidget->setState(myRWPortBreak[set]);
  myWRPortBreakWidget->setState(myWRPortBreak[set]);
#endif
  // Thumb ARM emulation exception
  myThumbExceptionWidget->setState(myThumbException[set]);
  myArmSpeedWidget->setValue(myArmSpeed[set]);
  handleConsole();

  // TIA tab
  myTIATypeWidget->setSelected(myTIAType[set], "standard");
  handleTia();

  // Debug colors
  myDebugColorsWidget->setState(myDebugColors[set]);
  // PAL color-loss effect
  myColorLossWidget->setState(myColorLoss[set]);
  // Jitter
  myTVJitterWidget->setState(myTVJitter[set]);
  myTVJitterSenseWidget->setValue(myTVJitterSense[set]);
  myTVJitterRecWidget->setValue(myTVJitterRec[set]);

  handleTVJitterChange();

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
  const bool devSettings = instance().settings().getBool("dev.settings");
  handleSettings(devSettings);
  mySettings = devSettings;
  mySettingsGroupEmulation->setSelected(devSettings ? 1 : 0);
  mySettingsGroupTia->setSelected(devSettings ? 1 : 0);
  mySettingsGroupVideo->setSelected(devSettings ? 1 : 0);
  mySettingsGroupTM->setSelected(devSettings ? 1 : 0);

  // load both setting sets...
  loadSettings(SettingsSet::player);
  loadSettings(SettingsSet::developer);
  // ...and select the current one
  setWidgetStates(static_cast<SettingsSet>(mySettingsGroupEmulation->getSelected()));

  // Debug colours
  handleDebugColours(instance().settings().getString("tia.dbgcolors"));

#ifdef DEBUGGER_SUPPORT
  // Debugger size
  const Common::Size& ds = instance().settings().getSize("dbg.res");
  const int w = ds.w, h = ds.h;

  myDebuggerWidthSlider->setValue(w);
  myDebuggerHeightSlider->setValue(h);

  // Debugger font size
  const string size = instance().settings().getString("dbg.fontsize");
  myDebuggerFontSize->setSelected(size, "medium");

  // Debugger font style
  const int style = instance().settings().getInt("dbg.fontstyle");
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
  const bool devSettings = mySettingsGroupEmulation->getSelected() == SettingsSet::developer;

  instance().settings().setValue("dev.settings", devSettings);
  // copy current widget status into set...
  getWidgetStates(static_cast<SettingsSet>(mySettingsGroupEmulation->getSelected()));
  // ...and save both sets
  saveSettings(SettingsSet::player);
  saveSettings(SettingsSet::developer);
  // activate the current settings
  applySettings(devSettings ? SettingsSet::developer : SettingsSet::player);

  // Debug colours
  string dbgcolors;
  for(int i = 0; i < DEBUG_COLORS; ++i)
    dbgcolors += myDbgColour[i]->getSelectedTag().toString();
  if(instance().hasConsole() &&
     instance().console().tia().setFixedColorPalette(dbgcolors))
    instance().settings().setValue("tia.dbgcolors", dbgcolors);

#ifdef DEBUGGER_SUPPORT
  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
                                 myDebuggerFontStyle->getSelectedTag().toString());
  // Debugger size
  instance().settings().setValue("dbg.res",
                                 Common::Size(myDebuggerWidthSlider->getValue(),
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
  const bool devSettings = mySettings;
  const auto set = static_cast<SettingsSet>
      (mySettingsGroupEmulation->getSelected());

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation
      myFrameStats[set] = devSettings;
      myDetectedInfo[set] = devSettings;
      // AtariVox/SaveKey/PlusROM access
      myExternAccess[set] = devSettings;
      myConsole[set] = false;
      myPlusROM[set] = true;
      // Randomization
      myRandomBank[set] = devSettings;
      myRandomizeTIA[set] = devSettings;
      myRandomizeRAM[set] = true;
      myRandomizeCPU[set] = devSettings ? "SAXYP" : "AXYP";
      // Random hotspot peeks
      myRandomHotspots[set] = devSettings;
      // Undriven TIA pins
      myUndrivenPins[set] = devSettings;
    #ifdef DEBUGGER_SUPPORT
      // Reads from write ports
      myRWPortBreak[set] = devSettings;
      myWRPortBreak[set] = devSettings;
    #endif
      // Thumb ARM emulation exception
      myThumbException[set] = devSettings;
      myArmSpeed[set] = devSettings ? CartridgeELF::MIPS_DEF : CartridgeELF::MIPS_MAX;

      setWidgetStates(set);
      break;

    case 1: // TIA
      myTIAType[set] = "standard";
      // reset "custom" mode
      myPlInvPhase[set] = devSettings;
      myMsInvPhase[set] = devSettings;
      myBlInvPhase[set] = devSettings;
      myPFBits[set] = devSettings;
      myPFColor[set] = devSettings;
      myPFScore[set] = devSettings;
      myBKColor[set] = devSettings;
      myPlSwap[set] = devSettings;
      myBlSwap[set] = devSettings;

      setWidgetStates(set);
      break;

    case 2: // Video
      // Jitter
      myTVJitter[set] = true;
      myTVJitterSense[set] = devSettings
        ? JitterEmulation::DEV_SENSITIVITY
        : JitterEmulation::PLR_SENSITIVITY;
      myTVJitterRec[set] = devSettings ? 2 : 10;
      // PAL color-loss effect
      myColorLoss[set] = devSettings;
      // Debug colors
      myDebugColors[set] = false;
      handleDebugColours("roygpb");

      setWidgetStates(set);
      break;

    case 3: // States
      myTimeMachine[set] = true;
      myStateSize[set] = devSettings ? 1000 : 200;
      myUncompressed[set] = devSettings ? 600 : 60;
      myStateInterval[set] = devSettings ? "1f" : "30f";
      myStateHorizon[set] = devSettings ? "30s" : "10m";

      setWidgetStates(set);
      break;

    case 4: // Debugger options
    {
#ifdef DEBUGGER_SUPPORT
      const Common::Size& size = instance().frameBuffer().desktopSize(BufferType::Debugger);

      const uInt32 w = std::min(size.w, static_cast<uInt32>(DebuggerDialog::kMediumFontMinW));
      const uInt32 h = std::min(size.h, static_cast<uInt32>(DebuggerDialog::kMediumFontMinH));
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

    case kTIAType:
      handleTia();
      break;

    case kConsole:
      handleConsole();
      break;

    case kTVJitter:
      handleTVJitterChange();
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
  myPlusRomWidget->setEnabled(devSettings);
  myRandomHotspotsWidget->setEnabled(devSettings);
  myUndrivenPinsWidget->setEnabled(devSettings);
#ifdef DEBUGGER_SUPPORT
  myPortBreakLabel->setEnabled(devSettings);
  myRWPortBreakWidget->setEnabled(devSettings);
  myWRPortBreakWidget->setEnabled(devSettings);
#endif
  myThumbExceptionWidget->setEnabled(devSettings);
  myArmSpeedWidget->setEnabled(devSettings);

  if (mySettings != devSettings)
  {
    mySettings = devSettings; // block redundant events first!
    const SettingsSet set = devSettings ? SettingsSet::developer
                                        : SettingsSet::player;
    mySettingsGroupEmulation->setSelected(set);
    mySettingsGroupTia->setSelected(set);
    mySettingsGroupVideo->setSelected(set);
    mySettingsGroupTM->setSelected(set);
    // Save current widget states into old set
    getWidgetStates(devSettings ? SettingsSet::player
                                : SettingsSet::developer);
    // Load new set into widgets states
    setWidgetStates(set);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTVJitterChange()
{
  const bool enable = myTVJitterWidget->getState();

  myTVJitterSenseWidget->setEnabled(enable);
  myTVJitterRecWidget->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleConsole()
{
  const bool is7800 = myConsoleWidget->getSelected() == 1;

  myRandomizeRAMWidget->setEnabled(!is7800);
  if(is7800)
    myRandomizeRAMWidget->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTia()
{
  const string tiaType = myTIATypeWidget->getSelectedTag().toString();
  const bool enable = BSPF::equalsIgnoreCase("custom", tiaType);

  myTIATypeWidget->setEnabled(mySettings);
  myInvPhaseLabel->setEnabled(enable);
  myPlInvPhaseWidget->setEnabled(enable);
  myMsInvPhaseWidget->setEnabled(enable);
  myBlInvPhaseWidget->setEnabled(enable);
  myLateHMoveLabel->setEnabled(enable);
  myPlLateHMoveWidget->setEnabled(enable);
  myMsLateHMoveWidget->setEnabled(enable);
  myBlLateHMoveWidget->setEnabled(enable);
  myLateRespxLabel->setEnabled(enable);
  myPlLateRespxWidget->setEnabled(enable);
  myMsLateRespxWidget->setEnabled(enable);
  myBlLateRespxWidget->setEnabled(enable);
  myPlayfieldLabel->setEnabled(enable);
  myBackgroundLabel->setEnabled(enable);
  myPFBitsWidget->setEnabled(enable);
  myPFColorWidget->setEnabled(enable);
  myPFScoreWidget->setEnabled(enable);
  myBKColorWidget->setEnabled(enable);
  mySwapLabel->setEnabled(enable);
  myPlSwapWidget->setEnabled(enable);
  myBlSwapWidget->setEnabled(enable);

  if(BSPF::equalsIgnoreCase("custom", tiaType))
  {
    const SettingsSet set = SettingsSet::developer;

    myPlInvPhaseWidget->setState(myPlInvPhase[set]);
    myMsInvPhaseWidget->setState(myMsInvPhase[set]);
    myBlInvPhaseWidget->setState(myBlInvPhase[set]);
    myPlLateHMoveWidget->setState(myPlLateHMove[set]);
    myMsLateHMoveWidget->setState(myMsLateHMove[set]);
    myBlLateHMoveWidget->setState(myBlLateHMove[set]);
    myPlLateRespxWidget->setState(myPlLateRespx[set]);
    myMsLateRespxWidget->setState(myMsLateRespx[set]);
    myBlLateRespxWidget->setState(myBlLateRespx[set]);
    myPFBitsWidget->setState(myPFBits[set]);
    myPFColorWidget->setState(myPFColor[set]);
    myPFScoreWidget->setState(myPFScore[set]);
    myBKColorWidget->setState(myBKColor[set]);
    myPlSwapWidget->setState(myPlSwap[set]);
    myBlSwapWidget->setState(myBlSwap[set]);
  }
  else
  {
    myPlInvPhaseWidget->setState(BSPF::equalsIgnoreCase("koolaidman", tiaType));
    myMsInvPhaseWidget->setState(BSPF::equalsIgnoreCase("cosmicark", tiaType));
    myBlInvPhaseWidget->setState(false);
    myPlLateHMoveWidget->setState(BSPF::equalsIgnoreCase("flashmenu", tiaType));
    myMsLateHMoveWidget->setState(false);
    myBlLateHMoveWidget->setState(false);
    myPlLateRespxWidget->setState(BSPF::equalsIgnoreCase("lightsixer", tiaType));
    myMsLateRespxWidget->setState(BSPF::equalsIgnoreCase("lightsixer", tiaType) ||
                                  BSPF::equalsIgnoreCase("juniorbug", tiaType));
    myBlLateRespxWidget->setState(BSPF::equalsIgnoreCase("lightsixer", tiaType));
    myPFBitsWidget->setState(BSPF::equalsIgnoreCase("pesco", tiaType));
    myPFColorWidget->setState(BSPF::equalsIgnoreCase("quickstep", tiaType));
    myPFScoreWidget->setState(BSPF::equalsIgnoreCase("matchie", tiaType));
    myBKColorWidget->setState(BSPF::equalsIgnoreCase("indy500", tiaType));
    myPlSwapWidget->setState(BSPF::equalsIgnoreCase("heman", tiaType));
    myBlSwapWidget->setState(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTimeMachine()
{
  const bool enable = myTimeMachineWidget->getState();

  myStateSizeWidget->setEnabled(enable);
  myUncompressedWidget->setEnabled(enable);
  myStateIntervalWidget->setEnabled(enable);

  const uInt32 size = myStateSizeWidget->getValue();
  const uInt32 uncompressed = myUncompressedWidget->getValue();

  myStateHorizonWidget->setEnabled(enable && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSize()
{
  const uInt32 size = myStateSizeWidget->getValue();
  const uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i = 0;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt horizon and interval
  do
  {
    for(i = horizon; i < RewindManager::NUM_HORIZONS; ++i)
    {
      if(static_cast<uInt64>(size) * RewindManager::INTERVAL_CYCLES[interval]
         <= RewindManager::HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      --interval;
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
  const uInt32 size = myStateSizeWidget->getValue();
  const uInt32 uncompressed = myUncompressedWidget->getValue();

  if(size < uncompressed)
    myStateSizeWidget->setValue(uncompressed);
  myStateHorizonWidget->setEnabled(myTimeMachineWidget->getState() && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleInterval()
{
  uInt32 size = myStateSizeWidget->getValue();
  const uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i = 0;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt horizon and size
  do
  {
    for(i = horizon; i < RewindManager::NUM_HORIZONS; ++i)
    {
      if(static_cast<uInt64>(size) * RewindManager::INTERVAL_CYCLES[interval]
         <= RewindManager::HORIZON_CYCLES[i])
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
  const uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i = 0;

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
      if(static_cast<uInt64>(size) * RewindManager::INTERVAL_CYCLES[i]
         <= RewindManager::HORIZON_CYCLES[horizon])
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
    myDbgColour[idx]->clearFlags(Widget::FLAG_ENABLED);
    myDbgColourSwatch[idx]->clearFlags(Widget::FLAG_ENABLED);
    return;
  }

  static constexpr BSPF::array2D<ColorId, 3, DEBUG_COLORS> dbg_color = {{
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
    },
    {
      TIA::FixedColor::SECAM_RED,
      TIA::FixedColor::SECAM_ORANGE,
      TIA::FixedColor::SECAM_YELLOW,
      TIA::FixedColor::SECAM_GREEN,
      TIA::FixedColor::SECAM_PURPLE,
      TIA::FixedColor::SECAM_BLUE
    }
  }};

  const int timing = instance().console().timing() == ConsoleTiming::ntsc ? 0
    : instance().console().timing() == ConsoleTiming::pal ? 1 : 2;

  myDbgColourSwatch[idx]->setColor(dbg_color[timing][color]);
  myDbgColour[idx]->setSelectedIndex(color);

  // make sure the selected debug colors are all different
  std::array<bool, DEBUG_COLORS> usedCol = {false};

  // identify used colors
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    usedCol[i] = false;
    for(int j = 0; j < DEBUG_COLORS; ++j)
    {
      if(myDbgColourSwatch[j]->getColor() == dbg_color[timing][i])
      {
        usedCol[i] = true;
        break;
      }
    }
  }
  // check if currently changed color was used somewhere else
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    if (i != idx && myDbgColourSwatch[i]->getColor() == dbg_color[timing][color])
    {
      // if already used, change the other color to an unused one
      for(int j = 0; j < DEBUG_COLORS; ++j)
      {
        if(!usedCol[j])
        {
          myDbgColourSwatch[i]->setColor(dbg_color[timing][j]);
          myDbgColour[i]->setSelectedIndex(j);
          break;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(string_view colors)
{
  for(int i = 0; i < DEBUG_COLORS && std::cmp_less(i, colors.length()); ++i)
  {
    switch(colors[i])
    {
      case 'r': handleDebugColours(i, 0); break;
      case 'o': handleDebugColours(i, 1); break;
      case 'y': handleDebugColours(i, 2); break;
      case 'g': handleDebugColours(i, 3); break;
      case 'p': handleDebugColours(i, 4); break;
      case 'b': handleDebugColours(i, 5); break;
      default:                            break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleFontSize()
{
#ifdef DEBUGGER_SUPPORT
  uInt32 minW = 0, minH = 0;
  const int fontSize = myDebuggerFontSize->getSelected();

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
  const Common::Size& size = instance().frameBuffer().desktopSize(BufferType::Debugger);
  minW = std::min(size.w, minW);
  minH = std::min(size.h, minH);

  myDebuggerWidthSlider->setMinValue(minW);
  if(std::cmp_greater(minW, myDebuggerWidthSlider->getValue()))
    myDebuggerWidthSlider->setValue(minW);

  myDebuggerHeightSlider->setMinValue(minH);
  if(std::cmp_greater(minH, myDebuggerHeightSlider->getValue()))
    myDebuggerHeightSlider->setValue(minH);
#endif
}
