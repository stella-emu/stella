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

#ifndef MEM_VIEW_WINDOW_DIALOG_HXX
#define MEM_VIEW_WINDOW_DIALOG_HXX

class OSystem;
class DialogContainer;
class CheckboxWidget;
class PopUpWidget;
class SliderWidget;
class ButtonWidget;
class LabelWidget;
class ColorWidget;

#include <map>

#include "Cart.hxx"
#include "Dialog.hxx"
#include "MemViewWidget.hxx"

/**
  The base dialog of the debugger's memory-view window: a left column with
  the RAM grid (labeled, followed by a small cartridge RAM if any) on top and
  the settings panel below it, both as small as their content needs; the
  remaining space to the right is shared by a big cartridge RAM (labeled) and
  the ROM grids (labeled), in proportion to their sizes.

  @author  Stephen Anthony
*/
class MemViewWindowDialog : public Dialog
{
  public:
    static constexpr Cartridge::ImageScope extraScopeFirst = Cartridge::ImageScope::DISPLAY_DATA;
    static constexpr Cartridge::ImageScope extraScopeLast = Cartridge::ImageScope::DISPLAY_DATA;

    static constexpr uInt32 READ_COLOR_HIGH = 0xc9c5ff;
    static constexpr uInt32 READ_COLOR_MID = 0x302abc;

    static constexpr uInt32 WRITE_COLOR_HIGH = 0xffb2f1;
    static constexpr uInt32 WRITE_COLOR_MID = 0x9c2782;

    static constexpr uInt32 PC_COLOR_HIGH = 0xfff094;
    static constexpr uInt32 PC_COLOR_MID = 0xbfa62b;

    static constexpr int RAM_SIZE = 128;
    static constexpr uInt16 RAM_BASE = 0x80;
    static constexpr uInt16 ROM_BASE = 0x1000;
    static constexpr int MAX_BANK_HEIGHT = 512;
    static constexpr double ROM_ALPHA_MAX = 196.0;
    static constexpr double RAM_ALPHA_MAX = 128.0;

    static constexpr std::string_view TEXT_RAM = "RAM";
    static constexpr std::string_view TEXT_ROM = "ROM";
    static constexpr std::string_view TEXT_BANK_HEIGHT = "Bank height:";
    static constexpr std::string_view TEXT_SINGLE_ROW = "Single row";
    static constexpr std::string_view TEXT_SEPARATORS = "Separators";
    static constexpr std::string_view TEXT_INVERTED = "Inverted";
    static constexpr std::string_view TEXT_BYTE_FADE = "Byte fade";
    static constexpr std::string_view TEXT_SHOW_DATA = "Show data";
    static constexpr std::string_view TEXT_SHOW_PC = "Show PC";
    static constexpr std::string_view TEXT_SHOW_READS = "Show reads";
    static constexpr std::string_view TEXT_SHOW_WRITES = "Show writes";
    static constexpr std::string_view TEXT_DECAY_RATE = "Decay rate:";
    static constexpr std::string_view TEXT_CLEAR = "Clear";

  public:
    MemViewWindowDialog(OSystem& osystem, DialogContainer& parent,
                        int w, int h);
    ~MemViewWindowDialog() override = default;

    void loadConfig() override;
    void tick() override;

    // The layout tree's own answer, settled by the last layout() (see
    // DebuggerDialog::minSize() for the same pattern)
    Common::Size minSize() const { return myMinSize; }

  protected:
    void layout() override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        BankHeightChanged = GuiCmd::of("MemViewWindowDialog.BankHeightChanged"),
        SingleRowChanged  = GuiCmd::of("MemViewWindowDialog.SingleRowChanged"),
        SeparatorsChanged = GuiCmd::of("MemViewWindowDialog.SeparatorsChanged"),
        InvertedChanged   = GuiCmd::of("MemViewWindowDialog.InvertedChanged"),
        ByteFadeChanged   = GuiCmd::of("MemViewWindowDialog.ByteFadeChanged"),
        ShowDataChanged   = GuiCmd::of("MemViewWindowDialog.ShowDataChanged"),
        ShowPcChanged     = GuiCmd::of("MemViewWindowDialog.ShowPcChanged"),
        ShowReadsChanged  = GuiCmd::of("MemViewWindowDialog.ShowReadsChanged"),
        ShowWritesChanged = GuiCmd::of("MemViewWindowDialog.ShowWritesChanged"),
        DecayRateChanged  = GuiCmd::of("MemViewWindowDialog.DecayRateChanged"),
        ClearPressed      = GuiCmd::of("MemViewWindowDialog.ClearPressed");
    };

    // Apply the visual checkboxes to all views, and the layout settings to
    // the zoomable ones; shared by handleCommand() and loadConfig()
    void updateVisualParameters();
    void updateLayoutParameters();

    // Calculate color gradient based on a bright (high) color and the normal (mid) one to the tab
    static void calcColorDataTab(uInt32 colorHigh, uInt32 colorMid, double alphaMax,
                                 MemViewWidget::ColorTab& tab);

    void updateAccessData(uInt32 cyclesDiff = 0, int elapsedFrames = 0);

  private:
    // loadConfig() also runs on every expose; the persisted-settings read runs once
    bool mySettingsLoaded{false};

    uInt64 myLastCycles{0};
    uInt32 myLastFrames{0};
    bool myLastHadFrameWrap{false};

    // Settled by the last layout(); see minSize() above
    Common::Size myMinSize;

    MemViewWidget* myRamView{nullptr};
    MemViewWidget* myCartRamView{nullptr};
    bool           myBigCartRam{false};
    uInt32         myCartRamSize{0};
    std::multimap<Cartridge::ImageScope, MemViewWidget*> myViews;
    LabelWidget*   myRamLbl{nullptr};
    LabelWidget*   myCartRamLbl{nullptr};
    LabelWidget*   myRomLbl{nullptr};

    // Byte count of each view sharing the area right of the left column; the
    // layout splits that area in the same proportion
    std::map<const MemViewWidget*, uInt32> myMainAreaBytes;

    LabelWidget*    myBankHeightLbl{nullptr};
    PopUpWidget*    myBankHeight{nullptr};
    CheckboxWidget* mySingleRow{nullptr};
    CheckboxWidget* mySeparators{nullptr};
    CheckboxWidget* myInverted{nullptr};
    CheckboxWidget* myByteFade{nullptr};

    CheckboxWidget* myShowData{nullptr};
    CheckboxWidget* myShowPc{nullptr};
    ColorWidget*    myPcColor{nullptr};
    CheckboxWidget* myShowReads{nullptr};
    ColorWidget*    myReadColor{nullptr};
    CheckboxWidget* myShowWrites{nullptr};
    ColorWidget*    myWriteColor{nullptr};

    LabelWidget*  myDecayLbl{nullptr};
    SliderWidget* myDecaySlider{nullptr};
    ButtonWidget* myClearButton{nullptr};

    MemViewWidget::ColorTab myRamReadColorTab{};  // ARGB
    MemViewWidget::ColorTab myRamWriteColorTab{}; // ARGB
    MemViewWidget::ColorTab myRamPcColorTab{};  // ARGB

    MemViewWidget::ColorTab myRomReadColorTab{};  // ARGB
    MemViewWidget::ColorTab& myRomWriteColorTab{myRomReadColorTab}; // currently not needed
    MemViewWidget::ColorTab myRomPcColorTab{};  // ARGB

  private:
    // Following constructors and assignment operators not supported
    MemViewWindowDialog() = delete;
    MemViewWindowDialog(const MemViewWindowDialog&) = delete;
    MemViewWindowDialog(MemViewWindowDialog&&) = delete;
    MemViewWindowDialog& operator=(const MemViewWindowDialog&) = delete;
    MemViewWindowDialog& operator=(MemViewWindowDialog&&) = delete;
};

#endif  // MEM_VIEW_WINDOW_DIALOG_HXX
