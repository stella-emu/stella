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

#ifndef PROGRESS_DIALOG_HXX
#define PROGRESS_DIALOG_HXX

class GuiObject;
class LabelWidget;
class SliderWidget;
class ButtonWidget;

#include "bspf.hxx"
#include "Dialog.hxx"

/**
  Shows progress (message + slider) for a long-running operation, with
  a Cancel button the caller polls via isCancelled().

  @author  Stephen Anthony and Thomas Jentzsch
*/
class ProgressDialog : public Dialog
{
  public:
    ProgressDialog(GuiObject* boss, const GUI::Font& font, string_view message = "");
    ~ProgressDialog() override = default;

    // Changes the message text and re-lays out (the dialog is sized to it)
    void setMessage(string_view message);
    // Sets the value range; 'step' (0-100) is the percentage of it skipped
    // before the slider starts visibly advancing
    void setRange(int start, int finish, int step);
    // Resets the slider to 0 and clears the cancelled flag
    void resetProgress();
    // Advances the slider, throttled to at most 10 updates/second; also pumps
    // the frame buffer/event loop, since the caller is usually a tight loop
    // that never otherwise yields back to it
    void setProgress(int progress);
    void incProgress();
    bool isCancelled() const { return myIsCancelled; }

  protected:
    void layout() override;
    // The Cancel button sets myIsCancelled for the caller to notice
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    struct Cmd {
      static constexpr GuiCmd::Code
        Cancel = GuiCmd::of("ProgressDialog.Cancel");
    };

    LabelWidget*  myMessage{nullptr};
    SliderWidget* mySlider{nullptr};
    string        myMessageText;

    // Value range set by setRange(); myStep is the leading portion skipped
    int myStart{0}, myFinish{0}, myStep{0};
    // Current progress value, advanced by incProgress()/setProgress()
    int myProgress{0};
    // Last time the slider was actually updated, throttling setProgress()
    uInt64 myLastTick{0};
    bool myIsCancelled{false};

  private:
    // Following constructors and assignment operators not supported
    ProgressDialog() = delete;
    ProgressDialog(const ProgressDialog&) = delete;
    ProgressDialog(ProgressDialog&&) = delete;
    ProgressDialog& operator=(const ProgressDialog&) = delete;
    ProgressDialog& operator=(ProgressDialog&&) = delete;
};

#endif  // PROGRESS_DIALOG_HXX
