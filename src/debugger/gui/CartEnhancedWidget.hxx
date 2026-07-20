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

#ifndef CARTRIDGE_ENHANCED_WIDGET_HXX
#define CARTRIDGE_ENHANCED_WIDGET_HXX

class CartridgeEnhanced;
class EditTextWidget;
class StringListWidget;
class PopUpWidget;

namespace GUI {
  class Font;
}  // namespace GUI

#include "Variant.hxx"
#include "CartDebugWidget.hxx"

class CartridgeEnhancedWidget : public CartDebugWidget
{
  public:
    CartridgeEnhancedWidget(GuiObject* boss, const GUI::Font& lfont,
                            const GUI::Font& nfont,
                            CartridgeEnhanced& cart);
    ~CartridgeEnhancedWidget() override = default;

    void loadConfig() override;
    void saveOldState() override;
    string bankState() override;

    // Start of functions for Cartridge RAM tab
    uInt32 internalRamSize() override;
    uInt32 internalRamRPort(int start) override;
    string internalRamDescription() override;
    const ByteArray& internalRamOld(int start, int count) override;
    const ByteArray& internalRamCurrent(int start, int count) override;
    void internalRamSetValue(int addr, uInt8 value) override;
    uInt8 internalRamGetValue(int addr) override;
    string internalRamLabel(int addr) override;
    // End of functions for Cartridge RAM tab

  protected:
    // Create then lay out all of this widget's content; called by each leaf ctor
    void initialize();
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    virtual size_t size();
    virtual string manufacturer() = 0;
    virtual string description();
    virtual int descriptionLines();
    virtual string ramDescription();
    virtual string romDescription();
    // The bank pop-up's entries; it sizes its own box to the widest of them
    virtual void bankList(uInt16 bankCount, int seg, VariantList& items);
    virtual string hotspotStr(int bank = 0, int segment = 0, bool prefix = false);
    virtual uInt16 bankSegs() const; // { return myCart.myBankSegs; }

    // Create this widget's content at placeholder positions (create-only ctor)
    void createWidgets();
    void createPlusROM();
    virtual void createBankWidgets();

    // The PlusROM fields and the bank selectors, which every enhanced cart has.
    // A leaf adding rows of its own overrides layoutContent() and calls this
    // first; one that selects its banks differently overrides layoutBankSelect()
    void layoutContent(GUI::BoxLayout& col) const override;
    void layoutPlusROM(GUI::BoxLayout& col) const;
    virtual void layoutBankSelect(GUI::BoxLayout& col) const;

  protected:
    enum { kBankChanged = 'bkCH' };

    struct CartState {
      ByteArray internalRam;
      ByteArray banks;
      ByteArray send;
      ByteArray receive;
    };
    CartState myOldState;

    CartridgeEnhanced& myCart;

    // Distance between two hotspots
    int myHotspotDelta{1};

    StaticTextWidget* myPlusROMLabel{nullptr};
    StaticTextWidget* myPlusROMHostLabel{nullptr};
    StaticTextWidget* myPlusROMPathLabel{nullptr};
    StaticTextWidget* myPlusROMSendLabel{nullptr};
    StaticTextWidget* myPlusROMReceiveLabel{nullptr};
    EditTextWidget* myPlusROMHostWidget{nullptr};
    EditTextWidget* myPlusROMPathWidget{nullptr};
    EditTextWidget* myPlusROMSendWidget{nullptr};
    EditTextWidget* myPlusROMReceiveWidget{nullptr};

    std::vector<PopUpWidget*> myBankWidgets;

    // Display all addresses based on this
    static constexpr uInt16 ADDR_BASE = 0xF000;

  private:
    // Following constructors and assignment operators not supported
    CartridgeEnhancedWidget() = delete;
    CartridgeEnhancedWidget(const CartridgeEnhancedWidget&) = delete;
    CartridgeEnhancedWidget(CartridgeEnhancedWidget&&) = delete;
    CartridgeEnhancedWidget& operator=(const CartridgeEnhancedWidget&) = delete;
    CartridgeEnhancedWidget& operator=(CartridgeEnhancedWidget&&) = delete;
};

#endif  // CARTRIDGE_ENHANCED_WIDGET_HXX
