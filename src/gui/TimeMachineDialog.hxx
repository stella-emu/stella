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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
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

class TimeMachineDialog : public Dialog
{
  public:
    TimeMachineDialog(OSystem& osystem, DialogContainer& parent, int width);
    virtual ~TimeMachineDialog() = default;

    /** set/get number of winds when entering the dialog */
    void setEnterWinds(Int32 numWinds) { _enterWinds = numWinds; }
    Int32 getEnterWinds() { return _enterWinds; }

  private:
    void loadConfig() override;
    void handleKeyDown(StellaKey key, StellaMod mod) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    /** This dialog uses its own positioning, so we override Dialog::center() */
    void center() override;

    /** convert cycles into time */
    string getTimeString(uInt64 cycles);
    /** re/unwind and update display */
    void handleWinds(Int32 numWinds = 0);
    /** toggle Time Machine mode */
    void handleToggle();

  private:
    enum
    {
      kTimeline  = 'TMtl',
      kToggle    = 'TMtg',
      kPlay      = 'TMpl',
      kRewindAll = 'TMra',
      kRewind10  = 'TMr1',
      kRewind1   = 'TMre',
      kUnwindAll = 'TMua',
      kUnwind10  = 'TMu1',
      kUnwind1   = 'TMun',
    };

    TimeLineWidget* myTimeline;

    ButtonWidget* myToggleWidget;
    ButtonWidget* myPlayWidget;
    ButtonWidget* myRewindAllWidget;
    ButtonWidget* myRewind1Widget;
    ButtonWidget* myUnwind1Widget;
    ButtonWidget* myUnwindAllWidget;

    StaticTextWidget* myCurrentTimeWidget;
    StaticTextWidget* myLastTimeWidget;

    StaticTextWidget* myCurrentIdxWidget;
    StaticTextWidget* myLastIdxWidget;
    StaticTextWidget* myMessageWidget;

    Int32 _enterWinds;

  private:
    // Following constructors and assignment operators not supported
    TimeMachineDialog() = delete;
    TimeMachineDialog(const TimeMachineDialog&) = delete;
    TimeMachineDialog(TimeMachineDialog&&) = delete;
    TimeMachineDialog& operator=(const TimeMachineDialog&) = delete;
    TimeMachineDialog& operator=(TimeMachineDialog&&) = delete;
};

#endif
