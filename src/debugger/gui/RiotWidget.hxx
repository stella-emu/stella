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

#ifndef RIOT_WIDGET_HXX
#define RIOT_WIDGET_HXX

class GuiObject;
class StaticTextWidget;
class ButtonWidget;
class DataGridWidget;
class PopUpWidget;
class ToggleBitWidget;
class ControllerWidget;
class Controller;

#include "Command.hxx"

class RiotWidget : public Widget, public CommandSender
{
  public:
    RiotWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
               int x, int y, int w, int h);
    ~RiotWidget() override = default;

    void loadConfig() override;

    // Reflow entry point for the resizable debugger: move/resize the widget and
    // lay its two columns out for the available area
    void setArea(int x, int y, int w, int h) override;

    // My constructor cannot know how tall I am -- that is however tall
    // my two columns make me -- so report what my own layout tree comes to
    Common::Size naturalSize() const override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Build the layout tree from the current font and position/size everything;
    // shared by the ctor and setArea()
    void reflow();

    // The two columns as the engine sees them, built without positioning
    // anything, so reflow() and naturalSize() are one layout asked two questions
    unique_ptr<GUI::Layout> buildLayout() const;

    static ControllerWidget* addControlWidget(
        GuiObject* boss, const GUI::Font& font,
        int x, int y, Controller& controller);

    void handleConsole();

  private:
    // The six SW register labels (SWCHA(W)/SWACNT/... ), shared label column
    std::array<StaticTextWidget*, 6> myRegLabel{nullptr};
    ToggleBitWidget* mySWCHAReadBits{nullptr};
    ToggleBitWidget* mySWCHAWriteBits{nullptr};
    ToggleBitWidget* mySWACNTBits{nullptr};
    ToggleBitWidget* mySWCHBReadBits{nullptr};
    ToggleBitWidget* mySWCHBWriteBits{nullptr};
    ToggleBitWidget* mySWBCNTBits{nullptr};

    std::array<StaticTextWidget*, 3> myLeftINPTLabel{nullptr};
    std::array<StaticTextWidget*, 3> myRightINPTLabel{nullptr};
    DataGridWidget* myLeftINPT{nullptr};
    DataGridWidget* myRightINPT{nullptr};
    CheckboxWidget* myINPTLatch{nullptr};
    CheckboxWidget* myINPTDump{nullptr};

    std::array<StaticTextWidget*, 4> myTimWriteLabel{nullptr};
    std::array<StaticTextWidget*, 4> myTimReadLabel{nullptr};
    std::array<StaticTextWidget*, 3> myTimHash{nullptr};  // "#" cycle markers
    DataGridWidget* myTimWrite{nullptr};
    DataGridWidget* myTimAvail{nullptr};
    DataGridWidget* myTimRead{nullptr};
    DataGridWidget* myTimTotal{nullptr};
    DataGridWidget* myTimDivider{nullptr};

    ControllerWidget *myLeftControl{nullptr}, *myRightControl{nullptr};
    PopUpWidget *myP0Diff{nullptr}, *myP1Diff{nullptr};
    PopUpWidget *myTVType{nullptr};
    CheckboxWidget* mySelect{nullptr};
    CheckboxWidget* myReset{nullptr};
    CheckboxWidget* myPause{nullptr};

    StaticTextWidget* myConsoleLabel{nullptr};
    PopUpWidget *myConsole{nullptr};

    // ID's for the various widgets
    // We need ID's, since there are more than one of several types of widgets
    enum: uInt8 {
      kTim1TID, kTim8TID, kTim64TID, kTim1024TID, kTimWriteID,
      kSWCHABitsID, kSWACNTBitsID, kSWCHBBitsID, kSWBCNTBitsID,
      kP0DiffChanged, kP1DiffChanged, kTVTypeChanged, kSelectID, kResetID,
      kSWCHARBitsID, kSWCHBRBitsID, kPauseID, kConsoleID
    };

  private:
    // Following constructors and assignment operators not supported
    RiotWidget() = delete;
    RiotWidget(const RiotWidget&) = delete;
    RiotWidget(RiotWidget&&) = delete;
    RiotWidget& operator=(const RiotWidget&) = delete;
    RiotWidget& operator=(RiotWidget&&) = delete;
};

#endif  // RIOT_WIDGET_HXX
