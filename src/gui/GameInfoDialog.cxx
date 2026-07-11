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
GameInfoDialog::GameInfoDialog(
      OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
      GuiObject* boss)
  : Dialog(osystem, parent, font, "Game properties"),
    CommandSender(boss)
{
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() sizes the
  // dialog and positions everything from the current font.  The tab bar geometry
  // is recomputed in layout() via TabWidget::updateTabSizes().
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTab = new TabWidget(this, font, 0, 0, 1, 1);
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
  _w = static_cast<int>(tabSize.w) + 2 * xpos;

  myTab->setPos(xpos, 4 + _th);
  myTab->setWidth(_w - 2 * xpos);
  myTab->setHeight(static_cast<int>(tabSize.h));

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
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth();
  WidgetArray wid;
  VariantList items;

  // 1) Emulation properties.  The tab's controls are parented to a content pane;
  // the pane lays them out (see setLayout below) whenever the tab is sized
  const int tabID = myTab->addTab("Emulation", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  myBSTypeLabel = new StaticTextWidget(pane, _font, 0, 0, "Type (*)      ");
  myBSType = new PopUpWidget(pane, _font, 0, 0,
                             _font.getStringWidth("CM (SpectraVideo CompuMate)"),
                             lineHeight, items, "", 0, kBSTypeChanged);
  wid.push_back(myBSType);
  myBSFilter = new CheckboxWidget(pane, _font, 0, 0, "Filter", kBSFilterChanged);
  myBSFilter->setToolTip("Enable to filter types by ROM size");
  wid.push_back(myBSFilter);

  myTypeDetected = new StaticTextWidget(pane, ifont, 0, 0,
                                        "CM (SpectraVideo CompuMate) detected");

  // Start bank
  myStartBank = new PopUpWidget(pane, _font, 0, 0,
                                _font.getStringWidth("AUTO"), lineHeight, items, "Start bank (*)");
  wid.push_back(myStartBank);

  items.clear();
  VarList::push_back(items, "Auto-detect", "AUTO");
  VarList::push_back(items, "NTSC", "NTSC");
  VarList::push_back(items, "PAL", "PAL");
  VarList::push_back(items, "SECAM", "SECAM");
  VarList::push_back(items, "NTSC-50", "NTSC50");
  VarList::push_back(items, "PAL-60", "PAL60");
  VarList::push_back(items, "SECAM-60", "SECAM60");
  myFormat = new PopUpWidget(pane, _font, 0, 0,
                             _font.getStringWidth("Auto-detect"), lineHeight, items, "TV format");
  myFormat->setToolTip(Event::FormatDecrease, Event::FormatIncrease);
  wid.push_back(myFormat);

  myFormatDetected = new StaticTextWidget(pane, ifont, 0, 0, "SECAM-60 detected");

  // Phosphor
  myPhosphor = new CheckboxWidget(pane, _font, 0, 0,
                                  "Phosphor (auto-enabled/disabled for all ROMs)", kPhosphorChanged);
  myPhosphor->setToolTip(Event::TogglePhosphor);
  wid.push_back(myPhosphor);

  myPPBlend = new SliderWidget(pane, _font, 0, 0,
                               "Blend", 0, kPPBlendChanged, 4 * fontWidth, "%");
  myPPBlend->setMinValue(0); myPPBlend->setMaxValue(100);
  myPPBlend->setTickmarkIntervals(2);
  myPPBlend->setToolTip(Event::PhosphorDecrease, Event::PhosphorIncrease);
  wid.push_back(myPPBlend);

  myVCenter = new SliderWidget(pane, _font, 0, 0, "V-Center",
                               0, kVCenterChanged, 7 * fontWidth, "px", 0, true);
  myVCenter->setMinValue(TIAConstants::minVcenter);
  myVCenter->setMaxValue(TIAConstants::maxVcenter);
  myVCenter->setTickmarkIntervals(4);
  myVCenter->setToolTip(Event::VCenterDecrease, Event::VCenterIncrease);
  wid.push_back(myVCenter);

  mySound = new CheckboxWidget(pane, _font, 0, 0, "Stereo sound");
  wid.push_back(mySound);

  // Message concerning usage (positioned along the bottom in layout)
  myEmulInfo = new StaticTextWidget(pane, ifont, 0, 0,
                                    "(*) Change requires a ROM reload");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("EmulationProps");

  // Describe the layout once; the pane runs it on every resize
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::indentedItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();
    const int labelIndent = myBSTypeLabel->getWidth() + fontWidth;

    // These controls draw their own labels, so no layout can line their value
    // boxes up: each group is given ONE label column instead, sized to the
    // longest label in it.  The blend slider is indented, so its column is
    // narrowed to match and its track still meets the V-Center one below it
    GUI::alignLabels({{myStartBank}, {myFormat}});
    GUI::alignLabels({{myPPBlend, INDENT}, {myVCenter}});

    // Bankswitch-type row: label + type popup + filter checkbox
    auto bsRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    bsRow->addAuto(anchoredItem(myBSTypeLabel));
    bsRow->addSpace(fontWidth);
    bsRow->addAuto(anchoredItem(myBSType));
    bsRow->addSpace(fontWidth);
    bsRow->addAuto(anchoredItem(myBSFilter));

    // TV-format row, with the detected format beside the popup (it uses the
    // smaller info font, so it is centered on the row rather than filling it)
    auto formatRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    formatRow->addAuto(anchoredItem(myFormat));
    formatRow->addSpace(fontWidth);
    formatRow->addStretch(alignedItem(myFormatDetected, HAlign::Fill, VAlign::Center));

    // Every row is as tall as what it holds, so no height is stated here: the
    // pop-ups frame their text, and the detected-type/usage notes use the
    // smaller info font
    col.addAuto(std::move(bsRow));
    col.addSpace(VGAP);
    // Detected type, indented to line up under the type popup
    col.addAuto(indentedItem(myTypeDetected, labelIndent));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myStartBank));
    col.addSpace(VGAP * 4);
    col.addAuto(std::move(formatRow));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myPhosphor));
    col.addAuto(indentedItem(myPPBlend, INDENT));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myVCenter));
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

  myTVTypeLabel = new StaticTextWidget(pane, _font, 0, 0, "TV type");
  myTVTypeGroup = std::make_unique<RadioButtonGroup>();
  myTVType[0] = new RadioButtonWidget(pane, _font, 0, 0, "Color", myTVTypeGroup.get());
  myTVType[0]->setToolTip(Event::ConsoleColor, Event::ConsoleColorToggle);
  wid.push_back(myTVType[0]);
  myTVType[1] = new RadioButtonWidget(pane, _font, 0, 0, "B/W", myTVTypeGroup.get());
  myTVType[1]->setToolTip(Event::ConsoleBlackWhite, Event::ConsoleColorToggle);
  wid.push_back(myTVType[1]);

  myLeftDiffLabel = new StaticTextWidget(pane, _font, 0, 0, GUI::LEFT_DIFFICULTY);
  myLeftDiffGroup = std::make_unique<RadioButtonGroup>();
  myLeftDiff[0] = new RadioButtonWidget(pane, _font, 0, 0, "A (Expert)", myLeftDiffGroup.get());
  myLeftDiff[0]->setToolTip(Event::ConsoleLeftDiffA, Event::ConsoleLeftDiffToggle);
  wid.push_back(myLeftDiff[0]);
  myLeftDiff[1] = new RadioButtonWidget(pane, _font, 0, 0, "B (Novice)", myLeftDiffGroup.get());
  myLeftDiff[1]->setToolTip(Event::ConsoleLeftDiffB, Event::ConsoleLeftDiffToggle);
  wid.push_back(myLeftDiff[1]);

  myRightDiffLabel = new StaticTextWidget(pane, _font, 0, 0, GUI::RIGHT_DIFFICULTY);
  myRightDiffGroup = std::make_unique<RadioButtonGroup>();
  myRightDiff[0] = new RadioButtonWidget(pane, _font, 0, 0, "A (Expert)", myRightDiffGroup.get());
  myRightDiff[0]->setToolTip(Event::ConsoleRightDiffA, Event::ConsoleRightDiffToggle);
  wid.push_back(myRightDiff[0]);
  myRightDiff[1] = new RadioButtonWidget(pane, _font, 0, 0, "B (Novice)", myRightDiffGroup.get());
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
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight();
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

  const int pwidth = _font.getStringWidth("Paddles_IAxis");
  myLeftPortLabel = new StaticTextWidget(pane, _font, 0, 0, "Left port        ");
  myLeftPort = new PopUpWidget(pane, _font, 0, 0,
                               pwidth, lineHeight, items, "", 0, kLeftCChanged);
  myLeftPort->setToolTip(Event::PreviousLeftPort, Event::NextLeftPort);
  wid.push_back(myLeftPort);

  myLeftPortDetected = new StaticTextWidget(pane, ifont, 0, 0, "Sega Genesis detected");

  myRightPortLabel = new StaticTextWidget(pane, _font, 0, 0, "Right port       ");
  myRightPort = new PopUpWidget(pane, _font, 0, 0,
                                pwidth, lineHeight, items, "", 0, kRightCChanged);
  myRightPort->setToolTip(Event::PreviousRightPort, Event::NextRightPort);
  wid.push_back(myRightPort);

  myRightPortDetected = new StaticTextWidget(pane, ifont, 0, 0, "Sega Genesis detected");

  mySwapPorts = new CheckboxWidget(pane, _font, 0, 0, "Swap ports");
  mySwapPorts->setToolTip(Event::ToggleSwapPorts);
  wid.push_back(mySwapPorts);

  myQuadTariButton = new ButtonWidget(pane, _font, 0, 0,
                                      " QuadTari" + ELLIPSIS + " ", kQuadTariPressed);
  wid.push_back(myQuadTariButton);

  // EEPROM erase button for left/right controller (button as wide as a port popup)
  myEraseEEPROMLabel = new StaticTextWidget(pane, _font, 0, 0, "AtariVox/SaveKey ");
  myEraseEEPROMButton = new ButtonWidget(pane, _font, 0, 0,
                                         myRightPort->getWidth(), buttonHeight,
                                         "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);
  myEraseEEPROMInfo = new StaticTextWidget(pane, ifont, 0, 0, "(for this game only)");

  mySwapPaddles = new CheckboxWidget(pane, _font, 0, 0, "Swap paddles");
  mySwapPaddles->setToolTip(Event::ToggleSwapPaddles);
  wid.push_back(mySwapPaddles);

  // Paddles
  myPaddlesCenter = new StaticTextWidget(pane, _font, 0, 0, "Paddles center:");

  myPaddleXCenter = new SliderWidget(pane, _font, 0, 0, "X ", 0, kPXCenterChanged,
                                     fontWidth * 6, "px", 0 ,true);
  myPaddleXCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleXCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleXCenter->setTickmarkIntervals(4);
  myPaddleXCenter->setToolTip(Event::DecreasePaddleCenterX, Event::IncreasePaddleCenterX);
  wid.push_back(myPaddleXCenter);

  myPaddleYCenter = new SliderWidget(pane, _font, 0, 0, "Y ", 0, kPYCenterChanged,
                                     fontWidth * 6, "px", 0 ,true);
  myPaddleYCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleYCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleYCenter->setTickmarkIntervals(4);
  myPaddleYCenter->setToolTip(Event::DecreasePaddleCenterY, Event::IncreasePaddleCenterY);
  wid.push_back(myPaddleYCenter);

  // Mouse
  myMouseControl = new CheckboxWidget(pane, _font, 0, 0, "Specific mouse axes",
                                      kMCtrlChanged);
  wid.push_back(myMouseControl);

  // Mouse controller specific axis
  const int cpwidth = _font.getStringWidth("Right MindLink");
  VarList::push_back(ctrls, "None",           static_cast<uInt32>(MouseControl::Type::NoControl));
  VarList::push_back(ctrls, "Left Paddle A",  static_cast<uInt32>(MouseControl::Type::LeftPaddleA));
  VarList::push_back(ctrls, "Left Paddle B",  static_cast<uInt32>(MouseControl::Type::LeftPaddleB));
  VarList::push_back(ctrls, "Right Paddle A", static_cast<uInt32>(MouseControl::Type::RightPaddleA));
  VarList::push_back(ctrls, "Right Paddle B", static_cast<uInt32>(MouseControl::Type::RightPaddleB));
  VarList::push_back(ctrls, "Left Driving",   static_cast<uInt32>(MouseControl::Type::LeftDriving));
  VarList::push_back(ctrls, "Right Driving",  static_cast<uInt32>(MouseControl::Type::RightDriving));
  VarList::push_back(ctrls, "Left MindLink",  static_cast<uInt32>(MouseControl::Type::LeftMindLink));
  VarList::push_back(ctrls, "Right MindLink", static_cast<uInt32>(MouseControl::Type::RightMindLink));

  myMouseX = new PopUpWidget(pane, _font, 0, 0, cpwidth, lineHeight, ctrls, "X-Axis is ");
  wid.push_back(myMouseX);
  myMouseY = new PopUpWidget(pane, _font, 0, 0, cpwidth, lineHeight, ctrls, "Y-Axis is ");
  wid.push_back(myMouseY);

  myMouseRange = new SliderWidget(pane, _font, 0, 0,
                                  "Mouse axes range ", 0, 0, fontWidth * 4, "%");
  myMouseRange->setMinValue(1); myMouseRange->setMaxValue(100);
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
    using GUI::anchoredItem;
    using GUI::indentedItem;
    using GUI::widgetItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();

    // A port row is a label and the port popup, with room for what follows them
    const auto portRow = [&](StaticTextWidget* label, PopUpWidget* popup) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      row->addAuto(anchoredItem(label));
      row->addAuto(anchoredItem(popup));
      row->addSpace(fontWidth * 4);
      return row;
    };

    auto leftRow = portRow(myLeftPortLabel, myLeftPort);
    leftRow->addStretch(anchoredItem(mySwapPorts));

    auto rightRow = portRow(myRightPortLabel, myRightPort);
    rightRow->addFixed(alignedItem(myQuadTariButton, HAlign::Fill, VAlign::Center),
                       myQuadTariButton->getWidth());
    rightRow->addStretchSpace();

    // EEPROM erase button, with its label and its note beside it
    auto eepromRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    eepromRow->addAuto(anchoredItem(myEraseEEPROMLabel));
    eepromRow->addFixed(alignedItem(myEraseEEPROMButton, HAlign::Fill, VAlign::Center),
                        myRightPort->getWidth());
    eepromRow->addSpace(fontWidth / 2);
    eepromRow->addStretch(alignedItem(myEraseEEPROMInfo, HAlign::Fill, VAlign::Center));

    // The paddle options and the mouse axes run as two parallel columns
    auto paddleCol = std::make_unique<BoxLayout>(Dir::Vertical);
    paddleCol->addAuto(anchoredItem(mySwapPaddles));
    paddleCol->addSpace(VGAP);
    paddleCol->addAuto(anchoredItem(myPaddlesCenter));
    paddleCol->addSpace(VGAP);
    paddleCol->addAuto(indentedItem(myPaddleXCenter, INDENT));
    paddleCol->addSpace(VGAP);
    paddleCol->addAuto(indentedItem(myPaddleYCenter, INDENT));

    // The two axis popups are indented by the checkbox prefix, so they line up
    // under its text
    const int prefix = CheckboxWidget::prefixSize(_font);
    auto mouseCol = std::make_unique<BoxLayout>(Dir::Vertical);
    mouseCol->addAuto(anchoredItem(myMouseControl));
    mouseCol->addSpace(VGAP);
    mouseCol->addAuto(indentedItem(myMouseX, prefix));
    mouseCol->addSpace(VGAP);
    mouseCol->addAuto(indentedItem(myMouseY, prefix));
    mouseCol->addSpace(VGAP);
    mouseCol->addAuto(anchoredItem(myMouseRange));

    auto lowerRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    lowerRow->addFixed(std::move(paddleCol), fontWidth * 24 - INDENT);
    lowerRow->addStretch(std::move(mouseCol));

    // Every row is as tall as what it holds — including the two-column block at
    // the bottom, which is as tall as its taller column
    col.addAuto(std::move(leftRow));
    col.addSpace(VGAP);
    col.addAuto(indentedItem(myLeftPortDetected, myLeftPortLabel->getWidth()));
    col.addSpace(VGAP);
    col.addAuto(std::move(rightRow));
    col.addSpace(VGAP);
    col.addAuto(indentedItem(myRightPortDetected, myRightPortLabel->getWidth()));
    col.addSpace(VGAP * 2);
    col.addAuto(std::move(eepromRow));
    col.addSpace(VGAP * 4);
    col.addAuto(std::move(lowerRow));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::addCartridgeTab()
{
  // 4) Cartridge properties.  The tab's controls are parented to a content pane;
  // the pane lays them out (see setLayout below) whenever the tab is sized
  const int lineHeight = Dialog::lineHeight();
  const int bw = buttonWidth(">");
  WidgetArray wid;

  const int tabID = myTab->addTab("Cartridge", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  myCartLabels[0] = new StaticTextWidget(pane, _font, 0, 0, "Name");
  myName = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  wid.push_back(myName);

  myCartLabels[1] = new StaticTextWidget(pane, _font, 0, 0, "MD5");
  myMD5 = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  myMD5->setEditable(false);

  myCartLabels[2] = new StaticTextWidget(pane, _font, 0, 0, "Manufacturer");
  myManufacturer = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  wid.push_back(myManufacturer);

  myCartLabels[3] = new StaticTextWidget(pane, _font, 0, 0,
                                         "Model", TextAlign::Left);
  myModelNo = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  wid.push_back(myModelNo);

  myCartLabels[4] = new StaticTextWidget(pane, _font, 0, 0, "Rarity");
  myRarity = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  wid.push_back(myRarity);

  myCartLabels[5] = new StaticTextWidget(pane, _font, 0, 0, "Note");
  myNote = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  wid.push_back(myNote);

  myCartLabels[6] = new StaticTextWidget(pane, _font, 0, 0, "Link");
  myUrl = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  myUrl->setID(kLinkId);
  wid.push_back(myUrl);

  myUrlButton = new ButtonWidget(pane, _font, 0, 0, bw, myUrl->getHeight(),
                                 ">>", kLinkPressed);
  wid.push_back(myUrlButton);

#ifdef IMAGE_SUPPORT
  const GUI::Font& ifont = instance().frameBuffer().infoFont();

  myCartLabels[7] = new StaticTextWidget(pane, _font, 0, 0, "Bezelname");
  myBezelName = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight, "");
  myBezelName->setToolTip("Define the name of the bezel file.");
  wid.push_back(myBezelName);

  myBezelButton = new ButtonWidget(pane, _font, 0, 0, bw, myBezelName->getHeight(),
                                   ELLIPSIS, kBezelFilePressed);
  wid.push_back(myBezelButton);

  myBezelDetected = new StaticTextWidget(pane, ifont, 0, 0,
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
         .columnStretch(FIELD, 1, EditTextWidget::calcWidth(_font, 40))
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
    // The detected bezel, under the field it belongs to
    grid->place(FIELD, BEZEL_DETECTED, anchoredItem(myBezelDetected),
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
  const int lineHeight = Dialog::lineHeight();
  const int awidth  = EditTextWidget::calcWidth(_font, 4); // addresses
  const int swidth  = EditTextWidget::calcWidth(_font, HSM::MAX_SPECIAL_NAME); // special
  const int fwidth  = EditTextWidget::calcWidth(_font, 3); // variants
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

  myHighScores = new CheckboxWidget(pane, _font, 0, 0, "Enable High Scores",
                                    kHiScoresChanged);

  // Variations
  myVariationsLabel = new StaticTextWidget(pane, _font, 0, 0, "Variations");
  myVariations = new EditTextWidget(pane, _font, 0, 0, fwidth, lineHeight);
  myVariations->setTextFilter(fVars);
  myVariations->setMaxLen(3);
  myVariations->setToolTip("Define the number of game variations.");
  wid.push_back(myVariations);

  myVarAddressLabel = new StaticTextWidget(pane, _font, 0, 0, "Address ");
  myVarAddress = new EditTextWidget(pane, _font, 0, 0, awidth, lineHeight);
  myVarAddress->setTextFilter(fAddr);
  myVarAddress->setMaxLen(4);
  myVarAddress->setToolTip("Define the address (in hex format) where the variation number "
                           "is stored.");
  wid.push_back(myVarAddress);
  myVarAddressVal = new EditTextWidget(pane, _font, 0, 0,
                                       EditTextWidget::calcWidth(_font, 3), lineHeight);
  myVarAddressVal->setEditable(false);

  myVarsBCD = new CheckboxWidget(pane, _font, 0, 0, "BCD", kHiScoresChanged);
  myVarsBCD->setToolTip("Check when the variation number is stored as BCD.");
  wid.push_back(myVarsBCD);
  myVarsZeroBased = new CheckboxWidget(pane, _font, 0, 0, "0-based", kHiScoresChanged);
  myVarsZeroBased->setToolTip("Check when the variation number is stored zero-based.");
  wid.push_back(myVarsZeroBased);

  // Score
  myScoreLabel = new StaticTextWidget(pane, _font, 0, 0, "Score");

  items.clear();
  for(uInt32 i = 1; i <= HSM::MAX_SCORE_DIGITS; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));
  myScoreDigitsLabel = new StaticTextWidget(pane, _font, 0, 0, "Digits    ");
  myScoreDigits = new PopUpWidget(pane, _font, 0, 0, _font.getStringWidth("4"),
                                  lineHeight, items, "", 0, kHiScoresChanged);
  myScoreDigits->setToolTip("Select the number of score digits displayed.");
  wid.push_back(myScoreDigits);

  items.clear();
  for(uInt32 i = 0; i <= HSM::MAX_SCORE_DIGITS - 3; ++i)
    VarList::push_back(items, std::to_string(i), std::to_string(i));
  myTrailingZeroesLabel = new StaticTextWidget(pane, _font, 0, 0, "0-digits ");
  myTrailingZeroes = new PopUpWidget(pane, _font, 0, 0, _font.getStringWidth("0"),
                                     lineHeight, items, "", 0, kHiScoresChanged);
  myTrailingZeroes->setToolTip("Select the number of trailing score digits which are fixed to 0.");
  wid.push_back(myTrailingZeroes);

  myScoreBCD = new CheckboxWidget(pane, _font, 0, 0, "BCD", kHiScoresChanged);
  myScoreBCD->setToolTip("Check when the score is stored as BCD.");
  wid.push_back(myScoreBCD);
  myScoreInvert = new CheckboxWidget(pane, _font, 0, 0, "Invert");
  myScoreInvert->setToolTip("Check when a lower score (e.g. a timer) is better.");
  wid.push_back(myScoreInvert);

  // Score addresses
  myScoreAddressesLabel = new StaticTextWidget(pane, _font, 0, 0, "Addresses ");
  for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
  {
    myScoreAddress[a] = new EditTextWidget(pane, _font, 0, 0, awidth, lineHeight);
    myScoreAddress[a]->setTextFilter(fAddr);
    myScoreAddress[a]->setMaxLen(4);
    myScoreAddress[a]->setToolTip("Define the addresses (in hex format, highest byte first) "
                                  "where the score is stored.");
    wid.push_back(myScoreAddress[a]);
    myScoreAddressVal[a] = new EditTextWidget(pane, _font, 0, 0,
                                              EditTextWidget::calcWidth(_font, 2), lineHeight);
    myScoreAddressVal[a]->setEditable(false);
  }

  myCurrentScoreLabel = new StaticTextWidget(pane, _font, 0, 0, "Current   ");
  myCurrentScore = new StaticTextWidget(pane, _font, 0, 0, "12345678");
  myCurrentScore->setToolTip("The score read using the current definitions.");

  // Special
  mySpecialLabel = new StaticTextWidget(pane, _font, 0, 0, "Special");
  mySpecialName = new EditTextWidget(pane, _font, 0, 0, swidth, lineHeight);
  mySpecialName->setTextFilter(fText);
  mySpecialName->setMaxLen(HSM::MAX_SPECIAL_NAME);
  mySpecialName->setToolTip("Define a short label (up to 5 chars) for the optional,\ngame's "
                            "special value (e.g. 'Level', 'Wave', 'Round'" + ELLIPSIS + ")");
  wid.push_back(mySpecialName);

  mySpecialAddressLabel = new StaticTextWidget(pane, _font, 0, 0, "Address ");
  mySpecialAddress = new EditTextWidget(pane, _font, 0, 0, awidth, lineHeight);
  mySpecialAddress->setTextFilter(fAddr);
  mySpecialAddress->setMaxLen(4);
  mySpecialAddress->setToolTip("Define the address (in hex format) where the special "
                               "number is stored.");
  wid.push_back(mySpecialAddress);
  mySpecialAddressVal = new EditTextWidget(pane, _font, 0, 0,
                                           EditTextWidget::calcWidth(_font, 3), lineHeight);
  mySpecialAddressVal->setEditable(false);

  mySpecialBCD = new CheckboxWidget(pane, _font, 0, 0, "BCD", kHiScoresChanged);
  mySpecialBCD->setToolTip("Check when the special number is stored as BCD.");
  wid.push_back(mySpecialBCD);
  mySpecialZeroBased = new CheckboxWidget(pane, _font, 0, 0, "0-based", kHiScoresChanged);
  mySpecialZeroBased->setToolTip("Check when the special number is stored zero-based.");
  wid.push_back(mySpecialZeroBased);

  // Note
  myHighScoreNotesLabel = new StaticTextWidget(pane, _font, 0, 0, "Note");
  myHighScoreNotes = new EditTextWidget(pane, _font, 0, 0, 1, lineHeight);
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
    // The gap between the groups within a row, and the tight one that ties a
    // value field to the address it reads
    const int GAP = fontWidth * 2, TIE = fontWidth / 4;

    // An address group: its label, the address, and the value read from it
    const auto addrGroup = [&](StaticTextWidget* label, EditTextWidget* addr,
                               EditTextWidget* val) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      row->addSpace(GAP);
      row->addAuto(anchoredItem(label));
      row->addAuto(anchoredItem(addr));
      row->addSpace(TIE);
      row->addAuto(anchoredItem(val));
      return row;
    };

    // Columns: row label | value | address group | BCD | trailing option.
    // Rows alternate content and the gaps between them, so each gap is stated
    // where it falls rather than being one uniform spacing
    enum Col: int { LABEL, VALUE, ADDR, BCD, OPT, COLS };
    enum Row: int {
      VARS, GAP1, SCORE, GAP2, DIGITS, GAP3, ADDRS, GAP4, CURRENT, GAP5,
      SPECIAL, GAP6, NOTE, ROWS
    };
    auto grid = std::make_unique<GridLayout>(COLS, ROWS);

    for(const int c: {LABEL, VALUE, ADDR, BCD})
      grid->columnAuto(c);
    grid->columnStretch(OPT);

    for(const int r: {VARS, SCORE, DIGITS, ADDRS, CURRENT, SPECIAL, NOTE})
      grid->rowAuto(r);
    grid->rowFixed(GAP1, VGAP * 3).rowFixed(GAP2, VGAP).rowFixed(GAP3, VGAP)
         .rowFixed(GAP4, VGAP).rowFixed(GAP5, VGAP * 3).rowFixed(GAP6, VGAP * 3);

    // Variations
    grid->place(LABEL, VARS, anchoredItem(myVariationsLabel));
    grid->place(VALUE, VARS, anchoredItem(myVariations));
    grid->place(ADDR,  VARS, addrGroup(myVarAddressLabel, myVarAddress,
                                       myVarAddressVal));
    grid->place(BCD,   VARS, indentedItem(myVarsBCD, GAP));
    grid->place(OPT,   VARS, indentedItem(myVarsZeroBased, GAP));

    // Score, whose three rows are indented under their heading
    grid->place(LABEL, SCORE, anchoredItem(myScoreLabel));

    auto zeroesGroup = std::make_unique<BoxLayout>(Dir::Horizontal);
    zeroesGroup->addSpace(GAP);
    zeroesGroup->addAuto(anchoredItem(myTrailingZeroesLabel));
    zeroesGroup->addAuto(anchoredItem(myTrailingZeroes));

    grid->place(LABEL, DIGITS, indentedItem(myScoreDigitsLabel, INDENT));
    grid->place(VALUE, DIGITS, anchoredItem(myScoreDigits));
    grid->place(ADDR,  DIGITS, std::move(zeroesGroup));
    grid->place(BCD,   DIGITS, indentedItem(myScoreBCD, GAP));
    grid->place(OPT,   DIGITS, indentedItem(myScoreInvert, GAP));

    // The score addresses run past the columns above them, so they span the rest
    auto scoreAddrs = std::make_unique<BoxLayout>(Dir::Horizontal);
    for(uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
    {
      scoreAddrs->addAuto(anchoredItem(myScoreAddress[a]));
      scoreAddrs->addSpace(TIE);
      scoreAddrs->addAuto(anchoredItem(myScoreAddressVal[a]));
      scoreAddrs->addSpace(GAP);
    }
    scoreAddrs->addStretchSpace();

    grid->place(LABEL, ADDRS, indentedItem(myScoreAddressesLabel, INDENT));
    grid->place(VALUE, ADDRS, std::move(scoreAddrs), COLS - VALUE);

    grid->place(LABEL, CURRENT, indentedItem(myCurrentScoreLabel, INDENT));
    grid->place(VALUE, CURRENT, anchoredItem(myCurrentScore), COLS - VALUE);

    // Special
    grid->place(LABEL, SPECIAL, anchoredItem(mySpecialLabel));
    grid->place(VALUE, SPECIAL, anchoredItem(mySpecialName));
    grid->place(ADDR,  SPECIAL, addrGroup(mySpecialAddressLabel, mySpecialAddress,
                                          mySpecialAddressVal));
    grid->place(BCD,   SPECIAL, indentedItem(mySpecialBCD, GAP));
    grid->place(OPT,   SPECIAL, indentedItem(mySpecialZeroBased, GAP));

    // Note, whose field takes all the width the other columns leave
    grid->place(LABEL, NOTE, anchoredItem(myHighScoreNotesLabel));
    grid->place(VALUE, NOTE,
                alignedItem(myHighScoreNotes, HAlign::Fill, VAlign::Center),
                COLS - VALUE);

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
  myMouseX->setEnabled(!autoAxis);
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

  for (uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
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

  if (myHighScores->getState())
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

    for (uInt32 a = 0; a < HSM::MAX_SCORE_ADDR; ++a)
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
  myStartBank->setEnabled(!isMulti && instance().hasConsole());
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
  myPaddleXCenter->setEnabled(enablePaddles);
  myPaddleYCenter->setEnabled(enablePaddles);

  const bool enableMouse = enablePaddles ||
    BSPF::startsWithIgnoreCase(contrLeft, "Driving") ||
    BSPF::startsWithIgnoreCase(contrRight, "Driving") ||
    BSPF::startsWithIgnoreCase(contrLeft, "MindLink") ||
    BSPF::startsWithIgnoreCase(contrRight, "MindLink");

  myMouseControl->setEnabled(enableMouse);
  myMouseX->setEnabled(enableMouse && myMouseControl->getState());
  myMouseY->setEnabled(enableMouse && myMouseControl->getState());

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

  if (instance().hasConsole() && valWidget->isEnabled())
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

  if (myGameProperties.save(repo))
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
      if (myVCenter->getValue() == 0)
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
      myMouseX->setEnabled(state);
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
