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

#ifndef MESSAGE_DIALOG_HXX
#define MESSAGE_DIALOG_HXX

//class Properties;
class CommandSender;
class DialogContainer;
class OSystem;
class EditTextWidget;
class PopUpWidget;

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
                     const GUI::Font& font, int max_w, int max_h);
    virtual ~HighScoresDialog();

  protected:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    PopUpWidget*      myVariation;
    StaticTextWidget* myPositions[NUM_POSITIONS];
    StaticTextWidget* myScores[NUM_POSITIONS];
    EditTextWidget*   myNames[NUM_POSITIONS];
    StaticTextWidget* myMD5;

  private:
    // Following constructors and assignment operators not supported
    HighScoresDialog() = delete;
    HighScoresDialog(const HighScoresDialog&) = delete;
    HighScoresDialog(HighScoresDialog&&) = delete;
    HighScoresDialog& operator=(const HighScoresDialog&) = delete;
    HighScoresDialog& operator=(HighScoresDialog&&) = delete;
};
#endif
