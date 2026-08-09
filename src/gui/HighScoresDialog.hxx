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

#ifndef HIGH_SCORES_DIALOG_HXX
#define HIGH_SCORES_DIALOG_HXX

class CommandSender;
class DialogContainer;
class OSystem;
class EditTextWidget;
class PopUpWidget;
class Serializer;

#include "Dialog.hxx"
#include "HighScoresManager.hxx"
#include "json/json_lib.hxx"

using json = nlohmann::json;

/**
  The dialog for displaying high scores in Stella.

  @author  Thomas Jentzsch
*/

class HighScoresDialog : public Dialog
{
  public:
    // Rows in the score table
    static constexpr uInt32 NUM_RANKS = 10;

    HighScoresDialog(OSystem& osystem, DialogContainer& parent, AppMode mode);
    ~HighScoresDialog() override;

    // Populates the variation list, current scores, and MD5/notes readouts
    void loadConfig() override;
    // Saves the edited initials (if any) and the current scores
    void saveConfig() override;

  protected:
    // OK saves and exits menu mode/closes; Reset clears the variation;
    // a delete button removes its rank; variation change reloads its scores
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void layout() override;

    // Refreshes every rank row from myScores; 'init' also seeds the name editor
    void updateWidgets(bool init = false);
    // Switches to the variation currently selected in the pop-up, prompting to
    // save first if the previous one is dirty (see handleDirty())
    void handleVariation(bool init = false);
    // Inserts the just-finished game's score into the rank table, if it qualifies
    void handlePlayedVariation();

    // Removes 'rank', shifting the ranks below it up
    void deleteRank(int rank);
    // Returns true if it's safe to proceed; false while an (async) save-changes
    // confirmation is pending
    bool handleDirty();

    // The current cart's display name: from the running console, else looked
    // up by MD5 in the properties set, falling back to the launcher's current
    // directory name
    string cartName() const;
    // Current time, formatted for the score table's date field
    static string now();

    // Command ids for the variation controls and delete buttons, dispatched
    // in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        VariationChanged = GuiCmd::of("HighScoresDialog.VariationChanged"),
        PrevVariation    = GuiCmd::of("HighScoresDialog.PrevVariation"),
        NextVariation    = GuiCmd::of("HighScoresDialog.NextVariation"),
        DeleteSingle     = GuiCmd::of("HighScoresDialog.DeleteSingle");
    };

  private:
    bool myUserDefVar{false}; // allow the user to define the variation
    // True once scores have changed since the last save; handleDirty() then
    // prompts to save before switching variation/closing
    bool myDirty{false};
    bool myHighScoreSaved{false};  // remember if current high score was already saved
                                   // (avoids double HS)
    // Player initials; remembered across sessions via the 'initials' setting
    string myInitials;
    // Rank currently showing its editable name field, or -1
    Int32 myEditRank{-1};
    // Rank the just-played score was inserted at, or -1
    Int32 myHighScoreRank{-1};
    // Timestamp captured at loadConfig(), stamped on a new high score
    string myNow;

    // The current variation's score table
    HSM::ScoresData myScores;

    // Cartridge name shown at the top of the dialog (see cartName())
    LabelWidget* myGameNameWidget{nullptr};

    // Variation selector, and the prev/next buttons beside it
    LabelWidget*  myVariationLbl{nullptr};
    PopUpWidget*  myVariationPopup{nullptr};
    ButtonWidget* myPrevVarButton{nullptr};
    ButtonWidget* myNextVarButton{nullptr};

    // Score-table column headers
    LabelWidget* myRankLbl{nullptr};
    LabelWidget* myScoreLbl{nullptr};
    LabelWidget* mySpecialLbl{nullptr};
    LabelWidget* myNameLbl{nullptr};
    LabelWidget* myDateLbl{nullptr};

    // One row of widgets per rank, parallel to myScores.scores; myEditNameWidgets
    // swaps in for myNameWidgets on whichever rank is myEditRank
    LabelWidget*    myRankWidgets[NUM_RANKS]{nullptr};
    LabelWidget*    myScoreWidgets[NUM_RANKS]{nullptr};
    LabelWidget*    mySpecialWidgets[NUM_RANKS]{nullptr};
    LabelWidget*    myNameWidgets[NUM_RANKS]{nullptr};
    EditTextWidget* myEditNameWidgets[NUM_RANKS]{nullptr};
    LabelWidget*    myDateWidgets[NUM_RANKS]{nullptr};
    ButtonWidget*   myDeleteButtons[NUM_RANKS]{nullptr};

    // Game notes, and the MD5/properties-checksum readouts below the table
    LabelWidget* myNotesWidget{nullptr};
    LabelWidget* myMD5Widget{nullptr};
    LabelWidget* myCheckSumWidget{nullptr};

    // Whether this dialog was opened from the launcher or during emulation
    // (governs Close vs. leaveMenuMode() in handleCommand())
    AppMode myMode{AppMode::emulator};

  private:
    // Following constructors and assignment operators not supported
    HighScoresDialog() = delete;
    HighScoresDialog(const HighScoresDialog&) = delete;
    HighScoresDialog(HighScoresDialog&&) = delete;
    HighScoresDialog& operator=(const HighScoresDialog&) = delete;
    HighScoresDialog& operator=(HighScoresDialog&&) = delete;
};

#endif  // HIGH_SCORES_DIALOG_HXX
