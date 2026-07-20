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

#ifndef CART_RAM_WIDGET_HXX
#define CART_RAM_WIDGET_HXX

class GuiObject;
class DataGridOpsWidget;
class WrappedTextWidget;
class InternalRamWidget;
class CartDebugWidget;

#include "RamWidget.hxx"
#include "Widget.hxx"
#include "Command.hxx"

class CartRamWidget : public Widget, public CommandSender
{
  public:
    CartRamWidget(GuiObject* boss, const GUI::Font& lfont,
                  const GUI::Font& nfont,
                  CartDebugWidget& cartDebug);
    ~CartRamWidget() override = default;

    void loadConfig() override;
    void setOpsWidget(DataGridOpsWidget* w);

    void setArea(int x, int y, int w, int h) override;

    // My constructor cannot know how tall I am -- that is however tall my fields
    // make me -- so report what my own layout tree comes to.  The RAM view below
    // them fills, and so adds nothing of its own
    Common::Size naturalSize() const override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    // Lay the tab out for its current area/font: the RAM size and description
    // fields widen with it (the description re-wrapping itself), and the RAM view
    // fills whatever is left below them.  Driven by setArea()
    void reflow();

  private:
    // The tab as the engine sees it, built without positioning anything, so that
    // reflow() and naturalSize() are the same layout asked two questions
    unique_ptr<GUI::Layout> buildLayout() const;

  protected:
    // Font used for 'normal' text; _font is for 'label' text
    const GUI::Font& _nfont;

  private:
    class InternalRamWidget : public RamWidget
    {
      public:
        InternalRamWidget(GuiObject* boss, const GUI::Font& lfont,
                          const GUI::Font& nfont,
                          CartDebugWidget& dbg);
        ~InternalRamWidget() override = default;
        string getLabel(int addr) const override;

      private:
        uInt8 getValue(int addr) const override;
        void setValue(int addr, uInt8 value) override;

        void fillList(uInt32 start, uInt32 size, IntArray& alist,
                      IntArray& vlist, BoolArray& changed) const override;
        uInt32 readPort(uInt32 start) const override;
        const ByteArray& currentRam(uInt32 start) const override;

      private:
        CartDebugWidget& myCart;

      private:
        // Following constructors and assignment operators not supported
        InternalRamWidget() = delete;
        InternalRamWidget(const InternalRamWidget&) = delete;
        InternalRamWidget(InternalRamWidget&&) = delete;
        InternalRamWidget& operator=(const InternalRamWidget&) = delete;
        InternalRamWidget& operator=(InternalRamWidget&&) = delete;
    };

  private:
    // The most lines of description to show before it scrolls
    static constexpr uInt16 MAX_DESC_LINES = 6;

    StaticTextWidget* myRamSizeLabel{nullptr};
    StaticTextWidget* myDescLabel{nullptr};
    EditTextWidget* myRamSize{nullptr};
    WrappedTextWidget* myDesc{nullptr};
    InternalRamWidget* myRam{nullptr};

  private:
    // Following constructors and assignment operators not supported
    CartRamWidget() = delete;
    CartRamWidget(const CartRamWidget&) = delete;
    CartRamWidget(CartRamWidget&&) = delete;
    CartRamWidget& operator=(const CartRamWidget&) = delete;
    CartRamWidget& operator=(CartRamWidget&&) = delete;
};

#endif  // CART_RAM_WIDGET_HXX
