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
#include "RomListWidget.hxx"
#include "DisasmColorsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DisasmColorsDialog::DisasmColorsDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent()),
    CommandSender(boss)
{
  // UI to be implemented
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DisasmColorsDialog::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DisasmColorsDialog::saveConfig()
{
  // Build comma-separated list of 14 palette indices (roles 1..14).
  // Each value is 0..15 for a DisasmPaletteArray slot, or
  // CartDebug::DISASM_COLOR_TEXT (255) for "use UI text colour".
  // Placeholder: saves the Default theme until the UI is implemented.
  string result;
  for(int i = 1; i <= CartDebug::NUM_DISASM_ROLES; ++i)
  {
    if(i > 1) result += ',';
    result += std::to_string(CartDebug::ourDisasmThemes[0].map[i]);
  }
  instance().settings().setValue("dis.color", result);
  sendCommand(RomListWidget::kDisasmColorsChangedCmd, 0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DisasmColorsDialog::handleCommand(CommandSender* sender, int cmd,
                                       int data, int id)
{
  if(cmd == GuiObject::kOKCmd)
  {
    saveConfig();
    close();
  }
  else
    Dialog::handleCommand(sender, cmd, data, id);
}
