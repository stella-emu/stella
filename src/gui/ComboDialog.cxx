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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "Control.hxx"
#include "Dialog.hxx"
#include "EventHandler.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "StringList.hxx"
#include "Widget.hxx"

#include "ComboDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ComboDialog::ComboDialog(GuiObject* boss, const GUI::Font& font,
                         const StringMap& combolist)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 0, 0),
    myComboEvent(Event::NoType)
{
#define ADD_EVENT_POPUP(IDX, LABEL) \
  myEvents[IDX] = new PopUpWidget(this, font, xpos, ypos,  \
                    pwidth, lineHeight, combolist, LABEL); \
  wid.push_back(myEvents[IDX]); \
  ypos += lineHeight + 4;

  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  _w = 35 * fontWidth + 10;
  _h = 11 * (lineHeight + 4) + 10;
  xpos = ypos = 5;

  // Get maximum width of popupwidget
  int pwidth = 0;
  for(uInt32 i = 0; i < combolist.size(); ++i)
    pwidth = BSPF_max(font.getStringWidth(combolist[i].first), pwidth);

  // Label for dialog, indicating which combo is being changed
  myComboName = new StaticTextWidget(this, font, xpos, ypos, _w - xpos - 10,
                                     fontHeight, "", kTextAlignCenter);
  ypos += (lineHeight + 4) + 5;

  // Add event popup for 8 events
  xpos = 10;
  ADD_EVENT_POPUP(0, "Event 1: ");
  ADD_EVENT_POPUP(1, "Event 2: ");
  ADD_EVENT_POPUP(2, "Event 3: ");
  ADD_EVENT_POPUP(3, "Event 4: ");
  ADD_EVENT_POPUP(4, "Event 5: ");
  ADD_EVENT_POPUP(5, "Event 6: ");
  ADD_EVENT_POPUP(6, "Event 7: ");
  ADD_EVENT_POPUP(7, "Event 8: ");

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ComboDialog::~ComboDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::show(Event::Type event, const string& name)
{
  // Make sure the event is allowed
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    myComboEvent = event;
    myComboName->setLabel("Add events for " + name);
    parent().addDialog(this);
  }
  else
    myComboEvent = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::loadConfig()
{
  const StringList& events =
    instance().eventHandler().getComboListForEvent(myComboEvent);

  int size = BSPF_min(events.size(), 8u);
  for(int i = 0; i < size; ++i)
    myEvents[i]->setSelected("", events[i]);

  // Fill any remaining items to 'None'
  if(size < 8)
    for(int i = size; i < 8; ++i)
      myEvents[i]->setSelected("None", "-1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::saveConfig()
{
  StringList events;
  for(int i = 0; i < 8; ++i)
    events.push_back(myEvents[i]->getSelectedTag());

  instance().eventHandler().setComboListForEvent(myComboEvent, events);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::setDefaults()
{
  for(int i = 0; i < 8; ++i)
    myEvents[i]->setSelected("None", "-1");

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ComboDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
