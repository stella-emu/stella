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
  const int lineHeight = Dialog::lineHeight();
  int pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  WidgetArray wid;
  VariantList items;
  const GUI::Font& infofont = instance().frameBuffer().infoFont();

  // Bankswitch type
  myBSLabel = new StaticTextWidget(this, font, 0, 0, "Bankswitch type");
  for(const auto& [name, desc] : Bankswitch::BSList)
    VarList::push_back(items, desc, name);
  myBSType = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, items, "");
  wid.push_back(myBSType);

  pwidth = font.getStringWidth("A (Expert)");

  // TV type
  myTVLabel = new StaticTextWidget(this, font, 0, 0, "TV type");
  items.clear();
  VarList::push_back(items, "Default", "DEFAULT");
  VarList::push_back(items, "Color", "COLOR");
  VarList::push_back(items, "B/W", "BW");
  myTVType = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, items, "");
  wid.push_back(myTVType);

  // Left difficulty
  myLeftDiffLabel = new StaticTextWidget(this, font, 0, 0, GUI::LEFT_DIFFICULTY);
  items.clear();
  VarList::push_back(items, "Default", "DEFAULT");
  VarList::push_back(items, "A (Expert)", "A");
  VarList::push_back(items, "B (Novice)", "B");
  myLeftDiff = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, items, "");
  wid.push_back(myLeftDiff);

  // Right difficulty
  myRightDiffLabel = new StaticTextWidget(this, font, 0, 0, GUI::RIGHT_DIFFICULTY);
  // ... use same items as above
  myRightDiff = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, items, "");
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
int GlobalPropsDialog::layoutHoldWidgets(int x, int y)
{
  const int fontWidth = Dialog::fontWidth(),
            VGAP      = Dialog::vGap();
  int xpos = x, ypos = y;
  const int xdiff = CheckboxWidget::boxSize(_font) - 9;

  // Left joystick, arranged as a directional cross
  myLeftJoyLabel->setPos(xpos, ypos + 2);
  xpos += myLeftJoyLabel->getWidth()/2 - xdiff - 2;
  ypos += myLeftJoyLabel->getHeight() + VGAP;
  myJoy[kJ0Up]->setPos(xpos, ypos);
  ypos += myJoy[kJ0Up]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ0Down]->setPos(xpos, ypos);
  xpos -= myJoy[kJ0Up]->getWidth() + xdiff;
  ypos -= myJoy[kJ0Up]->getHeight() + VGAP;
  myJoy[kJ0Left]->setPos(xpos, ypos);
  xpos += (myJoy[kJ0Up]->getWidth() + xdiff) * 2;
  myJoy[kJ0Right]->setPos(xpos, ypos);
  xpos -= (myJoy[kJ0Up]->getWidth() + xdiff) * 2;
  ypos += myJoy[kJ0Down]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ0Fire]->setPos(xpos, ypos);

  xpos = _w / 3;  ypos = y;

  // Right joystick
  myRightJoyLabel->setPos(xpos, ypos + 2);
  xpos += myRightJoyLabel->getWidth()/2 - xdiff - 2;
  ypos += myRightJoyLabel->getHeight() + VGAP;
  myJoy[kJ1Up]->setPos(xpos, ypos);
  ypos += myJoy[kJ1Up]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ1Down]->setPos(xpos, ypos);
  xpos -= myJoy[kJ1Up]->getWidth() + xdiff;
  ypos -= myJoy[kJ1Up]->getHeight() + VGAP;
  myJoy[kJ1Left]->setPos(xpos, ypos);
  xpos += (myJoy[kJ1Up]->getWidth() + xdiff) * 2;
  myJoy[kJ1Right]->setPos(xpos, ypos);
  xpos -= (myJoy[kJ1Up]->getWidth() + xdiff) * 2;
  ypos += myJoy[kJ1Down]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ1Fire]->setPos(xpos, ypos);

  xpos = 2 * _w / 3 + fontWidth;  ypos = y;

  // Console Select/Reset
  myConsoleLabel->setPos(xpos, ypos + 2);
  ypos += myConsoleLabel->getHeight() + VGAP;
  myHoldSelect->setPos(xpos, ypos);
  ypos += myHoldSelect->getHeight() + VGAP;
  myHoldReset->setPos(xpos, ypos);

  return myJoy[kJ0Fire]->getBottom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  const int infoLineHeight = infofont.getLineHeight();
  const int lwidth = _font.getStringWidth("Right difficulty ");
  const int pwidth = _font.getStringWidth("CM (SpectraVideo CompuMate)");

  // Size the (fixed) dialog from the current font so it reflows on font change
  _w = HBORDER * 2 + std::max(lwidth + pwidth + PopUpWidget::dropDownWidth(_font),
                              49 * infofont.getMaxCharWidth());
  _h = _th + 11 * (lineHeight + VGAP) + 3 * infoLineHeight + VGAP * 12
       + buttonHeight + VBORDER * 2;

  // Top section: a vertical stack of the four labelled pop-ups followed by the
  // "held down" heading.  Each pop-up row is a label in a fixed lwidth column
  // with the control to its right.
  auto popupRow = [&](StaticTextWidget* label, PopUpWidget* popup) {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(anchoredItem(label), lwidth);
    row->addFixed(anchoredItem(popup), popup->getWidth());
    return row;
  };

  auto top = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  top->addFixed(popupRow(myBSLabel, myBSType), lineHeight);
  top->addSpace(VGAP * 3);
  top->addFixed(popupRow(myTVLabel, myTVType), lineHeight);
  top->addSpace(VGAP);
  top->addFixed(popupRow(myLeftDiffLabel, myLeftDiff), lineHeight);
  top->addSpace(VGAP);
  top->addFixed(popupRow(myRightDiffLabel, myRightDiff), lineHeight);
  top->addSpace(VGAP * 3);
  top->addFixed(anchoredItem(myHeldLabel), lineHeight);
  top->addFixed(anchoredItem(myReleasedLabel), infoLineHeight);
  top->doLayout(0, _th, _w, _h - _th);

  // The joystick/console "held down" checkboxes use a custom cross layout
  const int holdY = myReleasedLabel->getTop() + infoLineHeight + VGAP * 2;
  const int fireBottom = layoutHoldWidgets(fontWidth * 4, holdY);

  // Start in debugger mode
  myDebug->setPos(HBORDER, fireBottom + VGAP * 4);

  // Usage message, anchored just above the button row
  const int infoY = _h - VBORDER - buttonHeight - VGAP * 3 - infoLineHeight * 2;
  myInfo1->setPos(HBORDER, infoY);
  myInfo2->setPos(HBORDER, infoY + infoLineHeight);

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
