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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef EVENT_HXX
#define EVENT_HXX

#include <mutex>
#include <set>

#include "bspf.hxx"
#include "StellaKeys.hxx"

/**
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
    enum Type
    {
      NoType = 0,
      ConsoleColor, ConsoleBlackWhite,
      ConsoleLeftDiffA, ConsoleLeftDiffB,
      ConsoleRightDiffA, ConsoleRightDiffB,
      ConsoleSelect, ConsoleReset,
      ConsoleLeftDiffToggle, ConsoleRightDiffToggle, ConsoleColorToggle,
      Console7800Pause,

      JoystickZeroUp, JoystickZeroDown, JoystickZeroLeft, JoystickZeroRight,
        JoystickZeroFire, JoystickZeroFire5, JoystickZeroFire9,
      JoystickOneUp, JoystickOneDown, JoystickOneLeft, JoystickOneRight,
        JoystickOneFire, JoystickOneFire5, JoystickOneFire9,

      PaddleZeroDecrease, PaddleZeroIncrease, PaddleZeroAnalog, PaddleZeroFire,
      PaddleOneDecrease, PaddleOneIncrease, PaddleOneAnalog, PaddleOneFire,
      PaddleTwoDecrease, PaddleTwoIncrease, PaddleTwoAnalog, PaddleTwoFire,
      PaddleThreeDecrease, PaddleThreeIncrease, PaddleThreeAnalog, PaddleThreeFire,

      KeyboardZero1, KeyboardZero2, KeyboardZero3,
      KeyboardZero4, KeyboardZero5, KeyboardZero6,
      KeyboardZero7, KeyboardZero8, KeyboardZero9,
      KeyboardZeroStar, KeyboardZero0, KeyboardZeroPound,

      KeyboardOne1, KeyboardOne2, KeyboardOne3,
      KeyboardOne4, KeyboardOne5, KeyboardOne6,
      KeyboardOne7, KeyboardOne8, KeyboardOne9,
      KeyboardOneStar, KeyboardOne0, KeyboardOnePound,

      Combo1, Combo2, Combo3, Combo4, Combo5, Combo6, Combo7, Combo8,
      Combo9, Combo10, Combo11, Combo12, Combo13, Combo14, Combo15, Combo16,

      MouseAxisXMove, MouseAxisYMove,
      MouseButtonLeftValue, MouseButtonRightValue,

      ChangeState, LoadState, SaveState, TakeSnapshot, Quit,
      TogglePauseMode, OptionsMenuMode, CmdMenuMode, TimeMachineMode, DebuggerMode, ExitMode,
      Fry, VolumeDecrease, VolumeIncrease, SoundToggle, VidmodeDecrease, VidmodeIncrease,
      RewindPause, UnwindPause,

      UIUp, UIDown, UILeft, UIRight, UIHome, UIEnd, UIPgUp, UIPgDown,
      UISelect, UINavPrev, UINavNext, UIOK, UICancel, UIPrevDir,
      UITabPrev, UITabNext,

      ToggleFullScreen,
      VidmodeStd, VidmodeRGB, VidmodeSVideo, VidModeComposite, VidModeBad, VidModeCustom,
      PreviousAttribute, NextAttribute, DecreaseAttribute, IncreaseAttribute,
      ScanlinesDecrease, ScanlinesIncrease, VCenterDecrease, VCenterIncrease,

      ToggleP0Collision, ToggleP0Bit, ToggleP1Collision, ToggleP1Bit,
      ToggleM0Collision, ToggleM0Bit, ToggleM1Collision, ToggleM1Bit,
      ToggleBLCollision, ToggleBLBit, TogglePFCollision, TogglePFBit,
      ToggleCollisions, ToggleBits, ToggleFixedColors,

      PhosphorDecrease, PhosphorIncrease, TogglePhosphor, ToggleJitter,
      ToggleFrameStats, ToggleTimeMachine,
      ToggleContSnapshots, ToggleContSnapshotsFrame,
      ToggleColorLoss, TogglePalette,

      HandleMouseControl, ToggleGrabMouse, ToggleSAPortOrder,
      FormatDecrease, FormatIncrease, ReloadConsole,

      Rewind1Menu, Rewind10Menu, RewindAllMenu,
      Unwind1Menu, Unwind10Menu, UnwindAllMenu,

      StartPauseMode, SaveAllStates, LoadAllStates,
      OverscanDecrease, OverscanIncrease,

      ToggleAutoSlot,

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

      ToggleInter,
      MouseAxisXValue, MouseAxisYValue,

      LastType
    };

    // Event categorizing groups
    enum Group
    {
      Menu, Emulation,
      Misc, AudioVideo, States, Console, Joystick, Paddles, Keyboard,
      Debug, Combo,
      LastGroup
    };

    // Event list version, update if the id of existing event types changed
    static constexpr Int32 VERSION = 1;

    using EventSet = std::set<Event::Type>;

  public:
    /**
      Create a new event object.
    */
    Event() { clear(); }

  public:
    /**
      Get the value associated with the event of the specified type.
    */
    Int32 get(Type type) const {
      std::lock_guard<std::mutex> lock(myMutex);

      return myValues[type];
    }

    /**
      Set the value associated with the event of the specified type.
    */
    void set(Type type, Int32 value) {
      std::lock_guard<std::mutex> lock(myMutex);

      myValues[type] = value;
    }

    /**
      Clears the event array (resets to initial state).
    */
    void clear()
    {
      std::lock_guard<std::mutex> lock(myMutex);

      myValues.fill(Event::NoType);
    }

    /**
      Tests if a given event represents continuous or analog values.
    */
    static bool isAnalog(Type type)
    {
      switch(type)
      {
        case Event::PaddleZeroAnalog:
        case Event::PaddleOneAnalog:
        case Event::PaddleTwoAnalog:
        case Event::PaddleThreeAnalog:
          return true;
        default:
          return false;
      }
    }

  private:
    // Array of values associated with each event type
    std::array<Int32, LastType> myValues;

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
static const Event::EventSet LeftJoystickEvents = {
  Event::JoystickZeroUp, Event::JoystickZeroDown, Event::JoystickZeroLeft, Event::JoystickZeroRight,
  Event::JoystickZeroFire, Event::JoystickZeroFire5, Event::JoystickZeroFire9,
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Event::EventSet RightJoystickEvents = {
  Event::JoystickOneUp, Event::JoystickOneDown, Event::JoystickOneLeft, Event::JoystickOneRight,
  Event::JoystickOneFire, Event::JoystickOneFire5, Event::JoystickOneFire9
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Event::EventSet LeftPaddlesEvents = {
  Event::PaddleZeroDecrease, Event::PaddleZeroIncrease, Event::PaddleZeroAnalog, Event::PaddleZeroFire,
  Event::PaddleOneDecrease, Event::PaddleOneIncrease, Event::PaddleOneAnalog, Event::PaddleOneFire,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Event::EventSet RightPaddlesEvents = {
  Event::PaddleTwoDecrease, Event::PaddleTwoIncrease, Event::PaddleTwoAnalog, Event::PaddleTwoFire,
  Event::PaddleThreeDecrease, Event::PaddleThreeIncrease, Event::PaddleThreeAnalog, Event::PaddleThreeFire,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Event::EventSet LeftKeypadEvents = {
  Event::KeyboardZero1, Event::KeyboardZero2, Event::KeyboardZero3,
  Event::KeyboardZero4, Event::KeyboardZero5, Event::KeyboardZero6,
  Event::KeyboardZero7, Event::KeyboardZero8, Event::KeyboardZero9,
  Event::KeyboardZeroStar, Event::KeyboardZero0, Event::KeyboardZeroPound,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Event::EventSet RightKeypadEvents = {
  Event::KeyboardOne1, Event::KeyboardOne2, Event::KeyboardOne3,
  Event::KeyboardOne4, Event::KeyboardOne5, Event::KeyboardOne6,
  Event::KeyboardOne7, Event::KeyboardOne8, Event::KeyboardOne9,
  Event::KeyboardOneStar, Event::KeyboardOne0, Event::KeyboardOnePound,
};

#endif
