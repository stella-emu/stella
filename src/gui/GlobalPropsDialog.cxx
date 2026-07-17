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
#include "Bankswitch.hxx"
#include "Dialog.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "Layout.hxx"
#include "LauncherDialog.hxx"
#include "GlobalPropsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalPropsDialog::GlobalPropsDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent(), font, "Power-on options"),
    CommandSender(boss)
{
  WidgetArray wid;
  VariantList items;
  const GUI::Font& infofont = instance().frameBuffer().infoFont();

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Bankswitch type
  myBSLabel = new StaticTextWidget(this, font, 0, 0, "Bankswitch type");
  for(const auto& [name, desc] : Bankswitch::BSList)
    VarList::push_back(items, desc, name);
  myBSType = new PopUpWidget(this, font, 0, 0, items, "");
  wid.push_back(myBSType);

  // TV type
  myTVLabel = new StaticTextWidget(this, font, 0, 0, "TV type");
  items.clear();
  VarList::push_back(items, "Default", "DEFAULT");
  VarList::push_back(items, "Color", "COLOR");
  VarList::push_back(items, "B/W", "BW");
  myTVType = new PopUpWidget(this, font, 0, 0, items, "");
  wid.push_back(myTVType);

  // Left difficulty
  myLeftDiffLabel = new StaticTextWidget(this, font, 0, 0, GUI::LEFT_DIFFICULTY);
  items.clear();
  VarList::push_back(items, "Default", "DEFAULT");
  VarList::push_back(items, "A (Expert)", "A");
  VarList::push_back(items, "B (Novice)", "B");
  myLeftDiff = new PopUpWidget(this, font, 0, 0, items, "");
  wid.push_back(myLeftDiff);

  // Right difficulty
  myRightDiffLabel = new StaticTextWidget(this, font, 0, 0, GUI::RIGHT_DIFFICULTY);
  // ... use same items as above
  myRightDiff = new PopUpWidget(this, font, 0, 0, items, "");
  wid.push_back(myRightDiff);

  // Start console with buttons held down
  myHeldLabel = new StaticTextWidget(this, font, 0, 0,
      "Start with the following held down:");
  myReleasedLabel = new StaticTextWidget(this, infofont, 0, 0,
      "(automatically released shortly after start)");

  // Start with console joystick direction/buttons held down
  createHoldWidgets(font, wid);

  // Start in debugger mode
  myDebug = new CheckboxWidget(this, font, 0, 0, "Start in Debugger mode");
#ifndef DEBUGGER_SUPPORT
  myDebug->setEnabled(false);
#endif
  wid.push_back(myDebug);

  // Add message concerning usage
  myInfo1 = new StaticTextWidget(this, infofont, 0, 0,
    "(*) These options are not saved, but apply to all");
  myInfo2 = new StaticTextWidget(this, infofont, 0, 0,
    "    further ROMs until selecting 'Defaults'.");

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font, "Load ROM", "Cancel");

  addToFocusList(wid);

  setHelpAnchor("PowerOn");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::createHoldWidgets(const GUI::Font& font, WidgetArray& wid)
{
  // Left joystick
  myLeftJoyLabel = new StaticTextWidget(this, font, 0, 0, "Left joy");
  myJoy[kJ0Up]    = new CheckboxWidget(this, font, 0, 0, "", kJ0Up);
  myJoy[kJ0Down]  = new CheckboxWidget(this, font, 0, 0, "", kJ0Down);
  myJoy[kJ0Left]  = new CheckboxWidget(this, font, 0, 0, "", kJ0Left);
  myJoy[kJ0Right] = new CheckboxWidget(this, font, 0, 0, "", kJ0Right);
  myJoy[kJ0Fire]  = new CheckboxWidget(this, font, 0, 0, "Fire", kJ0Fire);

  // Right joystick
  myRightJoyLabel = new StaticTextWidget(this, font, 0, 0, "Right joy");
  myJoy[kJ1Up]    = new CheckboxWidget(this, font, 0, 0, "", kJ1Up);
  myJoy[kJ1Down]  = new CheckboxWidget(this, font, 0, 0, "", kJ1Down);
  myJoy[kJ1Left]  = new CheckboxWidget(this, font, 0, 0, "", kJ1Left);
  myJoy[kJ1Right] = new CheckboxWidget(this, font, 0, 0, "", kJ1Right);
  myJoy[kJ1Fire]  = new CheckboxWidget(this, font, 0, 0, "Fire", kJ1Fire);

  // Console Select/Reset
  myConsoleLabel = new StaticTextWidget(this, font, 0, 0, "Console");
  myHoldSelect = new CheckboxWidget(this, font, 0, 0, GUI::SELECT);
  myHoldReset  = new CheckboxWidget(this, font, 0, 0, "Reset");

  static constexpr std::array<int, 10> TAB_ORDER = {
    kJ0Up, kJ0Left, kJ0Right, kJ0Down, kJ0Fire,
    kJ1Up, kJ1Left, kJ1Right, kJ1Down, kJ1Fire
  };
  for(int i = kJ0Up; i <= kJ1Fire; ++i)
    wid.push_back(myJoy[TAB_ORDER[i]]);

  wid.push_back(myHoldSelect);
  wid.push_back(myHoldReset);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> GlobalPropsDialog::joyLayout(StaticTextWidget* label,
                                                     int base)
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::centeredItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = Dialog::vGap();

  // The four directions form a cross, with fire below it; every box is one
  // standard gap from its neighbours, so the cross follows the font
  auto cross = std::make_unique<GridLayout>(3, 4, VGAP, VGAP);
  for(int col = 0; col < 3; ++col)
    cross->columnAuto(col);
  for(int row = 0; row < 4; ++row)
    cross->rowAuto(row);

  cross->place(1, 0, anchoredItem(myJoy[base + kJ0Up]));
  cross->place(0, 1, anchoredItem(myJoy[base + kJ0Left]));
  cross->place(2, 1, anchoredItem(myJoy[base + kJ0Right]));
  cross->place(1, 2, anchoredItem(myJoy[base + kJ0Down]));
  // Fire is the only one with a label, so it lies across the whole cross
  cross->place(0, 3, anchoredItem(myJoy[base + kJ0Fire]), 3);

  // Center the cross in the column, and its heading over the cross
  auto crossRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  crossRow->addStretchSpace();
  crossRow->addAuto(std::move(cross));
  crossRow->addStretchSpace();

  auto column = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  column->addAuto(centeredItem(label));
  column->addAuto(std::move(crossRow));

  return column;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> GlobalPropsDialog::consoleLayout()
{
  using GUI::BoxLayout;
  using GUI::centeredItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = Dialog::vGap();

  auto stack = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  stack->addAuto(anchoredItem(myHoldSelect));
  stack->addAuto(anchoredItem(myHoldReset));

  // Centered in the column, like the joysticks' crosses beside it
  auto stackRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  stackRow->addStretchSpace();
  stackRow->addAuto(std::move(stack));
  stackRow->addStretchSpace();

  auto column = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  column->addAuto(centeredItem(myConsoleLabel));
  column->addAuto(std::move(stackRow));

  return column;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> GlobalPropsDialog::holdLayout()
{
  using GUI::BoxLayout;
  using Dir = BoxLayout::Dir;

  // The three groups share the width equally
  auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
  row->addStretch(joyLayout(myLeftJoyLabel, kJ0Up));
  row->addStretch(joyLayout(myRightJoyLabel, kJ1Up));
  row->addStretch(consoleLayout());

  return row;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // The four pop-ups share one label column; the three offering the same kind
  // of choice also share one value-box width, so their boxes line up (the
  // bankswitch list has far longer entries and keeps its own width)
  GUI::alignLabels({{myBSLabel}, {myTVLabel},
                    {myLeftDiffLabel}, {myRightDiffLabel}});
  GUI::alignPopUps({myTVType, myLeftDiff, myRightDiff});

  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(labeledRow(myBSLabel, myBSType));
  root->addSpace(VGAP * 3);
  root->addAuto(labeledRow(myTVLabel, myTVType));
  root->addSpace(VGAP);
  root->addAuto(labeledRow(myLeftDiffLabel, myLeftDiff));
  root->addSpace(VGAP);
  root->addAuto(labeledRow(myRightDiffLabel, myRightDiff));
  root->addSpace(VGAP * 3);
  root->addAuto(anchoredItem(myHeldLabel));
  root->addAuto(anchoredItem(myReleasedLabel));
  root->addSpace(VGAP * 2);
  root->addAuto(holdLayout());
  root->addSpace(VGAP * 4);
  root->addAuto(anchoredItem(myDebug));
  root->addSpace(VGAP * 3);
  root->addAuto(anchoredItem(myInfo1));
  root->addAuto(anchoredItem(myInfo2));

  // The dialog is as large as its content asks to be, and at least wide enough
  // for the button row below it (which the content knows nothing about)
  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / Load ROM / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  myBSType->setSelected(settings.getString("bs"), "AUTO");
  myLeftDiff->setSelected(settings.getString("ld"), "DEFAULT");
  myRightDiff->setSelected(settings.getString("rd"), "DEFAULT");
  myTVType->setSelected(settings.getString("tv"), "DEFAULT");

  const string& holdjoy0 = settings.getString("holdjoy0");
  for(int i = kJ0Up; i <= kJ0Fire; ++i)
    myJoy[i]->setState(BSPF::containsIgnoreCase(holdjoy0, ourJoyState[i]));
  const string& holdjoy1 = settings.getString("holdjoy1");
  for(int i = kJ1Up; i <= kJ1Fire; ++i)
    myJoy[i]->setState(BSPF::containsIgnoreCase(holdjoy1, ourJoyState[i]));

  myHoldSelect->setState(settings.getBool("holdselect"));
  myHoldReset->setState(settings.getBool("holdreset"));
  myDebug->setState(settings.getBool("debug"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::saveConfig()
{
  Settings& settings = instance().settings();
  string s;

  s = myBSType->getSelectedTag().toString();
  if(s == "AUTO") s = "";
  settings.setValue("bs", s);

  s = myLeftDiff->getSelectedTag().toString();
  if(s == "DEFAULT") s = "";
  settings.setValue("ld", s);

  s = myRightDiff->getSelectedTag().toString();
  if(s == "DEFAULT") s = "";
  settings.setValue("rd", s);

  s = myTVType->getSelectedTag().toString();
  if(s == "DEFAULT") s = "";
  settings.setValue("tv", s);

  s = "";
  for(int i = kJ0Up; i <= kJ0Fire; ++i)
    if(myJoy[i]->getState())  s += ourJoyState[i];
  settings.setValue("holdjoy0", s);
  s = "";
  for(int i = kJ1Up; i <= kJ1Fire; ++i)
    if(myJoy[i]->getState())  s += ourJoyState[i];
  settings.setValue("holdjoy1", s);

  settings.setValue("holdselect", myHoldSelect->getState());
  settings.setValue("holdreset", myHoldReset->getState());

  settings.setValue("debug", myDebug->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::setDefaults()
{
  myBSType->setSelected("AUTO");
  myLeftDiff->setSelected("DEFAULT");
  myRightDiff->setSelected("DEFAULT");
  myTVType->setSelected("DEFAULT");

  for(int i = kJ0Up; i <= kJ1Fire; ++i)
    myJoy[i]->setState(false);

  myHoldSelect->setState(false);
  myHoldReset->setState(false);

  myDebug->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::handleCommand(CommandSender* sender, int cmd,
                                      int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      // Inform parent to load the ROM
      sendCommand(LauncherDialog::kLoadROMCmd, 0, 0);
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      saveConfig();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
