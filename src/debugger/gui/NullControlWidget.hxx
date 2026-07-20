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

#ifndef NULL_CONTROL_WIDGET_HXX
#define NULL_CONTROL_WIDGET_HXX

class GuiObject;

#include "ControllerWidget.hxx"

class NullControlWidget : public ControllerWidget
{
  public:
    NullControlWidget(GuiObject* boss, const GUI::Font& font,
                      Controller& controller, bool embedded = false);
    ~NullControlWidget() override = default;

  protected:
    void layoutContent(GUI::BoxLayout& col) override;

  private:
    // The two "not available" lines (shorter when embedded in a QuadTari)
    StaticTextWidget* myLine1{nullptr};
    StaticTextWidget* myLine2{nullptr};

  private:
    // Following constructors and assignment operators not supported
    NullControlWidget() = delete;
    NullControlWidget(const NullControlWidget&) = delete;
    NullControlWidget(NullControlWidget&&) = delete;
    NullControlWidget& operator=(const NullControlWidget&) = delete;
    NullControlWidget& operator=(NullControlWidget&&) = delete;
};

#endif  // NULL_CONTROL_WIDGET_HXX
