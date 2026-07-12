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
#include "EventHandler.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "StringListWidget.hxx"
#include "Variant.hxx"
#include "Layout.hxx"
#include "JoystickDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoystickDialog::JoystickDialog(GuiObject* boss, const GUI::Font& font,
                               int max_w, int max_h)
  : Dialog(boss->instance(), boss->parent(), font, "Controller database", 0, 0, max_w, max_h)
{
  WidgetArray wid;

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Joystick list
  myJoyList = new StringListWidget(this, font, 0, 0, 1, 1);
  myJoyList->setEditable(false);
  wid.push_back(myJoyList);

  // Joystick ID
  myIDLabel = new StaticTextWidget(this, font, 0, 0, "Controller ID");
  myJoyText = new EditTextWidget(this, font, 0, 0,
      EditTextWidget::calcWidth(font, "Unplugged"), "");
  myJoyText->setEditable(false);

  // Port
  VariantList ports;
  VarList::push_back(ports, "Auto",  static_cast<Int32>(PhysicalJoystick::Port::AUTO));
  VarList::push_back(ports, "Left",  static_cast<Int32>(PhysicalJoystick::Port::LEFT));
  VarList::push_back(ports, "Right", static_cast<Int32>(PhysicalJoystick::Port::RIGHT));

  myJoyPort = new PopUpWidget(this, font, 0, 0, ports, "Port", 0, kPortCmd);
  myJoyPort->setToolTip("Define default mapping port.");
  wid.push_back(myJoyPort);

  // Buttons at bottom
  myCloseBtn = new ButtonWidget(this, font, 0, 0, "Close", GuiObject::kCloseCmd);
  addOKWidget(myCloseBtn);  addCancelWidget(myCloseBtn);

  myRemoveBtn = new ButtonWidget(this, font, 0, 0, "Remove", kRemoveCmd);
  myRemoveBtn->clearFlags(Widget::FLAG_ENABLED);

  // Now we can finally add the widgets to the focus list
  wid.push_back(myRemoveBtn);
  wid.push_back(myCloseBtn);
  addToFocusList(wid);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder();

  // Remove and Close share one width, the wider of the two
  GUI::alignButtons({myRemoveBtn, myCloseBtn});

  // The popup draws its own label, so give it a label column of its own
  GUI::alignLabels({{myJoyPort}});

  // The joystick list fills the area above the bottom control/button row.  This
  // dialog takes all the space it is given, so its size is not derived from the
  // content.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addStretch(widgetItem(myJoyList));
  root->addSpace(VBORDER);
  root->addSpace(buttonHeight);  // reserve the button row
  root->doLayout(0, _th, _w, _h - _th);

  const int by = _h - VBORDER - buttonHeight;  // button row top

  // Controller ID label + (non-editable) value + port popup on the left; each
  // keeps its natural size and is centered in the (taller) button row, so all
  // three texts line up with the buttons beside them
  auto idRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  idRow->addAuto(anchoredItem(myIDLabel));
  idRow->addSpace(fontWidth);
  idRow->addAuto(anchoredItem(myJoyText));
  idRow->addSpace(fontWidth * 2);
  idRow->addAuto(anchoredItem(myJoyPort));
  idRow->doLayout(HBORDER, by, _w - HBORDER * 2, buttonHeight);

  // Remove / Close buttons, right-aligned on the button row
  auto buttons = std::make_unique<BoxLayout>(Dir::Horizontal);
  buttons->addStretchSpace();
  buttons->addAuto(anchoredItem(myRemoveBtn));
  buttons->addSpace(fontWidth);
  buttons->addAuto(anchoredItem(myCloseBtn));
  buttons->doLayout(HBORDER, by, _w - HBORDER * 2, buttonHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickDialog::loadConfig()
{
  myJoyIDs.clear();

  StringList sticks;
  for(const auto& entry: instance().eventHandler().physicalJoystickList())
  {
    sticks.push_back(entry.name);
    myJoyIDs.push_back(entry.ID);
    myJoyPorts.push_back(static_cast<int>(entry.port));
  }
  myJoyList->setList(sticks);
  myJoyList->setSelected(0);
  if(sticks.empty())
  {
    myRemoveBtn->setEnabled(false);
    myJoyText->setText("");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickDialog::handleEvent(Event::Type event)
{
  if(event == Event::Type::UIReload)
    loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      close();
      break;

    case kPortCmd:
      myJoyPorts[myJoyList->getSelected()] = myJoyPort->getSelected();
      instance().eventHandler().setPhysicalJoystickPortInDatabase(
          myJoyList->getSelectedString(),
          static_cast<PhysicalJoystick::Port>(myJoyPort->getSelected()));
      break;

    case kRemoveCmd:
      instance().eventHandler().removePhysicalJoystickFromDatabase(
          myJoyList->getSelectedString());
      loadConfig();
      break;

    case ListWidget::kSelectionChangedCmd:
    {
      const bool isPlugged = myJoyIDs[data] >= 0;
      if(isPlugged)
      {
        myJoyText->setText(std::format("C{}", myJoyIDs[data]));
        myJoyPort->setSelected(myJoyPorts[data]);
      }
      else
      {
        myJoyText->setText("Unplugged");
        myJoyPort->setText("");
      }
      myJoyPort->setEnabled(isPlugged);
      myRemoveBtn->setEnabled(!isPlugged);
      break;
    }
    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}
