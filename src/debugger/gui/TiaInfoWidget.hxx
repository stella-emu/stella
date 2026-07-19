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

#ifndef TIA_INFO_WIDGET_HXX
#define TIA_INFO_WIDGET_HXX

class GuiObject;
class EditTextWidget;
class StaticTextWidget;
class CheckboxWidget;

#include "Widget.hxx"
#include "Command.hxx"

namespace GUI {
  class BoxLayout;
}  // namespace GUI


class TiaInfoWidget : public Widget, public CommandSender
{
  public:
    TiaInfoWidget(GuiObject *boss, const GUI::Font& lfont, const GUI::Font& nfont);
    ~TiaInfoWidget() override = default;

    void loadConfig() override;

    // Reflow entry point for the resizable debugger: move the widget and
    // re-lay-out the two columns of fields for the width given, choosing the
    // short or long label variants to fit
    void setArea(int x, int y, int w, int h) override;

    // My constructor cannot know how tall I am -- that is however tall my rows,
    // gaps and margins make me -- so report what my own layout tree comes to
    Common::Size naturalSize() const override;

    // The narrowest width the fields still fit into, i.e. the width at which
    // the short label variants are used.  Bounds the status area, which shares
    // the debugger's top band with the (growable) TIA image.  Measuring means
    // showing the short labels, so this restores the form that was on screen
    int minWidth();

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;

  private:
    // Lay the fields out within the width the parent layout gave us, choosing
    // the label form that fits; shared by the ctor and setArea()
    void reflow();

    // Show the long or the short form of every row label.  The labels resize
    // themselves to what they show, so the layout's own size follows suit
    void setLabels(bool longstr);

    // The two columns of rows, as the engine sees them.  Asking this tree for
    // its natural size is where the widget's own width and height come from,
    // so no width or height is ever added up here by hand
    unique_ptr<GUI::BoxLayout> buildLayout() const;

    // The width the given label form wants: clearances, gap and fields included
    int naturalWidthFor(bool longstr);

    // The gap between the two columns of fields
    int columnGap() const { return _fontWidth * 5 / 4; }

  private:
    EditTextWidget* myFrameCount{nullptr};
    EditTextWidget* myFrameCycles{nullptr};
    EditTextWidget* myTotalCycles{nullptr};
    EditTextWidget* myDeltaCycles{nullptr};
    EditTextWidget* myWSyncCylces{nullptr};
    EditTextWidget* myTimerCylces{nullptr};

    EditTextWidget* myScanlineCount{nullptr};
    EditTextWidget* myScanlineCountLast{nullptr};
    EditTextWidget* myScanlineCycles{nullptr};
    EditTextWidget* myPixelPosition{nullptr};
    EditTextWidget* myColorClocks{nullptr};

    // Labels promoted from anonymous locals so the reflow can switch each one
    // between its short and long text and reposition it
    StaticTextWidget* myFrameCyclesLabel{nullptr};
    StaticTextWidget* myWSyncCyclesLabel{nullptr};
    StaticTextWidget* myTimerCyclesLabel{nullptr};
    StaticTextWidget* myTotalLabel{nullptr};
    StaticTextWidget* myDeltaLabel{nullptr};
    StaticTextWidget* myFrameCountLabel{nullptr};
    StaticTextWidget* myScanlineLabel{nullptr};
    StaticTextWidget* myScanCycleLabel{nullptr};
    StaticTextWidget* myPixelPosLabel{nullptr};
    StaticTextWidget* myColorClockLabel{nullptr};

    // Which label form is currently on screen
    bool myLongLabels{false};

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Following constructors and assignment operators not supported
    TiaInfoWidget() = delete;
    TiaInfoWidget(const TiaInfoWidget&) = delete;
    TiaInfoWidget(TiaInfoWidget&&) = delete;
    TiaInfoWidget& operator=(const TiaInfoWidget&) = delete;
    TiaInfoWidget& operator=(TiaInfoWidget&&) = delete;
};

#endif  // TIA_INFO_WIDGET_HXX
