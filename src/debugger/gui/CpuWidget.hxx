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

#ifndef CPU_WIDGET_HXX
#define CPU_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class DataGridWidget;
class DataGridOpsWidget;
class EditTextWidget;
class ToggleBitWidget;

#include "Widget.hxx"
#include "Command.hxx"

namespace GUI {
  class Layout;
}  // namespace GUI

class CpuWidget : public Widget, public CommandSender
{
  public:
    CpuWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont);
    ~CpuWidget() override = default;

    void setOpsWidget(DataGridOpsWidget* w);
    void loadConfig() override;

    // Reflow entry point for the resizable debugger: move the widget and lay
    // the registers/labels out for the width given
    void setArea(int x, int y, int w, int h) override;

    // My constructor cannot know how tall I am -- that is however tall my three
    // register rows make me -- so report what my own layout tree comes to
    Common::Size naturalSize() const override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Lay the registers out within the area the parent layout gave us; shared
    // by the ctor and setArea()
    void reflow();

    // The three register rows as the engine sees them.  They share one set of
    // columns, which is what puts the destination field under the source ones
    // without anyone reading back where those ended up
    unique_ptr<GUI::Layout> buildLayout() const;

  private:
    // ID's for the various widgets
    // We need ID's, since there are more than one of several types of widgets
    enum: uInt8 {
      kPCRegID,
      kCpuRegID,
      kCpuRegDecID,
      kCpuRegBinID
    };

    enum: uInt8 {
      kPCRegAddr,
      kSPRegAddr,
      kARegAddr,
      kXRegAddr,
      kYRegAddr
    };

    enum: uInt8 {
      kPSRegN = 0,
      kPSRegV = 1,
      kPSRegB = 3,
      kPSRegD = 4,
      kPSRegI = 5,
      kPSRegZ = 6,
      kPSRegC = 7
    };

    DataGridWidget*  myPCGrid{nullptr};
    DataGridWidget*  myCpuGrid{nullptr};
    DataGridWidget*  myCpuGridDecValue{nullptr};
    DataGridWidget*  myCpuGridBinValue{nullptr};
    ToggleBitWidget* myPSRegister{nullptr};
    EditTextWidget*  myPCLabel{nullptr};
    std::array<EditTextWidget*, 4> myCpuDataSrc{nullptr};
    EditTextWidget*  myCpuDataDest{nullptr};

    // Labels promoted from anonymous locals so the reflow can reposition them
    StaticTextWidget* myPCText{nullptr};
    StaticTextWidget* myPSText{nullptr};
    StaticTextWidget* myDestText{nullptr};
    std::array<StaticTextWidget*, 4> myRegLabels{nullptr};  // SP/A/X/Y
    std::array<StaticTextWidget*, 4> myDecPrefix{nullptr};  // '#'
    std::array<StaticTextWidget*, 4> myBinPrefix{nullptr};  // '%'

  private:
    // Following constructors and assignment operators not supported
    CpuWidget() = delete;
    CpuWidget(const CpuWidget&) = delete;
    CpuWidget(CpuWidget&&) = delete;
    CpuWidget& operator=(const CpuWidget&) = delete;
    CpuWidget& operator=(CpuWidget&&) = delete;
};

#endif  // CPU_WIDGET_HXX
