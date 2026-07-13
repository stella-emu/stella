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

  // A couple of characters more than the items strictly need, which looks better
  // overall -- so these state a width instead of taking the self-sizing ctor
  const int pwidth = PopUpWidget::calcWidth(font, ctrls) + Dialog::fontWidth() * 2;

  // Widgets are only created here (at placeholder position); layout() assigns
  // all geometry from the current font.  The two ports are laid out as two
  // side-by-side columns of identical structure.
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // An auto-detect label is filled in at load time and takes its width from the
  // column it sits in, so it starts out empty
  const auto detectedLabel = [&]() {
    return new StaticTextWidget(this, ifont, 0, 0, "");
  };

  myLeftPortLabel = new StaticTextWidget(this, font, 0, 0, "Left port");
  myLeft1Port = new PopUpWidget(this, font, 0, 0, pwidth, ctrls, "P1");
  wid.push_back(myLeft1Port);
  myLeft1PortDetected = detectedLabel();
  myLeft2Port = new PopUpWidget(this, font, 0, 0, pwidth, ctrls, "P3");
  wid.push_back(myLeft2Port);
  myLeft2PortDetected = detectedLabel();

  myRightPortLabel = new StaticTextWidget(this, font, 0, 0, "Right port");
  myRight1Port = new PopUpWidget(this, font, 0, 0, pwidth, ctrls, "P2");
  wid.push_back(myRight1Port);
  myRight1PortDetected = detectedLabel();
  myRight2Port = new PopUpWidget(this, font, 0, 0, pwidth, ctrls, "P4");
  wid.push_back(myRight2Port);
  myRight2PortDetected = detectedLabel();

  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("Quadtari");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // The four popups draw their own labels ("P1".."P4"), so one shared column
  // keeps their value boxes the same width and in line across both ports
  GUI::alignLabels({{myLeft1Port},  {myLeft2Port},
                    {myRight1Port}, {myRight2Port}});

  // Both ports have the same structure: a header label, then two popups each
  // followed by its (indented) auto-detect label, which fills the column
  const auto portColumn = [&](StaticTextWidget* label,
                              PopUpWidget* popup1, StaticTextWidget* detected1,
                              PopUpWidget* popup2, StaticTextWidget* detected2)
  {
    const auto detectedRow = [&](StaticTextWidget* detected) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      row->addSpace(fontWidth * 3);
      row->addStretch(stretchedItem(detected));
      return row;
    };

    auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, 0, 0);
    col->addAuto(anchoredItem(label));
    col->addSpace(VGAP * 2);
    col->addAuto(anchoredItem(popup1));
    col->addSpace(VGAP);
    col->addAuto(detectedRow(detected1));
    col->addSpace(VGAP);
    col->addAuto(anchoredItem(popup2));
    col->addSpace(VGAP);
    col->addAuto(detectedRow(detected2));
    return col;
  };

  // The two port columns side by side, each as wide as its popups need
  auto root = std::make_unique<BoxLayout>(Dir::Horizontal, 0, HBORDER, VBORDER);
  root->addAuto(portColumn(myLeftPortLabel, myLeft1Port, myLeft1PortDetected,
                           myLeft2Port, myLeft2PortDetected));
  root->addSpace(fontWidth * 3);
  root->addAuto(portColumn(myRightPortLabel, myRight1Port, myRight1PortDetected,
                           myRight2Port, myRight2PortDetected));

  // The dialog is as large as the two columns ask to be, and at least wide
  // enough for the button row below them
  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

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
