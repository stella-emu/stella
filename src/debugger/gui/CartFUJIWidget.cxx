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

#include "EditTextWidget.hxx"
#include "Widget.hxx"
#include "CartFUJI.hxx"
#include "CartFUJIWidget.hxx"

namespace {
  string bootStateText(uInt8 state, uInt8 pct, uInt8 err)
  {
    switch(state)
    {
      case FN_BOOT_IDLE:   return "idle";
      case FN_BOOT_XFER:   return std::format("receiving image ({}%)", pct);
      case FN_BOOT_READY:  return "image staged, ready to boot";
      case FN_BOOT_FAILED:
        switch(err)
        {
          case FN_BOOT_ERR_TOOBIG:    return "failed: image too large";
          case FN_BOOT_ERR_TRUNCATED: return "failed: transfer truncated";
          case FN_BOOT_ERR_NOMAP:     return "failed: unsupported layout";
          case FN_BOOT_ERR_STOREBUSY: return "failed: store busy";
          default:                    return "failed";
        }
      default: return std::format("unknown ({})", state);
    }
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartFUJIWidget::CartFUJIWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeFUJI& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
  constexpr string_view info =
    "FujiNet cartridge\n"
    "The low 2K at $1000 is a banked client image; the high 2K is fixed and "
    "is always the LAST 2K of the file, so a client is (N+1) * 2048 bytes.\n"
    "$1800 - $1AFF are six text planes the cartridge composes, $1B00 - $1CFF "
    "the current 512 byte slice of a reply, and $1F00 the status page.\n"
    "$1D00 - $1EFF are write-only and are never driven: reads there are open "
    "bus.  A store to $1D00+n arms register n and a store to $1DFF commits it, "
    "because with no R/W line on the connector a read of a write port cannot "
    "be told from a write to one.\n"
    "Requests go to a fujinet-pc instance over TCP, on the mailbox's own "
    "thread; the console polls $1F00 for its sequence to come back.\n";

  createBaseInformation(cart.getImage().size(), "FujiNet", info);

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myLinkLbl = new LabelWidget(boss, _font, "Link");
  myLink = new EditTextWidget(boss, _nfont, 1);
  myLink->setEditable(false, true);

  myBootLbl = new LabelWidget(boss, _font, "Boot");
  myBoot = new EditTextWidget(boss, _nfont, 1);
  myBoot->setEditable(false, true);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myLabelColumn.emplace_back(myLinkLbl);
  myLabelColumn.emplace_back(myBootLbl);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartFUJIWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::labeledRow;

  // Both fields are filled rather than sized: their text arrives at
  // loadConfig() time, so neither can report a useful natural width
  col.addAuto(labeledRow(myLinkLbl, myLink, 0, 0, true));
  col.addAuto(labeledRow(myBootLbl, myBoot, 0, 0, true));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartFUJIWidget::loadConfig()
{
  myLink->setText(myCart.linkStatus());

  // Read straight out of the served window rather than from any shadow: this
  // is what the console itself would see on its next fetch
  const auto& mem = myCart.myMem;
  myBoot->setText(bootStateText(mem.win[FN_R_BOOT_STATE - FN_WINDOW_BASE],
                                mem.win[FN_R_BOOT_PCT - FN_WINDOW_BASE],
                                mem.win[FN_R_BOOT_ERR - FN_WINDOW_BASE]));

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartFUJIWidget::bankState()
{
  return std::format("low 2K = bank {} of {}, high 2K fixed",
                     myCart.getBank(0x1000), myCart.romBankCount());
}
