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
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "EmulationDialog.hxx"
#include "VideoAudioDialog.hxx"
#include "InputDialog.hxx"
#include "UIDialog.hxx"
#include "SnapshotDialog.hxx"
#include "RomAuditDialog.hxx"
#include "GameInfoDialog.hxx"
#include "LoggerDialog.hxx"
#include "DeveloperDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"
#include "Launcher.hxx"
#include "Settings.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatCodeDialog.hxx"
#endif

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem& osystem, DialogContainer& parent,
                             GuiObject* boss, AppMode mode)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "Options"),
    myBoss{boss},
    myMode{mode}
{
  // Only sizes needed to create the buttons; layout() computes _w/_h and all
  // geometry from the current font, so the dialog reflows on font change
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() positions
  // them via a GridLayout.  myButtons keeps them in grid order for that pass.
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto ADD_OD_BUTTON = [&](string_view label, GuiCmd::Code cmd, string_view toolTip = {})
  {
    auto* bw = new ButtonWidget(this, _font, label, cmd);
    bw->setToolTip(toolTip);
    myButtons.push_back(bw);
    wid.push_back(bw);
    return bw;
  };

  // First column
  ADD_OD_BUTTON("Video & Audio" + ELLIPSIS, Cmd::Video,
    "Change display modes, colors, TV effects,\n"
    "volume, stereo mode" + ELLIPSIS);
  ADD_OD_BUTTON("Emulation" + ELLIPSIS, Cmd::Emulation,
    "Change emulation speed, save state settings" + ELLIPSIS);
  ADD_OD_BUTTON("Input" + ELLIPSIS, Cmd::Input,
    "Map and configure keyboard, mouse and controllers.");
  ADD_OD_BUTTON("User Interface" + ELLIPSIS, Cmd::UserInterface,
    "Change themes, fonts, launcher layout\n"
    "and paths for ROMs and images.");
  ADD_OD_BUTTON("Snapshots" + ELLIPSIS, Cmd::Snapshots,
    "Define snapshot save location, format" + ELLIPSIS);
  ADD_OD_BUTTON("Developer" + ELLIPSIS, Cmd::Developer,
    "Change options which support programming Atari 2600 games.");

  // Second column
  myGameInfoButton = ADD_OD_BUTTON("Game Properties" + ELLIPSIS, Cmd::GameInfo,
    "Change game-specific info and options (TV format,\n"
    "console switches, controllers" + ELLIPSIS + ")");
  myCheatCodeButton = ADD_OD_BUTTON("Cheat Codes" + ELLIPSIS, Cmd::Cheats,
    "Use and manage cheat codes.");
#ifndef CHEATCODE_SUPPORT
  myCheatCodeButton->clearFlags(Widget::Flag::Enabled);
#endif
  myRomAuditButton = ADD_OD_BUTTON("Audit ROMs" + ELLIPSIS, Cmd::RomAudit,
    "Rename your ROMs according to Stella's internal database.");
  ADD_OD_BUTTON("System Logs" + ELLIPSIS, Cmd::Logger,
    "Configure, view and save Stella's system log.");
  ADD_OD_BUTTON("Help" + ELLIPSIS, Cmd::Help,
    "Display Stella's essential keyboard commands.");
  ADD_OD_BUTTON("About" + ELLIPSIS, Cmd::About,
    "Display info about the installed Stella version.");

  // Centered Close button on its own row spanning both columns.  The padding is
  // its own: a button centers its text, so a roomier label simply makes a roomier
  // button, and it still sizes itself
  auto* closeButton = new ButtonWidget(this, _font, "   Close   ", Cmd::Exit);
  myButtons.push_back(closeButton);
  wid.push_back(closeButton);
  addCancelWidget(closeButton);

  addToFocusList(wid);

  // Certain buttons are disabled depending on mode
  if(myMode == AppMode::launcher)
    myCheatCodeButton->clearFlags(Widget::Flag::Enabled);
  else
    myRomAuditButton->clearFlags(Widget::Flag::Enabled);

  setHelpAnchor("Options");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::~OptionsDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::layout()
{
  using GUI::GridLayout;
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VBORDER = Dialog::vBorder();
  const int HBORDER = Dialog::hBorder();
  const int VGAP    = Dialog::vGap();
  const int HGAP    = Dialog::buttonGap();

  // Every option button is as wide as the widest of them, so the two columns come
  // out even.  The Close button below is NOT one of them -- it keeps the width its
  // own (deliberately roomy) label needs -- hence the subrange
  GUI::alignButtons(std::span{myButtons}.first(myButtons.size() - 1));

  // Two columns of equal buttons over six rows, plus a seventh row holding the
  // Close button centered across both columns
  static constexpr int COLS = 2, ROWS = 7, PER_COL = 6;
  auto grid = std::make_unique<GridLayout>(COLS, ROWS, HGAP, VGAP,
                                           HBORDER, VBORDER);
  grid->columnAuto(0).columnAuto(1);
  for(int r = 0; r < ROWS; ++r)
    grid->rowAuto(r);

  for(int r = 0; r < PER_COL; ++r)
  {
    grid->place(0, r, anchoredItem(myButtons[r]));
    grid->place(1, r, anchoredItem(myButtons[PER_COL + r]));
  }
  auto closeRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  closeRow->addStretchSpace();
  closeRow->addAuto(anchoredItem(myButtons.back()));
  closeRow->addStretchSpace();
  grid->place(0, PER_COL, std::move(closeRow), COLS);

  // The dialog is exactly as large as the button grid asks to be
  const Common::Size natural = grid->naturalSize();

  _w = static_cast<int>(natural.w);
  _h = _th + static_cast<int>(natural.h);

  // Position the grid in the dialog area below the title bar
  grid->doLayout(0, _th, _w, _h - _th);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::loadConfig()
{
  // Determine whether we should show the 'Game Information' button
  // We always show it in emulation mode, or if a valid ROM is selected
  // in launcher mode
  switch(instance().eventHandler().state())
  {
    case EventHandlerState::EMULATION:
      myGameInfoButton->setFlags(Widget::Flag::Enabled);
      break;
    case EventHandlerState::LAUNCHER:
      if(!instance().launcher().selectedRomMD5().empty())
        myGameInfoButton->setFlags(Widget::Flag::Enabled);
      else
        myGameInfoButton->clearFlags(Widget::Flag::Enabled);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                  int data, int id)
{
  switch(cmd)
  {
    case Cmd::Video:
    {
      uInt32 w = 0, h = 0;

      getDynamicBounds(w, h);
      myDialog = std::make_unique<VideoAudioDialog>(instance(), parent(), _font);
      myDialog->open();
      break;
    }
    case Cmd::Emulation:
      myDialog = std::make_unique<EmulationDialog>(instance(), parent(), _font);
      myDialog->open();
      break;
    case Cmd::Input:
      myDialog = std::make_unique<InputDialog>(instance(), parent(), _font);
      myDialog->open();
      break;

    case Cmd::UserInterface:
      myDialog = std::make_unique<UIDialog>(instance(), parent(), _font, myBoss);
      myDialog->open();
      break;

    case Cmd::Snapshots:
      myDialog = std::make_unique<SnapshotDialog>(instance(), parent(), _font);
      myDialog->open();
      break;

    case Cmd::Developer:
      myDialog = std::make_unique<DeveloperDialog>(instance(), parent(), _font);
      myDialog->open();
      break;

    case Cmd::GameInfo:
      myDialog = std::make_unique<GameInfoDialog>(instance(), parent(), _font, this);
      myDialog->open();
      break;

#ifdef CHEATCODE_SUPPORT
    case Cmd::Cheats:
      myDialog = std::make_unique<CheatCodeDialog>(instance(), parent(), _font);
      myDialog->open();
      break;
#endif

    case Cmd::RomAudit:
      myDialog = std::make_unique<RomAuditDialog>(instance(), parent(), _font);
      myDialog->open();
      break;
    case Cmd::Logger:
    {
      uInt32 w = 0, h = 0;
      const bool uselargefont = getDynamicBounds(w, h);

      myDialog = std::make_unique<LoggerDialog>(instance(), parent(), _font, w, h, uselargefont);
      myDialog->open();
      break;
    }

    case Cmd::Help:
      myDialog = std::make_unique<HelpDialog>(instance(), parent(), _font);
      myDialog->open();
      break;

    case Cmd::About:
      myDialog = std::make_unique<AboutDialog>(instance(), parent(), _font);
      myDialog->open();
      break;

    case Cmd::Exit:
      if(myMode != AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
