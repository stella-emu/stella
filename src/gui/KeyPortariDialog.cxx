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
#include "Layout.hxx"
#include "Font.hxx"
#include "Variant.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "Launcher.hxx"
#include "ControllerDetector.hxx"
#include "KeyPortari.hxx"
#include "KeyPortariDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

KeyPortariDialog::KeyPortariDialog(GuiObject* boss, const GUI::Font& font, Properties& properties)
  : Dialog(boss->instance(), boss->parent(), font),
    myGameProperties{properties}
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  WidgetArray wid;
  VariantList protocols;
  VarList::push_back(protocols, "ALPHANUMERIC");
  VarList::push_back(protocols, "ASCII");

  VariantList ctrls;

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
  //VarList::push_back(ctrls, "QuadTari", "QUADTARI");

  // A couple of characters more than the items strictly need, which looks better
  // overall -- so these state a width instead of taking the self-sizing ctor.
  // layout() re-applies it, so it follows a live font change
  const int pwidth = PopUpWidget::calcWidth(font, protocols) + Dialog::fontWidth() * 2;

  myProtocolLabel = new LabelWidget(this, font, "Protocol");
  myProtocol = new PopUpWidget(this, font, pwidth, protocols);
  wid.push_back(myProtocol);

  // A couple of characters more than the items strictly need, which looks better
  // overall -- so these state a width instead of taking the self-sizing ctor.
  // layout() re-applies it, so it follows a live font change
  const int cwidth = PopUpWidget::calcWidth(font, protocols) + Dialog::fontWidth() * 2;
  
  myLeftPortLabel = new LabelWidget(this, font, "Left port");
  myLeft1Port = new PopUpWidget(this, font, cwidth, ctrls);
  wid.push_back(myLeft1Port);

  myRightPortLabel = new LabelWidget(this, font, "Right port");
  myRight1Port = new PopUpWidget(this, font, cwidth, ctrls);
  wid.push_back(myRight1Port);

  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("KeyPortari");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // See the c'tor: the width is the dialog's choice, so it is re-applied here
  // from the live font rather than left at whatever the c'tor computed
  const int pwidth = _font.getStringWidth("Alphanumeric  ");
  for(auto* p: {myProtocol, myLeft1Port, myRight1Port})
    p->setBoxWidth(pwidth);

  // The popups' labels share one column, keeping their
  // value boxes the same width and in line across both ports
  GUI::alignLabels({{myProtocolLabel}, {myLeftPortLabel}, {myRightPortLabel}});

  // Both ports have the same structure: a header label, then a popup
  const auto portColumn = [&](LabelWidget* label, PopUpWidget* popup)
  {
    auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, 0, 0);
    col->addAuto(anchoredItem(label));
    col->addSpace(VGAP * 2);
    col->addAuto(labeledRow(label, popup));
    col->addSpace(VGAP);
    return col;
  };

  // The protocol and port columns stacked
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(portColumn(myProtocolLabel, myProtocol));
  root->addSpace(VGAP);
  root->addAuto(portColumn(myLeftPortLabel, myLeft1Port));
  root->addSpace(VGAP);
  root->addAuto(portColumn(myRightPortLabel, myRight1Port));

  // The dialog is as large as the columns ask to be, and at least wide
  // enough for the button row below them
  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::loadControllerProperties(const Properties& props)
{
  myProtocol->setSelected(props.get(PropType::Controller_KeyPortari), "ASCII");
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
  myGameProperties.set(PropType::Controller_KeyPortari, protocol);
  string controller = myLeft1Port->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Left1, controller);
  controller = myRight1Port->getSelectedTag().toString();
  myGameProperties.set(PropType::Controller_Right1, controller);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::setDefaults()
{
  // Load the default properties
  const string_view md5 = myGameProperties.get(PropType::Cart_MD5);
  Properties defaultProperties;

  instance().propSet().getMD5(md5, defaultProperties, true);
  loadControllerProperties(defaultProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyPortariDialog::handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id)
{
  switch(cmd)
  {
    case GuiObject::Cmd::OK:
      saveConfig();
      close();
      break;

    case GuiObject::Cmd::Defaults:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}

