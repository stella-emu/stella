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

#ifndef DEBUGGER_DIALOG_HXX
#define DEBUGGER_DIALOG_HXX

class Debugger;
class OSystem;
class DialogContainer;
class FSNode;
class ButtonWidget;
class CpuWidget;
class PromptWidget;
class RamWidget;
class RomWidget;
class TabWidget;
class EditTextWidget;
class TiaInfoWidget;
class TiaOutputWidget;
class TiaZoomWidget;
class CartDebugWidget;
class CartRamWidget;
class DataGridOpsWidget;
class TiaWidget;
class RiotWidget;
class AudioWidget;
class OptionsDialog;

namespace GUI {
  class Layout;
}  // namespace GUI

#include "Dialog.hxx"

class DebuggerDialog : public Dialog
{
  public:
    DebuggerDialog(OSystem& osystem, DialogContainer& parent,
                   int w, int h);
    ~DebuggerDialog() override;

    const GUI::Font& lfont() const;
    const GUI::Font& nfont() const;
    const GUI::Font& dfont() const;
    PromptWidget& prompt() const       { return *myPrompt;       }
    TiaInfoWidget& a() const           { return *myTiaInfo;      }
    TiaOutputWidget& tiaOutput() const { return *myTiaOutput;    }
    TiaZoomWidget& tiaZoom() const     { return *myTiaZoom;      }
    RomWidget& rom() const             { return *myRom;          }
    CartDebugWidget* cartDebug() const { return myCartDebug;    }
    CartRamWidget& cartRam() const     { return *myCartRam;      }
    EditTextWidget& message() const    { return *myMessageBox;   }
    ButtonWidget& rewindButton() const { return *myRewindButton; }
    ButtonWidget& unwindButton() const { return *myUnwindButton; }

    void showFatalMessage(string_view msg);
    void loadConfig() override;
    void saveConfig() override;

    // Ask the FontManager to swap the label/normal font descriptors in place,
    // so every widget's reference to them picks up the new glyphs and metrics
    // without being recreated.
    void changeFont();

    // Refresh cached font-derived widget state and re-flow after changeFont()
    // -- like Dialog::refreshFont(), but also re-fonts the tooltip to nfont()
    // (the debugger's own font, not the shared dialog font used by _font).
    void refreshFont() override;

    void setPosition() override { positionAt(0); }

    /**
      The smallest window in which nothing clips: answered by the layout tree
      built in layout(), so it accounts for the proportional TIA band, the
      centre split and whatever the current ROM's tabs ask for, without anyone
      restating that geometry here.  Zero until the dialog has laid out once.
    */
    Common::Size minSize() const { return myMinSize; }

  protected:
    void layout() override;

    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleKeyUp(StellaKey key, StellaMod mod) override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    void doStep();
    void doTrace();
    void doScanlineAdvance();
    void doAdvance();
    void doRewind();
    void doUnwind();
    void doRewind10();
    void doUnwind10();
    void doRewindAll();
    void doUnwindAll();
    void doExitDebugger();
    void doExitRom();

    void addTiaArea();
    void addTabArea();
    void addStatusArea();
    void addRomArea();

    /**
      The whole dialog as the engine sees it: the centre split, the TIA band
      above the tabs, and the two areas' own columns.  Built without positioning
      anything, so layout() and the window minimum are the same tree asked two
      questions -- which is what keeps the minimum independent of the current
      size, and of anyone's idea of where the areas are.
    */
    unique_ptr<GUI::Layout> buildLayout();
    unique_ptr<GUI::Layout> buildTopBand();
    unique_ptr<GUI::Layout> buildStatusArea();
    unique_ptr<GUI::Layout> buildRomArea();

    /**
      The TIA image band's height, and the image width its proportion then calls
      for.  The band is declared to the tree as TIA_BAND_PERCENT with a floor of
      TIA_BAND_MIN, so these two read the same law the tree enforces; the image
      is the one leaf whose width follows from a height, which no cell policy can
      state for it.
    */
    int tiaBandHeight() const;
    int tiaImageWidth() const;

  private:
    struct Cmd {
      static constexpr GuiCmd::Code
        Step            = GuiCmd::of("DebuggerDialog.Step"),
        Trace           = GuiCmd::of("DebuggerDialog.Trace"),
        AdvanceFrame    = GuiCmd::of("DebuggerDialog.AdvanceFrame"),
        AdvanceScanline = GuiCmd::of("DebuggerDialog.AdvanceScanline"),
        Rewind          = GuiCmd::of("DebuggerDialog.Rewind"),
        Unwind          = GuiCmd::of("DebuggerDialog.Unwind"),
        Run             = GuiCmd::of("DebuggerDialog.Run"),
        Options         = GuiCmd::of("DebuggerDialog.Options");
    };

    TabWidget *myTab{nullptr}, *myRomTab{nullptr};

    PromptWidget*    myPrompt{nullptr};
    TiaWidget*       myTiaTab{nullptr};
    RiotWidget*      myRiotTab{nullptr};
    AudioWidget*     myAudioTab{nullptr};
    TiaInfoWidget*   myTiaInfo{nullptr};
    TiaOutputWidget* myTiaOutput{nullptr};
    TiaZoomWidget*   myTiaZoom{nullptr};
    CpuWidget*       myCpu{nullptr};
    RamWidget*       myRam{nullptr};
    RomWidget*       myRom{nullptr};
    CartDebugWidget* myCartInfo{nullptr};
    CartDebugWidget* myCartDebug{nullptr};
    CartRamWidget*   myCartRam{nullptr};
    EditTextWidget*  myMessageBox{nullptr};
    ButtonWidget*    myRewindButton{nullptr};
    ButtonWidget*    myUnwindButton{nullptr};
    ButtonWidget*    myOptionsButton{nullptr};
    DataGridOpsWidget* myDataGridOps{nullptr};

    // Step / Trace / Scan +1 / Frame +1 / Run
    std::array<ButtonWidget*, 5> myStepButtons{};

    unique_ptr<OptionsDialog>   myOptions;

    Widget* myFocusedWidget{nullptr};
    bool myExitPressed{false};

    // What the layout tree last reported it cannot be squeezed below
    Common::Size myMinSize;

    // The TIA image band's share of the dialog height, and the floor below which
    // it does not shrink -- a full PAL frame, so the image is never scaled down
    static constexpr int TIA_BAND_PERCENT = 35;

  private:
    // Following constructors and assignment operators not supported
    DebuggerDialog() = delete;
    DebuggerDialog(const DebuggerDialog&) = delete;
    DebuggerDialog(DebuggerDialog&&) = delete;
    DebuggerDialog& operator=(const DebuggerDialog&) = delete;
    DebuggerDialog& operator=(DebuggerDialog&&) = delete;
};

#endif  // DEBUGGER_DIALOG_HXX
