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
#include "QuadTari.hxx"
#include "QuadTariDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTariDialog::QuadTariDialog(GuiObject* boss, const GUI::Font& font,
                               Properties& properties)
  : Dialog(boss->instance(), boss->parent(), font, "QuadTari controllers"),
    myGameProperties{properties}
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight();
  WidgetArray wid;
  VariantList ctrls;

  VarList::push_back(ctrls, "Auto-detect", "AUTO");
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
  VarList::push_back(ctrls, "AtariVox", "ATARIVOX");
  VarList::push_back(ctrls, "SaveKey", "SAVEKEY");
  //VarList::push_back(ctrls, "Sega Genesis", "GENESIS");
  //VarList::push_back(items, "Joy2B+", "JOY_2B+");
  //VarList::push_back(ctrls, "Kid Vid", "KIDVID");
  //VarList::push_back(ctrls, "Light Gun", "LIGHTGUN");
  //VarList::push_back(ctrls, "MindLink", "MINDLINK");
  //VarList::push_back(ctrls, "QuadTari", "QUADTARI");

  const int pwidth = font.getStringWidth("Auto-detect  "); // a bit wider looks better overall

  // Widgets are only created here (at placeholder position); layout() assigns
  // all geometry from the current font.  The two ports are laid out as two
  // side-by-side columns of identical structure.
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myLeftPortLabel = new StaticTextWidget(this, font, 0, 0, "Left port");
  myLeft1Port = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, ctrls, "P1 ");
  wid.push_back(myLeft1Port);
  myLeft1PortDetected = new StaticTextWidget(this, ifont, 0, 0, "                 ");
  myLeft2Port = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, ctrls, "P3 ");
  wid.push_back(myLeft2Port);
  myLeft2PortDetected = new StaticTextWidget(this, ifont, 0, 0, "                 ");

  myRightPortLabel = new StaticTextWidget(this, font, 0, 0, "Right port");
  myRight1Port = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, ctrls, "P2 ");
  wid.push_back(myRight1Port);
  myRight1PortDetected = new StaticTextWidget(this, ifont, 0, 0, "                 ");
  myRight2Port = new PopUpWidget(this, font, 0, 0, pwidth, lineHeight, ctrls, "P4 ");
  wid.push_back(myRight2Port);
  myRight2PortDetected = new StaticTextWidget(this, ifont, 0, 0, "                 ");

  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("Quadtari");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Size the dialog from the current font.  The height must fit the tallest
  // port column (a header plus two popup/auto-detect-label pairs) above the
  // button group; the previous caller-supplied height (10 * fontHeight) was too
  // short once the auto-detect labels are shown.
  _w = 42 * fontWidth;
  _h = _th + VBORDER * 2 + buttonHeight + 5 * lineHeight + VGAP * 9;

  // Both ports share the same column width and vertical structure: a header
  // label, then two popups each followed by its (indented) auto-detect label
  const int columnWidth = myLeft1Port->getWidth();
  const auto portColumn = [&](StaticTextWidget* label,
                              PopUpWidget* popup1, StaticTextWidget* detected1,
                              PopUpWidget* popup2, StaticTextWidget* detected2)
  {
    auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, 0, 0);
    col->addFixed(anchoredItem(label), lineHeight);
    col->addSpace(VGAP * 2);
    col->addFixed(anchoredItem(popup1), lineHeight);
    col->addSpace(VGAP);
    col->addFixed(indentedItem(detected1, fontWidth * 3), lineHeight);
    col->addSpace(VGAP);
    col->addFixed(anchoredItem(popup2), lineHeight);
    col->addSpace(VGAP);
    col->addFixed(indentedItem(detected2, fontWidth * 3), lineHeight);
    return col;
  };

  // Left column, a flexible gap, then the right column aligned to the right edge
  auto root = std::make_unique<BoxLayout>(Dir::Horizontal, 0, HBORDER, VBORDER);
  root->addFixed(portColumn(myLeftPortLabel, myLeft1Port, myLeft1PortDetected,
                            myLeft2Port, myLeft2PortDetected), columnWidth);
  root->addSpace(_w - HBORDER * 2 - columnWidth * 2);
  root->addFixed(portColumn(myRightPortLabel, myRight1Port, myRight1PortDetected,
                            myRight2Port, myRight2PortDetected), columnWidth);

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::show(bool enableLeft, bool enableRight)
{
  myLeftPortLabel->setEnabled(enableLeft);
  myLeft1Port->setEnabled(enableLeft);
  myLeft2Port->setEnabled(enableLeft);
  myRightPortLabel->setEnabled(enableRight);
  myRight1Port->setEnabled(enableRight);
  myRight2Port->setEnabled(enableRight);

  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::loadControllerProperties(const Properties& props)
{
  if(myLeftPortLabel->isEnabled())
  {
    defineController(props, PropType::Controller_Left1, Controller::Jack::Left,
      myLeft1Port, myLeft1PortDetected);
    defineController(props, PropType::Controller_Left2, Controller::Jack::Left,
      myLeft2Port, myLeft2PortDetected, false);
  }

  if(myRightPortLabel->isEnabled())
  {
    defineController(props, PropType::Controller_Right1, Controller::Jack::Right,
      myRight1Port, myRight1PortDetected);
    defineController(props, PropType::Controller_Right2, Controller::Jack::Right,
      myRight2Port, myRight2PortDetected, false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::defineController(const Properties& props, PropType key,
  Controller::Jack jack, PopUpWidget* popupWidget, StaticTextWidget* labelWidget, bool first)
{
  ByteArray image;
  string_view controllerName = props.get(key);
  popupWidget->setSelected(controllerName, "AUTO");

  // Try to load the image for auto detection
  if(!instance().hasConsole())
  {
    const FSNode& node = FSNode(instance().launcher().selectedRom());
    string md5{myGameProperties.get(PropType::Cart_MD5)};
    if(node.exists() && !node.isDirectory())
      image = instance().openROM(node, md5);
  }
  string label;
  const Controller::Type type = Controller::getType(popupWidget->getSelectedTag().toString());

  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
    {
      const Controller& controller = (jack == Controller::Jack::Left
        ? instance().console().leftController()
        : instance().console().rightController());
      if(BSPF::startsWithIgnoreCase(controller.name(), "QT"))
      {
        const auto& qt = static_cast<const QuadTari&>(controller);
        label = (first
          ? qt.firstController().name()
          : qt.secondController().name())
        + " detected";
      }
      else
        label = "nothing detected";
    }
    else if(!image.empty())
      label = std::format("{} detected",
        ControllerDetector::detectName(image, type, jack, instance().settings(), true));
  }
  labelWidget->setLabel(label);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::loadConfig()
{
  loadControllerProperties(myGameProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::saveConfig()
{
  if(myLeftPortLabel->isEnabled())
  {
    string controller = myLeft1Port->getSelectedTag().toString();
    myGameProperties.set(PropType::Controller_Left1, controller);
    controller = myLeft2Port->getSelectedTag().toString();
    myGameProperties.set(PropType::Controller_Left2, controller);
  }
  else
  {
    myGameProperties.set(PropType::Controller_Left1, "");
    myGameProperties.set(PropType::Controller_Left2, "");
  }

  if(myRightPortLabel->isEnabled())
  {
    string controller = myRight1Port->getSelectedTag().toString();
    myGameProperties.set(PropType::Controller_Right1, controller);
    controller = myRight2Port->getSelectedTag().toString();
    myGameProperties.set(PropType::Controller_Right2, controller);
  }
  else
  {
    myGameProperties.set(PropType::Controller_Right1, "");
    myGameProperties.set(PropType::Controller_Right2, "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::setDefaults()
{
  // Load the default properties
  string_view md5 = myGameProperties.get(PropType::Cart_MD5);
  Properties defaultProperties;

  instance().propSet().getMD5(md5, defaultProperties, true);
  loadControllerProperties(defaultProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
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
