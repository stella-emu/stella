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

#ifndef PADDLE_WIDGET_HXX
#define PADDLE_WIDGET_HXX

class Controller;

#include "ControllerWidget.hxx"

class PaddleWidget : public ControllerWidget
{
  public:
    PaddleWidget(GuiObject* boss, const GUI::Font& font,
                 Controller& controller,
                 bool embedded = false, bool second = false);
    ~PaddleWidget() override = default;

    void loadConfig() override;

  protected:
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    void layoutContent(GUI::BoxLayout& col) override;

  private:
    struct Cmd {
      static constexpr GuiCmd::Code
        Paddle0Changed = GuiCmd::of("PaddleWidget.Paddle0Changed"),
        Paddle1Changed = GuiCmd::of("PaddleWidget.Paddle1Changed"),
        Paddle0Fire    = GuiCmd::of("PaddleWidget.Paddle0Fire"),
        Paddle1Fire    = GuiCmd::of("PaddleWidget.Paddle1Fire");
    };

    bool myEmbedded{false};
    SliderWidget *myP0Resistance{nullptr}, *myP1Resistance{nullptr};
    CheckboxWidget *myP0Fire{nullptr}, *myP1Fire{nullptr};
    // Short pot labels when embedded in a QuadTari; the resistance sliders'
    // own labels otherwise (the two uses are mutually exclusive)
    LabelWidget *myP0Lbl{nullptr}, *myP1Lbl{nullptr};

  private:
    // Following constructors and assignment operators not supported
    PaddleWidget() = delete;
    PaddleWidget(const PaddleWidget&) = delete;
    PaddleWidget(PaddleWidget&&) = delete;
    PaddleWidget& operator=(const PaddleWidget&) = delete;
    PaddleWidget& operator=(PaddleWidget&&) = delete;
};

#endif  // PADDLE_WIDGET_HXX
