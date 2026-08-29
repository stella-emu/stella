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

#ifndef CARTRIDGE_4A50_WIDGET_HXX
#define CARTRIDGE_4A50_WIDGET_HXX

class Cartridge4A50;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class Cartridge4A50Widget : public CartDebugWidget
{
  public:
    Cartridge4A50Widget(GuiObject* boss, const GUI::Font& lfont,
                        const GUI::Font& nfont,
                        Cartridge4A50& cart);
    ~Cartridge4A50Widget() override = default;

    void loadConfig() override;
    string bankState() override;

  protected:
    void layoutContent(GUI::BoxLayout& col) const override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // One region: a heading, with its ROM and RAM selectors indented beneath it
    void layoutRegion(GUI::BoxLayout& col, LabelWidget* label,
                      LabelWidget* romLabel, PopUpWidget* rom,
                      LabelWidget* ramLabel, PopUpWidget* ram) const;

  private:
    Cartridge4A50& myCart;
    LabelWidget *myLowerLbl{nullptr}, *myMiddleLbl{nullptr}, *myHighLbl{nullptr};
    LabelWidget *myROMLowerLbl{nullptr}, *myRAMLowerLbl{nullptr};
    LabelWidget *myROMMiddleLbl{nullptr}, *myRAMMiddleLbl{nullptr};
    LabelWidget *myROMHighLbl{nullptr}, *myRAMHighLbl{nullptr};
    PopUpWidget *myROMLower{nullptr}, *myRAMLower{nullptr};
    PopUpWidget *myROMMiddle{nullptr}, *myRAMMiddle{nullptr};
    PopUpWidget *myROMHigh{nullptr}, *myRAMHigh{nullptr};

    struct Cmd {
      static constexpr GuiCmd::Code
        RomLowerChanged  = GuiCmd::of("Cartridge4A50Widget.RomLowerChanged"),
        RamLowerChanged  = GuiCmd::of("Cartridge4A50Widget.RamLowerChanged"),
        RomMiddleChanged = GuiCmd::of("Cartridge4A50Widget.RomMiddleChanged"),
        RamMiddleChanged = GuiCmd::of("Cartridge4A50Widget.RamMiddleChanged"),
        RomHighChanged   = GuiCmd::of("Cartridge4A50Widget.RomHighChanged"),
        RamHighChanged   = GuiCmd::of("Cartridge4A50Widget.RamHighChanged");
    };

  private:
    // Following constructors and assignment operators not supported
    Cartridge4A50Widget() = delete;
    Cartridge4A50Widget(const Cartridge4A50Widget&) = delete;
    Cartridge4A50Widget(Cartridge4A50Widget&&) = delete;
    Cartridge4A50Widget& operator=(const Cartridge4A50Widget&) = delete;
    Cartridge4A50Widget& operator=(Cartridge4A50Widget&&) = delete;
};

#endif  // CARTRIDGE_4A50_WIDGET_HXX
