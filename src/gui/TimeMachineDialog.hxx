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

#ifndef TIME_MACHINE_DIALOG_HXX
#define TIME_MACHINE_DIALOG_HXX

class CommandSender;
class DialogContainer;
class OSystem;
class TimeLineWidget;

#include "Dialog.hxx"

/**
  The Time Machine HUD: timeline scrubber, transport buttons (rewind/
  unwind/play), and elapsed-time/state-index readouts.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class TimeMachineDialog : public Dialog
{
  public:
    // 'width' comes from the parent (the HUD spans the window); height follows
    // from the two rows it lays out (see layout())
    TimeMachineDialog(OSystem& osystem, DialogContainer& parent, int width);
    ~TimeMachineDialog() override = default;

    // Enables translucency and refreshes the timeline/readouts (see initBar())
    void loadConfig() override;

    /** Set/get number of winds when entering the dialog */
    void setEnterWinds(Int32 numWinds) { _enterWinds = numWinds; }
    Int32 getEnterWinds() const { return _enterWinds; }

    /** This dialog uses its own positioning, so we override Dialog::setPosition() */
    void setPosition() override;

  protected:
    void layout() override;
    // Hotkey shortcuts that duplicate EventHandler's, routed to the same
    // commands the transport buttons send (mode switches happen on key-up)
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleKeyUp(StellaKey key, StellaMod mod) override;
    // Dispatches a transport button/timeline drag to handleWinds() or the
    // matching EventHandler action
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    /** initialize timeline bar */
    void initBar();
    /** convert cycles into time */
    static string getTimeString(uInt64 cycles, size_t scanlines);
    /** re/unwind and update display */
    void handleWinds(Int32 numWinds = 0);
    /** toggle Time Machine mode */
    void handleToggle();

  private:
    // Command ids for the timeline and each transport button, dispatched in
    // handleCommand() (several are also reachable via hotkey, see handleKeyDown())
    enum
    {
      kTimeline  = 'TMtl',
      kToggle    = 'TMtg',
      kExit      = 'TMex',
      kPlayBack  = 'TMpb',
      kRewindAll = 'TMra',
      kRewind10  = 'TMr1',
      kRewind1   = 'TMre',
      kUnwindAll = 'TMua',
      kUnwind10  = 'TMu1',
      kUnwind1   = 'TMun',
      kSaveAll   = 'TMsv',
      kLoadAll   = 'TMld',
    };

    // The scrubber; dragging/paging it issues kTimeline
    TimeLineWidget* myTimeline{nullptr};

    // Transport buttons: record/stop toggle, exit, rewind (all/10/1), play back,
    // unwind (1/10/all), and save/load all states
    ButtonWidget* myToggleWidget{nullptr};
    ButtonWidget* myExitWidget{ nullptr };
    ButtonWidget* myPlayBackWidget{nullptr};
    ButtonWidget* myRewindAllWidget{nullptr};
    ButtonWidget* myRewind1Widget{nullptr};
    ButtonWidget* myUnwind1Widget{nullptr};
    ButtonWidget* myUnwindAllWidget{nullptr};
    ButtonWidget* mySaveAllWidget{nullptr};
    ButtonWidget* myLoadAllWidget{nullptr};

    // Current / total elapsed time readouts
    LabelWidget* myCurrentTimeWidget{nullptr};
    LabelWidget* myLastTimeWidget{nullptr};

    // Current / total state-index readouts
    LabelWidget* myCurrentIdxWidget{nullptr};
    LabelWidget* myLastIdxWidget{nullptr};
    // Transient "(+/-<amount>)" message shown after a rewind/unwind
    LabelWidget* myMessageWidget{nullptr};

    // Winds to apply once, right after the dialog opens (see setEnterWinds())
    Int32 _enterWinds{0};

  private:
    // Following constructors and assignment operators not supported
    TimeMachineDialog() = delete;
    TimeMachineDialog(const TimeMachineDialog&) = delete;
    TimeMachineDialog(TimeMachineDialog&&) = delete;
    TimeMachineDialog& operator=(const TimeMachineDialog&) = delete;
    TimeMachineDialog& operator=(TimeMachineDialog&&) = delete;
};

#endif  // TIME_MACHINE_DIALOG_HXX
