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
#include "Settings.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferConstants.hxx"
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "OptionsDialog.hxx"
#include "StellaSettingsDialog.hxx"
#include "CommandDialog.hxx"
#include "HighScoresDialog.hxx"
#include "MessageDialog.hxx"
#include "PlusRomsSetupDialog.hxx"
#include "OverlayMenu.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OverlayMenu::OverlayMenu(OSystem& osystem)
  : DialogContainer(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OverlayMenu::~OverlayMenu() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayMenu::setDialog(Dialog* dialog)
{
  myTransientDialog.reset(dialog);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog* OverlayMenu::baseDialog()
{
  // The active built-in menu is determined by the current state; transient
  // dialogs (set via setDialog) are returned for the generic OVERLAYMENU state
  switch(myOSystem.eventHandler().state())
  {
    case EventHandlerState::OPTIONSMENU:
      return &cached(myOSystem.settings().getBool("basic_settings")
                     ? Cached::StellaSettings : Cached::Options);

    case EventHandlerState::CMDMENU:
      return &cached(Cached::Command);

    case EventHandlerState::HIGHSCORESMENU:
      return &cached(Cached::HighScores);

    case EventHandlerState::MESSAGEMENU:
      return &cached(Cached::Message);

    case EventHandlerState::PLUSROMSMENU:
      return &cached(Cached::PlusRoms);

    default:
      return myTransientDialog.get();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog& OverlayMenu::cached(Cached id)
{
  auto& slot = myCached[static_cast<size_t>(id)];
  if(slot == nullptr)
    slot.reset(createDialog(id));
  return *slot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog* OverlayMenu::createDialog(Cached id)
{
  switch(id)
  {
    case Cached::Options:
      return new OptionsDialog(myOSystem, *this, nullptr,
        FBMinimum::Width, FBMinimum::Height, Dialog::AppMode::emulator);

    case Cached::StellaSettings:
      return new StellaSettingsDialog(myOSystem, *this,
        1280, 720, Dialog::AppMode::emulator);

    case Cached::Command:
      return new CommandDialog(myOSystem, *this);

    case Cached::HighScores:
      return new HighScoresDialog(myOSystem, *this,
        FBMinimum::Width, FBMinimum::Height, Dialog::AppMode::emulator);

    case Cached::Message:
      return new MessageDialog(myOSystem, *this,
        myOSystem.frameBuffer().font(), FBMinimum::Width, FBMinimum::Height);

    case Cached::PlusRoms:
      return new PlusRomsSetupDialog(myOSystem, *this,
        myOSystem.frameBuffer().font());

    default:
      return nullptr;
  }
}
