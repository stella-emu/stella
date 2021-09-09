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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "EventHandler.hxx"

#include "PlusRomsSetupDialog.hxx"

static const int MAX_NICK_LEN = 16;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlusRomsSetupDialog::PlusRomsSetupDialog(OSystem& osystem, DialogContainer& parent,
                                         const GUI::Font& font)
  : InputTextDialog(osystem, parent, font, "Nickname", "PlusROMs setup", MAX_NICK_LEN)
{
  EditableWidget::TextFilter filter = [](char c) {
    return isalnum(c) || (c == '_');
  };

  setTextFilter(filter);
  setToolTip("Enter your PlusCart High Score Club nickname here.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusRomsSetupDialog::loadConfig()
{
  setText(instance().settings().getString("plusroms.nick"), 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusRomsSetupDialog::saveConfig()
{
  instance().settings().setValue("plusroms.nick", getResult(0));
  if(instance().settings().getString("plusroms.id") == EmptyString)
    instance().settings().setValue("plusroms.id", "12345678901234567890123456789012"); // TODO: generate in PlusROM class
  // Note: The user can cancel, so the existance of an ID must be checked (and generated if not existing) when transmitting scores
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusRomsSetupDialog::handleCommand(CommandSender* sender, int cmd,
                                        int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
    case EditableWidget::kAcceptCmd:
      saveConfig();
      instance().eventHandler().leaveMenuMode();
      break;

    case kCloseCmd:
      instance().eventHandler().leaveMenuMode();
      break;

    case EditableWidget::kCancelCmd:
      break;

    default:
      InputTextDialog::handleCommand(sender, cmd, data, id);
      break;
  }
}