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
    static constexpr uInt32 NUM_RANKS = 10;

    HighScoresDialog(OSystem& osystem, DialogContainer& parent, AppMode mode);
    ~HighScoresDialog() override;

    void loadConfig() override;
    void saveConfig() override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void layout() override;

    void updateWidgets(bool init = false);
    void handleVariation(bool init = false);
    void handlePlayedVariation();

    void deleteRank(int rank);
    bool handleDirty();

    string cartName() const;
    static string now();

    enum {
      kVariationChanged = 'Vach',
      kPrevVariation    = 'PrVr',
      kNextVariation    = 'NxVr',
      kDeleteSingle     = 'DeSi'
    };

  private:
    bool myUserDefVar{false}; // allow the user to define the variation
    bool myDirty{false};
    bool myHighScoreSaved{false};  // remember if current high score was already saved
                                   // (avoids double HS)
    string myInitials;
    Int32 myEditRank{-1};
    Int32 myHighScoreRank{-1};
    string myNow;

    HSM::ScoresData myScores;

    StaticTextWidget* myGameNameWidget{nullptr};

    StaticTextWidget* myVariationLabel{nullptr};
    PopUpWidget*      myVariationPopup{nullptr};
    ButtonWidget*     myPrevVarButton{nullptr};
    ButtonWidget*     myNextVarButton{nullptr};

    // Score-table column headers
    StaticTextWidget* myRankLabel{nullptr};
    StaticTextWidget* myScoreLabel{nullptr};
    StaticTextWidget* mySpecialLabelWidget{nullptr};
    StaticTextWidget* myNameLabel{nullptr};
    StaticTextWidget* myDateLabel{nullptr};

    StaticTextWidget* myRankWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* myScoreWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* mySpecialWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* myNameWidgets[NUM_RANKS]{nullptr};
    EditTextWidget*   myEditNameWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* myDateWidgets[NUM_RANKS]{nullptr};
    ButtonWidget*     myDeleteButtons[NUM_RANKS]{nullptr};

    StaticTextWidget* myNotesWidget{nullptr};
    StaticTextWidget* myMD5Widget{nullptr};
    StaticTextWidget* myCheckSumWidget{nullptr};

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
