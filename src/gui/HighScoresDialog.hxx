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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef HIGHSCORE_DIALOG_HXX
#define HIGHSCORE_DIALOG_HXX

#define HIGHSCORE_HEADER "06000000highscores"

//class Properties;
class CommandSender;
class DialogContainer;
class OSystem;
class EditTextWidget;
class PopUpWidget;
namespace GUI {
  class MessageBox;
}
class Serializer;

#include "Menu.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "json_lib.hxx"

using json = nlohmann::json;


/**
  The dialog for displaying high scores in Stella.

  @author  Thomas Jentzsch
*/

class HighScoresDialog : public Dialog
{
  public:
    static const uInt32 NUM_RANKS = 10;

    HighScoresDialog(OSystem& osystem, DialogContainer& parent,
                     int max_w, int max_h, Menu::AppMode mode);
    virtual ~HighScoresDialog();

  protected:
    void loadConfig() override;
    void saveConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void updateWidgets(bool init = false);
    void handleVariation(bool init = false);
    void handlePlayedVariation();

    void deleteRank(int rank);
    bool handleDirty();

    string cartName() const;
    void saveHighScores(Int32 variation) const;
    void loadHighScores(Int32 variation);

    /**
      Saves the current high scores for this game and variation to the given file system node.

      @param node  The file system node to save to.
      @return  The result of the save.  True on success, false on failure.
    */
    bool save(FilesystemNode& node, Int32 variation) const;

    /**
      Loads the current high scores for this game and variation from the given JSON object.

      @param  hsData  The JSON to parse
      @return The result of the load.  True on success, false on failure.
    */
    bool load(const json& hsData, Int32 variation);

    string now() const;

    enum {
      kVariationChanged = 'Vach',
      kPrevVariation    = 'PrVr',
      kNextVariation    = 'NxVr',
      kDeleteSingle     = 'DeSi',
      kConfirmSave      = 'CfSv',
      kCancelSave       = 'CcSv'
    };

  private:
    static const string VERSION;
    static const string MD5;
    static const string VARIATION;
    static const string SCORES;
    static const string SCORE;
    static const string SPECIAL;
    static const string NAME;
    static const string DATE;

  private:
    bool myUserDefVar;      // allow the user to define the variation
    bool myDirty;
    bool myHighScoreSaved;  // remember if current high score was already saved (avoids double HS)
    unique_ptr<GUI::MessageBox> myConfirmMsg;
    int _max_w;
    int _max_h;

    Int32 myVariation;

    string myInitials;
    Int32 myEditRank;
    Int32 myHighScoreRank;
    string myNow;

    Int32 myHighScores[NUM_RANKS];
    Int32 mySpecials[NUM_RANKS];
    string myNames[NUM_RANKS];
    string myDates[NUM_RANKS];
    string myMD5;

    StaticTextWidget* myGameNameWidget{nullptr};

    PopUpWidget*      myVariationPopup{nullptr};
    ButtonWidget*     myPrevVarButton{nullptr};
    ButtonWidget*     myNextVarButton{nullptr};

    StaticTextWidget* mySpecialLabelWidget{nullptr};

    StaticTextWidget* myRankWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* myScoreWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* mySpecialWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* myNameWidgets[NUM_RANKS]{nullptr};
    EditTextWidget*   myEditNameWidgets[NUM_RANKS]{nullptr};
    StaticTextWidget* myDateWidgets[NUM_RANKS]{nullptr};
    ButtonWidget*     myDeleteButtons[NUM_RANKS]{nullptr};

    StaticTextWidget* myNotesWidget{nullptr};
    StaticTextWidget* myMD5Widget{nullptr};

    Menu::AppMode myMode{Menu::AppMode::emulator};

  private:
    // Following constructors and assignment operators not supported
    HighScoresDialog() = delete;
    HighScoresDialog(const HighScoresDialog&) = delete;
    HighScoresDialog(HighScoresDialog&&) = delete;
    HighScoresDialog& operator=(const HighScoresDialog&) = delete;
    HighScoresDialog& operator=(HighScoresDialog&&) = delete;
};
#endif
