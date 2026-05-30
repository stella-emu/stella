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

#ifndef DISASM_COLORS_DIALOG_HXX
#define DISASM_COLORS_DIALOG_HXX

#include "CartDebug.hxx"
#include "Dialog.hxx"
#include "Command.hxx"

/**
  Dialog for customising the syntax-highlighting colour map used in the
  disassembly view.  Each DisasmSegColor role can be assigned any of the
  16 DisasmPaletteArray hues, or set to DISASM_COLOR_TEXT (no highlight).
  Built-in named themes pre-populate the entire map at once.

  On OK the dialog writes the chosen indices to Settings ("dis.color"
  for N = 1..14) and sends kDisasmColorsChangedCmd to its boss so that
  RomListWidget reloads its cached colour map.
*/
class DisasmColorsDialog : public Dialog, public CommandSender
{
  public:
    DisasmColorsDialog(GuiObject* boss, const GUI::Font& font);
    ~DisasmColorsDialog() override = default;

    void loadConfig() override;
    void saveConfig() override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Following constructors and assignment operators not supported
    DisasmColorsDialog() = delete;
    DisasmColorsDialog(const DisasmColorsDialog&) = delete;
    DisasmColorsDialog(DisasmColorsDialog&&) = delete;
    DisasmColorsDialog& operator=(const DisasmColorsDialog&) = delete;
    DisasmColorsDialog& operator=(DisasmColorsDialog&&) = delete;
};

#endif  // DISASM_COLORS_DIALOG_HXX
