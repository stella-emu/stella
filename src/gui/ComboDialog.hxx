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

#ifndef COMBO_DIALOG_HXX
#define COMBO_DIALOG_HXX

class PopUpWidget;
class LabelWidget;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

/**
  Dialog for assigning up to 8 events to one of the combo
  (Combo1-Combo16) events.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class ComboDialog : public Dialog
{
  public:
    // 'combolist' is the set of events any of the 8 slots may be assigned
    ComboDialog(GuiObject* boss, const GUI::Font& font, const VariantList& combolist);
    ~ComboDialog() override = default;

    /** Place the dialog onscreen and center it */
    void show(Event::Type event, string_view name);

    // Populates the 8 pop-ups from myComboEvent's current combo list
    void loadConfig() override;
    // Writes the 8 pop-ups' selections back as myComboEvent's combo list
    void saveConfig() override;
    // Resets all 8 slots to "None"
    void setDefaults() override;

  protected:
    // OK saves and closes; Defaults resets every slot
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void layout() override;

  private:
    // Which Combo1-Combo16 event this dialog is currently editing (see show())
    Event::Type myComboEvent{Event::NoType};
    // The 8 label+pop-up rows for the combo's member events
    std::array<LabelWidget*, 8> myEventLabels{nullptr};
    std::array<PopUpWidget*, 8> myEvents{nullptr};

  private:
    // Following constructors and assignment operators not supported
    ComboDialog() = delete;
    ComboDialog(const ComboDialog&) = delete;
    ComboDialog(ComboDialog&&) = delete;
    ComboDialog& operator=(const ComboDialog&) = delete;
    ComboDialog& operator=(ComboDialog&&) = delete;
};

#endif  // COMBO_DIALOG_HXX
