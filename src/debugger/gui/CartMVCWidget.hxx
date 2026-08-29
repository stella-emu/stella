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

#ifndef CARTRIDGE_MVC_WIDGET_HXX
#define CARTRIDGE_MVC_WIDGET_HXX

class CartridgeMVC;

#include "CartDebugWidget.hxx"

class CartridgeMVCWidget : public CartDebugWidget
{
  public:
    CartridgeMVCWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont, CartridgeMVC& cart);
    ~CartridgeMVCWidget() override = default;

    // There are no banks to report: the 2600 always sees the same 1K window,
    // whose contents the streamed movie data replaces as it plays
    string bankState() override { return "streaming (non-bankswitched)"; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeMVCWidget() = delete;
    CartridgeMVCWidget(const CartridgeMVCWidget&) = delete;
    CartridgeMVCWidget(CartridgeMVCWidget&&) = delete;
    CartridgeMVCWidget& operator=(const CartridgeMVCWidget&) = delete;
    CartridgeMVCWidget& operator=(CartridgeMVCWidget&&) = delete;
};

#endif  // CARTRIDGE_MVC_WIDGET_HXX
