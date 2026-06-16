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
  Holds the current value of every event type, plus a per-frame schedule of
  input transitions used to model controllers as devices whose state changes
  continuously through a frame (rather than being latched once per frame).

  Input is polled once per frame, but a program may sample a controller several
  times within a single frame (e.g. the fire button before and after its
  display kernel) and expect to observe a press/release that happened part way
  through.  To support this, every change to an input value during the poll is
  recorded as a transition; finalizeInputWindow() then spreads the frame's
  transitions across [0,1) and get(type, pos) replays the value as of a given
  sub-frame position (see Controller::read).

  NOTE: the transitions are spread in arrival order, not by real time.  SDL
  stamps events at drain time rather than physical arrival, so the true
  sub-frame timing of an input is not recoverable; a lone press/release is
  therefore placed mid-frame.

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

      FormatDecrease, FormatIncrease, PaletteDecrease, PaletteIncrease, ToggleColorLoss,
      PreviousPaletteAttribute, NextPaletteAttribute,
      PaletteAttributeDecrease, PaletteAttributeIncrease,
      ToggleFullScreen, VidmodeDecrease, VidmodeIncrease,
      VCenterDecrease, VCenterIncrease, VSizeAdjustDecrease, VSizeAdjustIncrease,
      OverscanDecrease, OverscanIncrease,

      VidmodeStd, VidmodeRGB, VidmodeSVideo, VidModeComposite, VidModeBad, VidModeCustom,
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
    static constexpr Int32 VERSION = 8;

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
      Get the value of the event of the specified type as of the given
      sub-frame position [0,1).  This replays the transitions recorded
      during the most recent input poll, spread across the frame, so a
      device sampling an input at different scanlines sees it change
      mid-frame.  Falls back to the latest value when the type had no
      transitions this frame.
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
      The sub-frame position, in CPU cycles, of the point currently being
      sampled within the input window.  'nowCycles' is the caller's current
      System::cycles(); the result is the offset from the start of the frame.
    */
    uInt64 framePosition(uInt64 nowCycles) const {
      return nowCycles - myFrameStartCycle;
    }

    /**
      Whether any input transitions were recorded during the most recent
      poll.  Hot-path readers (Controller::read, Switches::read) use this
      to skip the sub-frame replay on the common frame where nothing
      changed: when false, every input is constant across the frame and
      equals its latched value, so the (mutex-locked) transition scan in
      get(type, pos) can be avoided entirely.  Read lock-free; the value
      is published under the mutex during the input poll, which happens
      before the worker thread reads it for the same frame.
    */
    bool hasTransitions() const {
      return myHasTransitions.load(std::memory_order_relaxed);
    }

    /**
      Set the value associated with the event of the specified type.  While the
      input window is open (see beginInputWindow), a change of value is also
      recorded in the per-frame transition schedule, in arrival order; the
      transitions are later spread across the frame by finalizeInputWindow().
    */
    void set(Type type, Int32 value) {
      const std::scoped_lock lock(myMutex);

      if(myRecording && value != myValues[type])
      {
        // Record the pre-change baseline once, so get(type, pos) has a value
        // for positions before the first transition this frame
        if(std::ranges::none_of(myTransitions,
            [type](const Transition& t){ return t.type == type; }))
          myTransitions.push_back({type, 0, myValues[type], false});

        // Position is assigned later by finalizeInputWindow(); 'pending' marks it
        myTransitions.push_back({type, 0, value, true});
      }

      myValues[type] = value;
    }

    /**
      Open the input window: clear the previous frame's transition schedule and
      begin recording new transitions.  Called once before draining input.
    */
    void beginInputWindow(uInt64 nowCycles) {
      const std::scoped_lock lock(myMutex);

      // The input window spans one frame on the system (CPU) clock
      myCyclesLastFrame = nowCycles - myFrameStartCycle;
      myFrameStartCycle = nowCycles;

      myTransitions.clear();
      myRecording = true;
      myHasTransitions.store(false, std::memory_order_relaxed);
    }

    /**
      Close the input window and spread the recorded transitions evenly across
      the frame in arrival order.  SDL only timestamps events at drain time
      (not real arrival), so we cannot recover the true sub-frame timing;
      instead we distribute N transitions at 1/(N+1) .. N/(N+1), which places a
      lone press/release mid-frame and keeps multiple transitions ordered.
    */
    void finalizeInputWindow() {
      const std::scoped_lock lock(myMutex);

      myRecording = false;

      uInt32 pending = 0;
      for(const auto& t: myTransitions)
        if(t.pending) ++pending;

      // Publish the fast-path gate read lock-free by get()'s hot-path callers
      myHasTransitions.store(pending != 0, std::memory_order_relaxed);
      if(pending == 0)
        return;

      // Spread the pending transitions evenly across the previous frame's
      // cycle span, in arrival order
      uInt32 k = 0;
      for(auto& t: myTransitions)
        if(t.pending)
        {
          t.pos = myCyclesLastFrame * (++k) / (pending + 1);
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
      myHasTransitions.store(false, std::memory_order_relaxed);
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

  private:
    // A single recorded input transition within the current frame: the event
    // 'type' took on 'value' at sub-frame position 'pos' [0,1)
    struct Transition {
      Type  type{NoType};
      uInt64 pos{0};
      Int32 value{0};
      bool  pending{false};   // true until finalizeInputWindow() assigns pos
    };

  private:
    // Array of values associated with each event type
    std::array<Int32, LastType> myValues;

    // Ordered (arrival-order) input transitions recorded during the current
    // frame's input poll, replayed by get(type, pos)
    std::vector<Transition> myTransitions;

    // True while the input window is open and set() should record transitions
    bool myRecording{false};

    // Start of the current input window on the system (CPU) clock, and the
    // length of the previous window.  These place transitions at their
    // sub-frame cycle position (see framePosition and finalizeInputWindow).
    // Plain (non-atomic): written under myMutex during the poll, which
    // happens-before the worker thread reads them.
    uInt64 myFrameStartCycle{0}, myCyclesLastFrame{0};

    // Lock-free fast-path gate: true when myTransitions holds any transition
    // for the current frame.  Lets hot-path readers skip the mutex-locked
    // transition scan when nothing changed (see hasTransitions()).
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
