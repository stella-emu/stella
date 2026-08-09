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

#ifndef DEVELOPER_DIALOG_HXX
#define DEVELOPER_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class CheckboxWidget;
class PopUpWidget;
class RadioButtonGroup;
class RadioButtonWidget;
class SliderWidget;
class LabelWidget;
class ColorWidget;

namespace GUI {
  class Font;
}  // namespace GUI

#include "bspf.hxx"
#include "Dialog.hxx"
#include "DevSettingsHandler.hxx"

/**
  Dialog for editing developer/debug settings, with separate 'player'
  and 'developer' setting sets, only one of which is active at a time.

  @author  Thomas Jentzsch
*/
class DeveloperDialog : public Dialog, DevSettingsHandler
{
  public:
    DeveloperDialog(OSystem& osystem, DialogContainer& parent,
                    const GUI::Font& font);
    // Out-of-line: mySettingsGroupEmulation/mySettingsGroupTia/
    // mySettingsGroupVideo/mySettingsGroupTM (unique_ptr<RadioButtonGroup>)
    // need their complete type here
    ~DeveloperDialog() override;

    // Loads both the player and developer setting sets, then shows
    // whichever is currently active
    void loadConfig() override;
    // Saves both setting sets and activates the one currently selected
    void saveConfig() override;
    // Resets the currently active tab's settings (in the active set) to
    // their player/developer default
    void setDefaults() override;

  protected:
    void layout() override;
    // Cmd::PlayerSettings/Cmd::DeveloperSettings switch the active setting set; the TIA/
    // console/jitter/state-buffer ids dispatch to their handle*() methods;
    // the debug-colour ids keep the six swatches distinct; OK saves and
    // exits (informing the debugger of a font change); Defaults resets
    // the active tab
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        PlayerSettings          = GuiCmd::of("DeveloperDialog.PlayerSettings"),
        DeveloperSettings       = GuiCmd::of("DeveloperDialog.DeveloperSettings"),
        Console                 = GuiCmd::of("DeveloperDialog.Console"),
        ArmSpeedChanged         = GuiCmd::of("DeveloperDialog.ArmSpeedChanged"),
        TiaType                 = GuiCmd::of("DeveloperDialog.TiaType"),
        TvJitter                = GuiCmd::of("DeveloperDialog.TvJitter"),
        TimeMachine             = GuiCmd::of("DeveloperDialog.TimeMachine"),
        SizeChanged             = GuiCmd::of("DeveloperDialog.SizeChanged"),
        UncompressedChanged     = GuiCmd::of("DeveloperDialog.UncompressedChanged"),
        IntervalChanged         = GuiCmd::of("DeveloperDialog.IntervalChanged"),
        HorizonChanged          = GuiCmd::of("DeveloperDialog.HorizonChanged"),
        Player0ColourChanged    = GuiCmd::of("DeveloperDialog.Player0ColourChanged"),
        Missile0ColourChanged   = GuiCmd::of("DeveloperDialog.Missile0ColourChanged"),
        Player1ColourChanged    = GuiCmd::of("DeveloperDialog.Player1ColourChanged"),
        Missile1ColourChanged   = GuiCmd::of("DeveloperDialog.Missile1ColourChanged"),
        PlayfieldColourChanged  = GuiCmd::of("DeveloperDialog.PlayfieldColourChanged"),
        BallColourChanged       = GuiCmd::of("DeveloperDialog.BallColourChanged"),
        DebuggerFontSizeChanged = GuiCmd::of("DeveloperDialog.DebuggerFontSizeChanged");
    };

    // Number of TIA objects that get a fixed debug color (P0/M0/P1/M1/PF/BL)
    static constexpr int DEBUG_COLORS = 6;

    // Hosts the five settings tabs (Emulation, TIA, Video, Time Machine,
    // Debugger)
    TabWidget* myTab{nullptr};
    // Per-tab "Player/Developer settings" radio pairs + bottom info labels
    // (promoted from anonymous locals so layout() can position them)
    std::array<RadioButtonWidget*, 2> myEmuSettings{nullptr};
    std::array<RadioButtonWidget*, 2> myTiaSettings{nullptr};
    std::array<RadioButtonWidget*, 2> myVideoSettings{nullptr};
    std::array<RadioButtonWidget*, 2> myTMSettings{nullptr};
    LabelWidget* myEmuInfo{nullptr};
    LabelWidget* myVideoInfo{nullptr};
    LabelWidget* myTMInfo{nullptr};
    LabelWidget* myDebuggerInfo{nullptr};

    // Emulator widgets
    unique_ptr<RadioButtonGroup> mySettingsGroupEmulation;
    CheckboxWidget*     myFrameStatsWidget{nullptr};
    CheckboxWidget*     myDetectedInfoWidget{nullptr};
    CheckboxWidget*     myExternAccessWidget{nullptr};
    CheckboxWidget*     myPlusRomWidget{nullptr};
    LabelWidget*        myConsoleWidgetLbl{nullptr};
    PopUpWidget*        myConsoleWidget{nullptr};
    LabelWidget*        myLoadingROMLbl{nullptr};
    CheckboxWidget*     myRandomBankWidget{nullptr};
    CheckboxWidget*     myRandomizeTIAWidget{nullptr};
    CheckboxWidget*     myRandomizeRAMWidget{nullptr};
    LabelWidget*        myRandomizeCPULbl{nullptr};
    std::array<CheckboxWidget*, 5> myRandomizeCPUWidget{nullptr};
    CheckboxWidget*     myRandomHotspotsWidget{nullptr};
    CheckboxWidget*     myUndrivenPinsWidget{nullptr};
#ifdef DEBUGGER_SUPPORT
    LabelWidget*        myPortBreakLbl{nullptr};
    CheckboxWidget*     myRWPortBreakWidget{nullptr};
    CheckboxWidget*     myWRPortBreakWidget{nullptr};
#endif
    CheckboxWidget*     myThumbExceptionWidget{nullptr};
    LabelWidget*        myArmSpeedWidgetLbl{nullptr};
    SliderWidget*       myArmSpeedWidget{nullptr};

    // TIA widgets
    unique_ptr<RadioButtonGroup> mySettingsGroupTia;
    LabelWidget*        myTIATypeWidgetLbl{nullptr};
    PopUpWidget*        myTIATypeWidget{nullptr};

    LabelWidget*        myInvPhaseLbl{nullptr};
    CheckboxWidget*     myPlInvPhaseWidget{nullptr};
    CheckboxWidget*     myMsInvPhaseWidget{nullptr};
    CheckboxWidget*     myBlInvPhaseWidget{nullptr};

    LabelWidget*        myLateHMoveLbl{nullptr};
    CheckboxWidget*     myPlLateHMoveWidget{nullptr};
    CheckboxWidget*     myMsLateHMoveWidget{nullptr};
    CheckboxWidget*     myBlLateHMoveWidget{nullptr};

    LabelWidget*        myLateRespxLbl{nullptr};
    CheckboxWidget*     myPlLateRespxWidget{nullptr};
    CheckboxWidget*     myMsLateRespxWidget{nullptr};
    CheckboxWidget*     myBlLateRespxWidget{nullptr};

    LabelWidget*        myPlayfieldLbl{nullptr};
    CheckboxWidget*     myPFBitsWidget{nullptr};
    CheckboxWidget*     myPFColorWidget{nullptr};
    CheckboxWidget*     myPFScoreWidget{nullptr};

    LabelWidget*        myBackgroundLbl{nullptr};
    CheckboxWidget*     myBKColorWidget{nullptr};
    LabelWidget*        mySwapLbl{nullptr};
    CheckboxWidget*     myPlSwapWidget{nullptr};
    CheckboxWidget*     myBlSwapWidget{nullptr};

    // Video widgets
    unique_ptr<RadioButtonGroup> mySettingsGroupVideo;
    CheckboxWidget*     myTVJitterWidget{nullptr};
    LabelWidget*        myTVJitterRecWidgetLbl{nullptr};
    SliderWidget*       myTVJitterRecWidget{nullptr};
    LabelWidget*        myTVJitterSenseWidgetLbl{nullptr};
    SliderWidget*       myTVJitterSenseWidget{nullptr};
    CheckboxWidget*     myColorLossWidget{nullptr};
    CheckboxWidget*     myDebugColorsWidget{nullptr};
    std::array<LabelWidget*, DEBUG_COLORS> myDbgColourLbl{nullptr};
    std::array<PopUpWidget*, DEBUG_COLORS> myDbgColour{nullptr};
    std::array<ColorWidget*, DEBUG_COLORS> myDbgColourSwatch{nullptr};

    // States widgets
    unique_ptr<RadioButtonGroup> mySettingsGroupTM;
    CheckboxWidget*     myTimeMachineWidget{nullptr};
    LabelWidget*        myStateSizeWidgetLbl{nullptr};
    SliderWidget*       myStateSizeWidget{nullptr};
    LabelWidget*        myUncompressedWidgetLbl{nullptr};
    SliderWidget*       myUncompressedWidget{nullptr};
    LabelWidget*        myStateIntervalWidgetLbl{nullptr};
    PopUpWidget*        myStateIntervalWidget{nullptr};
    LabelWidget*        myStateHorizonWidgetLbl{nullptr};
    PopUpWidget*        myStateHorizonWidget{nullptr};

#ifdef DEBUGGER_SUPPORT
    // Debugger UI widgets
    LabelWidget*        myDebuggerFontSizeLbl{nullptr};
    PopUpWidget*        myDebuggerFontSize{nullptr};
    LabelWidget*        myDebuggerFontStyleLbl{nullptr};
    PopUpWidget*        myDebuggerFontStyle{nullptr};
    CheckboxWidget*     myGhostReadsTrapWidget{nullptr};
#endif

    // True when the 'Developer settings' set is active (mirrors
    // mySettingsGroup*'s selection)
    bool mySettings{false};
    // Single-letter codes for the 5 CPU registers, as stored in the
    // myRandomizeCPU setting string
    static constexpr std::array<string_view, 5> ourCPURegs = {
      "S", "A", "X", "Y", "P"
    };

  private:
    // Builds the 'Emulation' tab: console info/PlusROM, startup
    // randomization, undriven-pin/port-break behaviour, ARM speed limit
    void addEmulationTab(const GUI::Font& font);
    // Builds the 'Time Machine' tab: enable, buffer size/uncompressed
    // size, interval, horizon
    void addTimeMachineTab(const GUI::Font& font);
    // Builds the 'TIA' tab: chip type, and the per-object timing/color
    // quirks it can emulate
    void addTiaTab(const GUI::Font& font);
    // Builds the 'Video' tab: TV jitter, PAL color-loss, and the six
    // fixed debug colors
    void addVideoTab(const GUI::Font& font);
    // Builds the 'Debugger' tab: font size/style and the ghost-reads trap
    // (or a placeholder message when built without debugger support)
    void addDebuggerTab(const GUI::Font& font);

    // Copies every tab's current widget values into the given setting set
    void getWidgetStates(SettingsSet set);
    // Loads the given setting set's values into every tab's widgets, then
    // refreshes their enabled state
    void setWidgetStates(SettingsSet set);

    // Switches the active setting set (player/developer), saving the
    // previous set's widget values first
    void handleSettings(bool devSettings);
    // Enables the jitter sensitivity/recovery sliders only while jitter
    // is on
    void handleTVJitterChange();
    // Disables RAM randomization for the 7800, which lacks the RAM this
    // randomizes
    void handleConsole();

    // Enables the per-object quirk widgets only for 'Custom' chip type,
    // and either restores their developer-set values or shows the fixed
    // quirks a known faulty-chip type implies
    void handleTia();

    // Sets debug-color 'idx' to 'color' (for the console's current TV
    // timing), swapping it with whichever other index already used that
    // color
    void handleDebugColours(int idx, int color);
    // Applies a 6-character 'roygpb'-style color string, one call to the
    // int overload per character
    void handleDebugColours(string_view colors);

    // Enables the buffer-size/interval controls only while Time Machine
    // is on, and the horizon control only when the buffer holds more
    // than its uncompressed portion
    void handleTimeMachine();
    // After a buffer-size change, shrinks uncompressed size to fit and
    // finds the largest interval/horizon combination the new size supports
    void handleSize();
    // After an uncompressed-size change, grows the buffer size to fit it
    // if needed
    void handleUncompressed();
    // After an interval change, shrinks the buffer size until the
    // interval/horizon combination fits, then clamps uncompressed size to it
    void handleInterval();
    // After a horizon change, shrinks the buffer size until the
    // interval/horizon combination fits, then clamps uncompressed size to it
    void handleHorizon();

  private:
    // Following constructors and assignment operators not supported
    DeveloperDialog() = delete;
    DeveloperDialog(const DeveloperDialog&) = delete;
    DeveloperDialog(DeveloperDialog&&) = delete;
    DeveloperDialog& operator=(const DeveloperDialog&) = delete;
    DeveloperDialog& operator=(DeveloperDialog&&) = delete;
};

#endif  // DEVELOPER_DIALOG_HXX
