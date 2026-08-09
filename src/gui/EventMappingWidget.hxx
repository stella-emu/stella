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

#ifndef EVENT_MAPPING_WIDGET_HXX
#define EVENT_MAPPING_WIDGET_HXX

class CommandSender;
class ButtonWidget;
class EditTextWidget;
class LabelWidget;
class StringListWidget;
class PopUpWidget;
class GuiObject;
class ComboDialog;
class InputDialog;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

/**
  Widget for remapping keyboard/joystick input to game/menu events,
  filtered by event group.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class EventMappingWidget : public Widget, public CommandSender
{
  friend class InputDialog;

  public:
    // Builds the filter pop-up, actions list, the five action buttons, and
    // the selected-mapping readout
    EventMappingWidget(GuiObject* boss, const GUI::Font& font);
    ~EventMappingWidget() override = default;

    bool isRemapping() const { return myRemapStatus; }

    // Reposition/resize all sub-widgets for the given area (font-sensitive)
    void setArea(int x, int y, int w, int h) override;

    // What we ask our tab for: enough width for the actions list and the button
    // column beside it.  We fill whatever height we are given, so we report none
    Common::Size naturalSize() const override;

    // Selects the first filter entry on first use, cancels any stale remap
    // left over from before, and refreshes the actions list
    void loadConfig() override;
    // Resets the mapping for every event in the current filter group to its default
    void setDefaults();

    // While remapping, records key/joystick input as the candidate mapping;
    // press-and-release pairs (axis/hat) are captured on release, since only
    // then is the direction known for certain (see the .cxx for the full detail)
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    bool handleKeyUp(StellaKey key, StellaMod mod) override;
    void handleJoyDown(int stick, int button, bool longPress = false) override;
    void handleJoyUp(int stick, int button) override;
    void handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button) override;
    bool handleJoyHat(int stick, int hat, JoyHatDir hdir, int button) override;

  protected:
    // Reacts to the filter pop-up, a list selection/double-click, the five
    // action buttons, and the combo dialog
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Command ids for the filter pop-up and the five action buttons
    struct Cmd {
      static constexpr GuiCmd::Code
        Filter   = GuiCmd::of("EventMappingWidget.Filter"),
        StartMap = GuiCmd::of("EventMappingWidget.StartMap"),
        StopMap  = GuiCmd::of("EventMappingWidget.StopMap"),
        Erase    = GuiCmd::of("EventMappingWidget.Erase"),
        Reset    = GuiCmd::of("EventMappingWidget.Reset"),
        Combo    = GuiCmd::of("EventMappingWidget.Combo");
    };

    // The actions list shows an event's description, so it needs room for one.
    // How much is this widget's ONE design decision: everything else it needs —
    // and, through naturalSize(), the width of the dialog holding it — follows
    static constexpr int ACTION_CHARS = 40;

    int listWidth() const { return dialog().fontWidth() * ACTION_CHARS; }

    // Refills the actions list for the current filter group and refreshes
    // the selection/mapping display
    void updateActions();
    // Enter/leave remap mode for the selected action (see handleKeyDown() etc.)
    void startRemapping();
    void stopRemapping();
    // Erase/reset the selected action's mapping to empty/default
    void eraseRemapping();
    void resetRemapping();

    // Refreshes the read-only mapping display for the selected action
    void drawKeyMapping();
    // Enables/disables the list and buttons together (false while remapping)
    void enableButtons(bool state);

  private:
    // Map/Cancel/Erase/Reset/Combo action buttons, sharing one column width
    ButtonWidget*     myMapButton{nullptr};
    ButtonWidget*     myCancelMapButton{nullptr};
    ButtonWidget*     myEraseButton{nullptr};
    ButtonWidget*     myResetButton{nullptr};
    ButtonWidget*     myComboButton{nullptr};
    // Filters the actions list to one Event::Group
    LabelWidget*      myFilterPopupLbl{nullptr};
    PopUpWidget*      myFilterPopup{nullptr};
    // Every action in the current filter group, one per row
    StringListWidget* myActionsList{nullptr};
    // The selected action's name, and its current (read-only) mapping display
    LabelWidget*      myActionLbl{nullptr};
    EditTextWidget*   myKeyMapping{nullptr};

    // Popup for assigning a combo event's member events
    unique_ptr<ComboDialog> myComboDialog;

    // Since this widget can be used for different collections of events,
    // we need to specify exactly which group of events we are remapping
    EventMode myEventMode{EventMode::kEmulationMode};

    // Since we can filter events, the event mode is not specific enough
    Event::Group myEventGroup{Event::Group::Emulation};

    // Indicates the event that is currently selected
    int myActionSelected{-1};

    // Indicates if we're currently in remap mode
    // In this mode, the next event received is remapped to some action
    bool myRemapStatus{false};

    // Joystick axes and hats can be more problematic than ordinary buttons
    // or keys, in that there can be 'drift' in the values
    // Therefore, we map these events when they've been 'released', rather
    // than on their first occurrence (aka, when they're 'pressed')
    // As a result, we need to keep track of their old values
    int myLastStick{JOY_CTRL_NONE};
    int myLastHat{JOY_CTRL_NONE};
    JoyAxis myLastAxis{JoyAxis::NONE};
    JoyDir myLastDir{JoyDir::NONE};
    JoyHatDir myLastHatDir{JoyHatDir::CENTER};

    // Aggregates the modifier flags of the mapping
    StellaMod myMod{StellaMod::NONE};
    // Saves the last *pressed* key
    StellaKey myLastKey{StellaKey::UNKNOWN};
    // Saves the last *pressed* button
    int myLastButton{JOY_CTRL_NONE};

    // True until loadConfig() has selected the initial filter entry once
    bool myFirstTime{true};

  private:
    // Clears the in-progress candidate mapping (see the handle* methods above)
    void resetLastEvent() {
      myLastStick  = myLastHat = JOY_CTRL_NONE;
      myLastButton = JOY_CTRL_NONE;
      myLastAxis   = JoyAxis::NONE;
      myLastDir    = JoyDir::NONE;
      myLastHatDir = JoyHatDir::CENTER;
      myLastKey    = StellaKey::UNKNOWN;
      myMod        = StellaMod::NONE;
    }

  private:
    // Following constructors and assignment operators not supported
    EventMappingWidget() = delete;
    EventMappingWidget(const EventMappingWidget&) = delete;
    EventMappingWidget(EventMappingWidget&&) = delete;
    EventMappingWidget& operator=(const EventMappingWidget&) = delete;
    EventMappingWidget& operator=(EventMappingWidget&&) = delete;
};

#endif  // EVENT_MAPPING_WIDGET_HXX
