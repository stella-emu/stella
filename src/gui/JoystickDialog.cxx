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
  : Dialog(boss->instance(), boss->parent(), font, "Controller database", max_w, max_h)
{
  WidgetArray wid;

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Joystick list
  myJoyList = new StringListWidget(this, font);
  myJoyList->setEditable(false);
  wid.push_back(myJoyList);

  // Joystick ID
  myIDLabel = new StaticTextWidget(this, font, "Controller ID");
  myJoyText = new EditTextWidget(this, font,
      static_cast<int>(string_view("Unplugged").size()), "");
  myJoyText->setEditable(false);

  // Port
  VariantList ports;
  VarList::push_back(ports, "Auto",  static_cast<Int32>(PhysicalJoystick::Port::AUTO));
  VarList::push_back(ports, "Left",  static_cast<Int32>(PhysicalJoystick::Port::LEFT));
  VarList::push_back(ports, "Right", static_cast<Int32>(PhysicalJoystick::Port::RIGHT));

  myJoyPortLabel = new StaticTextWidget(this, font, "Port");
  myJoyPort = new PopUpWidget(this, font, ports, kPortCmd);
  myJoyPort->setToolTip("Define default mapping port.");
  wid.push_back(myJoyPort);

  // Buttons at bottom
  myCloseBtn = new ButtonWidget(this, font, "Close", GuiObject::kCloseCmd);
  addOKWidget(myCloseBtn);  addCancelWidget(myCloseBtn);

  myRemoveBtn = new ButtonWidget(this, font, "Remove", kRemoveCmd);
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
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder();

  // Remove and Close share one width, the wider of the two
  GUI::alignButtons({myRemoveBtn, myCloseBtn});

  GUI::alignLabels({{myJoyPortLabel}});

  // The joystick list fills the area above the bottom control/button row.  This
  // dialog takes all the space it is given, so its size is not derived from the
  // content.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addStretch(widgetItem(myJoyList));
  root->addSpace(VBORDER);
  root->addSpace(buttonHeight);  // reserve the button row
  root->doLayout(0, _th, _w, _h - _th);

  // The button band is entirely this dialog's own: the controller ID readout on
  // the left, Remove / Close (which keep their natural widths, not the standard
  // group's) on the right.  Everything keeps its own size and is centered in the
  // (taller) band, so all the texts line up
  auto band = std::make_unique<BoxLayout>(Dir::Horizontal);
  band->addAuto(anchoredItem(myIDLabel));
  band->addSpace(fontWidth);
  band->addAuto(anchoredItem(myJoyText));
  band->addSpace(fontWidth * 2);
  band->addAuto(labeledRow(myJoyPortLabel, myJoyPort));
  band->addStretchSpace();
  band->addAuto(anchoredItem(myRemoveBtn));
  band->addSpace(fontWidth);
  band->addAuto(anchoredItem(myCloseBtn));
  layoutButtonBand(std::move(band));
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
      myJoyPortLabel->setEnabled(isPlugged);
      myJoyPort->setEnabled(isPlugged);
      myRemoveBtn->setEnabled(!isPlugged);
      break;
    }
    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}
