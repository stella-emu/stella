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

#ifndef CARTRIDGE_E7_WIDGET_HXX
#define CARTRIDGE_E7_WIDGET_HXX

class CartridgeE7;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeE7Widget : public CartDebugWidget
{
  public:
    CartridgeE7Widget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      CartridgeE7& cart);
    ~CartridgeE7Widget() override = default;

    void saveOldState() override;
    void loadConfig() override;
    string bankState() override;

    // Start of functions for Cartridge RAM tab
    uInt32 internalRamSize() override;
    uInt32 internalRamRPort(int start) override;
    string internalRamDescription() override;
    const ByteArray& internalRamOld(int start, int count) override;
    const ByteArray& internalRamCurrent(int start, int count) override;
    void internalRamSetValue(int addr, uInt8 value) override;
    uInt8 internalRamGetValue(int addr) override;
    // End of functions for Cartridge RAM tab

  protected:
    CartridgeE7& myCart;

    LabelWidget *myLower2KLbl{nullptr}, *myUpper256BLbl{nullptr};
    PopUpWidget *myLower2K{nullptr}, *myUpper256B{nullptr};

    struct CartState
    {
      ByteArray internalram;
      uInt16 lowerBank{0};
      uInt16 upperBank{0};
    };
    CartState myOldState;

    struct Cmd {
      static constexpr GuiCmd::Code
        LowerChanged = GuiCmd::of("CartridgeE7Widget.LowerChanged"),
        UpperChanged = GuiCmd::of("CartridgeE7Widget.UpperChanged");
    };

  protected:
    void initialize(GuiObject* boss, const CartridgeE7& cart, string_view info);
    void layoutContent(GUI::BoxLayout& col) const override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    static string_view getSpotLower(int idx, int bankcount);
    static string_view getSpotUpper(int idx);

  private:
    // Following constructors and assignment operators not supported
    CartridgeE7Widget() = delete;
    CartridgeE7Widget(const CartridgeE7Widget&) = delete;
    CartridgeE7Widget(CartridgeE7Widget&&) = delete;
    CartridgeE7Widget& operator=(const CartridgeE7Widget&) = delete;
    CartridgeE7Widget& operator=(CartridgeE7Widget&&) = delete;
};

#endif  // CARTRIDGE_E7_WIDGET_HXX
