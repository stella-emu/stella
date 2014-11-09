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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"

#include "Cart.hxx"
#include "Control.hxx"
#include "Dialog.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "LauncherDialog.hxx"

#include "GlobalPropsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalPropsDialog::GlobalPropsDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent()),
    CommandSender(boss)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  int lwidth = font.getStringWidth("Right Difficulty: "),
      pwidth = font.getStringWidth("CM (SpectraVideo CompuMate)");
  WidgetArray wid;
  VariantList items;
  const GUI::Font& infofont = instance().frameBuffer().infoFont();

  // Set real dimensions
  _w = lwidth + pwidth + fontWidth*3 + 15;
  _h = 17 * (lineHeight + 4) + buttonHeight + 20;

  xpos = 10;  ypos = 10;

  // Bankswitch type
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Bankswitch type:", kTextAlignLeft);
  items.clear();
  for(int i = 0; i < Cartridge::ourNumBSTypes; ++i)
    VList::push_back(items, Cartridge::ourBSList[i].desc, Cartridge::ourBSList[i].type);
  myBSType = new PopUpWidget(this, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myBSType);
  ypos += lineHeight + 10;

  // Left difficulty
  pwidth = font.getStringWidth("Debugger");
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Left Difficulty:", kTextAlignLeft);
  items.clear();
  VList::push_back(items, "Default", "DEFAULT");
  VList::push_back(items, "B", "B");
  VList::push_back(items, "A", "A");
  myLeftDiff = new PopUpWidget(this, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myLeftDiff);
  ypos += lineHeight + 5;

  // Right difficulty
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Difficulty:", kTextAlignLeft);
  // ... use same items as above
  myRightDiff = new PopUpWidget(this, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myRightDiff);
  ypos += lineHeight + 5;

  // TV type
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "TV Type:", kTextAlignLeft);
  items.clear();
  VList::push_back(items, "Default", "DEFAULT");
  VList::push_back(items, "Color", "COLOR");
  VList::push_back(items, "B & W", "BW");
  myTVType = new PopUpWidget(this, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myTVType);
  ypos += lineHeight + 10;

  // Start in debugger mode
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Startup Mode:", kTextAlignLeft);
  items.clear();
  VList::push_back(items, "Console", "false");
  VList::push_back(items, "Debugger", "true");
  myDebug = new PopUpWidget(this, font, xpos+lwidth, ypos,
                            pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myDebug);
  ypos += lineHeight + 10;

  // Start console with buttons held down
  new StaticTextWidget(this, font, xpos, ypos+1,
      font.getStringWidth("Start console with the following held down:"),
      fontHeight, "Start console with the following held down:",
      kTextAlignLeft);
  xpos += 10;  ypos += lineHeight;
  new StaticTextWidget(this, infofont, xpos, ypos+1, _w - 40, infofont.getFontHeight(),
      "(*) Buttons are automatically released shortly",
      kTextAlignLeft);
  ypos += infofont.getLineHeight();
  new StaticTextWidget(this, infofont, xpos, ypos+1, _w - 40, infofont.getFontHeight(),
      "    after emulation has started",
      kTextAlignLeft);

  // Start with console joystick direction/buttons held down
  xpos = 30;  ypos += lineHeight + 10;
  ypos = addHoldWidgets(font, xpos, ypos, wid);

  // Add message concerning usage
  xpos = 10;  ypos += 2 * fontHeight;
  new StaticTextWidget(this, infofont, xpos, ypos,  _w - 20, infofont.getFontHeight(),
    "(*) These options are not saved, but apply to all", kTextAlignLeft);
  ypos += infofont.getLineHeight();
  new StaticTextWidget(this, infofont, xpos, ypos,  _w - 20, infofont.getFontHeight(),
    "    further ROMs until clicking 'Defaults'", kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font, "Load ROM", "Close");

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalPropsDialog::~GlobalPropsDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int GlobalPropsDialog::addHoldWidgets(const GUI::Font& font, int x, int y,
                                      WidgetArray& wid)
{
  const int fontHeight = font.getFontHeight();
  int xpos = x, ypos = y;

  // Left joystick
  StaticTextWidget* t = new StaticTextWidget(this, font, xpos, ypos+2,
    font.getStringWidth("Left Joy:"), fontHeight, "Left Joy:",
    kTextAlignLeft);
  xpos += t->getWidth()/2 - 5;  ypos += t->getHeight() + 10;
  myJoy[kJ0Up] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Up);
  ypos += myJoy[kJ0Up]->getHeight() * 2 + 10;
  myJoy[kJ0Down] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Down);
  xpos -= myJoy[kJ0Up]->getWidth() + 5;
  ypos -= myJoy[kJ0Up]->getHeight() + 5;
  myJoy[kJ0Left] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Left);
  xpos += (myJoy[kJ0Up]->getWidth() + 5) * 2;
  myJoy[kJ0Right] = new CheckboxWidget(this, font, xpos, ypos, "", kJ0Right);
  xpos -= (myJoy[kJ0Up]->getWidth() + 5) * 2;
  ypos += myJoy[kJ0Down]->getHeight() * 2 + 10;
  myJoy[kJ0Fire] = new CheckboxWidget(this, font, xpos, ypos, "Fire", kJ0Fire);

  int final_y = ypos;
  xpos = _w / 3;  ypos = y;

  // Right joystick
  t = new StaticTextWidget(this, font, xpos, ypos+2,
    font.getStringWidth("Right Joy:"), fontHeight, "Right Joy:",
    kTextAlignLeft);
  xpos += t->getWidth()/2 - 5;  ypos += t->getHeight() + 10;
  myJoy[kJ1Up] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Up);
  ypos += myJoy[kJ1Up]->getHeight() * 2 + 10;
  myJoy[kJ1Down] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Down);
  xpos -= myJoy[kJ1Up]->getWidth() + 5;
  ypos -= myJoy[kJ1Up]->getHeight() + 5;
  myJoy[kJ1Left] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Left);
  xpos += (myJoy[kJ1Up]->getWidth() + 5) * 2;
  myJoy[kJ1Right] = new CheckboxWidget(this, font, xpos, ypos, "", kJ1Right);
  xpos -= (myJoy[kJ1Up]->getWidth() + 5) * 2;
  ypos += myJoy[kJ1Down]->getHeight() * 2 + 10;
  myJoy[kJ1Fire] = new CheckboxWidget(this, font, xpos, ypos, "Fire", kJ1Fire);

  xpos = 2 * _w / 3;  ypos = y;

  // Console Select/Reset
  t = new StaticTextWidget(this, font, xpos, ypos+2,
    font.getStringWidth("Console:"), fontHeight, "Console:",
    kTextAlignLeft);
  xpos -= 10;  ypos += t->getHeight() + 10;
  myHoldSelect = new CheckboxWidget(this, font, xpos, ypos, "Select");
  ypos += myHoldSelect->getHeight() + 5;
  myHoldReset = new CheckboxWidget(this, font, xpos, ypos, "Reset");

  for(int i = kJ0Up; i <= kJ1Fire; ++i)
    wid.push_back(myJoy[i]);

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
    myJoy[i]->setState(BSPF_containsIgnoreCase(holdjoy0, ourJoyState[i]));
  const string& holdjoy1 = settings.getString("holdjoy1");
  for(int i = kJ1Up; i <= kJ1Fire; ++i)
    myJoy[i]->setState(BSPF_containsIgnoreCase(holdjoy1, ourJoyState[i]));

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

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::handleCommand(CommandSender* sender, int cmd,
                                      int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      // Inform parent to load the ROM
      sendCommand(LauncherDialog::kLoadROMCmd, 0, 0);
      break;

    case kDefaultsCmd:
      setDefaults();
      saveConfig();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* GlobalPropsDialog::ourJoyState[10] = {
  "U", "D", "L", "R", "F", "U", "D", "L", "R", "F"
};
