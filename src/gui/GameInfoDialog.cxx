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

#include "Bankswitch.hxx"
#include "Console.hxx"
#include "Cart.hxx"
#include "MouseControl.hxx"
#include "SaveKey.hxx"
#include "EditTextWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"
#include "CartDetector.hxx"
#include "ControllerDetector.hxx"
#include "Paddles.hxx"
#include "PopUpWidget.hxx"
#include "PropsSet.hxx"
#include "BrowserDialog.hxx"
#include "QuadTariDialog.hxx"
#include "TabWidget.hxx"
#include "TabPaneWidget.hxx"
#include "TIAConstants.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "Font.hxx"

#include "repository/KeyValueRepositoryPropertyFile.hxx"
#include "FrameBuffer.hxx"
#include "TIASurface.hxx"
#include "Switches.hxx"
#include "AudioSettings.hxx"
#include "bspf.hxx"
#include "MediaFactory.hxx"

#include "GameInfoDialog.hxx"

using namespace std;
using namespace BSPF;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(OSystem& osystem, DialogContainer& parent,
                               const GUI::Font& font, GuiObject* boss)
  : Dialog(osystem, parent, font, "Game properties"),
    CommandSender(boss)
{
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() sizes the
  // dialog and positions everything from the current font.  The tab bar geometry
  // is recomputed in layout() via TabWidget::updateTabSizes().
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTab = new TabWidget(this, font);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////////////////////////
  addEmulationTab();
  addConsoleTab();
  addControllersTab();
  addCartridgeTab();
  addHighScoresTab();
  //////////////////////////////////////////////////////////////////////////////

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  addDefaultsExtraOKCancelBGroup(wid, font, "Export" + ELLIPSIS, kExportPressed);
  _extraWidget->setToolTip("Export the current ROM's properties\n"
                           "into the default directory.");
  addBGroupToFocusList(wid);

  setHelpAnchor("Properties");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::~GameInfoDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::layout()
{
  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            VGAP         = Dialog::vGap();
  constexpr int xpos = 2;

  // Both dimensions come from what the tabs ask for: nothing here counts rows or
  // columns, or reasons about labels and borders.  A tab whose fields simply
  // widen with the dialog says how much room they need where the fields are (see
  // the cartridge grid's field column), so even that is not guessed at here
  const Common::Size tabSize = myTab->naturalSize();

  myTab->setPos(xpos, VGAP + _th);
  myTab->setWidth(static_cast<int>(tabSize.w));
  myTab->setHeight(static_cast<int>(tabSize.h));

  _w = myTab->getWidth() + 2 * xpos;
  _h = _th + VGAP + myTab->getHeight() + VBORDER + buttonHeight + VBORDER;

  // Recompute the tab-bar geometry for the current font/width
  myTab->updateTabSizes();

  // Standard button group (Defaults / Export / OK / Cancel) along the bottom
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addEmulationTab()
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  WidgetArray wid;
  VariantList items;

  // 1) Emulation properties.  The tab's controls are parented to a content pane;
  // the pane lays them out (see setLayout below) whenever the tab is sized
  const int tabID = myTab->addTab("Emulation", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  myBSTypeLabel = new StaticTextWidget(pane, _font, "Type (*)");
  // The list is refilled per ROM, but never with an entry wider than the full
  // scheme list, so size the box to that -- the way GlobalPropsDialog sizes this
  // same list, rather than measuring a copy of its widest entry
  for(const auto& [name, desc]: Bankswitch::BSList)
    VarList::push_back(items, desc, name);
  myBSType = new PopUpWidget(pane, _font, items, kBSTypeChanged);
  wid.push_back(myBSType);
  myBSFilter = new CheckboxWidget(pane, _font, "Filter", kBSFilterChanged);
  myBSFilter->setToolTip("Enable to filter types by ROM size");
  wid.push_back(myBSFilter);

  myTypeDetected = new StaticTextWidget(pane, ifont,
                                        "CM (SpectraVideo CompuMate) detected");

  // Start bank -- "Auto" is always present and the widest fixed entry, so the
  // box sizes to it; the per-ROM bank numbers are refilled later
  items.clear();
  VarList::push_back(items, "Auto", "AUTO");
  myStartBankLabel = new StaticTextWidget(pane, _font, "Start bank (*)");
  myStartBank = new PopUpWidget(pane, _font, items);
  wid.push_back(myStartBank);

  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC", "NTSC");
  VarList::push_back(items, "PAL", "PAL");
  VarList::push_back(items, "SECAM", "SECAM");
  VarList::push_back(items, "NTSC-50", "NTSC50");
  VarList::push_back(items, "PAL-60", "PAL60");
  VarList::push_back(items, "SECAM-60", "SECAM60");
  myFormatLabel = new StaticTextWidget(pane, _font, "TV format");
  myFormat = new PopUpWidget(pane, _font, items);
  myFormat->setToolTip(Event::FormatDecrease, Event::FormatIncrease);
  wid.push_back(myFormat);

  myFormatDetected = new StaticTextWidget(pane, ifont, "SECAM-60 detected");

  // Phosphor
  myPhosphor = new CheckboxWidget(pane, _font,
                                  "Phosphor (auto-enabled/disabled for all ROMs)", kPhosphorChanged);
  myPhosphor->setToolTip(Event::TogglePhosphor);
  wid.push_back(myPhosphor);

  myPPBlendLabel = new StaticTextWidget(pane, _font, "Blend");
  myPPBlend = new SliderWidget(pane, _font, 0, kPPBlendChanged, 4, "%");
  myPPBlend->setMinValue(0);
  myPPBlend->setMaxValue(100);
  myPPBlend->setTickmarkIntervals(2);
  myPPBlend->setToolTip(Event::PhosphorDecrease, Event::PhosphorIncrease);
  wid.push_back(myPPBlend);

  myVCenterLabel = new StaticTextWidget(pane, _font, "V-Center");
  myVCenter = new SliderWidget(pane, _font, 0, kVCenterChanged, 7, "px", 0, true);
  myVCenter->setMinValue(TIAConstants::minVcenter);
  myVCenter->setMaxValue(TIAConstants::maxVcenter);
  myVCenter->setTickmarkIntervals(4);
  myVCenter->setToolTip(Event::VCenterDecrease, Event::VCenterIncrease);
  wid.push_back(myVCenter);

  mySound = new CheckboxWidget(pane, _font, "Stereo sound");
  wid.push_back(mySound);

  // Message concerning usage (positioned along the bottom in layout)
  myEmulInfo = new StaticTextWidget(pane, ifont, "(*) Change requires a ROM reload");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("EmulationProps");

  // Describe the layout once; the pane runs it on every resize
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using GUI::indentedItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();
    // Each group is given ONE label column, sized to the longest label in it, so
    // the value boxes beside them line up.  A label is a label whether it names
    // a pop-up (the bankswitch type) or a slider (the blend/V-Center pair), so
    // all three line up the same way.  The blend row's label is indented, so
    // its column is narrowed to match and its track still meets V-Center's
    GUI::alignLabels({{myBSTypeLabel}, {myStartBankLabel}, {myFormatLabel}});
    GUI::alignLabels({{myPPBlendLabel, INDENT}, {myVCenterLabel}});

    // Bankswitch-type row: label + type popup + filter checkbox
    auto bsRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    bsRow->addAuto(anchoredItem(myBSTypeLabel));
    bsRow->addAuto(anchoredItem(myBSType));
    bsRow->addSpace(fontWidth);
    bsRow->addAuto(anchoredItem(myBSFilter));

    // TV-format row, with the detected format beside the popup (it uses the
    // smaller info font, so it is centered on the row rather than filling it)
    auto formatRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    formatRow->addAuto(labeledRow(myFormatLabel, myFormat));
    formatRow->addSpace(fontWidth);
    formatRow->addStretch(alignedItem(myFormatDetected, HAlign::Fill, VAlign::Center));

    // Every row is as tall as what it holds, so no height is stated here: the
    // pop-ups frame their text, and the detected-type/usage notes use the
    // smaller info font
    col.addAuto(std::move(bsRow));
    col.addSpace(VGAP);
    // Detected type, indented to line up under the type popup
    col.addAuto(indentedItem(myTypeDetected, myBSTypeLabel->getWidth()));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myStartBankLabel, myStartBank));
    col.addSpace(VGAP * 4);
    col.addAuto(std::move(formatRow));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myPhosphor));
    col.addAuto(labeledRow(myPPBlendLabel, myPPBlend, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myVCenterLabel, myVCenter));
    col.addSpace(VGAP * 3);
    col.addAuto(anchoredItem(mySound));
    // Usage note along the bottom of the tab, never closer than this to the row
    // above it
    col.addSpace(VGAP * 3);
    col.addStretchSpace();
    col.addAuto(anchoredItem(myEmulInfo));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addConsoleTab()
{
  WidgetArray wid;

  // 2) Console properties.  The tab's controls are parented to a content pane;
  // the pane lays them out (see setLayout below) whenever the tab is sized
  const int tabID = myTab->addTab(" Console ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  myTVTypeLabel = new StaticTextWidget(pane, _font, "TV type");
  myTVTypeGroup = std::make_unique<RadioButtonGroup>();
  myTVType[0] = new RadioButtonWidget(pane, _font, "Color", myTVTypeGroup.get());
  myTVType[0]->setToolTip(Event::ConsoleColor, Event::ConsoleColorToggle);
  wid.push_back(myTVType[0]);
  myTVType[1] = new RadioButtonWidget(pane, _font, "B/W", myTVTypeGroup.get());
  myTVType[1]->setToolTip(Event::ConsoleBlackWhite, Event::ConsoleColorToggle);
  wid.push_back(myTVType[1]);

  myLeftDiffLabel = new StaticTextWidget(pane, _font, GUI::LEFT_DIFFICULTY);
  myLeftDiffGroup = std::make_unique<RadioButtonGroup>();
  myLeftDiff[0] = new RadioButtonWidget(pane, _font, "A (Expert)", myLeftDiffGroup.get());
  myLeftDiff[0]->setToolTip(Event::ConsoleLeftDiffA, Event::ConsoleLeftDiffToggle);
  wid.push_back(myLeftDiff[0]);
  myLeftDiff[1] = new RadioButtonWidget(pane, _font, "B (Novice)", myLeftDiffGroup.get());
  myLeftDiff[1]->setToolTip(Event::ConsoleLeftDiffB, Event::ConsoleLeftDiffToggle);
  wid.push_back(myLeftDiff[1]);

  myRightDiffLabel = new StaticTextWidget(pane, _font, GUI::RIGHT_DIFFICULTY);
  myRightDiffGroup = std::make_unique<RadioButtonGroup>();
  myRightDiff[0] = new RadioButtonWidget(pane, _font, "A (Expert)", myRightDiffGroup.get());
  myRightDiff[0]->setToolTip(Event::ConsoleRightDiffA, Event::ConsoleRightDiffToggle);
  wid.push_back(myRightDiff[0]);
  myRightDiff[1] = new RadioButtonWidget(pane, _font, "B (Novice)", myRightDiffGroup.get());
  myRightDiff[1]->setToolTip(Event::ConsoleRightDiffB, Event::ConsoleRightDiffToggle);
  wid.push_back(myRightDiff[1]);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("ConsoleProps");

  // Describe the layout once; the pane runs it on every resize.  Each switch is
  // a label with its two radio buttons stacked to the right of it, and the three
  // switches line up because they share the grid's label column — which is as
  // wide as the longest of the three labels, without anyone measuring one
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::GridLayout;
    using GUI::anchoredItem;

    const int VGAP = Dialog::vGap();

    enum Col: int { LABEL, OPTION, COLS };
    enum Row: int {
      TV_A, TV_B, GAP1, LEFT_A, LEFT_B, GAP2, RIGHT_A, RIGHT_B, ROWS
    };
    auto grid = std::make_unique<GridLayout>(COLS, ROWS,
                                             Dialog::fontWidth(), 0);
    grid->columnAuto(LABEL).columnStretch(OPTION);
    for(const int r: {TV_A, TV_B, LEFT_A, LEFT_B, RIGHT_A, RIGHT_B})
      grid->rowAuto(r);
    grid->rowFixed(GAP1, VGAP * 2).rowFixed(GAP2, VGAP * 2);

    const auto section = [&](int row, StaticTextWidget* label,
                             RadioButtonWidget* a, RadioButtonWidget* b) {
      grid->place(LABEL,  row, anchoredItem(label));
      grid->place(OPTION, row, anchoredItem(a));
      grid->place(OPTION, row + 1, anchoredItem(b));
    };

    section(TV_A,    myTVTypeLabel,    myTVType[0],    myTVType[1]);
    section(LEFT_A,  myLeftDiffLabel,  myLeftDiff[0],  myLeftDiff[1]);
    section(RIGHT_A, myRightDiffLabel, myRightDiff[0], myRightDiff[1]);

    col.addAuto(std::move(grid));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addControllersTab()
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  VariantList items, ctrls;
  WidgetArray wid;

  // 3) Controller properties.  The tab's controls are parented to a content pane;
  // the pane lays them out (see setLayout below) whenever the tab is sized
  const int tabID = myTab->addTab("Controllers", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "Joystick", "JOYSTICK");
  VarList::push_back(items, "Paddles", "PADDLES");
  VarList::push_back(items, "Paddles_IAxis", "PADDLES_IAXIS");
  VarList::push_back(items, "Paddles_IAxDr", "PADDLES_IAXDR");
  VarList::push_back(items, "Booster Grip", "BOOSTERGRIP");
  VarList::push_back(items, "Driving", "DRIVING");
  VarList::push_back(items, "Keyboard", "KEYBOARD");
  VarList::push_back(items, "Amiga mouse", "AMIGAMOUSE");
  VarList::push_back(items, "Atari mouse", "ATARIMOUSE");
  VarList::push_back(items, "Trak-Ball", "TRAKBALL");
  VarList::push_back(items, "AtariVox", "ATARIVOX");
  VarList::push_back(items, "SaveKey", "SAVEKEY");
  VarList::push_back(items, "Sega Genesis", "GENESIS");
  VarList::push_back(items, "Joy2B+", "JOY_2B+");
  VarList::push_back(items, "Kid Vid", "KIDVID");
  VarList::push_back(items, "Light Gun", "LIGHTGUN");
  VarList::push_back(items, "MindLink", "MINDLINK");
  VarList::push_back(items, "QuadTari", "QUADTARI");

  myLeftPortLabel = new StaticTextWidget(pane, _font, "Left port");
  myLeftPort = new PopUpWidget(pane, _font, items, kLeftCChanged);
  myLeftPort->setToolTip(Event::PreviousLeftPort, Event::NextLeftPort);
  wid.push_back(myLeftPort);

  myLeftPortDetected = new StaticTextWidget(pane, ifont, "Sega Genesis detected");

  myRightPortLabel = new StaticTextWidget(pane, _font, "Right port");
  myRightPort = new PopUpWidget(pane, _font, items, kRightCChanged);
  myRightPort->setToolTip(Event::PreviousRightPort, Event::NextRightPort);
  wid.push_back(myRightPort);

  myRightPortDetected = new StaticTextWidget(pane, ifont, "Sega Genesis detected");

  mySwapPorts = new CheckboxWidget(pane, _font, "Swap ports");
  mySwapPorts->setToolTip(Event::ToggleSwapPorts);
  wid.push_back(mySwapPorts);

  myQuadTariButton =
    new ButtonWidget(pane, _font, " QuadTari" + ELLIPSIS + " ", kQuadTariPressed);
  wid.push_back(myQuadTariButton);

  // EEPROM erase button for left/right controller
  myEraseEEPROMLabel = new StaticTextWidget(pane, _font, "AtariVox/SaveKey");
  myEraseEEPROMButton =
    new ButtonWidget(pane, _font, "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);
  myEraseEEPROMInfo = new StaticTextWidget(pane, ifont, "(for this game only)");

  mySwapPaddles = new CheckboxWidget(pane, _font, "Swap paddles");
  mySwapPaddles->setToolTip(Event::ToggleSwapPaddles);
  wid.push_back(mySwapPaddles);

  // Paddles
  myPaddlesCenter = new StaticTextWidget(pane, _font, "Paddles center:");

  myPaddleXCenterLabel = new StaticTextWidget(pane, _font, "X");
  myPaddleXCenter = new SliderWidget(pane, _font, 0, kPXCenterChanged, 6, "px", 0 , true);
  myPaddleXCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleXCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleXCenter->setTickmarkIntervals(4);
  myPaddleXCenter->setToolTip(Event::DecreasePaddleCenterX, Event::IncreasePaddleCenterX);
  wid.push_back(myPaddleXCenter);

  myPaddleYCenterLabel = new StaticTextWidget(pane, _font, "Y");
  myPaddleYCenter = new SliderWidget(pane, _font, 0, kPYCenterChanged, 6, "px", 0 , true);
  myPaddleYCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleYCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleYCenter->setTickmarkIntervals(4);
  myPaddleYCenter->setToolTip(Event::DecreasePaddleCenterY, Event::IncreasePaddleCenterY);
  wid.push_back(myPaddleYCenter);

  // Mouse
  myMouseControl = new CheckboxWidget(pane, _font, "Specific mouse axes", kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  VarList::push_back(ctrls, "None",           static_cast<uInt32>(MouseControl::Type::NoControl));
  VarList::push_back(ctrls, "Left Paddle A",  static_cast<uInt32>(MouseControl::Type::LeftPaddleA));
  VarList::push_back(ctrls, "Left Paddle B",  static_cast<uInt32>(MouseControl::Type::LeftPaddleB));
  VarList::push_back(ctrls, "Right Paddle A", static_cast<uInt32>(MouseControl::Type::RightPaddleA));
  VarList::push_back(ctrls, "Right Paddle B", static_cast<uInt32>(MouseControl::Type::RightPaddleB));
  VarList::push_back(ctrls, "Left Driving",   static_cast<uInt32>(MouseControl::Type::LeftDriving));
  VarList::push_back(ctrls, "Right Driving",  static_cast<uInt32>(MouseControl::Type::RightDriving));
  VarList::push_back(ctrls, "Left MindLink",  static_cast<uInt32>(MouseControl::Type::LeftMindLink));
  VarList::push_back(ctrls, "Right MindLink", static_cast<uInt32>(MouseControl::Type::RightMindLink));

  myMouseXLabel = new StaticTextWidget(pane, _font, "X-Axis is");
  myMouseX = new PopUpWidget(pane, _font, ctrls);
  wid.push_back(myMouseX);
  myMouseYLabel = new StaticTextWidget(pane, _font, "Y-Axis is");
  myMouseY = new PopUpWidget(pane, _font, ctrls);
  wid.push_back(myMouseY);

  myMouseRangeLabel = new StaticTextWidget(pane, _font, "Mouse axes range");
  myMouseRange = new SliderWidget(pane, _font, 0, 0, 4, "%");
  myMouseRange->setMinValue(1);
  myMouseRange->setMaxValue(100);
  myMouseRange->setTickmarkIntervals(4);
  myMouseRange->setToolTip("Adjust paddle range emulated by the mouse.",
    Event::DecreaseMouseAxesRange, Event::IncreaseMouseAxesRange);
  wid.push_back(myMouseRange);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("ControllerProps");

  // Describe the layout once; the pane runs it on every resize
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::GridLayout;
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using GUI::indentedItem;
    using GUI::stretchedItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();

    // The two ports and the EEPROM all take the same form: a label, the control
    // it names, and something beside that.  A grid IS that form -- the labels get
    // one column, as wide as the widest of them, and the controls get another --
    // so the three line up without anyone measuring a label.  What each port
    // detected goes in the control column too, which is what puts it under the
    // pop-up that reported it
    enum Col: int { LABEL, CTRL, EXTRA, COLS };
    enum Row: int { LEFT, LEFTDET, RIGHT, RIGHTDET, BREAK, EEPROM, ROWS };

    auto ports = std::make_unique<GridLayout>(COLS, ROWS, fontWidth, VGAP);
    ports->columnAuto(LABEL).columnAuto(CTRL).columnStretch(EXTRA);
    for(int r = 0; r < ROWS; ++r)
      ports->rowAuto(r);
    // The EEPROM is not one of the ports: an empty row sets it apart
    ports->rowFixed(BREAK, 0);

    // Each pop-up sizes its box to its own list; one shared width keeps the two
    // ports (and the two mouse axes below) flush with one another
    GUI::alignPopUps({myLeftPort, myRightPort});
    GUI::alignPopUps({myMouseX, myMouseY});

    ports->place(LABEL, LEFT, anchoredItem(myLeftPortLabel));
    ports->place(CTRL,  LEFT, anchoredItem(myLeftPort));
    ports->place(EXTRA, LEFT, indentedItem(mySwapPorts, fontWidth * 3));
    ports->place(CTRL,  LEFTDET, stretchedItem(myLeftPortDetected), COLS - CTRL);

    ports->place(LABEL, RIGHT, anchoredItem(myRightPortLabel));
    ports->place(CTRL,  RIGHT, anchoredItem(myRightPort));
    ports->place(EXTRA, RIGHT, indentedItem(myQuadTariButton, fontWidth * 3));
    ports->place(CTRL,  RIGHTDET, stretchedItem(myRightPortDetected), COLS - CTRL);

    // The Erase button fills the pop-ups' column, so it is as wide as they are
    ports->place(LABEL, EEPROM, anchoredItem(myEraseEEPROMLabel));
    ports->place(CTRL,  EEPROM, alignedItem(myEraseEEPROMButton, HAlign::Fill,
                                            VAlign::Center));
    ports->place(EXTRA, EEPROM, stretchedItem(myEraseEEPROMInfo));

    // Each of these has its own label, so each group gets its own column: the
    // two paddle-centre sliders, the two mouse-axis pop-ups, and the range slider
    // (which lines up with nothing)
    const int prefix = CheckboxWidget::prefixSize(_font);
    GUI::alignLabels({{myPaddleXCenterLabel, INDENT}, {myPaddleYCenterLabel, INDENT}});
    GUI::alignLabels({{myMouseXLabel, prefix}, {myMouseYLabel, prefix}});
    GUI::alignLabels({{myMouseRangeLabel}});
    GUI::alignPopUps({myMouseX, myMouseY});

    // The paddle options and the mouse axes run as two parallel columns
    auto paddleCol = std::make_unique<BoxLayout>(Dir::Vertical);
    paddleCol->addAuto(anchoredItem(mySwapPaddles));
    paddleCol->addSpace(VGAP);
    paddleCol->addAuto(anchoredItem(myPaddlesCenter));
    paddleCol->addSpace(VGAP);
    paddleCol->addAuto(labeledRow(myPaddleXCenterLabel, myPaddleXCenter, 0, INDENT));
    paddleCol->addSpace(VGAP);
    paddleCol->addAuto(labeledRow(myPaddleYCenterLabel, myPaddleYCenter, 0, INDENT));

    // The two axis popups are indented by the checkbox prefix, so they line up
    // under its text
    auto mouseCol = std::make_unique<BoxLayout>(Dir::Vertical);
    mouseCol->addAuto(anchoredItem(myMouseControl));
    mouseCol->addSpace(VGAP);
    auto mouseXRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    mouseXRow->addSpace(prefix);
    mouseXRow->addStretch(labeledRow(myMouseXLabel, myMouseX));
    mouseCol->addAuto(std::move(mouseXRow));
    mouseCol->addSpace(VGAP);
    auto mouseYRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    mouseYRow->addSpace(prefix);
    mouseYRow->addStretch(labeledRow(myMouseYLabel, myMouseY));
    mouseCol->addAuto(std::move(mouseYRow));
    mouseCol->addSpace(VGAP);
    mouseCol->addAuto(labeledRow(myMouseRangeLabel, myMouseRange));

    auto lowerRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    lowerRow->addFixed(std::move(paddleCol), fontWidth * 24 - INDENT);
    lowerRow->addStretch(std::move(mouseCol));

    // Every row is as tall as what it holds — including the two-column block at
    // the bottom, which is as tall as its taller column
    col.addAuto(std::move(ports));
    col.addSpace(VGAP * 4);
    col.addAuto(std::move(lowerRow));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addCartridgeTab()
{
  // 4) Cartridge properties.  The tab's controls are parented to a content pane;
  // the pane lays them out (see setLayout below) whenever the tab is sized
  // The link button is a pair of chevrons, so it is sized as if it were one
  const int bw = ButtonWidget::calcWidth(_font, ">");
  WidgetArray wid;

  const int tabID = myTab->addTab("Cartridge", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  myCartLabels[0] = new StaticTextWidget(pane, _font, "Name");
  myName = new EditTextWidget(pane, _font, 1);
  wid.push_back(myName);

  myCartLabels[1] = new StaticTextWidget(pane, _font, "MD5");
  myMD5 = new EditTextWidget(pane, _font, 1);
  myMD5->setEditable(false);

  myCartLabels[2] = new StaticTextWidget(pane, _font, "Manufacturer");
  myManufacturer = new EditTextWidget(pane, _font, 1);
  wid.push_back(myManufacturer);

  myCartLabels[3] = new StaticTextWidget(pane, _font, "Model", TextAlign::Left);
  myModelNo = new EditTextWidget(pane, _font, 1);
  wid.push_back(myModelNo);

  myCartLabels[4] = new StaticTextWidget(pane, _font, "Rarity");
  myRarity = new EditTextWidget(pane, _font, 1);
  wid.push_back(myRarity);

  myCartLabels[5] = new StaticTextWidget(pane, _font, "Note");
  myNote = new EditTextWidget(pane, _font, 1);
  wid.push_back(myNote);

  myCartLabels[6] = new StaticTextWidget(pane, _font, "Link");
  myUrl = new EditTextWidget(pane, _font, 1);
  myUrl->setID(kLinkId);
  wid.push_back(myUrl);

  myUrlButton =
    new ButtonWidget(pane, _font, bw, myUrl->getHeight(), ">>", kLinkPressed);
  wid.push_back(myUrlButton);

#ifdef IMAGE_SUPPORT
  const GUI::Font& ifont = instance().frameBuffer().infoFont();

  myCartLabels[7] = new StaticTextWidget(pane, _font, "Bezelname");
  myBezelName = new EditTextWidget(pane, _font, 1);
  myBezelName->setToolTip("Define the name of the bezel file.");
  wid.push_back(myBezelName);

  myBezelButton = new ButtonWidget(pane, _font, bw, myBezelName->getHeight(),
                                   ELLIPSIS, kBezelFilePressed);
  wid.push_back(myBezelButton);

  myBezelDetected = new StaticTextWidget(pane, ifont,
    "'1234567890123456789012345678901234567' selected");
#endif

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("CartridgeProps");

  // Describe the layout once; the pane runs it on every resize
  // The properties are a form: a label column, a field column that widens with
  // the dialog, and a button column used by the rows that have one.  A grid says
  // exactly that — and its label column is as wide as the longest label in it,
  // so adding a longer one needs no change here
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::GridLayout;
    using GUI::anchoredItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;

    enum Col: int { LABEL, FIELD, BUTTON, COLS };
    enum Row: int {
      NAME, MD5, MANUFACTURER, MODEL, RARITY, NOTE, LINK,
#ifdef IMAGE_SUPPORT
      BEZEL, BEZEL_DETECTED,
#endif
      ROWS
    };
    auto grid = std::make_unique<GridLayout>(COLS, ROWS, Dialog::fontWidth(),
                                             Dialog::vGap());
    // The fields widen with the dialog, but they are what the dialog is FOR, so
    // they say how much room they need — and the dialog's width follows from it
    grid->columnAuto(LABEL)
         .columnStretch(FIELD, 1, EditTextWidget::calcWidth(_font, 30))
         .columnAuto(BUTTON);
    for(int r = 0; r < ROWS; ++r)
      grid->rowAuto(r);

    // A plain property row: its field takes the width the button column does not
    const auto field = [&](int row, StaticTextWidget* label,
                           EditTextWidget* edit) {
      grid->place(LABEL, row, anchoredItem(label));
      grid->place(FIELD, row, alignedItem(edit, HAlign::Fill, VAlign::Center),
                  COLS - FIELD);
    };
    // ...and one whose field is followed by a browse button
    const auto browseField = [&](int row, StaticTextWidget* label,
                                 EditTextWidget* edit, ButtonWidget* button) {
      grid->place(LABEL,  row, anchoredItem(label));
      grid->place(FIELD,  row, alignedItem(edit, HAlign::Fill, VAlign::Center));
      grid->place(BUTTON, row, anchoredItem(button));
    };

    field(NAME,         myCartLabels[0], myName);
    field(MD5,          myCartLabels[1], myMD5);
    field(MANUFACTURER, myCartLabels[2], myManufacturer);
    field(MODEL,        myCartLabels[3], myModelNo);
    field(RARITY,       myCartLabels[4], myRarity);
    field(NOTE,         myCartLabels[5], myNote);
    browseField(LINK,   myCartLabels[6], myUrl, myUrlButton);
#ifdef IMAGE_SUPPORT
    browseField(BEZEL,  myCartLabels[7], myBezelName, myBezelButton);
    // The detected bezel, under the field it belongs to.  It FILLS what the
    // field above leaves it (like every other "detected" caption here), so the
    // specimen name it was built with cannot decide how wide the tab is
    grid->place(FIELD, BEZEL_DETECTED,
                alignedItem(myBezelDetected, HAlign::Fill, VAlign::Center),
                COLS - FIELD);
#endif

    col.addAuto(std::move(grid));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addHighScoresTab()
{
  // 4) High Scores properties.  The tab's controls are parented to a content
  // pane; the pane lays them out (see setLayout below) whenever the tab is sized
  WidgetArray wid;
  VariantList items;

  const int tabID = myTab->addTab("High Scores", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  const EditableWidget::TextFilter fAddr = [](char c) {
    return (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9');
  };
  const EditableWidget::TextFilter fVars = [](char c) {
    return (c >= '0' && c <= '9');
  };
  const EditableWidget::TextFilter fText = [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= ' ' && c < ',') || (c > ',' && c < '@');
  };

  myHighScores = new CheckboxWidget(pane, _font, "Enable High Scores",
                                    kHiScoresChanged);

  // Variations
  myVariationsLabel = new StaticTextWidget(pane, _font, "Variations");
  myVariations = new EditTextWidget(pane, _font, 3);
  myVariations->setTextFilter(fVars);
  myVariations->setMaxLen(3);
  myVariations->setToolTip("Define the number of game variations.");
  wid.push_back(myVariations);

  myVarAddressLabel = new StaticTextWidget(pane, _font, "Address");
  myVarAddress = new EditTextWidget(pane, _font, 4);
  myVarAddress->setTextFilter(fAddr);
  myVarAddress->setMaxLen(4);
  myVarAddress->setToolTip("Define the address (in hex format) where the variation number "
                           "is stored.");
  wid.push_back(myVarAddress);
  myVarAddressVal = new EditTextWidget(pane, _font, 3);
  myVarAddressVal->setEditable(false);

  myVarsBCD = new CheckboxWidget(pane, _font, "BCD", kHiScoresChanged);
  myVarsBCD->setToolTip("Check when the variation number is stored as BCD.");
  wid.push_back(myVarsBCD);
  myVarsZeroBased = new CheckboxWidget(pane, _font, "0-based", kHiScoresChanged);
  myVarsZeroBased->setToolTip("Check when the variation number is stored zero-based.");
  wid.push_back(myVarsZeroBased);

  // Score
  myScoreLabel = new StaticTextWidget(pane, _font, "Score");

  items.clear();
  for(uInt32 i = 1; i <= HSM::MAX_SCORE_DIGITS; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));
  myScoreDigitsLabel = new StaticTextWidget(pane, _font, "Digits");
  myScoreDigits = new PopUpWidget(pane, _font, items, kHiScoresChanged);
  myScoreDigits->setToolTip("Select the number of score digits displayed.");
  wid.push_back(myScoreDigits);

  items.clear();
  for(uInt32 i = 0; i <= HSM::MAX_SCORE_DIGITS - 3; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));
  myTrailingZeroesLabel = new StaticTextWidget(pane, _font, "0-digits");
  myTrailingZeroes = new PopUpWidget(pane, _font, items, kHiScoresChanged);
  myTrailingZeroes->setToolTip("Select the number of trailing score digits which are fixed to 0.");
  wid.push_back(myTrailingZeroes);

  myScoreBCD = new CheckboxWidget(pane, _font, "BCD", kHiScoresChanged);
  myScoreBCD->setToolTip("Check when the score is stored as BCD.");
  wid.push_back(myScoreBCD);
  myScoreInvert = new CheckboxWidget(pane, _font, "Invert");
  myScoreInvert->setToolTip("Check when a lower score (e.g. a timer) is better.");
  wid.push_back(myScoreInvert);

  // Score addresses
  myScoreAddressesLabel = new StaticTextWidget(pane, _font, "Addresses");
  for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
  {
    myScoreAddress[a] = new EditTextWidget(pane, _font, 4);
    myScoreAddress[a]->setTextFilter(fAddr);
    myScoreAddress[a]->setMaxLen(4);
    myScoreAddress[a]->setToolTip("Define the addresses (in hex format, highest byte first) "
                                  "where the score is stored.");
    wid.push_back(myScoreAddress[a]);
    myScoreAddressVal[a] = new EditTextWidget(pane, _font, 2);
    myScoreAddressVal[a]->setEditable(false);
  }

  myCurrentScoreLabel = new StaticTextWidget(pane, _font, "Current");
  myCurrentScore = new StaticTextWidget(pane, _font, "12345678");
  myCurrentScore->setToolTip("The score read using the current definitions.");

  // Special
  mySpecialLabel = new StaticTextWidget(pane, _font, "Special");
  mySpecialName = new EditTextWidget(pane, _font, HSM::MAX_SPECIAL_NAME);
  mySpecialName->setTextFilter(fText);
  mySpecialName->setMaxLen(HSM::MAX_SPECIAL_NAME);
  mySpecialName->setToolTip("Define a short label (up to 5 chars) for the optional,\ngame's "
                            "special value (e.g. 'Level', 'Wave', 'Round'" + ELLIPSIS + ")");
  wid.push_back(mySpecialName);

  mySpecialAddressLabel = new StaticTextWidget(pane, _font, "Address");
  mySpecialAddress = new EditTextWidget(pane, _font, 4);
  mySpecialAddress->setTextFilter(fAddr);
  mySpecialAddress->setMaxLen(4);
  mySpecialAddress->setToolTip("Define the address (in hex format) where the special "
                               "number is stored.");
  wid.push_back(mySpecialAddress);
  mySpecialAddressVal = new EditTextWidget(pane, _font, 3);
  mySpecialAddressVal->setEditable(false);

  mySpecialBCD = new CheckboxWidget(pane, _font, "BCD", kHiScoresChanged);
  mySpecialBCD->setToolTip("Check when the special number is stored as BCD.");
  wid.push_back(mySpecialBCD);
  mySpecialZeroBased = new CheckboxWidget(pane, _font, "0-based", kHiScoresChanged);
  mySpecialZeroBased->setToolTip("Check when the special number is stored zero-based.");
  wid.push_back(mySpecialZeroBased);

  // Note
  myHighScoreNotesLabel = new StaticTextWidget(pane, _font, "Note");
  myHighScoreNotes = new EditTextWidget(pane, _font, 1);
  myHighScoreNotes->setTextFilter(fText);
  myHighScoreNotes->setToolTip("Define some free text which explains the high scores properties.");
  wid.push_back(myHighScoreNotes);

  // Add items for tab 4
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("HighScoreProps");

  // Describe the layout once; the pane runs it on every resize.  The rows all
  // cross-reference each other's columns (the BCD checkboxes line up, as do the
  // address groups), which is exactly what a grid is for: a column is as wide as
  // the widest thing in it, and everything in it lines up by construction
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::GridLayout;
    using GUI::anchoredItem;
    using GUI::indentedItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();
    // The gap between the groups within a row -- it is the grid's own column
    // spacing, so no group has to open one for itself -- and the tight one that
    // ties a value field to the address it reads
    const int GAP = fontWidth * 2, TIE = fontWidth / 4;

    // The tab reads as three groups, and the values line up WITHIN a group, not
    // across the tab: Variations on its own, the three rows under Score, and
    // Special with the Note beneath it.  Each group's labels therefore get a
    // column of their own -- so a long label in one group cannot push the values
    // of another out -- and the clearance after a label comes with it
    GUI::alignLabels({{myVariationsLabel}});
    GUI::alignLabels({{myScoreDigitsLabel}, {myScoreAddressesLabel},
                      {myCurrentScoreLabel}});
    GUI::alignLabels({{mySpecialLabel}, {myHighScoreNotesLabel}});
    // The two address groups line up with each other; the trailing-zeroes label
    // names its own pop-up and lines up with nothing
    GUI::alignLabels({{myVarAddressLabel}, {mySpecialAddressLabel}});
    GUI::alignLabels({{myTrailingZeroesLabel}});

    // A label and the thing it names.  The gap between them is the label's own
    // (alignLabels sized it), so nothing here opens one.  Optionally indented,
    // to sit under a heading
    const auto labelled = [&](StaticTextWidget* label, int indent = 0) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      if(indent > 0)
        row->addSpace(indent);
      row->addAuto(anchoredItem(label));
      return row;
    };

    // An address group: its label, the address, and the value read from it
    const auto addrGroup = [&](StaticTextWidget* label, EditTextWidget* addr,
                               EditTextWidget* val) {
      auto row = labelled(label);
      row->addAuto(anchoredItem(addr));
      row->addSpace(TIE);
      row->addAuto(anchoredItem(val));
      return row;
    };

    // A row's label and its value, as one cell
    const auto field = [&](StaticTextWidget* label, Widget* value, int indent = 0) {
      auto row = labelled(label, indent);
      row->addAuto(anchoredItem(value));
      return row;
    };

    // Columns: the row's own label+value | address group | BCD | trailing option,
    // one GAP apart.  It is these three that cross-reference each other down the
    // tab, so they are the ones a grid has to line up; the label and value beside
    // them belong to their row alone, and pairing them keeps the tab no wider
    // than the widest single row.  Rows alternate content and the gaps between
    // them, so each gap is stated where it falls rather than being one uniform
    // spacing
    enum Col: int { FIELD, ADDR, BCD, OPT, COLS };
    enum Row: int {
      VARS, GAP1, SCORE, GAP2, DIGITS, GAP3, ADDRS, GAP4, CURRENT, GAP5,
      SPECIAL, GAP6, NOTE, ROWS
    };
    auto grid = std::make_unique<GridLayout>(COLS, ROWS, GAP);

    for(const int c: {FIELD, ADDR, BCD})
      grid->columnAuto(c);
    grid->columnStretch(OPT);

    for(const int r: {VARS, SCORE, DIGITS, ADDRS, CURRENT, SPECIAL, NOTE})
      grid->rowAuto(r);
    grid->rowFixed(GAP1, VGAP * 3).rowFixed(GAP2, VGAP).rowFixed(GAP3, VGAP)
         .rowFixed(GAP4, VGAP).rowFixed(GAP5, VGAP * 3).rowFixed(GAP6, VGAP * 3);

    // Variations
    grid->place(FIELD, VARS, field(myVariationsLabel, myVariations));
    grid->place(ADDR,  VARS, addrGroup(myVarAddressLabel, myVarAddress,
                                       myVarAddressVal));
    grid->place(BCD,   VARS, anchoredItem(myVarsBCD));
    grid->place(OPT,   VARS, anchoredItem(myVarsZeroBased));

    // Score, whose three rows are indented under their heading
    grid->place(FIELD, SCORE, anchoredItem(myScoreLabel));

    grid->place(FIELD, DIGITS, field(myScoreDigitsLabel, myScoreDigits, INDENT));
    grid->place(ADDR,  DIGITS, field(myTrailingZeroesLabel, myTrailingZeroes));
    grid->place(BCD,   DIGITS, anchoredItem(myScoreBCD));
    grid->place(OPT,   DIGITS, anchoredItem(myScoreInvert));

    // The score addresses run past the columns beside them, so they span the rest
    auto scoreAddrs = labelled(myScoreAddressesLabel, INDENT);
    for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
    {
      scoreAddrs->addAuto(anchoredItem(myScoreAddress[a]));
      scoreAddrs->addSpace(TIE);
      scoreAddrs->addAuto(anchoredItem(myScoreAddressVal[a]));
      scoreAddrs->addSpace(GAP);
    }
    scoreAddrs->addStretchSpace();
    grid->place(FIELD, ADDRS, std::move(scoreAddrs), COLS - FIELD);

    // The score read back is wider than the fields above it, so it spans too
    grid->place(FIELD, CURRENT, field(myCurrentScoreLabel, myCurrentScore, INDENT),
                COLS - FIELD);

    // Special
    grid->place(FIELD, SPECIAL, field(mySpecialLabel, mySpecialName));
    grid->place(ADDR,  SPECIAL, addrGroup(mySpecialAddressLabel, mySpecialAddress,
                                          mySpecialAddressVal));
    grid->place(BCD,   SPECIAL, anchoredItem(mySpecialBCD));
    grid->place(OPT,   SPECIAL, anchoredItem(mySpecialZeroBased));

    // Note, whose field takes all the width the other columns leave
    auto noteRow = labelled(myHighScoreNotesLabel);
    noteRow->addStretch(alignedItem(myHighScoreNotes, HAlign::Fill, VAlign::Center));
    grid->place(FIELD, NOTE, std::move(noteRow), COLS - FIELD);

    // Everything below the enable checkbox lines up under its label
    auto gridRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    gridRow->addSpace(CheckboxWidget::prefixSize(_font));
    gridRow->addStretch(std::move(grid));

    col.addAuto(anchoredItem(myHighScores));
    col.addSpace(VGAP * 2);
    col.addAuto(std::move(gridRow));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConfig()
{
  if(instance().hasConsole())
  {
    myGameProperties = instance().console().properties();
    myGameFile = instance().romFile();
  }
  else
  {
    const string& md5 = instance().launcher().selectedRomMD5();
    instance().propSet().getMD5(md5, myGameProperties);
    myGameFile = FSNode(instance().launcher().selectedRom());
  }

  string title = std::format("Game properties - {}", myGameProperties.get(PropType::Cart_Name));
  if(_font.getStringWidth(title) > getWidth() - fontWidth() * 6)
    title = title.substr(0, getWidth() / fontWidth() - 7) + ELLIPSIS;
  setTitle(title);

  loadEmulationProperties(myGameProperties);
  loadConsoleProperties(myGameProperties);
  loadControllerProperties(myGameProperties);
  loadCartridgeProperties(myGameProperties);
  loadHighScoresProperties(myGameProperties);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadEmulationProperties(const Properties& props)
{
  myBSFilter->setState(instance().settings().getBool("filterbstypes"));
  updateBSTypes();

  VariantList items;
  string bsDetected;

  myBSType->setSelected(props.get(PropType::Cart_Type), "AUTO");
  if(myBSType->getSelectedTag().toString() == "AUTO")
  {
    if(instance().hasConsole())
    {
      string bs = instance().console().about().BankSwitch;
      const size_t pos = bs.find_first_of('*');
      // remove '*':
      if(pos != string::npos)
        bs = bs.substr(0, pos) + bs.substr(pos + 1);
      bsDetected = std::format("{} detected", bs);
    }
    else
    {
      string md5{props.get(PropType::Cart_MD5)};

      // Try to load the image for auto detection
      if(myGameFile.exists() && !myGameFile.isDirectory())
        if(ByteArray image = instance().openROM(myGameFile, md5); !image.empty())
          bsDetected = std::format("{} detected",
              Bankswitch::typeToDesc(CartDetector::autodetectType(image)));
    }
  }
  myTypeDetected->setLabel(bsDetected);

  // Start bank
  VarList::push_back(items, "Auto", "AUTO");
  if(instance().hasConsole())
  {
    const uInt16 numBanks = instance().console().cartridge().romBankCount();

    for(uInt16 i = 0; i < numBanks; ++i)
      VarList::push_back(items, i, i);
  }
  else
  {
    string_view startBank = props.get(PropType::Cart_StartBank);

    VarList::push_back(items, startBank, startBank);
  }
  myStartBank->addItems(items);
  myStartBank->setSelected(props.get(PropType::Cart_StartBank), "Auto");

  myFormat->setSelected(props.get(PropType::Display_Format), "AUTO");
  if(instance().hasConsole() && myFormat->getSelectedTag().toString() == "AUTO")
  {
    const string& format = instance().console().about().DisplayFormat;
    string label;
    if (format.at(format.length() - 1) == '*')
      label = format.substr(0, format.length() - 1);
    else
      label = format;
    myFormatDetected->setLabel(label + " detected");
  }
  else
    myFormatDetected->setLabel("");

  // if phosphor is always enabled, disable game specific phosphor settings
  const string mode = instance().settings().getString(PhosphorHandler::SETTING_MODE);
  const bool usePhosphor = props.get(PropType::Display_Phosphor) == "YES";
  myPhosphor->setState(usePhosphor);
  if (mode == PhosphorHandler::VALUE_ALWAYS)
    myPhosphor->setLabel("Phosphor (enabled for all ROMs");
  else if (mode == PhosphorHandler::VALUE_AUTO)
    myPhosphor->setLabel("Phosphor (auto-enabled/disabled for all ROMs)");
  else if (mode == PhosphorHandler::VALUE_AUTO_ON)
    myPhosphor->setLabel("Phosphor (auto-enabled for all ROMs)");
  else
    myPhosphor->setLabel("Phosphor");

  string_view blend = props.get(PropType::Display_PPBlend);
  myPPBlend->setValue(BSPF::stoi(blend));

  // set vertical center
  const Int32 vcenter = BSPF::stoi(props.get(PropType::Display_VCenter));
  myVCenter->setValueLabel(vcenter);
  myVCenter->setValue(vcenter);
  myVCenter->setValueUnit(vcenter ? "px" : "");

  mySound->setState(props.get(PropType::Cart_Sound) == "STEREO");

  updateMultiCart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadConsoleProperties(const Properties& props)
{
  myLeftDiffGroup->setSelected(props.get(PropType::Console_LeftDiff) == "A" ? 0 : 1);
  myRightDiffGroup->setSelected(props.get(PropType::Console_RightDiff) == "A" ? 0 : 1);
  myTVTypeGroup->setSelected(props.get(PropType::Console_TVType) == "BW" ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadControllerProperties(const Properties& props)
{
  string controller{props.get(PropType::Controller_Left)};
  myLeftPort->setSelected(controller, "AUTO");
  controller = props.get(PropType::Controller_Right);
  myRightPort->setSelected(controller, "AUTO");

  mySwapPorts->setState(props.get(PropType::Console_SwapPorts) == "YES");
  mySwapPaddles->setState(props.get(PropType::Controller_SwapPaddles) == "YES");

  // Paddle centers
  myPaddleXCenter->setValue(BSPF::stoi(props.get(PropType::Controller_PaddlesXCenter)));
  myPaddleYCenter->setValue(BSPF::stoi(props.get(PropType::Controller_PaddlesYCenter)));

  // MouseAxis property (potentially contains 'range' information)
  const string_view axisStr = props.get(PropType::Controller_MouseAxis);
  const bool autoAxis = equalsIgnoreCase(axisStr, "AUTO") ||
                        axisStr.empty() ||
                        axisStr[0] == 'A';
  myMouseControl->setState(!autoAxis);
  if(autoAxis)
  {
    myMouseX->setSelectedIndex(0);
    myMouseY->setSelectedIndex(0);
  }
  else
  {
    myMouseX->setSelected(axisStr[0] - '0');
    myMouseY->setSelected(axisStr[1] - '0');
  }
  myMouseXLabel->setEnabled(!autoAxis);
  myMouseX->setEnabled(!autoAxis);
  myMouseYLabel->setEnabled(!autoAxis);
  myMouseY->setEnabled(!autoAxis);

  // Parse optional range value after the control string
  const auto spacePos = axisStr.find(' ');
  if(spacePos != string_view::npos)
  {
    int range = 100;
    const string_view rangeStr = axisStr.substr(spacePos + 1);
    std::from_chars(rangeStr.data(), rangeStr.data() + rangeStr.size(), range);
    myMouseRange->setValue(range);
  }
  else
    myMouseRange->setValue(100);

  updateControllerStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadCartridgeProperties(const Properties& props)
{
  myName->setText(props.get(PropType::Cart_Name));
  myMD5->setText(props.get(PropType::Cart_MD5));
  myManufacturer->setText(props.get(PropType::Cart_Manufacturer));
  myModelNo->setText(props.get(PropType::Cart_ModelNo));
  myRarity->setText(props.get(PropType::Cart_Rarity));
  myNote->setText(props.get(PropType::Cart_Note));
  myUrl->setText(props.get(PropType::Cart_Url));

#ifdef IMAGE_SUPPORT
  bool autoSelected = false;
  string bezelName{props.get(PropType::Bezel_Name)};
  if(bezelName.empty())
  {
    bezelName = Bezel::getName(instance().bezelDir().getPath(), props);
    if(bezelName != "default")
      autoSelected = true;
    else
      bezelName = "";
  }
  myBezelName->setText(bezelName);

  if(autoSelected)
    myBezelDetected->setLabel("auto-selected");
  else
    myBezelDetected->setLabel("");
#endif

  updateLink();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::loadHighScoresProperties(const Properties& props)
{
  HSM::ScoresProps info;
  uInt32 numVariations = 0;
  const bool enable = instance().highScores().get(props, numVariations, info);

  myHighScores->setState(enable);

  myVariations->setText(to_string(numVariations));

  myScoreDigits->setSelected(info.numDigits);
  myTrailingZeroes->setSelected(info.trailingZeroes);
  myScoreBCD->setState(info.scoreBCD);
  myScoreInvert->setState(info.scoreInvert);
  myVarsBCD->setState(info.varsBCD);
  myVarsZeroBased->setState(info.varsZeroBased);
  mySpecialName->setText(info.special);
  mySpecialBCD->setState(info.specialBCD);
  mySpecialZeroBased->setState(info.specialZeroBased);

  myHighScoreNotes->setText(info.notes);

  myVarAddress->setText(std::format("{:X}", info.varsAddr));
  mySpecialAddress->setText(std::format("{:X}", info.specialAddr));

  for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
  {
    if(a < HighScoresManager::numAddrBytes(info.numDigits, info.trailingZeroes))
      myScoreAddress[a]->setText(std::format("{:X}", info.scoreAddr[a]));
    else
      myScoreAddress[a]->setText("");
  }
  updateHighScoresWidgets();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveProperties()
{
  instance().settings().setValue("filterbstypes", myBSFilter->getState());

  // Emulation properties
  myGameProperties.set(PropType::Cart_Type, myBSType->getSelectedTag().toString());
  myGameProperties.set(PropType::Cart_StartBank, myStartBank->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Format, myFormat->getSelectedTag().toString());
  myGameProperties.set(PropType::Display_Phosphor, myPhosphor->getState() ? "YES" : "NO");
  myGameProperties.set(PropType::Display_PPBlend, myPPBlend->getValueLabel() == "Off" ? "0" :
                       myPPBlend->getValueLabel());
  const Int32 vcenter = myVCenter->getValue();

  myGameProperties.set(PropType::Display_VCenter, to_string(vcenter));
  myGameProperties.set(PropType::Cart_Sound, mySound->getState() ? "STEREO" : "MONO");

  // Console properties
  myGameProperties.set(PropType::Console_TVType, myTVTypeGroup->getSelected() ? "BW" : "COLOR");
  myGameProperties.set(PropType::Console_LeftDiff, myLeftDiffGroup->getSelected() ? "B" : "A");
  myGameProperties.set(PropType::Console_RightDiff, myRightDiffGroup->getSelected() ? "B" : "A");

  // Controller properties
  string controller = myLeftPort->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Left, controller);
  if(controller != "AUTO" && controller != "QUADTARI")
  {
    myGameProperties.set(PropType::Controller_Left1, "");
    myGameProperties.set(PropType::Controller_Left2, "");
  }

  controller = myRightPort->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Right, controller);
  if(controller != "AUTO" && controller != "QUADTARI")
  {
    myGameProperties.set(PropType::Controller_Right1, "");
    myGameProperties.set(PropType::Controller_Right2, "");
  }

  myGameProperties.set(PropType::Console_SwapPorts, (mySwapPorts->isEnabled() && mySwapPorts->getState()) ? "YES" : "NO");
  myGameProperties.set(PropType::Controller_SwapPaddles, mySwapPaddles->getState() ? "YES" : "NO");

  // Paddle center
  myGameProperties.set(PropType::Controller_PaddlesXCenter, std::to_string(myPaddleXCenter->getValue()));
  myGameProperties.set(PropType::Controller_PaddlesYCenter, std::to_string(myPaddleYCenter->getValue()));

  // MouseAxis property (potentially contains 'range' information)
  string mcontrol = "AUTO";
  if(myMouseControl->getState())
    mcontrol = myMouseX->getSelectedTag().toString() +
               myMouseY->getSelectedTag().toString();
  const string range = myMouseRange->getValueLabel();
  if(range != "100")
    mcontrol += " " + range;
  myGameProperties.set(PropType::Controller_MouseAxis, mcontrol);

  // Cartridge properties
  myGameProperties.set(PropType::Cart_Name, myName->getText());
  myGameProperties.set(PropType::Cart_Manufacturer, myManufacturer->getText());
  myGameProperties.set(PropType::Cart_ModelNo, myModelNo->getText());
  myGameProperties.set(PropType::Cart_Rarity, myRarity->getText());
  myGameProperties.set(PropType::Cart_Note, myNote->getText());
  myGameProperties.set(PropType::Cart_Url, myUrl->getText());
#ifdef IMAGE_SUPPORT
  // avoid saving auto-selected bezel names:
  if(myBezelName->getText() == Bezel::getName(instance().bezelDir().getPath(), myGameProperties))
    myGameProperties.reset(PropType::Bezel_Name);
  else
    myGameProperties.set(PropType::Bezel_Name, myBezelName->getText());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveConfig()
{
  saveProperties();

  saveHighScoresProperties();

  // Always insert; if the properties are already present, nothing will happen
  instance().propSet().insert(myGameProperties);
  instance().saveConfig();

  // In any event, inform the Console
  if(instance().hasConsole())
  {
    instance().console().setProperties(myGameProperties);

    // update 'Emulation' tab settings immediately
    instance().console().setFormat(myFormat->getSelected());
    instance().frameBuffer().tiaSurface().enablePhosphor(myPhosphor->getState(), myPPBlend->getValue());
    instance().console().updateVcenter(myVCenter->getValue());
    instance().console().initializeAudio();

    // update 'Console' tab settings immediately
    instance().console().switches().setTvColor(myTVTypeGroup->getSelected() == 0);
    instance().console().switches().setLeftDifficultyA(myLeftDiffGroup->getSelected() == 0);
    instance().console().switches().setRightDifficultyA(myRightDiffGroup->getSelected() == 0);

    // update 'Controllers' tab settings immediately
    instance().console().setControllers(myGameProperties.get(PropType::Cart_MD5));

    Paddles::setAnalogXCenter(myPaddleXCenter->getValue());
    Paddles::setAnalogYCenter(myPaddleYCenter->getValue());
    Paddles::setDigitalPaddleRange(myMouseRange->getValue());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::saveHighScoresProperties()
{
  HSM::ScoresProps info;

  if(myHighScores->getState())
  {
    // limit variants and special size
    myVariations->setText(myVariations->getText().substr(0, 3));
    mySpecialName->setText(mySpecialName->getText().substr(0, HSM::MAX_SPECIAL_NAME));

    // fill format
    info.varsZeroBased = myVarsZeroBased->getState();
    info.varsBCD = myVarsBCD->getState();

    info.numDigits = myScoreDigits->getSelected() + 1;
    info.trailingZeroes = myTrailingZeroes->getSelected();
    info.scoreBCD = myScoreBCD->getState();
    info.scoreInvert = myScoreInvert->getState();

    info.special = mySpecialName->getText();
    info.specialZeroBased = mySpecialZeroBased->getState();
    info.specialBCD = mySpecialBCD->getState();

    info.notes = myHighScoreNotes->getText();

    // fill addresses
    string strAddr;

    strAddr = myVarAddress->getText();
    info.varsAddr = BSPF::stoi<16>(strAddr, HSM::DEFAULT_ADDRESS);
    strAddr = mySpecialAddress->getText();
    info.specialAddr = BSPF::stoi<16>(strAddr, HSM::DEFAULT_ADDRESS);

    for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
    {
      strAddr = myScoreAddress[a]->getText();
      info.scoreAddr[a] = BSPF::stoi<16>(strAddr, HSM::DEFAULT_ADDRESS);
    }

    const string strVars = myVariations->getText();

    HighScoresManager::set(myGameProperties, BSPF::stoi(strVars,
      HSM::DEFAULT_VARIATION), info);
  }
  else
  {
    myGameProperties.reset(PropType::Cart_Highscore);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setDefaults()
{
  // Load the default properties
  Properties defaultProperties;
  string_view md5 = myGameProperties.get(PropType::Cart_MD5);

  instance().propSet().getMD5(md5, defaultProperties, true);

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation properties
      loadEmulationProperties(defaultProperties);
      myBSFilter->setState(true);
      break;

    case 1: // Console properties
      loadConsoleProperties(defaultProperties);
      break;

    case 2: // Controller properties
      loadControllerProperties(defaultProperties);
      break;

    case 3: // Cartridge properties
      loadCartridgeProperties(defaultProperties);
      break;

    case 4: // High Scores properties
      loadHighScoresProperties(defaultProperties);
      break;

    default: // make the compiler happy
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateMultiCart()
{
  static constexpr std::array<string_view, Bankswitch::NumMulti> MultiCart = {
    "2IN1", "4IN1", "8IN1", "16IN1", "32IN1", "64IN1", "128IN1"
  };
  const string& selected = myBSType->getSelectedTag().toString();
  const string& detected = myTypeDetected->getLabel();

  const bool isMulti = std::ranges::any_of(MultiCart,
      [&](string_view entry) { return entry == selected; });

  const bool isInMulti = !isMulti && std::ranges::any_of(MultiCart,
      [&](string_view entry) {
          return detected.contains(std::format("{} [", entry));
      });

  // en/disable Emulation tab widgets
  myBSTypeLabel->setEnabled(!isInMulti);
  myBSType->setEnabled(!isInMulti); // TODO: currently only auto-detected, add using properties
  myBSFilter->setEnabled(!isInMulti);
  myStartBankLabel->setEnabled(!isMulti && instance().hasConsole());
  myStartBank->setEnabled(!isMulti && instance().hasConsole());
  myFormatLabel->setEnabled(!isMulti);
  myFormat->setEnabled(!isMulti);

  // if phosphor is always enabled, disable game specific phosphor settings
  const bool globalPhosphor = isMulti
    || instance().settings().getString(PhosphorHandler::SETTING_MODE) != PhosphorHandler::VALUE_BYROM;
  myPhosphor->setEnabled(!globalPhosphor);
  myPPBlend->setEnabled(!globalPhosphor && myPhosphor->getState());

  myVCenter->setEnabled(!isMulti);
  // if stereo is always enabled, disable game specific stereo setting
  mySound->setEnabled(!instance().audioSettings().stereo() && !isMulti);

  myTab->enableTab(1, !isMulti); // en/disable Console tab
  myTab->enableTab(2, !isMulti); // en/disable Controller tab
  myTab->enableTab(4, !isMulti); // en/disable Highscore tab
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateBSTypes()
{
  VariantList items;

  const size_t gameSize = myGameFile.getSize();
  for(const auto i : std::views::iota(0U, Bankswitch::NumSchemes))
  {
      const auto& [minSize, maxSize] = Bankswitch::Sizes[i];
      if(!myBSFilter->getState() ||
        ((minSize == Bankswitch::any_KB || gameSize >= minSize) &&
         (maxSize == Bankswitch::any_KB || gameSize <= maxSize)))
      {
          const auto& [name, desc] = Bankswitch::BSList[i];
          VarList::push_back(items, desc, name);
      }
  }
  myBSType->addItems(items);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateControllerStates()
{
  const bool swapPorts = mySwapPorts->getState();
  ByteArray image;
  string md5{myGameProperties.get(PropType::Cart_MD5)};

  // try to load the image for auto detection
  if(!instance().hasConsole())
  {
    const FSNode& node = FSNode(instance().launcher().selectedRom());

    if(node.exists() && !node.isDirectory())
      image = instance().openROM(node, md5);
  }
  string label;
  Controller::Type type = Controller::getType(myLeftPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
    {
      label = (!swapPorts ? instance().console().leftController().name()
               : instance().console().rightController().name()) + " detected";
      if(BSPF::startsWithIgnoreCase(label, "QT"))
        label = "QuadTari detected"; // remove plugged-in controller names
    }
    else if(!image.empty())
      label = std::format("{} detected", ControllerDetector::detectName(image, type,
                                             !swapPorts ? Controller::Jack::Left : Controller::Jack::Right,
                                             instance().settings()));
  }
  myLeftPortDetected->setLabel(label);

  label = "";
  type = Controller::getType(myRightPort->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
    {
      label = (!swapPorts ? instance().console().rightController().name()
               : instance().console().leftController().name()) + " detected";
      if(BSPF::startsWithIgnoreCase(label, "QT"))
        label = "QuadTari detected"; // remove plugged-in controller names
    }
    else if(!image.empty())
      label = std::format("{} detected", ControllerDetector::detectName(image, type,
                                             !swapPorts ? Controller::Jack::Right : Controller::Jack::Left,
                                             instance().settings()));
  }
  myRightPortDetected->setLabel(label);

  const string& contrLeft = myLeftPort->getSelectedTag().toString();
  const string& contrRight = myRightPort->getSelectedTag().toString();
  bool enableEEEraseButton = false;

  // Compumate bankswitching scheme doesn't allow to select controllers
  const bool enableSelectControl = myBSType->getSelectedTag() != "CM";
  // Enable Swap Paddles checkbox only for paddle games
  const bool enablePaddles = BSPF::startsWithIgnoreCase(contrLeft, "PADDLES") ||
    BSPF::startsWithIgnoreCase(contrRight, "PADDLES") ||
    BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "Paddles") ||
    BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "Paddles");

  if(instance().hasConsole())
  {
    const Controller& lport = instance().console().leftController();
    const Controller& rport = instance().console().rightController();

    // we only enable the button if we have a valid previous and new controller.
    const bool enableBtnForLeft =
      (contrLeft == "AUTO" || contrLeft == "SAVEKEY" || contrLeft == "ATARIVOX") &&
      (lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox);
    const bool enableBtnForRight =
      (contrRight == "AUTO" || contrRight == "SAVEKEY" || contrRight == "ATARIVOX") &&
      (rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox);
    enableEEEraseButton = enableBtnForLeft || enableBtnForRight;
  }

  myLeftPortLabel->setEnabled(enableSelectControl);
  myRightPortLabel->setEnabled(enableSelectControl);
  myLeftPort->setEnabled(enableSelectControl);
  myRightPort->setEnabled(enableSelectControl);
  myQuadTariButton->setEnabled(BSPF::startsWithIgnoreCase(contrLeft, "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(contrRight, "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "QT") ||
                               BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "QUADTARI") ||
                               BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "QT"));

  mySwapPorts->setEnabled(enableSelectControl);
  mySwapPaddles->setEnabled(enablePaddles);

  myEraseEEPROMLabel->setEnabled(enableEEEraseButton);
  myEraseEEPROMButton->setEnabled(enableEEEraseButton);
  myEraseEEPROMInfo->setEnabled(enableEEEraseButton);

  myPaddlesCenter->setEnabled(enablePaddles);
  myPaddleXCenterLabel->setEnabled(enablePaddles);
  myPaddleXCenter->setEnabled(enablePaddles);
  myPaddleYCenterLabel->setEnabled(enablePaddles);
  myPaddleYCenter->setEnabled(enablePaddles);

  const bool enableMouse = enablePaddles ||
    BSPF::startsWithIgnoreCase(contrLeft, "Driving") ||
    BSPF::startsWithIgnoreCase(contrRight, "Driving") ||
    BSPF::startsWithIgnoreCase(contrLeft, "MindLink") ||
    BSPF::startsWithIgnoreCase(contrRight, "MindLink");

  myMouseControl->setEnabled(enableMouse);
  myMouseXLabel->setEnabled(enableMouse && myMouseControl->getState());
  myMouseX->setEnabled(enableMouse && myMouseControl->getState());
  myMouseYLabel->setEnabled(enableMouse && myMouseControl->getState());
  myMouseY->setEnabled(enableMouse && myMouseControl->getState());

  myMouseRangeLabel->setEnabled(enablePaddles);
  myMouseRange->setEnabled(enablePaddles);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::eraseEEPROM()
{
  Controller& lport = instance().console().leftController();
  Controller& rport = instance().console().rightController();

  if(lport.type() == Controller::Type::SaveKey ||
     lport.type() == Controller::Type::AtariVox)
  {
    auto& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::Type::SaveKey ||
     rport.type() == Controller::Type::AtariVox)
  {
    auto& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateLink()
{
  const string& link = myUrl->getText();
  const bool enable = startsWithIgnoreCase(link, "http://")
    || startsWithIgnoreCase(link, "https://")
    || startsWithIgnoreCase(link, "www.");

  myUrlButton->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateHighScoresWidgets()
{
  const bool enable = myHighScores->getState();
  const bool enableVars = enable && myVariations->getText() > "1";
  const bool enableSpecial = enable && !mySpecialName->getText().empty();
  const bool enableConsole = instance().hasConsole();
  const uInt32 numAddr = HighScoresManager::numAddrBytes(
      myScoreDigits->getSelected() + 1, myTrailingZeroes->getSelected());

  // enable widgets
  //myARMGame->setEnabled(enable);
  myVariationsLabel->setEnabled(enable);
  myVariations->setEnabled(enable);
  myVariations->setEditable(enable);
  myVarAddressLabel->setEnabled(enableVars);
  myVarAddress->setEnabled(enableVars);
  myVarAddress->setEditable(enableVars);
  myVarAddressVal->setEnabled(enableVars && enableConsole);
  myVarsBCD->setEnabled(enableVars && BSPF::stoi(myVariations->getText(), 1) >= 10);
  myVarsZeroBased->setEnabled(enableVars);

  myScoreLabel->setEnabled(enable);
  myScoreDigitsLabel->setEnabled(enable);
  myScoreDigits->setEnabled(enable);
  myScoreBCD->setEnabled(enable);
  myTrailingZeroesLabel->setEnabled(enable);
  myTrailingZeroes->setEnabled(enable);
  myScoreInvert->setEnabled(enable);

  myScoreAddressesLabel->setEnabled(enable);

  for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
  {
    myScoreAddress[a]->setEnabled(enable && numAddr > a);
    myScoreAddressVal[a]->setEnabled(enable && numAddr > a&& enableConsole);
  }

  myCurrentScoreLabel->setEnabled(enable && enableConsole);
  myCurrentScore->setEnabled(enable && enableConsole);

  mySpecialLabel->setEnabled(enable);
  mySpecialName->setEnabled(enable);
  mySpecialName->setEditable(enable);
  mySpecialAddressLabel->setEnabled(enableSpecial);
  mySpecialAddress->setEnabled(enableSpecial);
  mySpecialAddress->setEditable(enableSpecial);
  mySpecialAddressVal->setEnabled(enableSpecial && enableConsole);
  mySpecialBCD->setEnabled(enableSpecial);
  mySpecialZeroBased->setEnabled(enableSpecial);

  myHighScoreNotesLabel->setEnabled(enable);
  myHighScoreNotes->setEnabled(enable);

  // verify and update widget data

  // update variations RAM value
  setAddressVal(myVarAddress, myVarAddressVal, myVarsBCD->getState(),
                myVarsZeroBased->getState(), BSPF::stoi(myVariations->getText(), 1));

  setAddressVal(mySpecialAddress, mySpecialAddressVal, mySpecialBCD->getState(),
                mySpecialZeroBased->getState());

  // update score RAM values and resulting scores
  HSM::ScoreAddresses scoreAddr{};

  for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
  {
    if(a < numAddr)
    {
      setAddressVal(myScoreAddress[a], myScoreAddressVal[a]);
      const string strAddr = myScoreAddress[a]->getText();
      scoreAddr[a] = BSPF::stoi<16>(strAddr, HSM::DEFAULT_ADDRESS);
    }
    else
      myScoreAddressVal[a]->setText("");
  }

  const Int32 score = instance().highScores().score(numAddr, myTrailingZeroes->getSelected(),
                                                    myScoreBCD->getState(), scoreAddr);

  myCurrentScore->setLabel(instance().highScores().formattedScore(score));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::setAddressVal(const EditTextWidget* addressWidget, EditTextWidget* valWidget,
                                   bool isBCD, bool zeroBased, uInt8 maxVal)
{
  string strAddr;

  // limit address size
  strAddr = addressWidget->getText();
  strAddr = strAddr.substr(0, HSM::MAX_ADDR_CHARS);

  if(instance().hasConsole() && valWidget->isEnabled())
  {
    // convert to number and read from memory
    const uInt16 addr = BSPF::stoi<16>(strAddr, HSM::DEFAULT_ADDRESS);
    uInt8 val = instance().highScores().peek(addr);
    val = HighScoresManager::convert(val, maxVal, isBCD, zeroBased);

    // format output and display in value widget
    valWidget->setText(std::format("{}", static_cast<uInt16>(val)));
  }
  else
    valWidget->setText("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::exportCurrentPropertiesToDisk(const FSNode& node)
{
  saveProperties();

  KeyValueRepositoryPropertyFile repo(node);

  if(myGameProperties.save(repo))
    instance().frameBuffer().showTextMessage("ROM properties exported");
  else
    instance().frameBuffer().showTextMessage("Error exporting ROM properties");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, int cmd,
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

    case kExportPressed:
      BrowserDialog::show(this, _font, "Export Properties as",
                          instance().userDir().getPath() +
                            myGameFile.getNameWithExt(".pro"),
                          BrowserDialog::Mode::FileSave,
                          [this](bool OK, const FSNode& node) {
                            if(OK) exportCurrentPropertiesToDisk(node);
                          });
      break;

    case TabWidget::kTabChangedCmd:
      if(data == 2)  // 'Controllers' tab selected
        updateControllerStates();

      // The underlying dialog still needs access to this command
      Dialog::handleCommand(sender, cmd, data, 0);
      break;

    case kLeftCChanged:
    case kRightCChanged:
      updateControllerStates();
      break;

    case kQuadTariPressed:
    {
      const bool enableLeft =
        BSPF::startsWithIgnoreCase(myLeftPort->getSelectedTag().toString(), "QUADTARI") ||
        BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "QT") ||
        BSPF::startsWithIgnoreCase(myLeftPortDetected->getLabel(), "QUADTARI");
      const bool enableRight =
        BSPF::startsWithIgnoreCase(myRightPort->getSelectedTag().toString(), "QUADTARI") ||
        BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "QT") ||
        BSPF::startsWithIgnoreCase(myRightPortDetected->getLabel(), "QUADTARI");

      if(!myQuadTariDialog)
        myQuadTariDialog = std::make_unique<QuadTariDialog>
          (this, _font, myGameProperties);
      myQuadTariDialog->show(enableLeft, enableRight);
      break;
    }
    case kEEButtonPressed:
      eraseEEPROM();
      break;

    case kBSTypeChanged:
      updateMultiCart();
      break;

    case kBSFilterChanged:
      updateBSTypes();
      break;

    case kPhosphorChanged:
    {
      const bool status = myPhosphor->getState();
      myPPBlend->setEnabled(status);
      break;
    }

    case kPPBlendChanged:
      if(myPPBlend->getValue() == 0)
      {
        myPPBlend->setValueLabel("Off");
        myPPBlend->setValueUnit("");
      }
      else
        myPPBlend->setValueUnit("%");
      break;

    case kVCenterChanged:
      if(myVCenter->getValue() == 0)
      {
        myVCenter->setValueLabel("Default");
        myVCenter->setValueUnit("");
      }
      else
        myVCenter->setValueUnit("px");
      break;

    case kPXCenterChanged:
      myPaddleXCenter->setValueLabel(myPaddleXCenter->getValue() * 5);
      break;

    case kPYCenterChanged:
      myPaddleYCenter->setValueLabel(myPaddleYCenter->getValue() * 5);
      break;

    case kMCtrlChanged:
    {
      const bool state = myMouseControl->getState();
      myMouseXLabel->setEnabled(state);
      myMouseX->setEnabled(state);
      myMouseYLabel->setEnabled(state);
      myMouseY->setEnabled(state);
      break;
    }

    case kLinkPressed:
      MediaFactory::openURL(myUrl->getText());
      break;

#ifdef IMAGE_SUPPORT
    case kBezelFilePressed:
      BrowserDialog::show(this, _font, "Select bezel image",
                          instance().bezelDir().getPath() + myBezelName->getText(),
                          BrowserDialog::Mode::FileLoadNoDirs,
                          [this](bool OK, const FSNode& node) {
                            if(OK)
                            {
                              myBezelName->setText(node.getBaseName());
                              myBezelDetected->setLabel("");
                            }
                          },
                          [](const FSNode& node) {
                            return node.hasExtension(".png");
                          });
      break;
#endif

    case EditTextWidget::kChangedCmd:
      if(id == kLinkId)
      {
        updateLink();
        break;
      }
      [[fallthrough]];
    case kHiScoresChanged:
      updateHighScoresWidgets();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
