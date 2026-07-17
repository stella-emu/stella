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

#ifndef CONTROLLER_WIDGET_HXX
#define CONTROLLER_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class StaticTextWidget;
class CheckboxWidget;

namespace GUI {
  class Layout;
  class BoxLayout;
}  // namespace GUI

#include "Font.hxx"
#include "Widget.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "Command.hxx"
#include "ControlLowLevel.hxx"

class ControllerWidget : public Widget, public CommandSender, public ControllerLowLevel
{
  public:
    ControllerWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                     Controller& controller)
      : Widget(boss, font, x, y, 16, 16),
        CommandSender(boss),
        ControllerLowLevel(controller)
    {
      _w = 18 * font.getMaxCharWidth();
      _h = 8 * font.getLineHeight();
    }
    ~ControllerWidget() override = default;

    void loadConfig() override { }

    // Reposition/resize this widget's content when its area changes; drives
    // reflow() so a controller re-flows live with the debugger window
    void setArea(int x, int y, int w, int h) override;

  protected:
    // Create the shared "Left (X)" / "Right (X)" caption at a placeholder
    // position; a controller EMBEDDED in another (QuadTari) has none, so it
    // simply does not call this.  reflow() is what positions it
    void createHeader();

    // Lay this widget out for its current area and font.  EVERY controller has
    // the same skeleton -- an optional header above its content, centered in a
    // fixed block -- so it is written once, here.  A controller states only what
    // goes below the header: see layoutContent()
    void reflow();

    // THE hook: add this controller's own pins/cross/grid to the column
    virtual void layoutContent(GUI::BoxLayout& col) { }

    // Build the shared 4-direction cross (Up/Left/Right/Down) as a tight grid,
    // so every joystick-style controller's cross is the same size whatever the
    // labeled buttons it stacks below it in layoutContent()
    unique_ptr<GUI::Layout> layoutCross(CheckboxWidget* up, CheckboxWidget* down,
                                        CheckboxWidget* left, CheckboxWidget* right);

    bool isLeftPort()
    {
      const bool swappedPorts =
        instance().console().properties().get(PropType::Console_SwapPorts) == "YES";
      return (controller().jack() == Controller::Jack::Left) ^ swappedPorts;
    }

    string getHeader()
    {
      return (isLeftPort() ? "Left (" : "Right (") + controller().name() + ")";
    }

    void handleCommand(CommandSender* sender, int cmd, int data, int id) override { }

  private:
    // The shared two-line header, laid out by reflow(): the controller name over
    // its "(Left)"/"(Right)" port, both centered (null when embedded)
    StaticTextWidget* myHeaderName{nullptr};
    StaticTextWidget* myHeaderPort{nullptr};

  private:
    // Following constructors and assignment operators not supported
    ControllerWidget() = delete;
    ControllerWidget(const ControllerWidget&) = delete;
    ControllerWidget(ControllerWidget&&) = delete;
    ControllerWidget& operator=(const ControllerWidget&) = delete;
    ControllerWidget& operator=(ControllerWidget&&) = delete;
};

#endif  // CONTROLLER_WIDGET_HXX
