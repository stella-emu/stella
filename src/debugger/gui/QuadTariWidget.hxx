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

#ifndef QUADTARI_WIDGET_HXX
#define QUADTARI_WIDGET_HXX

#include "Control.hxx"
#include "ControllerWidget.hxx"

class QuadTariWidget: public ControllerWidget
{
  public:
    QuadTariWidget(GuiObject* boss, const GUI::Font& font,
                   Controller& controller);
    ~QuadTariWidget() override = default;

    void loadConfig() override;

  protected:
    // The two embedded controllers side by side, with the active-controller
    // pointer between them; the row lays each of them out via its own setArea()
    void layoutContent(GUI::BoxLayout& col) override;

  private:
    ControllerWidget* addController(GuiObject* boss, Controller& controller,
                                    bool second);

    StaticTextWidget* myPointer{nullptr};
    ControllerWidget* myFirst{nullptr};
    ControllerWidget* mySecond{nullptr};

    // Following constructors and assignment operators not supported
    QuadTariWidget() = delete;
    QuadTariWidget(const QuadTariWidget&) = delete;
    QuadTariWidget(QuadTariWidget&&) = delete;
    QuadTariWidget& operator=(const QuadTariWidget&) = delete;
    QuadTariWidget& operator=(QuadTariWidget&&) = delete;
};

#endif  // QUADTARI_WIDGET_HXX
