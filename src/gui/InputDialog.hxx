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

#ifndef INPUT_DIALOG_HXX
#define INPUT_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class EventMappingWidget;
class CheckboxWidget;
class JoystickDialog;
class PopUpWidget;
class SliderWidget;
class LabelWidget;

#include "Dialog.hxx"
#include "bspf.hxx"

/**
  Dialog for configuring input devices: event mapping, joystick/paddle/
  trackball settings, and mouse control, across three tabs.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class InputDialog : public Dialog
{
  public:
    // Builds the three tabs: Event Mappings, Devices & Ports, Mouse
    InputDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    // Out-of-line: myJoyDialog (unique_ptr<JoystickDialog>) needs its complete type here
    ~InputDialog() override;

    void loadConfig() override;
    void saveConfig() override;
    // Resets only the currently active tab to its defaults
    void setDefaults() override;

  protected:
    void layout() override;

    // disable repeat during and directly after mapping events
    bool repeatEnabled() override;

    // While the event mapper is remapping, input goes to it instead of the
    // usual dialog navigation
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleKeyUp(StellaKey key, StellaMod mod) override;
    void handleJoyDown(int stick, int button, bool longPress) override;
    void handleJoyUp(int stick, int button) override;
    void handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button) override;
    bool handleJoyHat(int stick, int hat, JoyHatDir hdir, int button) override;
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Build one tab's controls and layout; called once from the ctor
    void addDevicePortTab();
    void addMouseTab();

    // Enable/disable controls that depend on the mouse-control / cursor-state pop-ups
    void handleMouseControlState();
    void handleCursorState();
    // Update a dejitter slider's value label ("Off" at 0)
    void updateDejitterAveraging();
    void updateDejitterReaction();
    // Enables/disables the rate slider with the Autofire checkbox, and updates its label
    void updateAutoFireRate();
    // Erases the AtariVox/SaveKey EEPROM on whichever port has one
    void eraseEEPROM();

  private:
    // Command ids for the sliders/pop-ups/buttons across both tabs, dispatched
    // in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        DigitalDeadzoneChanged    = GuiCmd::of("InputDialog.DigitalDeadzoneChanged"),
        AnalogDeadzoneChanged     = GuiCmd::of("InputDialog.AnalogDeadzoneChanged"),
        PaddleSpeedChanged        = GuiCmd::of("InputDialog.PaddleSpeedChanged"),
        DejitterBaseChanged       = GuiCmd::of("InputDialog.DejitterBaseChanged"),
        DejitterDiffChanged       = GuiCmd::of("InputDialog.DejitterDiffChanged"),
        DigitalPaddleSpeedChanged = GuiCmd::of("InputDialog.DigitalPaddleSpeedChanged"),
        AutoFireChanged           = GuiCmd::of("InputDialog.AutoFireChanged"),
        AutoFireRate              = GuiCmd::of("InputDialog.AutoFireRate"),
        TrackBallSpeedChanged     = GuiCmd::of("InputDialog.TrackBallSpeedChanged"),
        DrivingSpeedChanged       = GuiCmd::of("InputDialog.DrivingSpeedChanged"),
        ControllerDatabase        = GuiCmd::of("InputDialog.ControllerDatabase"),
        EraseEeprom               = GuiCmd::of("InputDialog.EraseEeprom"),
        MouseControlChanged       = GuiCmd::of("InputDialog.MouseControlChanged"),
        CursorStateChanged        = GuiCmd::of("InputDialog.CursorStateChanged"),
        MousePaddleSpeedChanged   = GuiCmd::of("InputDialog.MousePaddleSpeedChanged");
    };

    TabWidget* myTab{nullptr};

    // Tab 1: Event Mappings (a self-contained composite; see addTab() in the .cxx)
    EventMappingWidget* myEventMapper{nullptr};

    // Tab 2: Devices & Ports
    CheckboxWidget*   mySAPort{nullptr};

    LabelWidget* myAVoxPortLbl{nullptr};
    PopUpWidget*      myAVoxPort{nullptr};

    LabelWidget*    myDigitalDeadzoneLbl{nullptr};
    SliderWidget*   myDigitalDeadzone{nullptr};
    LabelWidget*    myAnalogDeadzoneLbl{nullptr};
    SliderWidget*   myAnalogDeadzone{nullptr};
    LabelWidget*    myPaddleSpeedLbl{nullptr};
    SliderWidget*   myPaddleSpeed{nullptr};
    LabelWidget*    myPaddleLinearityLbl{nullptr};
    SliderWidget*   myPaddleLinearity{nullptr};
    LabelWidget*    myDejitterBaseLbl{nullptr};
    SliderWidget*   myDejitterBase{nullptr};
    LabelWidget*    myDejitterDiffLbl{nullptr};
    SliderWidget*   myDejitterDiff{nullptr};
    LabelWidget*    myDPaddleSpeedLbl{nullptr};
    SliderWidget*   myDPaddleSpeed{nullptr};
    LabelWidget*    myAnalogPaddleLbl{nullptr};
    CheckboxWidget* myAutoFire{nullptr};
    LabelWidget*    myAutoFireRateLbl{nullptr};
    SliderWidget*   myAutoFireRate{nullptr};
    CheckboxWidget* myAllowAll4{nullptr};
    CheckboxWidget* myModCombo{nullptr};

    LabelWidget*    myAtariVoxLbl{nullptr};
    ButtonWidget*   myJoyDlgButton{nullptr};
    ButtonWidget*   myEraseEEPROMButton{nullptr};

    // Tab 3: Mouse
    LabelWidget*    myMouseControlLbl{nullptr};
    PopUpWidget*    myMouseControl{nullptr};
    LabelWidget*    myMouseSensitivity{nullptr};
    LabelWidget*    myMPaddleSpeedLbl{nullptr};
    SliderWidget*   myMPaddleSpeed{nullptr};
    LabelWidget*    myTrackBallSpeedLbl{nullptr};
    SliderWidget*   myTrackBallSpeed{nullptr};
    LabelWidget*    myDrivingSpeedLbl{nullptr};
    SliderWidget*   myDrivingSpeed{nullptr};
    LabelWidget*    myCursorStateLbl{nullptr};
    PopUpWidget*    myCursorState{nullptr};
    CheckboxWidget* myGrabMouse{nullptr};

    // Show the list of joysticks that the eventhandler knows about
    unique_ptr<JoystickDialog> myJoyDialog;

  private:
    // Following constructors and assignment operators not supported
    InputDialog() = delete;
    InputDialog(const InputDialog&) = delete;
    InputDialog(InputDialog&&) = delete;
    InputDialog& operator=(const InputDialog&) = delete;
    InputDialog& operator=(InputDialog&&) = delete;
};

#endif  // INPUT_DIALOG_HXX
