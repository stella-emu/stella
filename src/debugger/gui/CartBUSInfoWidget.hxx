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

#ifndef CARTRIDGE_BUS_INFO_WIDGET_HXX
#define CARTRIDGE_BUS_INFO_WIDGET_HXX

#include "CartBUS.hxx"
#include "CartDebugWidget.hxx"

class CartridgeBUSInfoWidget : public CartDebugWidget
{
  public:
    CartridgeBUSInfoWidget(GuiObject* boss, const GUI::Font& lfont,
                            const GUI::Font& nfont,
                            int x, int y, int w, int h,
                            CartridgeBUS& cart);
    ~CartridgeBUSInfoWidget() override = default;

  private:
    static constexpr string_view describeBUSVersion(
        CartridgeBUS::BUSSubtype subtype) {
      switch(subtype)
      {
        using enum CartridgeBUS::BUSSubtype;
        case BUS0:  return "BUS (v0)";
        case BUS1:  return "BUS (v1)";
        case BUS2:  return "BUS (v2)";
        case BUS3:  return "BUS (v3)";
        default:    throw std::runtime_error("unreachable");
      }
    }

  private:
    // Following constructors and assignment operators not supported
    CartridgeBUSInfoWidget() = delete;
    CartridgeBUSInfoWidget(const CartridgeBUSInfoWidget&) = delete;
    CartridgeBUSInfoWidget(CartridgeBUSInfoWidget&&) = delete;
    CartridgeBUSInfoWidget& operator=(const CartridgeBUSInfoWidget&) = delete;
    CartridgeBUSInfoWidget& operator=(CartridgeBUSInfoWidget&&) = delete;
};

#endif  // CARTRIDGE_BUS_INFO_WIDGET_HXX
