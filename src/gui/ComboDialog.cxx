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
#include "Control.hxx"
#include "Dialog.hxx"
#include "EventHandler.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "Font.hxx"
#include "ComboDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ComboDialog::ComboDialog(GuiObject* boss, const GUI::Font& font,
                         const VariantList& combolist)
  : Dialog(boss->instance(), boss->parent(), font, "Add...")
{
  WidgetArray wid;

  // Widgets are only created here (at placeholder position); layout() assigns
  // all geometry from the current font.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Add event popup for 8 events; each sizes itself to the events it can show
  const auto ADD_EVENT_POPUP = [&](int idx, string_view label)
  {
    myEventLabels[idx] = new StaticTextWidget(this, font, label);
    myEvents[idx] = new PopUpWidget(this, font, combolist);
    wid.push_back(myEvents[idx]);
  };
  ADD_EVENT_POPUP(0, "Event 1");
  ADD_EVENT_POPUP(1, "Event 2");
  ADD_EVENT_POPUP(2, "Event 3");
  ADD_EVENT_POPUP(3, "Event 4");
  ADD_EVENT_POPUP(4, "Event 5");
  ADD_EVENT_POPUP(5, "Event 6");
  ADD_EVENT_POPUP(6, "Event 7");
  ADD_EVENT_POPUP(7, "Event 8");

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);

  setHelpAnchor("Combo");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // One shared label column lines the popups' value boxes up down the dialog
  GUI::alignLabels({{myEventLabels[0]}, {myEventLabels[1]}, {myEventLabels[2]},
                    {myEventLabels[3]}, {myEventLabels[4]}, {myEventLabels[5]},
                    {myEventLabels[6]}, {myEventLabels[7]}});

  // Vertical stack of the eight labeled event popups; the button group sits
  // below, positioned by layoutButtonGroup().
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, HBORDER, VBORDER);
  for(size_t i = 0; i < myEvents.size(); ++i)
    root->addAuto(labeledRow(myEventLabels[i], myEvents[i]));

  // The dialog is as large as its content asks to be, and at least wide enough
  // for the button row below it (which the content knows nothing about)
  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::show(Event::Type event, string_view name)
{
  // Make sure the event is allowed
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    myComboEvent = event;
    setTitle("Add events for " + string{name});
    open();
  }
  else
    myComboEvent = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::loadConfig()
{
  StringList events = instance().eventHandler().getComboListForEvent(myComboEvent);

  const auto size = std::min(events.size(), 8UZ);
  for(auto i = 0UZ; i < size; ++i)
    myEvents[i]->setSelected("", events[i]);

  // Fill any remaining items to 'None'
  if(size < 8)
    for(auto i = size; i < 8; ++i)
      myEvents[i]->setSelected("None", "-1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::saveConfig()
{
  StringList events;
  events.reserve(myEvents.size());
  for(const auto* e: myEvents)
    events.push_back(e->getSelectedTag().toString());

  instance().eventHandler().setComboListForEvent(myComboEvent, events);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::setDefaults()
{
  for(auto* e: myEvents)
    e->setSelected("None", "-1");

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
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
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
