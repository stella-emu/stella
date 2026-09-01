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

#ifndef CARTRIDGE_AR_WIDGET_HXX
#define CARTRIDGE_AR_WIDGET_HXX

class CartridgeAR;
class CheckboxWidget;
class EditTextWidget;
class PopUpWidget;
class WrappedTextWidget;

#include "CartDebugWidget.hxx"

class CartridgeARWidget : public CartDebugWidget
{
  public:
    CartridgeARWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      CartridgeAR& cart);
    ~CartridgeARWidget() override = default;

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
    void layoutContent(GUI::BoxLayout& col) const override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    CartridgeAR& myCart;

    LabelWidget* myModeInfo{nullptr};
    LabelWidget* myModeDetail{nullptr};
    LabelWidget* mySliceLbl{nullptr};
    PopUpWidget* mySlice{nullptr};
    CheckboxWidget* myWriteEnable{nullptr};
    CheckboxWidget* myRomPower{nullptr};
    LabelWidget* myWriteStateLbl{nullptr};
    EditTextWidget* myWriteState{nullptr};

    // Sound-load carts only (myIsSoundLoad is fixed for the cart's lifetime,
    // so whether these exist at all is a one-time ctor decision, not a
    // runtime show/hide): the tape(s) CartCreator located, plus playback
    // start/exhaustion events as they occur
    WrappedTextWidget* myLoadLog{nullptr};

    // Snapshot of the full 6K of RAM, refreshed each saveOldState(); backs the
    // Cartridge RAM tab's change tracking (see internalRamOld())
    ByteArray myOldRAM;

    struct Cmd {
      static constexpr GuiCmd::Code
        ConfigChanged = GuiCmd::of("CartridgeARWidget.ConfigChanged");
    };

  private:
    // Following constructors and assignment operators not supported
    CartridgeARWidget() = delete;
    CartridgeARWidget(const CartridgeARWidget&) = delete;
    CartridgeARWidget(CartridgeARWidget&&) = delete;
    CartridgeARWidget& operator=(const CartridgeARWidget&) = delete;
    CartridgeARWidget& operator=(CartridgeARWidget&&) = delete;
};

#endif  // CARTRIDGE_AR_WIDGET_HXX
