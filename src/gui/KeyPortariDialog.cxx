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

#include "OSystem.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Widget.hxx"
#include "PopUpWidget.hxx"
#include "Font.hxx"
#include "Variant.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "Launcher.hxx"
#include "ControllerDetector.hxx"
#include "KeyPortari.hxx"
#include "KeyPortariDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyPortariDialog::KeyPortariDialog(GuiObject* boss, const GUI::Font& font, int max_w, int max_h,
                               Properties& properties)
  : Dialog(boss->instance(), boss->parent(), font, "KeyPortari Settings", 0, 0, max_w, max_h),
    myGameProperties{properties}
{
  const int lineHeight = Dialog::lineHeight(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap();
  WidgetArray wid;
  VariantList ctrls;

  int xpos = HBORDER, ypos = VBORDER + _th;

  const int pwidth = font.getStringWidth("Alphanumeric  "); // a bit wider looks better overall

  ctrls.clear();
  VarList::push_back(ctrls, "Alphanumeric", "ALPHANUMERIC");
  VarList::push_back(ctrls, "Ascii", "ASCII");

  myProtocolLabel = new StaticTextWidget(this, font, xpos, ypos + 1, "Protocol");

  ypos += lineHeight + VGAP * 2;
  myProtocol = new PopUpWidget(this, font, xpos, ypos,
                               pwidth, lineHeight, ctrls);
  wid.push_back(myProtocol);
  ypos += lineHeight + VGAP;

  ctrls.clear();
  //VarList::push_back(ctrls, "Auto-detect", "AUTO");
  VarList::push_back(ctrls, "Joystick", "JOYSTICK");
  VarList::push_back(ctrls, "Paddles", "PADDLES");
  //VarList::push_back(ctrls, "Paddles_IAxis", "PADDLES_IAXIS");
  //VarList::push_back(ctrls, "Paddles_IAxDr", "PADDLES_IAXDR");
  //VarList::push_back(ctrls, "Booster Grip", "BOOSTERGRIP");
  VarList::push_back(ctrls, "Driving", "DRIVING");
  //VarList::push_back(ctrls, "Keyboard", "KEYBOARD");
  //VarList::push_back(ctrls, "Amiga mouse", "AMIGAMOUSE");
  //VarList::push_back(ctrls, "Atari mouse", "ATARIMOUSE");
  //VarList::push_back(ctrls, "Trak-Ball", "TRAKBALL");
  //VarList::push_back(ctrls, "AtariVox", "ATARIVOX");
  //VarList::push_back(ctrls, "SaveKey", "SAVEKEY");
  //VarList::push_back(ctrls, "Sega Genesis", "GENESIS");
  //VarList::push_back(items, "Joy2B+", "JOY_2B+");
  //VarList::push_back(ctrls, "Kid Vid", "KIDVID");
  //VarList::push_back(ctrls, "Light Gun", "LIGHTGUN");
  //VarList::push_back(ctrls, "MindLink", "MINDLINK");
  //VarList::push_back(ctrls, "KeyPortari", "KeyPortari");

  myLeftPortLabel = new StaticTextWidget(this, font, xpos, ypos + 1, "Left port");

  ypos += lineHeight + VGAP * 2;
  myLeft1Port = new PopUpWidget(this, font, xpos, ypos,
                               pwidth, lineHeight, ctrls, "P1 ");
  myLeft1Port->setEnabled(true);
  wid.push_back(myLeft1Port);
  ypos += lineHeight + VGAP;

  xpos = _w - HBORDER - myLeft1Port->getWidth(); // aligned right
  ypos = myLeftPortLabel->getTop() - 1;
  myRightPortLabel = new StaticTextWidget(this, font, xpos, ypos + 1, "Right port");

  ypos += lineHeight + VGAP * 2;
  myRight1Port = new PopUpWidget(this, font, xpos, ypos,
                                pwidth, lineHeight, ctrls, "P2 ");
  wid.push_back(myRight1Port);
  ypos += lineHeight + VGAP;

  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("KeyPortari");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::loadControllerProperties(const Properties& props)
{
  myProtocol->setSelected(props.get(PropType::Controller_KeyPortariProtocol), "ASCII");
  myLeft1Port->setSelected(props.get(PropType::Controller_Left1), "JOYSTICK");
  myRight1Port->setSelected(props.get(PropType::Controller_Right1), "JOYSTICK");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::loadConfig()
{
  loadControllerProperties(myGameProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::saveConfig()
{
  string protocol = myProtocol->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_KeyPortariProtocol, protocol);
  string controller = myLeft1Port->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Left1, controller);
  controller = myRight1Port->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Right1, controller);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::setDefaults()
{
  // Load the default properties
  const string& md5 = myGameProperties.get(PropType::Cart_MD5);
  Properties defaultProperties;

  instance().propSet().getMD5(md5, defaultProperties, true);
  loadControllerProperties(defaultProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
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

    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}
