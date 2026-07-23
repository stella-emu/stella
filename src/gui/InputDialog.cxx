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
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Paddles.hxx"
#include "MindLink.hxx"
#include "PointingDevice.hxx"
#include "Driving.hxx"
#include "SaveKey.hxx"
#include "AtariVox.hxx"
#include "Settings.hxx"
#include "EventMappingWidget.hxx"
#include "JoystickDialog.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "TabPaneWidget.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "Font.hxx"
#include "MessageBox.hxx"
#include "MediaFactory.hxx"
#include "InputDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Input settings"),
    myMaxWidth{max_w},
    myMaxHeight{max_h}
{
  // Widgets are only created here (at placeholder geometry); layout() sizes the
  // dialog and positions everything from the current font, so it reflows on
  // font change.  The tab widget's bar geometry is (re)computed in layout() via
  // TabWidget::updateTabSizes(), so a placeholder size is fine here.
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTab = new TabWidget(this, _font);
  addTabWidget(myTab);

  // 1) Event mapper (the composite fills its tab; it reflows via setArea())
  const int tabID = myTab->addTab(" Event Mappings ", TabWidget::AUTO_WIDTH);
  myEventMapper = new EventMappingWidget(myTab, _font);
  myTab->setParentWidget(tabID, myEventMapper);
  addToFocusList(myEventMapper->getFocusList(), myTab, tabID);
  myTab->parentWidget(tabID)->setHelpAnchor("Remapping");

  // 2) Devices & ports
  addDevicePortTab();

  // 3) Mouse
  addMouseTab();

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("Remapping");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::~InputDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::layout()
{
  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            VGAP         = Dialog::vGap();

  // Size the dialog from the current font at its natural size (no clamp to the
  // available space): a clamped dialog would keep _w/_h within the screen while
  // its content overflowed, so Dialog::exceedsScreen() could not detect the
  // too-large case.  Sizing naturally lets that check fire the "too large"
  // message like every other dialog.  (myMaxWidth/myMaxHeight are still used to
  // bound the confirm MessageBox below.)
  // Both dimensions come from the tab widget: it reports what its largest tab's
  // content asks for, so nothing here counts rows, gaps or columns, and adding a
  // row to any tab needs no change at all
  constexpr int xpos = 2;
  const Common::Size tabSize = myTab->naturalSize();

  myTab->setPos(xpos, VGAP + _th);
  myTab->setWidth(static_cast<int>(tabSize.w));
  myTab->setHeight(static_cast<int>(tabSize.h));

  _w = myTab->getWidth() + 2 * xpos;
  _h = _th + VGAP + myTab->getHeight() + VBORDER + buttonHeight + VBORDER;

  // Recompute the tab-bar geometry for the current font/width
  myTab->updateTabSizes();

  // Every tab's content lays itself out via the tab widget (the Event Mappings
  // composite and the two content panes), so there is no per-tab code here

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addDevicePortTab()
{
  const int swidth = 13;
  WidgetArray wid;

  // Devices & ports.  The tab's controls are parented to a content pane; the
  // pane lays them out (see setLayout below) whenever the tab is sized — no
  // resize code
  const int tabID = myTab->addTab(" Devices & Ports ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  // Add digital dead zone setting
  myDigitalDeadzoneLabel = new StaticTextWidget(pane, _font, "Digital dead zone size");
  myDigitalDeadzone = new SliderWidget(pane, _font, swidth, kDDeadzoneChanged, 3, "%");
  myDigitalDeadzone->setMinValue(Controller::MIN_DIGITAL_DEADZONE);
  myDigitalDeadzone->setMaxValue(Controller::MAX_DIGITAL_DEADZONE);
  myDigitalDeadzone->setTickmarkIntervals(5);
  myDigitalDeadzone->setToolTip("Adjust dead zone size for analog joysticks when emulating digital controllers.",
    Event::DecreaseDeadzone, Event::IncreaseDeadzone);
  wid.push_back(myDigitalDeadzone);

  // Add analog dead zone
  myAnalogDeadzoneLabel = new StaticTextWidget(pane, _font, "Analog dead zone size");
  myAnalogDeadzone = new SliderWidget(pane, _font, swidth, kADeadzoneChanged, 3, "%");
  myAnalogDeadzone->setMinValue(Controller::MIN_ANALOG_DEADZONE);
  myAnalogDeadzone->setMaxValue(Controller::MAX_ANALOG_DEADZONE);
  myAnalogDeadzone->setTickmarkIntervals(5);
  myAnalogDeadzone->setToolTip("Adjust dead zone size for analog joysticks when emulating analog controllers.",
    Event::DecAnalogDeadzone, Event::IncAnalogDeadzone);
  wid.push_back(myAnalogDeadzone);

  myAnalogPaddleLabel = new StaticTextWidget(pane, _font, "Analog paddle:");

  // Add analog paddle sensitivity
  myPaddleSpeedLabel = new StaticTextWidget(pane, _font, "Sensitivity");
  myPaddleSpeed = new SliderWidget(pane, _font, swidth, kPSpeedChanged, 4, "%");
  myPaddleSpeed->setMinValue(0);
  myPaddleSpeed->setMaxValue(Paddles::MAX_ANALOG_SENSE);
  myPaddleSpeed->setTickmarkIntervals(3);
  myPaddleSpeed->setToolTip(Event::DecAnalogSense, Event::IncAnalogSense);
  wid.push_back(myPaddleSpeed);

  // Add analog paddle linearity
  myPaddleLinearityLabel = new StaticTextWidget(pane, _font, "Linearity");
  myPaddleLinearity = new SliderWidget(pane, _font, swidth, 0, 4, "%");
  myPaddleLinearity->setMinValue(Paddles::MIN_ANALOG_LINEARITY);
  myPaddleLinearity->setMaxValue(Paddles::MAX_ANALOG_LINEARITY);
  myPaddleLinearity->setStepValue(5);
  myPaddleLinearity->setTickmarkIntervals(3);
  myPaddleLinearity->setToolTip("Adjust paddle movement linearity.",
    Event::DecAnalogLinear, Event::IncAnalogLinear);
  wid.push_back(myPaddleLinearity);

  // Add dejitter (analog paddles)
  myDejitterBaseLabel = new StaticTextWidget(pane, _font, "Dejitter averaging");
  myDejitterBase = new SliderWidget(pane, _font, swidth, kDejitterAvChanged, 3);
  myDejitterBase->setMinValue(Paddles::MIN_DEJITTER);
  myDejitterBase->setMaxValue(Paddles::MAX_DEJITTER);
  myDejitterBase->setTickmarkIntervals(5);
  myDejitterBase->setToolTip("Adjust paddle input averaging.\n"
                             "Note: Already implemented in 2600-daptor",
    Event::DecDejtterAveraging, Event::IncDejtterAveraging);
  wid.push_back(myDejitterBase);

  myDejitterDiffLabel = new StaticTextWidget(pane, _font, "Dejitter reaction");
  myDejitterDiff = new SliderWidget(pane, _font, swidth, kDejitterReChanged, 3);
  myDejitterDiff->setMinValue(Paddles::MIN_DEJITTER);
  myDejitterDiff->setMaxValue(Paddles::MAX_DEJITTER);
  myDejitterDiff->setTickmarkIntervals(5);
  myDejitterDiff->setToolTip("Adjust paddle reaction to fast movements.",
    Event::DecDejtterReaction, Event::IncDejtterReaction);
  wid.push_back(myDejitterDiff);

  // Add paddle speed (digital emulation)
  myDPaddleSpeedLabel = new StaticTextWidget(pane, _font, "Digital paddle sensitivity");
  myDPaddleSpeed = new SliderWidget(pane, _font, swidth, kDPSpeedChanged, 4, "%");
  myDPaddleSpeed->setMinValue(1);
  myDPaddleSpeed->setMaxValue(20);
  myDPaddleSpeed->setTickmarkIntervals(4);
  myDPaddleSpeed->setToolTip(Event::DecDigitalSense, Event::IncDigitalSense);
  wid.push_back(myDPaddleSpeed);

  myAutoFire = new CheckboxWidget(pane, _font, "Autofire", kAutoFireChanged);
  myAutoFire->setToolTip(Event::ToggleAutoFire);
  wid.push_back(myAutoFire);

  myAutoFireRateLabel = new StaticTextWidget(pane, _font, "Rate");
  myAutoFireRate = new SliderWidget(pane, _font, swidth, kAutoFireRate, 5, "Hz");
  myAutoFireRate->setMinValue(0); myAutoFireRate->setMaxValue(30);
  myAutoFireRate->setTickmarkIntervals(6);
  myAutoFireRate->setToolTip(Event::DecreaseAutoFire, Event::IncreaseAutoFire);
  wid.push_back(myAutoFireRate);

  // Add 'allow all 4 directions' for joystick
  myAllowAll4 = new CheckboxWidget(pane, _font, "Allow all 4 directions on joystick");
  myAllowAll4->setToolTip(Event::ToggleFourDirections);
  wid.push_back(myAllowAll4);

  // Enable/disable modifier key-combos
  myModCombo = new CheckboxWidget(pane, _font, "Use modifier key combos");
  myModCombo->setToolTip(Event::ToggleKeyCombos);
  wid.push_back(myModCombo);

  // Stelladaptor mappings
  mySAPort = new CheckboxWidget(pane, _font, "Swap Stelladaptor ports");
  mySAPort->setToolTip(Event::ToggleSAPortOrder);
  wid.push_back(mySAPort);

  // EEPROM erase group heading (right column)
  myAtariVoxLabel = new StaticTextWidget(pane, _font, "AtariVox/SaveKey");

  // Show joystick database
  myJoyDlgButton = new ButtonWidget(pane, _font,
                                    "Controller Database" + ELLIPSIS, kDBButtonPressed);
  wid.push_back(myJoyDlgButton);

  // Erase EEPROM (right column, labelled by the heading above it)
  myEraseEEPROMButton = new ButtonWidget(pane, _font,
                                         "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);

  // Add AtariVox serial port
  myAVoxPortLabel = new StaticTextWidget(pane, _font, "AtariVox serial port");
  myAVoxPort = new PopUpWidget(pane, _font, 1, VariantList{}, kCursorStateChanged);
  myAVoxPort->setEditable(true);
  wid.push_back(myAVoxPort);

  // Add items for virtual device ports
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("DevicesPorts");

  // Describe the layout once; the pane runs it on every resize
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int VGAP   = Dialog::vGap(),
              INDENT = Dialog::indent();

    // The sliders each have their own label, so no layout can line their tracks
    // up: instead they share ONE label column, sized to the longest of their
    // labels.  The indented ones say so, and their columns are narrowed to
    // match, so every track still starts on the same line
    GUI::alignLabels({{myDigitalDeadzoneLabel}, {myAnalogDeadzoneLabel},
                      {myPaddleSpeedLabel, INDENT}, {myPaddleLinearityLabel, INDENT},
                      {myDejitterBaseLabel, INDENT}, {myDejitterDiffLabel, INDENT},
                      {myDPaddleSpeedLabel}});
    // The Autofire rate keeps its own column: it sits beside a checkbox rather
    // than in the slider column above, so it lines up with nothing
    GUI::alignLabels({{myAutoFireRateLabel}});

    // The serial port pop-up keeps its own column (it lines up with nothing),
    // but it still wants the clearance between a label and the box beside it
    GUI::alignLabels({{myAVoxPortLabel}});

    // Every row is as tall as what it holds (addAuto), so no height is stated
    // here and none can be wrong — the pop-ups frame their text and are taller
    // than the sliders and checkboxes, and the rows follow the font on their own
    col.addAuto(labeledRow(myDigitalDeadzoneLabel, myDigitalDeadzone));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myAnalogDeadzoneLabel, myAnalogDeadzone));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myAnalogPaddleLabel));
    col.addAuto(labeledRow(myPaddleSpeedLabel, myPaddleSpeed, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myPaddleLinearityLabel, myPaddleLinearity, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myDejitterBaseLabel, myDejitterBase, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myDejitterDiffLabel, myDejitterDiff, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myDPaddleSpeedLabel, myDPaddleSpeed));
    col.addSpace(VGAP);
    // The Autofire rate slider sits beside its checkbox, yet its track still
    // lines up with the sliders above: the checkbox takes the shared label
    // column, less the slider's own label
    auto autofireRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    autofireRow->addFixed(anchoredItem(myAutoFire),
                          myDPaddleSpeedLabel->getWidth() - myAutoFireRateLabel->getWidth());
    autofireRow->addStretch(labeledRow(myAutoFireRateLabel, myAutoFireRate));
    col.addAuto(std::move(autofireRow));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myAllowAll4));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myModCombo));
    col.addSpace(VGAP);

    // The ports section runs as two parallel columns, since the EEPROM group's
    // heading rides directly above its button and so does not share the left
    // column's rhythm.  Left: the Stelladaptor option above the controller
    // database button.  Right: the EEPROM heading above the Erase button, both
    // right-aligned (the left column takes the horizontal slack).
    // Each column ENDS with its button and takes up its vertical slack in a
    // stretch, so the two buttons land on the same line whatever is above them —
    // rather than the columns having to add up to the same height by hand
    auto dbRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    dbRow->addAuto(anchoredItem(myJoyDlgButton));
    dbRow->addStretchSpace();

    auto portsCol = std::make_unique<BoxLayout>(Dir::Vertical);
    portsCol->addAuto(anchoredItem(mySAPort));
    portsCol->addStretchSpace(1, VGAP * 2);
    portsCol->addAuto(std::move(dbRow));

    // The EEPROM column is as wide as its heading, and the button fills it
    auto eepromCol = std::make_unique<BoxLayout>(Dir::Vertical);
    eepromCol->addStretchSpace();
    eepromCol->addAuto(anchoredItem(myAtariVoxLabel));
    eepromCol->addAuto(alignedItem(myEraseEEPROMButton, HAlign::Fill, VAlign::Center));

    auto portsRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    portsRow->addStretch(std::move(portsCol));
    portsRow->addAuto(std::move(eepromCol));
    col.addAuto(std::move(portsRow));

    col.addSpace(VGAP * 3);
    // The serial-port pop-up widens with the dialog and keeps its own height (it
    // asks for no width of its own: the event mapper's list is what sets the
    // dialog's width, and a port name needs far less than that)
    col.addAuto(labeledRow(myAVoxPortLabel, myAVoxPort, 0, 0, true));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addMouseTab()
{
  constexpr int swidth = 13;
  WidgetArray wid;
  VariantList items;

  // Mouse.  The tab's controls are parented to a content pane; the pane lays
  // them out (see setLayout below) whenever the tab is sized — no resize code
  const int tabID = myTab->addTab("  Mouse  ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  // Use mouse as controller
  VarList::push_back(items, "Always", "always");
  VarList::push_back(items, "Analog devices", "analog");
  VarList::push_back(items, "Never", "never");
  myMouseControlLabel = new StaticTextWidget(pane, _font, "Use mouse as a controller");
  myMouseControl = new PopUpWidget(pane, _font, items, kMouseCtrlChanged);
  myMouseControl->setToolTip(Event::PrevMouseAsController, Event::NextMouseAsController);
  wid.push_back(myMouseControl);

  myMouseSensitivity = new StaticTextWidget(pane, _font, "Sensitivity:");

  // Add paddle speed (mouse emulation); the sensitivity sliders are indented, so
  // their reduced label widths keep the tracks aligned with the popups above
  myMPaddleSpeedLabel = new StaticTextWidget(pane, _font, "Paddle");
  myMPaddleSpeed = new SliderWidget(pane, _font, swidth, kMPSpeedChanged, 4, "%");
  myMPaddleSpeed->setMinValue(1);
  myMPaddleSpeed->setMaxValue(20);
  myMPaddleSpeed->setTickmarkIntervals(4);
  myMPaddleSpeed->setToolTip(Event::DecMousePaddleSense, Event::IncMousePaddleSense);
  wid.push_back(myMPaddleSpeed);

  // Add trackball speed
  myTrackBallSpeedLabel = new StaticTextWidget(pane, _font, "Trackball");
  myTrackBallSpeed = new SliderWidget(pane, _font, swidth, kTBSpeedChanged, 4, "%");
  myTrackBallSpeed->setMinValue(1);
  myTrackBallSpeed->setMaxValue(20);
  myTrackBallSpeed->setTickmarkIntervals(4);
  myTrackBallSpeed->setToolTip(Event::DecMouseTrackballSense, Event::IncMouseTrackballSense);
  wid.push_back(myTrackBallSpeed);

  // Add driving controller speed
  myDrivingSpeedLabel = new StaticTextWidget(pane, _font, "Driving controller");
  myDrivingSpeed = new SliderWidget(pane, _font, swidth, kDCSpeedChanged, 4, "%");
  myDrivingSpeed->setMinValue(1);
  myDrivingSpeed->setMaxValue(20);
  myDrivingSpeed->setTickmarkIntervals(4);
  myDrivingSpeed->setToolTip("Adjust driving controller sensitivity for digital and mouse input.",
    Event::DecreaseDrivingSense, Event::IncreaseDrivingSense);
  wid.push_back(myDrivingSpeed);

  // Mouse cursor state
  items.clear();
  VarList::push_back(items, "-UI, -Emulation", "0");
  VarList::push_back(items, "-UI, +Emulation", "1");
  VarList::push_back(items, "+UI, -Emulation", "2");
  VarList::push_back(items, "+UI, +Emulation", "3");
  myCursorStateLabel = new StaticTextWidget(pane, _font, "Mouse cursor visibility");
  myCursorState = new PopUpWidget(pane, _font, items, kCursorStateChanged);
  myCursorState->setToolTip(Event::PreviousCursorVisbility, Event::NextCursorVisbility);
  wid.push_back(myCursorState);
#ifndef WINDOWED_SUPPORT
  myCursorState->clearFlags(Widget::FLAG_ENABLED);
#endif

  // Grab mouse (in windowed mode)
  myGrabMouse = new CheckboxWidget(pane, _font, "Grab mouse in emulation mode");
  myGrabMouse->setToolTip(Event::ToggleGrabMouse);
  wid.push_back(myGrabMouse);
#ifndef WINDOWED_SUPPORT
  myGrabMouse->clearFlags(Widget::FLAG_ENABLED);
#endif

  // Add items for mouse
  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("Mouse");

  // Describe the layout once; the pane runs it on every resize
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::anchoredItem;
    using GUI::labeledRow;
    const int VGAP = Dialog::vGap(), INDENT = Dialog::indent();

    // The pop-up's and the sliders' labels share one label column, sized to
    // the longest of them (see the Devices tab).  The "Sensitivity:" heading
    // is not one of them: it is a plain label
    GUI::alignLabels({{myMouseControlLabel},
                      {myMPaddleSpeedLabel, INDENT}, {myTrackBallSpeedLabel, INDENT},
                      {myDrivingSpeedLabel, INDENT}, {myCursorStateLabel}});

    // Every row is as tall as what it holds (addAuto), so no height is stated
    // here and none can be wrong: the pop-up frames its text and is taller than
    // the sliders, and the rows follow the font on their own
    col.addAuto(labeledRow(myMouseControlLabel, myMouseControl));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myMouseSensitivity));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myMPaddleSpeedLabel, myMPaddleSpeed, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myTrackBallSpeedLabel, myTrackBallSpeed, 0, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myDrivingSpeedLabel, myDrivingSpeed, 0, INDENT));
    col.addSpace(VGAP * 4);
    col.addAuto(labeledRow(myCursorStateLabel, myCursorState));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myGrabMouse));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  // Left & right ports
  mySAPort->setState(settings.getString("saport") == "rl");

  // Use mouse as a controller
  myMouseControl->setSelected(settings.getString("usemouse"), "analog");
  handleMouseControlState();

  // Mouse cursor state
  myCursorState->setSelected(settings.getString("cursor"), "2");
  handleCursorState();

  // Digital dead zone
  myDigitalDeadzone->setValue(settings.getInt("joydeadzone"));
  // Analog dead zone
  myAnalogDeadzone->setValue(settings.getInt("adeadzone"));

  // Paddle speed (analog)
  myPaddleSpeed->setValue(settings.getInt("psense"));
  // Paddle linearity (analog)
  myPaddleLinearity->setValue(settings.getInt("plinear"));
  // Paddle dejitter (analog)
  myDejitterBase->setValue(settings.getInt("dejitter.base"));
  myDejitterDiff->setValue(settings.getInt("dejitter.diff"));

  // Paddle speed (digital and mouse)
  myDPaddleSpeed->setValue(settings.getInt("dsense"));
  myMPaddleSpeed->setValue(settings.getInt("msense"));

  // Trackball speed
  myTrackBallSpeed->setValue(settings.getInt("tsense"));
  // Driving controller speed
  myDrivingSpeed->setValue(settings.getInt("dcsense"));

  // Autofire
  myAutoFire->setState(settings.getBool("autofire"));

  // Autofire rate
  myAutoFireRate->setValue(settings.getInt("autofirerate"));

  // AtariVox serial port
  const string& avoxport = settings.getString("avoxport");
  const StringList ports = MediaFactory::createSerialPort()->portNames();
  VariantList items;

  for(const auto& port: ports)
    VarList::push_back(items, port, port);
  if(!avoxport.empty() && !BSPF::contains(ports, avoxport))
    VarList::push_back(items, avoxport, avoxport);
  if(items.empty())
    VarList::push_back(items, "None detected");

  myAVoxPort->addItems(items);
  myAVoxPort->setSelected(avoxport);

  // EEPROM erase (only enable in emulation mode and for valid controllers)
  if(instance().hasConsole())
  {
    const Controller& lport = instance().console().leftController();
    const Controller& rport = instance().console().rightController();

    myEraseEEPROMButton->setEnabled(
        lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox ||
        rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox);
  }
  else
    myEraseEEPROMButton->setEnabled(false);

  // Allow all 4 joystick directions
  myAllowAll4->setState(settings.getBool("joyallow4"));

  // Grab mouse
  myGrabMouse->setState(settings.getBool("grabmouse"));

  // Enable/disable modifier key-combos
  myModCombo->setState(settings.getBool("modcombo"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // *** Device & Ports ***
  // Digital dead zone
  int deadZone = myDigitalDeadzone->getValue();
  settings.setValue("joydeadzone", deadZone);
  Controller::setDigitalDeadZone(deadZone);
  // Analog dead zone
  deadZone = myAnalogDeadzone->getValue();
  settings.setValue("adeadzone", deadZone);
  Controller::setAnalogDeadZone(deadZone);

  // Paddle speed (analog)
  int sensitivity = myPaddleSpeed->getValue();
  settings.setValue("psense", sensitivity);
  Paddles::setAnalogSensitivity(sensitivity);
  // Paddle linearity (analog)
  const int linearity = myPaddleLinearity->getValue();
  settings.setValue("plinear", linearity);
  Paddles::setAnalogLinearity(linearity);

  // Paddle dejitter (analog)
  int dejitter = myDejitterBase->getValue();
  settings.setValue("dejitter.base", dejitter);
  Paddles::setDejitterBase(dejitter);
  dejitter = myDejitterDiff->getValue();
  settings.setValue("dejitter.diff", dejitter);
  Paddles::setDejitterDiff(dejitter);

  sensitivity = myDPaddleSpeed->getValue();
  settings.setValue("dsense", sensitivity);
  Paddles::setDigitalSensitivity(sensitivity);

  // Autofire mode & rate
  const bool enabled = myAutoFire->getState();
  settings.setValue("autofire", enabled);
  Controller::setAutoFire(enabled);

  const int rate = myAutoFireRate->getValue();
  settings.setValue("autofirerate", rate);
  Controller::setAutoFireRate(rate);

  // Allow all 4 joystick directions
  const bool allowall4 = myAllowAll4->getState();
  settings.setValue("joyallow4", allowall4);
  instance().eventHandler().allowAllDirections(allowall4);

  // Enable/disable modifier key-combos
  settings.setValue("modcombo", myModCombo->getState());

  // Left & right ports
  instance().eventHandler().mapStelladaptors(mySAPort->getState() ? "rl" : "lr");

  // AtariVox serial port
  settings.setValue("avoxport", myAVoxPort->getText());

  // *** Mouse ***
  // Use mouse as a controller
  const string& usemouse = myMouseControl->getSelectedTag().toString();
  settings.setValue("usemouse", usemouse);
  instance().eventHandler().setMouseControllerMode(usemouse);

  sensitivity = myMPaddleSpeed->getValue();
  settings.setValue("msense", sensitivity);
  Paddles::setMouseSensitivity(sensitivity);
  MindLink::setMouseSensitivity(sensitivity);

  // Trackball speed
  sensitivity = myTrackBallSpeed->getValue();
  settings.setValue("tsense", sensitivity);
  PointingDevice::setSensitivity(sensitivity);

  // Driving controller speed
  sensitivity = myDrivingSpeed->getValue();
  settings.setValue("dcsense", sensitivity);
  Driving::setSensitivity(sensitivity);

  // Grab mouse and hide cursor
  const string& cursor = myCursorState->getSelectedTag().toString();
  settings.setValue("cursor", cursor);

  // only allow grab mouse if cursor is hidden in emulation
  const int state = myCursorState->getSelected();
  const bool enableGrab = state != 1 && state != 3;
  const bool grab = enableGrab ? myGrabMouse->getState() : false;
  settings.setValue("grabmouse", grab);
  instance().frameBuffer().enableGrabMouse(grab);

  instance().eventHandler().saveKeyMapping();
  instance().eventHandler().saveJoyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Emulation events
      myEventMapper->setDefaults();
      break;

    case 1:  // Devices & Ports
      // Digital dead zone
      myDigitalDeadzone->setValue(0);

      // Analog dead zone
      myAnalogDeadzone->setValue(0);

      // Paddle speed (analog)
      myPaddleSpeed->setValue(20);

      // Paddle linearity
      myPaddleLinearity->setValue(100);
      myDejitterBase->setValue(0);
      myDejitterDiff->setValue(0);

      // Paddle speed (digital)
      myDPaddleSpeed->setValue(10);

      // Autofire
      myAutoFire->setState(false);

      // Autofire rate
      myAutoFireRate->setValue(0);

      // Allow all 4 joystick directions
      myAllowAll4->setState(false);

      // Enable/disable modifier key-combos
      myModCombo->setState(true);

      // Left & right ports
      mySAPort->setState(false);

      // AtariVox serial port
      myAVoxPort->setSelectedIndex(0);
      break;

    case 2:  // Mouse
      // Use mouse as a controller
      myMouseControl->setSelected("analog");

      // Paddle speed (mouse)
      myMPaddleSpeed->setValue(10);
      myTrackBallSpeed->setValue(10);
      myDrivingSpeed->setValue(10);

      // Mouse cursor state
      myCursorState->setSelected("2");

      // Grab mouse
      myGrabMouse->setState(true);

      handleMouseControlState();
      handleCursorState();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::repeatEnabled()
{
  return !myEventMapper->isRemapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if(myEventMapper->isRemapping())
    myEventMapper->handleKeyDown(key, mod);
  else
    Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyUp(StellaKey key, StellaMod mod)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if(myEventMapper->isRemapping())
    myEventMapper->handleKeyUp(key, mod);
  else
    Dialog::handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyDown(int stick, int button, bool longPress)
{
  // Remap joystick buttons in remap mode, otherwise pass to parent dialog
  if(myEventMapper->isRemapping())
    myEventMapper->handleJoyDown(stick, button);
  else
    Dialog::handleJoyDown(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyUp(int stick, int button)
{
  // Remap joystick buttons in remap mode, otherwise pass to parent dialog
  if(myEventMapper->isRemapping())
    myEventMapper->handleJoyUp(stick, button);
  else
    Dialog::handleJoyUp(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button)
{
  // Remap joystick axis in remap mode, otherwise pass to parent dialog
  if(myEventMapper->isRemapping())
    myEventMapper->handleJoyAxis(stick, axis, adir, button);
  else
    Dialog::handleJoyAxis(stick, axis, adir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::handleJoyHat(int stick, int hat, JoyHatDir hdir, int button)
{
  // Remap joystick hat in remap mode, otherwise pass to parent dialog
  if(myEventMapper->isRemapping())
    return myEventMapper->handleJoyHat(stick, hat, hdir, button);
  else
    return Dialog::handleJoyHat(stick, hat, hdir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::eraseEEPROM()
{
  // This method will only be callable if a console exists, so we don't
  // need to check again here
  Controller& lport = instance().console().leftController();
  Controller& rport = instance().console().rightController();

  if(lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox)
  {
    auto& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox)
  {
    auto& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
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

    case kDDeadzoneChanged:
      myDigitalDeadzone->setValueLabel(std::round(
          Controller::digitalDeadZoneValue(myDigitalDeadzone->getValue()) *
          100.F / (Paddles::ANALOG_RANGE / 2.F)));
      break;

    case kADeadzoneChanged:
      myAnalogDeadzone->setValueLabel(std::round(
          Controller::analogDeadZoneValue(myAnalogDeadzone->getValue()) *
          100.F / (Paddles::ANALOG_RANGE / 2.F)));
      break;

    case kPSpeedChanged:
      myPaddleSpeed->setValueLabel(std::round(Paddles::setAnalogSensitivity(
          myPaddleSpeed->getValue()) * 100.F));
      break;

    case kDejitterAvChanged:
      updateDejitterAveraging();
      break;

    case kDejitterReChanged:
      updateDejitterReaction();
      break;

    case kDPSpeedChanged:
      myDPaddleSpeed->setValueLabel(myDPaddleSpeed->getValue() * 10);
      break;

    case kDCSpeedChanged:
      myDrivingSpeed->setValueLabel(myDrivingSpeed->getValue() * 10);
      break;

    case kTBSpeedChanged:
      myTrackBallSpeed->setValueLabel(myTrackBallSpeed->getValue() * 10);
      break;

    case kAutoFireChanged:
    case kAutoFireRate:
      updateAutoFireRate();
      break;

    case kDBButtonPressed:
      if(!myJoyDialog)
      {
        const GUI::Font& font = instance().frameBuffer().font();
        myJoyDialog = std::make_unique<JoystickDialog>
          (this, font, fontWidth() * 60 + 20, fontHeight() * 18 + 20);
      }
      myJoyDialog->show();
      break;

    case kEEButtonPressed:
      if(!myConfirmMsg)
      {
        StringList msg;
        msg.emplace_back("This operation cannot be undone.");
        msg.emplace_back("All data stored on your AtariVox");
        msg.emplace_back("or SaveKey will be erased!");
        msg.emplace_back("");
        msg.emplace_back("If you are sure you want to erase");
        msg.emplace_back("the data, click 'OK', otherwise ");
        msg.emplace_back("click 'Cancel'.");
        myConfirmMsg = std::make_unique<GUI::MessageBox>
          (this, instance().frameBuffer().font(), msg,
           myMaxWidth, myMaxHeight, kConfirmEEEraseCmd,
           "OK", "Cancel", "Erase EEPROM", false);
      }
      myConfirmMsg->show();
      break;

    case kConfirmEEEraseCmd:
      eraseEEPROM();
      break;

    case kMouseCtrlChanged:
      handleMouseControlState();
      handleCursorState();
      break;

    case kCursorStateChanged:
      handleCursorState();
      break;

    case kMPSpeedChanged:
      myMPaddleSpeed->setValueLabel(myMPaddleSpeed->getValue() * 10);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateDejitterAveraging()
{
  const int strength = myDejitterBase->getValue();

  myDejitterBase->setValueLabel(strength ? std::to_string(strength) : "Off");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateDejitterReaction()
{
  const int strength = myDejitterDiff->getValue();

  myDejitterDiff->setValueLabel(strength ? std::to_string(strength) : "Off");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateAutoFireRate()
{
  const bool enable = myAutoFire->getState();
  const int rate = myAutoFireRate->getValue();

  myAutoFireRate->setEnabled(enable);
  myAutoFireRate->setValueLabel(rate ? std::to_string(rate) : "Off");
  myAutoFireRate->setValueUnit(rate ? " Hz" : "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleMouseControlState()
{
  const bool enable = myMouseControl->getSelected() != 2;

  myMPaddleSpeedLabel->setEnabled(enable);
  myMPaddleSpeed->setEnabled(enable);
  myTrackBallSpeedLabel->setEnabled(enable);
  myTrackBallSpeed->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleCursorState()
{
  const int state = myCursorState->getSelected();
  const bool enableGrab = state != 1 && state != 3 && myMouseControl->getSelected() != 2;

  myGrabMouse->setEnabled(enableGrab);
}
