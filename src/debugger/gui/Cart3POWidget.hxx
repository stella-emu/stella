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

#ifndef CARTRIDGE_3PO_WIDGET_HXX
#define CARTRIDGE_3PO_WIDGET_HXX

class Cartridge3PO;
class ButtonWidget;
class EditTextWidget;
class PopUpWidget;

#include "CartEnhancedWidget.hxx"

/**
  The 3PO cart's page in the debugger: Cartridge3EPlusWidget's, with bank
  numbers past 255 and the outer bank register ($2F).
*/
class Cartridge3POWidget : public CartridgeEnhancedWidget
{
  public:
    Cartridge3POWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       Cartridge3PO& cart);
    ~Cartridge3POWidget() override = default;

    void loadConfig() override;

    string internalRamDescription() override;

  protected:
    string manufacturer() override { return "Newbroo Games"; }
    string description() override;
    void createBankWidgets() override;
    void layoutBankSelect(GUI::BoxLayout& col) const override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void updateUIState();

  private:
    Cartridge3PO& myCart3PO;

    std::array<LabelWidget*, 4> mySegLbl{nullptr};
    std::array<LabelWidget*, 8> myAddrLbl{nullptr};
    std::array<LabelWidget*, 4> myBankWidgetLbl{nullptr};
    std::array<LabelWidget*, 4> myBankTypeLbl{nullptr};
    std::array<PopUpWidget*, 4> myBankType{nullptr};
    std::array<ButtonWidget*, 4> myBankCommit{nullptr};
    std::array<EditTextWidget*, 8> myBankState{nullptr};
    LabelWidget* myOuterLbl{nullptr};
    EditTextWidget* myOuterState{nullptr};
    LabelWidget* myRamOuterLbl{nullptr};
    EditTextWidget* myRamOuterState{nullptr};

    struct Cmd {
      static constexpr GuiCmd::Code
        RomRamChanged = GuiCmd::of("Cartridge3POWidget.RomRamChanged"),
        ChangeBank    = GuiCmd::of("Cartridge3POWidget.ChangeBank");
    };

  private:
    // Following constructors and assignment operators not supported
    Cartridge3POWidget() = delete;
    Cartridge3POWidget(const Cartridge3POWidget&) = delete;
    Cartridge3POWidget(Cartridge3POWidget&&) = delete;
    Cartridge3POWidget& operator=(const Cartridge3POWidget&) = delete;
    Cartridge3POWidget& operator=(Cartridge3POWidget&&) = delete;
};

#endif  // CARTRIDGE_3PO_WIDGET_HXX
