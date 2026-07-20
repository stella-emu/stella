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

#include "Console.hxx"
#include "PaletteHandler.hxx"
#include "TIA.hxx"
#include "Switches.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "Layout.hxx"
#include "EventHandler.hxx"
#include "StateManager.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "AudioSettings.hxx"
#include "Sound.hxx"
#include "TIASurface.hxx"
#include "CommandDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::CommandDialog(OSystem& osystem, DialogContainer& parent)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "Commands")
{
  // Only the button size is needed to create the widgets; layout() computes
  // _w/_h and positions everything from the current font
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() positions
  // them via a GridLayout.  myButtons keeps them in grid order (column-major).
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto ADD_CD_BUTTON = [&](string_view label, int cmd,
    Event::Type event1 = Event::NoType, Event::Type event2 = Event::NoType)
  {
    auto* b = new ButtonWidget(this, _font, 1, label, cmd);
    b->setToolTip(event1, event2);
    myButtons.push_back(b);
    wid.push_back(b);
    return b;
  };

  // Column 1
  ADD_CD_BUTTON(GUI::SELECT, kSelectCmd, Event::ConsoleSelect);
  ADD_CD_BUTTON("Reset", kResetCmd, Event::ConsoleReset);
  myColorButton = ADD_CD_BUTTON("", kColorCmd, Event::ConsoleColor, Event::ConsoleBlackWhite);
  myLeftDiffButton = ADD_CD_BUTTON("", kLeftDiffCmd,
    Event::ConsoleLeftDiffA, Event::ConsoleLeftDiffB);
  myRightDiffButton = ADD_CD_BUTTON("", kRightDiffCmd,
    Event::ConsoleRightDiffA, Event::ConsoleRightDiffB);

  // Column 2
  mySaveStateButton = ADD_CD_BUTTON("", kSaveStateCmd, Event::SaveState);
  myStateSlotButton = ADD_CD_BUTTON("Change Slot", kStateSlotCmd, Event::NextState);
  myLoadStateButton = ADD_CD_BUTTON("", kLoadStateCmd, Event::LoadState);
  ADD_CD_BUTTON("Snapshot", kSnapshotCmd, Event::TakeSnapshot);
  myTimeMachineButton = ADD_CD_BUTTON("", kTimeMachineCmd, Event::TimeMachineMode);
  ADD_CD_BUTTON("Exit Game", kExitCmd, Event::ExitMode);

  // Column 3
  myTVFormatButton = ADD_CD_BUTTON("", kFormatCmd, Event::FormatIncrease);
  myPaletteButton = ADD_CD_BUTTON("", kPaletteCmd, Event::PaletteIncrease);
  myPhosphorButton = ADD_CD_BUTTON("", kPhosphorCmd, Event::TogglePhosphor);
  mySoundButton = ADD_CD_BUTTON("", kSoundCmd, Event::SoundToggle);
  ADD_CD_BUTTON("Reload ROM", kReloadRomCmd, Event::ReloadConsole);

  addToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget(true);

  setHelpAnchor("CommandMenu");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::layout()
{
  using GUI::GridLayout;
  using GUI::anchoredItem;

  const int VBORDER = Dialog::vBorder();
  const int HBORDER = Dialog::hBorder();
  const int VGAP    = Dialog::vGap();
  const int HGAP    = Dialog::buttonGap();

  // These buttons are RE-LABELLED as the console state changes (loadConfig), so
  // they cannot size themselves: they must all be as wide as the widest label any
  // of them can ever show, or they would resize under the user
  const int buttonWidth = ButtonWidget::calcWidth(_font, "Time Machine On");
  for(auto* b: myButtons)
    b->setWidth(buttonWidth);

  // Three columns of buttons; columns 1 and 3 have five rows, column 2 has six
  static constexpr int COLS = 3, ROWS = 6;
  static constexpr std::array<int, COLS> colRows{5, 6, 5};

  auto grid = std::make_unique<GridLayout>(COLS, ROWS, HGAP, VGAP,
                                           HBORDER, VBORDER);
  for(int c = 0; c < COLS; ++c)
    grid->columnAuto(c);
  for(int r = 0; r < ROWS; ++r)
    grid->rowAuto(r);

  // myButtons is stored column-major; place each in its cell
  size_t idx = 0;
  for(int c = 0; c < COLS; ++c)
    for(int r = 0; r < colRows[c]; ++r)
      grid->place(c, r, anchoredItem(myButtons[idx++]));

  // The dialog is exactly as large as the button grid asks to be
  const Common::Size natural = grid->naturalSize();

  _w = static_cast<int>(natural.w);
  _h = _th + static_cast<int>(natural.h);

  // Position the grid in the dialog area below the title bar
  grid->doLayout(0, _th, _w, _h - _th);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::loadConfig()
{
  // Column 1
  myColorButton->setLabel(instance().console().switches().tvColor()
    ? "Color Mode" : "B/W Mode");
  myLeftDiffButton->setLabel(instance().console().switches().leftDifficultyA()
    ? GUI::LEFT_DIFF_A : GUI::LEFT_DIFF_B);
  myRightDiffButton->setLabel(instance().console().switches().rightDifficultyA()
    ? GUI::RIGHT_DIFF_A : GUI::RIGHT_DIFF_B);
  // Column 2
  updateSlot(instance().state().currentSlot());
  myTimeMachineButton->setLabel(instance().state().mode() == StateManager::Mode::TimeMachine
    ? "Time Machine On" : "No Time Machine");
  // Column 3
  updateTVFormat();
  updatePalette();
  myPhosphorButton->setLabel(instance().frameBuffer().tiaSurface().phosphorEnabled()
    ? "Phosphor On" : "Phosphor Off");
  mySoundButton->setLabel(instance().audioSettings().enabled()
    ? "Sound On" : "Sound Off");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  bool consoleCmd = false, stateCmd = false;
  Event::Type event = Event::NoType;

  switch(cmd)
  {
    // Column 1
    case kSelectCmd:
      event = Event::ConsoleSelect;
      consoleCmd = true;
      break;

    case kResetCmd:
      event = Event::ConsoleReset;
      consoleCmd = true;
      break;

    case kColorCmd:
      event = Event::ConsoleColorToggle;
      consoleCmd = true;
      break;

    case kLeftDiffCmd:
      event = Event::ConsoleLeftDiffToggle;
      consoleCmd = true;
      break;

    case kRightDiffCmd:
      event = Event::ConsoleRightDiffToggle;
      consoleCmd = true;
      break;

    // Column 2
    case kSaveStateCmd:
      event = Event::SaveState;
      consoleCmd = true;
      break;

    case kStateSlotCmd:
    {
      event = Event::NextState;
      stateCmd = true;
      updateSlot((instance().state().currentSlot() + 1) % 10);
      break;
    }
    case kLoadStateCmd:
      event = Event::LoadState;
      consoleCmd = true;
      break;

    case kSnapshotCmd:
      instance().eventHandler().leaveMenuMode();
      instance().eventHandler().handleEvent(Event::TakeSnapshot);
      break;

    case kTimeMachineCmd:
      instance().eventHandler().leaveMenuMode();
      instance().toggleTimeMachine();
      break;

    case kExitCmd:
      instance().eventHandler().leaveMenuMode();
      instance().eventHandler().handleEvent(Event::ExitGame);
      break;

    // Column 3
    case kFormatCmd:
      instance().console().selectFormat();
      updateTVFormat();
      break;

    case kPaletteCmd:
      instance().frameBuffer().tiaSurface().paletteHandler().cyclePalette();
      updatePalette();
      break;

    case kPhosphorCmd:
      instance().eventHandler().leaveMenuMode();
      instance().console().togglePhosphor();
      break;

    case kSoundCmd:
    {
      instance().eventHandler().leaveMenuMode();
      instance().sound().toggleMute();
      break;
    }
    case kReloadRomCmd:
      instance().eventHandler().leaveMenuMode();
      instance().eventHandler().handleEvent(Event::ReloadConsole);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }

  // Console commands should be performed right away, after leaving the menu
  // State commands require you to exit the menu manually
  if(consoleCmd)
  {
    instance().eventHandler().leaveMenuMode();
    instance().eventHandler().handleEvent(event);
    instance().console().switches().update();
    instance().console().tia().update();
    instance().eventHandler().handleEvent(event, false);
  }
  else if(stateCmd)
    instance().eventHandler().handleEvent(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::processCancel()
{
  instance().eventHandler().leaveMenuMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::updateSlot(int slot)
{
  mySaveStateButton->setLabel(std::format("Save State {}", slot));
  myLoadStateButton->setLabel(std::format("Load State {}", slot));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::updateTVFormat()
{
  myTVFormatButton->setLabel(instance().console().getFormatString() + " Mode");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::updatePalette()
{
  const string& palette = instance().settings().getString("palette");
  string label;
  if(BSPF::equalsIgnoreCase(palette, PaletteHandler::SETTING_STANDARD))
    label = "Stella Palette";
  else if(BSPF::equalsIgnoreCase(palette, PaletteHandler::SETTING_Z26))
    label = "Z26 Palette";
  else if(BSPF::equalsIgnoreCase(palette, PaletteHandler::SETTING_USER))
    label = "User Palette";
  else
    label = "Custom Palette";
  myPaletteButton->setLabel(label);
}
