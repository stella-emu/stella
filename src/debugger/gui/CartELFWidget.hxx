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

#ifndef CARTRIDGE_ELF_WIDGET_HXX
#define CARTRIDGE_ELF_WIDGET_HXX

#include "CartDebugWidget.hxx"

class CartridgeELF;
class EditTextWidget;
class WrappedTextWidget;
class FSNode;

class CartridgeELFWidget: public CartDebugWidget
{
  public:
    CartridgeELFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       CartridgeELF& cart);

    ~CartridgeELFWidget() override = default;

  protected:
    void layoutContent(GUI::BoxLayout& col) const override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    void initialize();
    void saveArmImage(const FSNode& node);

  private:
    // The most lines of the debug log to show before it scrolls
    static constexpr uInt16 VISIBLE_LOG_LINES = 19;

    CartridgeELF& myCart;

    WrappedTextWidget* myLog{nullptr};
    ButtonWidget* mySaveImageButton{nullptr};

    struct Cmd {
      static constexpr GuiCmd::Code
        SaveArmImage = GuiCmd::of("CartridgeELFWidget.SaveArmImage");
    };

  private:
    CartridgeELFWidget() = delete;
    CartridgeELFWidget(const CartridgeELFWidget&) = delete;
    CartridgeELFWidget(CartridgeELFWidget&&) = delete;
    CartridgeELFWidget& operator=(const CartridgeELFWidget&) = delete;
    CartridgeELFWidget& operator=(CartridgeELFWidget&&) = delete;
};

#endif  // CARTRIDGE_ELF_WIDGET_HXX
