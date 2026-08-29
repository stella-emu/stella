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

#ifndef ROM_LIST_WIDGET_HXX
#define ROM_LIST_WIDGET_HXX

class ScrollBarWidget;
class CheckListWidget;
class RomListSettings;
class DisasmColorsDialog;

#include "Base.hxx"
#include "CartDebug.hxx"
#include "EditableWidget.hxx"

/** RomListWidget */
class RomListWidget : public EditableWidget
{
  public:
    struct Cmd {
      static constexpr GuiCmd::Code
        BreakpointChanged   = GuiCmd::of("RomListWidget.BreakpointChanged"),
        RomChanged          = GuiCmd::of("RomListWidget.RomChanged"),
        SetPC               = GuiCmd::of("RomListWidget.SetPC"),
        RunToPC             = GuiCmd::of("RomListWidget.RunToPC"),
        SetTimer            = GuiCmd::of("RomListWidget.SetTimer"),
        Disassemble         = GuiCmd::of("RomListWidget.Disassemble"),
        TentativeCode       = GuiCmd::of("RomListWidget.TentativeCode"),
        PCAddresses         = GuiCmd::of("RomListWidget.PCAddresses"),
        GfxAsBinary         = GuiCmd::of("RomListWidget.GfxAsBinary"),
        AddressRelocation   = GuiCmd::of("RomListWidget.AddressRelocation"),
        DisasmColors        = GuiCmd::of("RomListWidget.DisasmColors"),
        DisasmColorsChanged = GuiCmd::of("RomListWidget.DisasmColorsChanged");
    };

  public:
    // lfont is only for the pop-up dialogs this widget owns; dfont is the
    // disassembly font, and everything in the list is drawn and measured in it
    RomListWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& dfont);
    ~RomListWidget() override = default;

    void setList(const CartDebug::Disassembly& disasm);

    int getSelected() const        { return _selectedItem; }
    int getHighlighted() const     { return _highlightedItem; }
    void setSelected(int item);
    void setHighlighted(int item);

    string getToolTip(const Common::Point& pos) const override;
    bool changedToolTip(const Common::Point& oldPos, const Common::Point& newPos) const override;

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleText(char text) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    bool handleKeyUp(StellaKey key, StellaMod mod) override;
    bool handleEvent(Event::Type e) override;

    // Reflow support so the owning dialog's layout() can move/resize the list
    // in place (used by the resizable debugger).  These grow the checkbox pool,
    // reposition the sibling scrollbar and recompute the row count / column
    // widths from the live geometry and font.
    using EditableWidget::setPos;
    void setPos(const Common::Point& pos) override;
    void setWidth(int w) override;
    void setHeight(int h) override;
    // Reports the full footprint (list area + scrollbar), so setWidth() is its
    // inverse (mirrors ListWidget)
    int getWidth() const override;
    void refreshFont() override;

  protected:
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void drawWidget(bool hilite) override;
    Common::Rect getLineRect() const;
    Common::Rect getEditRect() const override;

    int findItem(int x, int y) const;
    void recalc();

    void startEditMode() override;
    void endEditMode() override;
    void abortEditMode() override;
    void lostFocusWidget() override;

    bool hasToolTip() const override { return true; }

    void scrollToSelected()    { scrollToCurrent(_selectedItem);    }
    void scrollToHighlighted() { scrollToCurrent(_highlightedItem); }

    // Load the disassembly colour map from Settings (called on construction and
    // after DisasmColorsDialog saves new values).
    void loadDisasmColorMap();

    // Map a semantic DisasmSegColor to the cached ColorId for rendering.
    ColorId segColor(CartDebug::DisasmSegColor seg) const;

  private:
    void scrollToCurrent(int item);
    Common::Point getToolTipIndex(const Common::Point& pos) const;

    // Grow the checkbox pool to one per visible row (grow-only; widgets can't
    // be removed) and position every checkbox against the list's current
    // origin, hiding any beyond the visible row count
    void reflowCheckboxes();

    // (Re)compute the label and bytes column widths from the full footprint
    // width and the disassembly font (wider windows get wider label columns)
    void recalcColumnWidths(int w);

  private:
    unique_ptr<RomListSettings>    myMenu;
    unique_ptr<DisasmColorsDialog> myDisasmColorsDialog;

    // Cached rendering colours, indexed by DisasmSegColor (0..14).
    CartDebug::DisasmColorMap myDisasmColorMap{};
    ScrollBarWidget* myScrollBar{nullptr};

    int  _labelWidth{0};
    int  _bytesWidth{0};
    int  _rows{0};
    int  _currentPos{0}; // position of first line in visible window
    int  _selectedItem{-1};
    int  _highlightedItem{-1};
    StellaKey _currentKeyDown{StellaKey::UNKNOWN};
    Common::Base::Fmt _base{Common::Base::Fmt::DEFAULT};  // base used during editing

    const CartDebug::Disassembly* myDisasm{nullptr};
    vector<CheckboxWidget*> myCheckList;

  private:
    // Following constructors and assignment operators not supported
    RomListWidget() = delete;
    RomListWidget(const RomListWidget&) = delete;
    RomListWidget(RomListWidget&&) = delete;
    RomListWidget& operator=(const RomListWidget&) = delete;
    RomListWidget& operator=(RomListWidget&&) = delete;
};

#endif  // ROM_LIST_WIDGET_HXX
