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

/**
  The dialog for displaying high scores in Stella.

  @author  Thomas Jentzsch
*/

class HighScoresDialog : public Dialog
{
  public:
    static const uInt32 NUM_POSITIONS = 10;

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

    void deletePos(int pos);
    bool handleDirty();

    string cartName() const;
    void saveHighScores(Int32 variation) const;
    void loadHighScores(Int32 variation);

    /**
      Saves the current high scores for this game and variation to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out, Int32 variation) const;

    /**
      Loads the current high scores for this game and variation from the given Serializer.

      @param in The Serializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    bool load(Serializer& in, Int32 variation);

    string now() const;

    enum {
      kVariationChanged = 'Vach',
      kDeleteSingle     = 'DeSi',
      kConfirmSave      = 'CfSv',
      kCancelSave       = 'CcSv'
    };

  private:
    bool myDirty;
    unique_ptr<GUI::MessageBox> myConfirmMsg;
    int _max_w;
    int _max_h;

    Int32 myVariation;

    string myInitials;
    Int32 myEditPos;
    Int32 myHighScorePos;
    string myNow;

    Int32 myHighScores[NUM_POSITIONS];
    Int32 mySpecials[NUM_POSITIONS];
    string myNames[NUM_POSITIONS];
    string myDates[NUM_POSITIONS];
    string myMD5;

    PopUpWidget*      myVariationWidget{nullptr};
    StaticTextWidget* mySpecialLabelWidget{nullptr};
    StaticTextWidget* myPositionsWidget[NUM_POSITIONS]{nullptr};
    StaticTextWidget* myScoresWidget[NUM_POSITIONS]{nullptr};
    StaticTextWidget* mySpecialsWidget[NUM_POSITIONS]{nullptr};
    StaticTextWidget* myNamesWidget[NUM_POSITIONS]{nullptr};
    EditTextWidget*   myEditNamesWidget[NUM_POSITIONS]{nullptr};
    StaticTextWidget* myDatesWidget[NUM_POSITIONS]{nullptr};
    ButtonWidget*     myDeleteButtons[NUM_POSITIONS]{nullptr};

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
