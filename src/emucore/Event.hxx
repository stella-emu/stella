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

#ifndef EVENT_HXX
#define EVENT_HXX

#include <atomic>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "bspf.hxx"

/**
  Holds the current value of every event type, plus a schedule of input
  transitions recorded over the current input window.  This models controllers
  as devices whose state can change at any point within a window, rather than
  being latched once per window.  An input window spans the system (CPU) cycles
  between two successive input polls.

  Input is polled once per window, but a program may sample a controller
  several times within one window (e.g. the fire button before and after its
  display kernel) and expect to see a change that happened in between.  Each
  change is recorded as a transition; finalizeInputWindow() spreads them across
  the window and get(type, pos) replays the value at a position within it.

  Transitions are ordered by arrival, not real time: SDL timestamps events at
  drain time, so true sub-window timing is unrecoverable and a lone change is
  placed mid-window.

  @author  Stephen Anthony, Christian Speckner, Thomas Jentzsch
*/
class Event
{
  public:
    /**
      Enumeration of all possible events in Stella, including both
      console and controller event types as well as events that aren't
      technically part of the emulation core.
    */
    enum Type: uInt16
    {
      NoType = 0,
      ConsoleColor, ConsoleBlackWhite, ConsoleColorToggle, Console7800Pause,
      ConsoleLeftDiffA, ConsoleLeftDiffB, ConsoleLeftDiffToggle,
      ConsoleRightDiffA, ConsoleRightDiffB, ConsoleRightDiffToggle,
      ConsoleSelect, ConsoleReset,

      LeftJoystickUp, LeftJoystickDown, LeftJoystickLeft, LeftJoystickRight,
      LeftJoystickFire, LeftJoystickFire5, LeftJoystickFire9,
      RightJoystickUp, RightJoystickDown, RightJoystickLeft, RightJoystickRight,
      RightJoystickFire, RightJoystickFire5, RightJoystickFire9,

      LeftPaddleADecrease, LeftPaddleAIncrease, LeftPaddleAAnalog,
      LeftPaddleAFire, LeftPaddleAButton1, LeftPaddleAButton2,
      LeftPaddleBDecrease, LeftPaddleBIncrease, LeftPaddleBAnalog,
      LeftPaddleBFire, RightPaddleAButton1, RightPaddleAButton2,
      RightPaddleADecrease, RightPaddleAIncrease, RightPaddleAAnalog,
      RightPaddleAFire,
      RightPaddleBDecrease, RightPaddleBIncrease, RightPaddleBAnalog, RightPaddleBFire,

      LeftKeyboard1, LeftKeyboard2, LeftKeyboard3,
      LeftKeyboard4, LeftKeyboard5, LeftKeyboard6,
      LeftKeyboard7, LeftKeyboard8, LeftKeyboard9,
      LeftKeyboardStar, LeftKeyboard0, LeftKeyboardPound,

      RightKeyboard1, RightKeyboard2, RightKeyboard3,
      RightKeyboard4, RightKeyboard5, RightKeyboard6,
      RightKeyboard7, RightKeyboard8, RightKeyboard9,
      RightKeyboardStar, RightKeyboard0, RightKeyboardPound,

      LeftDrivingCCW, LeftDrivingCW, LeftDrivingFire, LeftDrivingAnalog,
      LeftDrivingButton1, LeftDrivingButton2,
      RightDrivingCCW, RightDrivingCW, RightDrivingFire, RightDrivingAnalog,
      RightDrivingButton1, RightDrivingButton2,

      CompuMateFunc, CompuMateShift,
      CompuMate0, CompuMate1, CompuMate2, CompuMate3, CompuMate4,
      CompuMate5, CompuMate6, CompuMate7, CompuMate8, CompuMate9,
      CompuMateA, CompuMateB, CompuMateC, CompuMateD, CompuMateE,
      CompuMateF, CompuMateG, CompuMateH, CompuMateI, CompuMateJ,
      CompuMateK, CompuMateL, CompuMateM, CompuMateN, CompuMateO,
      CompuMateP, CompuMateQ, CompuMateR, CompuMateS, CompuMateT,
      CompuMateU, CompuMateV, CompuMateW, CompuMateX, CompuMateY,
      CompuMateZ,
      CompuMateComma, CompuMatePeriod, CompuMateEnter, CompuMateSpace,
      CompuMateQuestion, CompuMateLeftBracket, CompuMateRightBracket, CompuMateMinus,
      CompuMateQuote, CompuMateBackspace, CompuMateEquals, CompuMatePlus,
      CompuMateSlash,

      Combo1, Combo2, Combo3, Combo4, Combo5, Combo6, Combo7, Combo8,
      Combo9, Combo10, Combo11, Combo12, Combo13, Combo14, Combo15, Combo16,

      UIUp, UIDown, UILeft, UIRight, UIHome, UIEnd, UIPgUp, UIPgDown,
      UISelect, UINavPrev, UINavNext, UIOK, UICancel, UIPrevDir,
      UITabPrev, UITabNext, UIReload,

      NextMouseControl, ToggleGrabMouse,
      MouseAxisXMove, MouseAxisYMove, MouseAxisXValue, MouseAxisYValue,
      MouseButtonLeftValue, MouseButtonRightValue,

      Quit, ReloadConsole, Fry,
      TogglePauseMode, StartPauseMode,
      OptionsMenuMode, CmdMenuMode, DebuggerMode, PlusRomsSetupMode, ExitMode,
      TakeSnapshot, ToggleContSnapshots, ToggleContSnapshotsFrame,
      ToggleTurbo,

      NextState, PreviousState, LoadState, SaveState,
      SaveAllStates, LoadAllStates,
      ToggleAutoSlot, ToggleTimeMachine, TimeMachineMode,
      Rewind1Menu, Rewind10Menu, RewindAllMenu,
      Unwind1Menu, Unwind10Menu, UnwindAllMenu,
      RewindPause, UnwindPause,

      FormatDecrease, FormatIncrease, PaletteDecrease, PaletteIncrease,
      PreviousPaletteAttribute, NextPaletteAttribute,
      PaletteAttributeDecrease, PaletteAttributeIncrease,
      ToggleFullScreen, VidmodeDecrease, VidmodeIncrease,
      VCenterDecrease, VCenterIncrease, VSizeAdjustDecrease, VSizeAdjustIncrease,
      OverscanDecrease, OverscanIncrease,

      VidmodeStd, VidmodeRGB, VidmodeSVideo, VidModeComposite, VidModeCustom,
      PreviousVideoMode, NextVideoMode,
      PreviousAttribute, NextAttribute, DecreaseAttribute, IncreaseAttribute,
      ScanlinesDecrease, ScanlinesIncrease,
      PreviousScanlineMask, NextScanlineMask,
      PhosphorDecrease, PhosphorIncrease, TogglePhosphor,
      PhosphorModeDecrease, PhosphorModeIncrease, ToggleInter,
      ToggleDeveloperSet, JitterRecDecrease, JitterRecIncrease,
      JitterSenseDecrease, JitterSenseIncrease, ToggleJitter,

      VolumeDecrease, VolumeIncrease, SoundToggle,

      ToggleP0Collision, ToggleP0Bit, ToggleP1Collision, ToggleP1Bit,
      ToggleM0Collision, ToggleM0Bit, ToggleM1Collision, ToggleM1Bit,
      ToggleBLCollision, ToggleBLBit, TogglePFCollision, TogglePFBit,
      ToggleCollisions, ToggleBits, ToggleFixedColors,

      ToggleFrameStats, ToggleBezel, ToggleSAPortOrder, ExitGame,
      SettingDecrease, SettingIncrease, PreviousSetting, NextSetting,
      ToggleAdaptRefresh, PreviousMultiCartRom,
      // add new (after Version 4) events from here to avoid that user remapped events get overwritten
      PreviousSettingGroup, NextSettingGroup,
      TogglePlayBackMode,
      ToggleAutoFire, DecreaseAutoFire, IncreaseAutoFire,
      DecreaseSpeed, IncreaseSpeed,

      QTJoystickThreeUp, QTJoystickThreeDown, QTJoystickThreeLeft, QTJoystickThreeRight,
      QTJoystickThreeFire,
      QTJoystickFourUp, QTJoystickFourDown, QTJoystickFourLeft, QTJoystickFourRight,
      QTJoystickFourFire,

      ToggleCorrectAspectRatio,

      MoveLeftChar, MoveRightChar, MoveLeftWord, MoveRightWord,
      MoveHome, MoveEnd,
      SelectLeftChar, SelectRightChar, SelectLeftWord, SelectRightWord,
      SelectHome, SelectEnd, SelectAll,
      Delete, DeleteLeftWord, DeleteRightWord, DeleteHome, DeleteEnd, Backspace,
      Cut, Copy, Paste, Undo, Redo,
      AbortEdit, EndEdit, ToggleUIPalette,

      HighScoresMenuMode,
      // Input settings
      DecreaseDeadzone, IncreaseDeadzone,
      DecAnalogDeadzone, IncAnalogDeadzone,
      DecAnalogSense, IncAnalogSense,
      DecAnalogLinear, IncAnalogLinear,
      DecDejtterAveraging, IncDejtterAveraging,
      DecDejtterReaction, IncDejtterReaction,
      DecDigitalSense, IncDigitalSense,
      ToggleFourDirections, ToggleKeyCombos,
      PrevMouseAsController, NextMouseAsController,
      DecMousePaddleSense, IncMousePaddleSense,
      DecMouseTrackballSense, IncMouseTrackballSense,
      DecreaseDrivingSense, IncreaseDrivingSense,
      PreviousCursorVisbility, NextCursorVisbility,
      // GameInfoDialog/Controllers
      PreviousLeftPort, NextLeftPort,
      PreviousRightPort, NextRightPort,
      ToggleSwapPorts, ToggleSwapPaddles,
      DecreasePaddleCenterX, IncreasePaddleCenterX,
      DecreasePaddleCenterY, IncreasePaddleCenterY,
      PreviousMouseControl,
      DecreaseMouseAxesRange, IncreaseMouseAxesRange,

      SALeftAxis0Value, SALeftAxis1Value, SARightAxis0Value, SARightAxis1Value,
      QTPaddle3AFire, QTPaddle3BFire, QTPaddle4AFire, QTPaddle4BFire,
      UIHelp,
      LastType
    };

    // Event categorizing groups
    enum Group: uInt8
    {
      Menu, Emulation,
      Misc, AudioVideo, States, Console, Joystick, Paddles, Driving, Keyboard,
      Devices,
      Debug, Combo,
      LastGroup
    };

    // Event list version, update only if the id of existing(!) event types changed
    static constexpr Int32 VERSION = 9;

    using EventSet = std::unordered_set<Event::Type>;

  public:
    /**
      Create a new event object.
    */
    Event() { clear(); }  // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    ~Event() = default;

  public:
    /**
      Get the value associated with the event of the specified type.
    */
    Int32 get(Type type) const {
      const std::scoped_lock lock(myMutex);

      return myValues[type];
    }

    /**
      Get the value of 'type' at sub-window position 'pos' (CPU cycles from the
      start of the window), replaying the transitions recorded this window.
    */
    Int32 get(Type type, uInt64 pos) const {
      const std::scoped_lock lock(myMutex);

      Int32 result = myValues[type];
      for(const auto& t: myTransitions)
        if(t.type == type && t.pos <= pos)
          result = t.value;

      return result;
    }

    /**
      Position within the current input window, in CPU cycles, of the point
      being sampled: the offset of 'nowCycles' (the caller's System::cycles())
      from the start of the window.
    */
    uInt64 windowPosition(uInt64 nowCycles) const {
      return nowCycles - myWindowStartCycle;
    }

    /**
      Whether any transition was recorded this input window.  When false, every
      input is constant across the window and equals its latched value.
    */
    bool hasTransitions() const {
      return myHasTransitions;
    }

    /**
      Set the value of 'type'.  While the input window is open (see
      beginInputWindow), a change of value is also recorded as a transition, in
      arrival order, for sub-window replay by get(type, pos).
    */
    void set(Type type, Int32 value) {
      const std::scoped_lock lock(myMutex);

      // Continuous inputs (analog axes, mouse motion) are read once per window
      // as a whole value and never replayed, so they aren't recorded
      if(myRecording && value != myValues[type] && !isContinuous(type))
      {
        // Seed a baseline so reads before the first transition see the prior value
        if(std::ranges::none_of(myTransitions,
            [type](const Transition& t){ return t.type == type; }))
          myTransitions.push_back({type, 0, myValues[type], false});

        // 'pending' marks the position for finalizeInputWindow() to assign
        myTransitions.push_back({type, 0, value, true});
      }

      myValues[type] = value;
    }

    /**
      Open the input window for the coming poll and start recording transitions.
      Transitions the previous window never reached (it turned out shorter than
      the estimate they were spread over) are carried forward rather than
      dropped, so an uneven run of window lengths can't swallow an input change.
    */
    void beginInputWindow(uInt64 nowCycles) {
      const std::scoped_lock lock(myMutex);

      // Length of the just-ended window, used as the estimate for this one
      myCyclesLastWindow = nowCycles - myWindowStartCycle;
      myWindowStartCycle = nowCycles;

      // If the just-ended window came out shorter than that estimate, positions
      // at or beyond its actual length were never sampled; carry those
      // transitions forward instead of dropping them.
      if(myCyclesLastWindow != 0 &&
         std::ranges::any_of(myTransitions,
           [this](const Transition& t){ return t.pos >= myCyclesLastWindow; }))
      {
        std::vector<Transition> carried;
        for(const auto& t: myTransitions)
          if(t.pos >= myCyclesLastWindow)
          {
            // Seed a baseline so reads before the carried transition see the
            // held value rather than the new latch
            if(std::ranges::none_of(carried,
                [&t](const Transition& b){ return b.type == t.type; }))
              carried.push_back(
                {t.type, 0, heldValueBefore(t.type, myCyclesLastWindow), false});

            carried.push_back({t.type, 0, t.value, true});
          }
        myTransitions = std::move(carried);
      }
      else
        myTransitions.clear();

      myRecording = true;
      myHasTransitions = false;
    }

    /**
      Close the input window and spread the recorded transitions evenly across
      it, in arrival order: N transitions at 1/(N+1) .. N/(N+1) of the window
      length, so a lone change lands mid-window.
    */
    void finalizeInputWindow() {
      const std::scoped_lock lock(myMutex);

      myRecording = false;

      uInt32 pending = 0;
      for(const auto& t: myTransitions)
        if(t.pending) ++pending;

      myHasTransitions = (pending != 0);
      if(pending == 0)
        return;

      uInt32 k = 0;
      for(auto& t: myTransitions)
        if(t.pending)
        {
          t.pos = myCyclesLastWindow * (++k) / (pending + 1);
          t.pending = false;
        }
    }

    /**
      Clears the event array (resets to initial state).
    */
    void clear()
    {
      const std::scoped_lock lock(myMutex);

      myValues.fill(Event::NoType);
      myTransitions.clear();
      myRecording = false;
      myHasTransitions = false;
    }

    /**
      Tests if a given event represents continuous or analog values.
    */
    static bool isAnalog(Type type)
    {
      switch(type)
      {
        case Event::LeftPaddleAAnalog:
        case Event::LeftPaddleBAnalog:
        case Event::RightPaddleAAnalog:
        case Event::RightPaddleBAnalog:
        case Event::LeftDrivingAnalog:
        case Event::RightDrivingAnalog:
          return true;
        default:
          return false;
      }
    }

    /**
      Whether 'type' carries a continuous whole-window value — an analog axis or
      mouse motion (the contiguous MouseAxis* range, NOT the mouse buttons that
      follow it, which are bound to digital pins).  These are read once per
      window via get(type) and never replayed, so set() leaves them out of the
      transition schedule.
    */
    static bool isContinuous(Type type)
    {
      return isAnalog(type) ||
          (type >= MouseAxisXMove && type <= MouseAxisYValue);
    }

  private:
    // A recorded input transition: 'type' took on 'value' at position 'pos'
    // (CPU cycles) within the input window
    struct Transition {
      Type  type{NoType};
      uInt64 pos{0};
      Int32 value{0};
      bool  pending{false};   // true until finalizeInputWindow() assigns pos
    };

    // Value of 'type' as last observed before position 'len' within the window
    // (its latest transition before 'len').  Seeds the baseline of a
    // carried-forward transition (see beginInputWindow).  Assumes myMutex held.
    Int32 heldValueBefore(Type type, uInt64 len) const {
      Int32 result = myValues[type];
      for(const auto& t: myTransitions)
        if(t.type == type && t.pos < len)
          result = t.value;
      return result;
    }

  private:
    // Current value of each event type
    std::array<Int32, LastType> myValues;

    // Input transitions recorded this window in arrival order, replayed by
    // get(type, pos)
    std::vector<Transition> myTransitions;

    // True while the input window is open and set() should record transitions
    bool myRecording{false};

    // Start of the current input window, and the length of the previous one,
    // on the system (CPU) clock (see windowPosition and finalizeInputWindow).
    // Written under myMutex during the poll, which happens-before the worker
    // thread reads them, so they need no atomic.
    uInt64 myWindowStartCycle{0}, myCyclesLastWindow{0};

    // Set when any transition was recorded this window; lets hasTransitions()
    // callers skip the replay when nothing changed
    std::atomic<bool> myHasTransitions{false};

    mutable std::mutex myMutex;

  private:
    // Following constructors and assignment operators not supported
    Event(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;
};

// Hold controller related events
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet LeftJoystickEvents = {
  Event::LeftJoystickUp, Event::LeftJoystickDown, Event::LeftJoystickLeft, Event::LeftJoystickRight,
  Event::LeftJoystickFire, Event::LeftJoystickFire5, Event::LeftJoystickFire9,
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet QTJoystick3Events = {
  Event::QTJoystickThreeUp, Event::QTJoystickThreeDown, Event::QTJoystickThreeLeft, Event::QTJoystickThreeRight,
  Event::QTJoystickThreeFire
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet RightJoystickEvents = {
  Event::RightJoystickUp, Event::RightJoystickDown, Event::RightJoystickLeft, Event::RightJoystickRight,
  Event::RightJoystickFire, Event::RightJoystickFire5, Event::RightJoystickFire9,
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet QTJoystick4Events = {
  Event::QTJoystickFourUp, Event::QTJoystickFourDown, Event::QTJoystickFourLeft, Event::QTJoystickFourRight,
  Event::QTJoystickFourFire
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet LeftPaddlesEvents = {
  Event::LeftPaddleADecrease, Event::LeftPaddleAIncrease, Event::LeftPaddleAAnalog,
  Event::LeftPaddleAFire, Event::LeftPaddleAButton1, Event::LeftPaddleAButton2,
  Event::LeftPaddleBDecrease, Event::LeftPaddleBIncrease, Event::LeftPaddleBAnalog,
  Event::LeftPaddleBFire,
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet QTPaddles3Events = {
  // Only fire buttons supported by QuadTari
  Event::QTPaddle3AFire, Event::QTPaddle3BFire
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet RightPaddlesEvents = {
  Event::RightPaddleADecrease, Event::RightPaddleAIncrease, Event::RightPaddleAAnalog,
  Event::RightPaddleAFire, Event::RightPaddleAButton1, Event::RightPaddleAButton2,
  Event::RightPaddleBDecrease, Event::RightPaddleBIncrease, Event::RightPaddleBAnalog,
  Event::RightPaddleBFire,
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet QTPaddles4Events = {
  // Only fire buttons supported by QuadTari
  Event::QTPaddle4AFire, Event::QTPaddle4BFire
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet LeftKeyboardEvents = {
  Event::LeftKeyboard1, Event::LeftKeyboard2, Event::LeftKeyboard3,
  Event::LeftKeyboard4, Event::LeftKeyboard5, Event::LeftKeyboard6,
  Event::LeftKeyboard7, Event::LeftKeyboard8, Event::LeftKeyboard9,
  Event::LeftKeyboardStar, Event::LeftKeyboard0, Event::LeftKeyboardPound,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet RightKeyboardEvents = {
  Event::RightKeyboard1, Event::RightKeyboard2, Event::RightKeyboard3,
  Event::RightKeyboard4, Event::RightKeyboard5, Event::RightKeyboard6,
  Event::RightKeyboard7, Event::RightKeyboard8, Event::RightKeyboard9,
  Event::RightKeyboardStar, Event::RightKeyboard0, Event::RightKeyboardPound,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet LeftDrivingEvents = {
  Event::LeftDrivingAnalog, Event::LeftDrivingCCW, Event::LeftDrivingCW,
  Event::LeftDrivingFire, Event::LeftDrivingButton1, Event::LeftDrivingButton2,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
inline const Event::EventSet RightDrivingEvents = {
  Event::RightDrivingAnalog, Event::RightDrivingCCW, Event::RightDrivingCW,
  Event::RightDrivingFire, Event::RightDrivingButton1, Event::RightDrivingButton2,
};

#endif  // EVENT_HXX
