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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE3EPLUS_WIDGET_HXX
#define CARTRIDGE3EPLUS_WIDGET_HXX

class Cartridge3EPlus;
class ButtonWidget;
class EditTextWidget;
class PopUpWidget;

#include "CartEnhancedWidget.hxx"

class Cartridge3EPlusWidget : public CartridgeEnhancedWidget
{
  public:
    Cartridge3EPlusWidget(GuiObject* boss, const GUI::Font& lfont,
                          const GUI::Font& nfont,
                          int x, int y, int w, int h,
                          Cartridge3EPlus& cart);
    virtual ~Cartridge3EPlusWidget() = default;

  private:
    string manufacturer() override { return "Thomas Jentzsch"; }

    string description() override;

    void bankSelect(int& ypos) override;

    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void updateUIState();

    void loadConfig() override;

    string internalRamDescription() override;

  private:
    Cartridge3EPlus& myCart3EP;

    std::array<PopUpWidget*, 4> myBankType{nullptr};
    std::array<ButtonWidget*, 4> myBankCommit{nullptr};
    std::array<EditTextWidget*, 8> myBankState{nullptr};

    enum BankID {
      kBank0Changed = 'b0CH',
      kBank1Changed = 'b1CH',
      kBank2Changed = 'b2CH',
      kBank3Changed = 'b3CH'
    };
    static const std::array<BankID, 4> bankEnum;

  private:
    // Following constructors and assignment operators not supported
    Cartridge3EPlusWidget() = delete;
    Cartridge3EPlusWidget(const Cartridge3EPlusWidget&) = delete;
    Cartridge3EPlusWidget(Cartridge3EPlusWidget&&) = delete;
    Cartridge3EPlusWidget& operator=(const Cartridge3EPlusWidget&) = delete;
    Cartridge3EPlusWidget& operator=(Cartridge3EPlusWidget&&) = delete;
};

#endif
