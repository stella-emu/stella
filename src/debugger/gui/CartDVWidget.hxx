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

#ifndef CARTRIDGE_DV_WIDGET_HXX
#define CARTRIDGE_DV_WIDGET_HXX

class CartridgeDV;

#include "CartEnhancedWidget.hxx"

class CartridgeDVWidget : public CartridgeEnhancedWidget
{
  public:
    CartridgeDVWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeDV& cart);
    ~CartridgeDVWidget() override = default;

  protected:
    string manufacturer() override { return "Digivision"; }
    string description() override;
    string hotspotStr(int bank, int seg, bool prefix = false) override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeDVWidget() = delete;
    CartridgeDVWidget(const CartridgeDVWidget&) = delete;
    CartridgeDVWidget(CartridgeDVWidget&&) = delete;
    CartridgeDVWidget& operator=(const CartridgeDVWidget&) = delete;
    CartridgeDVWidget& operator=(CartridgeDVWidget&&) = delete;
};

#endif  // CARTRIDGE_DV_WIDGET_HXX
