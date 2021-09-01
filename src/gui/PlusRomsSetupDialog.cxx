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

static const int MIN_NICK_LEN = 2;
static const int MAX_NICK_LEN = 16;
static const char* MIN_NICK_LEN_STR = "Two";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlusRomsSetupDialog::PlusRomsSetupDialog(OSystem& osystem, DialogContainer& parent,
                                         const GUI::Font& font, const StringList& labels, const string& title)
  : InputTextDialog(osystem, parent, font, "Nickname", "PlusROMs setup", MAX_NICK_LEN)
{
  setText(instance().settings().getString("plusroms.nick"), 0);
  EditableWidget::TextFilter filter = [](char c) {
    return isalnum(c) || (c == '_');
  };
  setTextFilter(filter, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusRomsSetupDialog::handleCommand(CommandSender* sender, int cmd,
                                        int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
    case EditableWidget::kAcceptCmd:
    {
      const string nick = getResult(0);

      if(nick.length() >= MIN_NICK_LEN)
      {
        instance().settings().setValue("plusroms.nick", nick);
        instance().eventHandler().leaveMenuMode();
      }
      else
        setMessage("Two characters minimum");
      break;
    }
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