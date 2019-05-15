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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Bankswitch.hxx"
#include "Control.hxx"
#include "Dialog.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "LauncherDialog.hxx"
#include "GlobalPropsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalPropsDialog::GlobalPropsDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent(), font, "Power-on options"),
    CommandSender(boss)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  int lwidth = font.getStringWidth("Right difficulty "),
      pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  const int VGAP = 4;
  WidgetArray wid;
  VariantList items;
  const GUI::Font& infofont = instance().frameBuffer().infoFont();

  // Set real dimensions
  _w = lwidth + pwidth + fontWidth*3 + 15;
  _h = 15 * (lineHeight + 4) + buttonHeight + 16 + _th;

  xpos = 10;  ypos = 10 + _th;

  // Bankswitch type
  new StaticTextWidget(this, font, xpos, ypos+1, "Bankswitch type");
  for(uInt32 i = 0; i < uInt32(Bankswitch::Type::NumSchemes); ++i)
    VarList::push_back(items, Bankswitch::BSList[i].desc, Bankswitch::BSList[i].name);
  myBSType = new PopUpWidget(this, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "");
  wid.push_back(myBSType);
  ypos += lineHeight + VGAP * 3;

  pwidth = font.getStringWidth("A (Expert)");

  // TV type
  new StaticTextWidget(this, font, xpos, ypos + 1, "TV type");
  items.clear();
  VarList::push_back(items, "Default", "DEFAULT");
  VarList::push_back(items, "Color", "COLOR");
  VarList::push_back(items, "B/W", "BW");
  myTVType = new PopUpWidget(this, font, xpos + lwidth, ypos,
                             pwidth, lineHeight, items, "");
  wid.push_back(myTVType);
  ypos += lineHeight + VGAP;

  // Left difficulty
  new StaticTextWidget(this, font, xpos, ypos+1, GUI::LEFT_DIFFICULTY);
  items.clear();
  VarList::push_back(items, "Default", "DEFAULT");
  VarList::push_back(items, "A (Expert)", "A");
  VarList::push_back(items, "B (Novice)", "B");
  myLeftDiff = new PopUpWidget(this, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, items, "");
  wid.push_back(myLeftDiff);
  ypos += lineHeight + VGAP;

  // Right difficulty
  new StaticTextWidget(this, font, xpos, ypos+1, GUI::RIGHT_DIFFICULTY);
  // ... use same items as above
  myRightDiff = new PopUpWidget(this, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, items, "");
  wid.push_back(myRightDiff);
  ypos += lineHeight + VGAP * 3;

  // Start in debugger mode
  new StaticTextWidget(this, font, xpos, ypos+1, "Startup mode");
  items.clear();
  VarList::push_back(items, "Console", "false");
#ifdef DEBUGGER_SUPPORT
  VarList::push_back(items, "Debugger", "true");
#endif
  myDebug = new PopUpWidget(this, font, xpos+lwidth, ypos,
                            pwidth, lineHeight, items, "");
  wid.push_back(myDebug);
  ypos += lineHeight + VGAP * 3;

  // Start console with buttons held down
  new StaticTextWidget(this, font, xpos, ypos+1,
      "Start with the following held down:");
  ypos += lineHeight;
  new StaticTextWidget(this, infofont, xpos, ypos+1,
      "(automatically released shortly after start)");

  // Start with console joystick direction/buttons held down
  xpos = 32;  ypos += infofont.getLineHeight() + VGAP * 2;
  ypos = addHoldWidgets(font, xpos, ypos, wid);

  // Add message concerning usage
  xpos = 10;  ypos += 2 * fontHeight;
  ypos = _h - fontHeight * 2 - infofont.getLineHeight() - 24;
  new StaticTextWidget(this, infofont, xpos, ypos,
    "(*) These options are not saved, but apply to all");
  ypos += infofont.getLineHeight();
  new StaticTextWidget(this, infofont, xpos, ypos,
    "    further ROMs until selecting 'Defaults'.");

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font, "Load ROM", "Cancel");

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int GlobalPropsDialog::addHoldWidgets(const GUI::Font& font, int x, int y,
                                      WidgetArray& wid)
{
  int xpos = x, ypos = y;
  const int VGAP = 4;

  // Left joystick
  StaticTextWidget* t = new StaticTextWidget(this, font, xpos, ypos+2, "Left joy");
  xpos += t->getWidth()/2 - 7;  ypos += t->getHeight() + VGAP;
  myJoy[kJ0Up] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Up);
  ypos += myJoy[kJ0Up]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ0Down] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Down);
  xpos -= myJoy[kJ0Up]->getWidth() + 5;
  ypos -= myJoy[kJ0Up]->getHeight() + VGAP;
  myJoy[kJ0Left] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Left);
  xpos += (myJoy[kJ0Up]->getWidth() + 5) * 2;
  myJoy[kJ0Right] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Right);
  xpos -= (myJoy[kJ0Up]->getWidth() + 5) * 2;
  ypos += myJoy[kJ0Down]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ0Fire] = new CheckboxWidget(this, font, xpos, ypos, "Fire", kJ0Fire);

  int final_y = ypos;
  xpos = _w / 3;  ypos = y;

  // Right joystick
  t = new StaticTextWidget(this, font, xpos, ypos + 2, "Right joy");
  xpos += t->getWidth()/2 - 7;  ypos += t->getHeight() + VGAP;
  myJoy[kJ1Up] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Up);
  ypos += myJoy[kJ1Up]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ1Down] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Down);
  xpos -= myJoy[kJ1Up]->getWidth() + 5;
  ypos -= myJoy[kJ1Up]->getHeight() + VGAP;
  myJoy[kJ1Left] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Left);
  xpos += (myJoy[kJ1Up]->getWidth() + 5) * 2;
  myJoy[kJ1Right] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Right);
  xpos -= (myJoy[kJ1Up]->getWidth() + 5) * 2;
  ypos += myJoy[kJ1Down]->getHeight() * 2 + VGAP * 2;
  myJoy[kJ1Fire] = new CheckboxWidget(this, font, xpos, ypos, "Fire", kJ1Fire);

  xpos = 2 * _w / 3 + 8;  ypos = y;

  // Console Select/Reset
  t = new StaticTextWidget(this, font, xpos, ypos+2, "Console");
  ypos += t->getHeight() + VGAP;
  myHoldSelect = new CheckboxWidget(this, font, xpos, ypos, GUI::SELECT);
  ypos += myHoldSelect->getHeight() + VGAP;
  myHoldReset = new CheckboxWidget(this, font, xpos, ypos, "Reset");

  const int TAB_ORDER[10] = {
    kJ0Up, kJ0Left, kJ0Right, kJ0Down, kJ0Fire,
    kJ1Up, kJ1Left, kJ1Right, kJ1Down, kJ1Fire
  };
  for(int i = kJ0Up; i <= kJ1Fire; ++i)
    wid.push_back(myJoy[TAB_ORDER[i]]);

  wid.push_back(myHoldSelect);
  wid.push_back(myHoldReset);

  return final_y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::loadConfig()
{
  Settings& settings = instance().settings();

  myBSType->setSelected(settings.getString("bs"), "AUTO");
  myLeftDiff->setSelected(settings.getString("ld"), "DEFAULT");
  myRightDiff->setSelected(settings.getString("rd"), "DEFAULT");
  myTVType->setSelected(settings.getString("tv"), "DEFAULT");
  myDebug->setSelected(settings.getBool("debug") ? "true" : "false");

  const string& holdjoy0 = settings.getString("holdjoy0");
  for(int i = kJ0Up; i <= kJ0Fire; ++i)
    myJoy[i]->setState(BSPF::containsIgnoreCase(holdjoy0, ourJoyState[i]));
  const string& holdjoy1 = settings.getString("holdjoy1");
  for(int i = kJ1Up; i <= kJ1Fire; ++i)
    myJoy[i]->setState(BSPF::containsIgnoreCase(holdjoy1, ourJoyState[i]));

  myHoldSelect->setState(settings.getBool("holdselect"));
  myHoldReset->setState(settings.getBool("holdreset"));
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

  settings.setValue("debug", myDebug->getSelectedTag().toBool());

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::setDefaults()
{
  myBSType->setSelected("AUTO");
  myLeftDiff->setSelected("DEFAULT");
  myRightDiff->setSelected("DEFAULT");
  myTVType->setSelected("DEFAULT");
  myDebug->setSelected("false");

  for(int i = kJ0Up; i <= kJ1Fire; ++i)
    myJoy[i]->setState(false);

  myHoldSelect->setState(false);
  myHoldReset->setState(false);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const GlobalPropsDialog::ourJoyState[10] = {
  "U", "D", "L", "R", "F", "U", "D", "L", "R", "F"
};
