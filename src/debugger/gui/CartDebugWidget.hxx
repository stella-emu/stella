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

#ifndef CART_DEBUG_WIDGET_HXX
#define CART_DEBUG_WIDGET_HXX

class GuiObject;
class EditTextWidget;
class WrappedTextWidget;

namespace GUI {
  class Font;
  class BoxLayout;
}  // namespace GUI

#include "Base.hxx"  // not needed here, but all child classes need it
#include "Command.hxx"
#include "Layout.hxx"
#include "Widget.hxx"

class CartDebugWidget : public Widget, public CommandSender
{
  public:
    CartDebugWidget(GuiObject* boss, const GUI::Font& lfont,
                    const GUI::Font& nfont,
                    int x, int y, int w, int h);
    ~CartDebugWidget() override = default;

  public:
    // Layout metrics shared by BOTH cart tabs (this one and CartRamWidget), so
    // their fields line up with each other.  The right margin is wider than the
    // left so the fields keep a gap from the window border
    static constexpr int HBORDER = 2, RBORDER = 12, VBORDER = 4, VGAP = 4;

    // The width a tab's content is laid out in, for the tab of width 'w'.  Ask for
    // it rather than subtracting the margins yourself: a word-wrapping widget must
    // be given its width before the column holding it is built, and that width has
    // to be the one the column will hand it (see Layout.hxx's heightForWidth note)
    static constexpr int contentWidth(int w) { return w - HBORDER - RBORDER; }

  public:
    // Reposition/resize this widget's content when its area changes; drives
    // reflow() so a cart tab re-flows live with the debugger window
    void setArea(int x, int y, int w, int h) override;

    // Lay this tab out for its current area and font.  EVERY cart tab has the
    // same skeleton -- one label column, the ROM info rows, the cart's own rows
    // beneath them, all within the shared margins -- so it is written once, here.
    // A cart says only what goes in the middle: see layoutContent()
    void reflow();

    // Inform the ROM Widget that the underlying cart has somehow changed
    void invalidate();

    // Some carts need to save old state in the debugger, so that we can
    // implement change tracking; most carts probably won't do anything here
    virtual void saveOldState() { }

    void loadConfig() override;

    // Query internal state of the cart (usually just bankswitching info)
    virtual string bankState() { return "0 (non-bankswitched)"; }

    // To make the Cartridge RAM show up in the debugger, implement
    // the following 9 functions for cartridges with internal RAM
    virtual uInt32 internalRamSize() { return 0; }
    virtual uInt32 internalRamRPort(int start) { return 0; }
    virtual string internalRamDescription() { return {}; }
    virtual const ByteArray& internalRamOld(int start, int count) { return myRamOld; }
    virtual const ByteArray& internalRamCurrent(int start, int count) { return myRamCurrent; }
    virtual void internalRamSetValue(int addr, uInt8 value) { }
    virtual uInt8 internalRamGetValue(int addr) { return 0; }
    virtual string internalRamLabel(int addr) { return "Not available/applicable"; }
    virtual string tabLabel() { return " Cartridge RAM "; }

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override { }

    // Create the ROM size / manufacturer / description fields, at a placeholder
    // position: reflow() is what positions them.  Call this from the ctor
    void createBaseInformation(size_t bytes, string_view manufacturer,
        string_view desc, uInt16 maxlines = 10);

    // THE hook: append this cart's own rows to the tab's column.  Everything
    // around them — the label column, the ROM info rows above, the margins — is
    // the skeleton's business (see reflow()), so a cart states only its content.
    // A cart adding to what its base class lays out calls the base first, then
    // appends; one REPLACING a part of it overrides that part instead (see
    // CartridgeEnhancedWidget's bank selectors)
    virtual void layoutContent(GUI::BoxLayout& col) { }

    // Append the ROM size / manufacturer / description rows to a vertical box
    void layoutBaseInformation(GUI::BoxLayout& col);

  protected:
    // The controls sharing this tab's label column.  A control says it belongs
    // here as it is CREATED — the ROM info rows below, the PlusROM fields and
    // bank selectors in the carts — and one GUI::alignLabels() call at reflow
    // time gives them all the same column.  So no label carries padding of its
    // own, and a cart whose selectors sit elsewhere simply does not join
    std::vector<GUI::LabeledControl> myLabelColumn;

    // Arrays used to hold current and previous internal RAM values
    ByteArray myRamOld, myRamCurrent;

    // Font used for 'normal' text; _font is for 'label' text
    const GUI::Font& _nfont;

  private:
    // The ROM size / manufacturer / description fields and their labels.
    // The description (myDesc) re-wraps itself whenever its width changes
    StaticTextWidget* myROMSizeLabel{nullptr};
    StaticTextWidget* myManufacturerLabel{nullptr};
    StaticTextWidget* myDescLabel{nullptr};
    EditTextWidget* myROMSize{nullptr};
    EditTextWidget* myManufacturer{nullptr};
    WrappedTextWidget* myDesc{nullptr};

  private:
    // Following constructors and assignment operators not supported
    CartDebugWidget() = delete;
    CartDebugWidget(const CartDebugWidget&) = delete;
    CartDebugWidget(CartDebugWidget&&) = delete;
    CartDebugWidget& operator=(const CartDebugWidget&) = delete;
    CartDebugWidget& operator=(CartDebugWidget&&) = delete;
};

#endif  // CART_DEBUG_WIDGET_HXX
