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

#include "Console.hxx"
#include "Event.hxx"
#include "FSNode.hxx"
#include "System.hxx"
#include "CompuMate.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompuMate::CompuMate(const Console& console, const Event& event,
                     const System& system)
  : myConsole{console},
    myEvent{event},
    mySystem{system},
    myCassette(system)
{
  // These controller pointers will be retrieved by the Console, which will
  // also take ownership of them
  myLeftController  = std::make_unique<CMControl>(*this, Controller::Jack::Left, event, system);
  myRightController = std::make_unique<CMControl>(*this, Controller::Jack::Right, event, system);

  myLeftController->setPin(Controller::AnalogPin::Nine, AnalogReadout::connectToGround());
  myLeftController->setPin(Controller::AnalogPin::Five, AnalogReadout::connectToVcc());
  myRightController->setPin(Controller::AnalogPin::Nine, AnalogReadout::connectToVcc());
  myRightController->setPin(Controller::AnalogPin::Five, AnalogReadout::connectToGround());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::loadCassette(const FSNode& romFile)
{
  myCassette.load(romFile, myConsole.timing());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::update()
{
  // Handle SWCHA changes - the following is modelled on code from z26
  Controller& lp = myConsole.leftController();
  Controller& rp = myConsole.rightController();

  using E  = Event::Type;
  using DP = Controller::DigitalPin;
  using AP = Controller::AnalogPin;

  lp.setPin(AP::Nine,  AnalogReadout::connectToGround());
  lp.setPin(AP::Five,  AnalogReadout::connectToVcc());
  lp.setPin(DP::Six,   true);
  rp.setPin(AP::Nine,  AnalogReadout::connectToVcc());
  rp.setPin(AP::Five,  AnalogReadout::connectToGround());
  rp.setPin(DP::Six,   true);
  rp.setPin(DP::Three, true);
  rp.setPin(DP::Four,  true);

  if(myEvent.get(Event::CompuMateFunc))
    lp.setPin(AP::Nine, AnalogReadout::connectToVcc());
  if(myEvent.get(Event::CompuMateShift))
    rp.setPin(AP::Five, AnalogReadout::connectToVcc());

  // LUT for CompuMate keys; faster than manual if statements
  struct ColMap {
    E lp6, rp3, rp6, rp4;
    E shiftKey;  // key that mimics Shift+lp6 (sets RP.A5 high)
    E funcKey;   // key that mimics Func+rp4  (sets LP.A9 high)
  };

  /**
    Note that several CompuMate keys are mapped to actual keyboard keys
    for convenience, in addition to their normal mapping.  This mapping
    is done at a higher level; we just handle the CompuMate-specific
    events here:
                                             Actual key on real keyboard
      // CompuMateQuote        (Shift-0)  -> "
      // CompuMatePlus         (Shift-1)  -> + (Shift =)
      // CompuMateMinus        (Shift-2)  -> -
      // CompuMateSlash        (Shift-4)  -> /
      // CompuMateEquals       (Shift-5)  -> =
      // CompuMateQuestion     (Shift-6)  -> ?
      // CompuMateLeftBracket  (Shift-8)  -> [
      // CompuMateRightBracket (Shift-9)  -> ]
      // CompuMateBackspace (Ctrl-space)  -> Backspace
  */
  static constexpr std::array<ColMap, 10> columns {{
    { E::CompuMate7, E::CompuMateU, E::CompuMateJ, E::CompuMateM,
      E::NoType, E::NoType },
    { E::CompuMate6, E::CompuMateY, E::CompuMateH, E::CompuMateN,
      E::CompuMateQuestion, E::NoType },
    { E::CompuMate8, E::CompuMateI, E::CompuMateK, E::CompuMateComma,
      E::CompuMateLeftBracket, E::NoType },
    { E::CompuMate2, E::CompuMateW, E::CompuMateS, E::CompuMateX,
      E::CompuMateMinus, E::NoType },
    { E::CompuMate3, E::CompuMateE, E::CompuMateD, E::CompuMateC,
      E::NoType, E::NoType },
    { E::CompuMate0, E::CompuMateP, E::CompuMateEnter, E::CompuMateSpace,
      E::CompuMateQuote, E::CompuMateBackspace },
    { E::CompuMate9, E::CompuMateO, E::CompuMateL, E::CompuMatePeriod,
      E::CompuMateRightBracket, E::NoType },
    { E::CompuMate5, E::CompuMateT, E::CompuMateG, E::CompuMateB,
      E::CompuMateEquals, E::NoType },
    { E::CompuMate1, E::CompuMateQ, E::CompuMateA, E::CompuMateZ,
      E::CompuMatePlus, E::NoType },
    { E::CompuMate4, E::CompuMateR, E::CompuMateF, E::CompuMateV,
      E::CompuMateSlash, E::NoType },
  }};

  // myColumn is updated inside CartCM class
  const auto& col = columns[myColumn];

  if(col.lp6        != E::NoType && myEvent.get(col.lp6))
    lp.setPin(DP::Six,   false);
  if(col.rp3        != E::NoType && myEvent.get(col.rp3))
    rp.setPin(DP::Three, false);
  if(col.rp6        != E::NoType && myEvent.get(col.rp6))
    rp.setPin(DP::Six,   false);
  if(col.rp4        != E::NoType && myEvent.get(col.rp4))
    rp.setPin(DP::Four,  false);

  if(col.shiftKey != E::NoType && myEvent.get(col.shiftKey)) {
    rp.setPin(AP::Five, AnalogReadout::connectToVcc());
    lp.setPin(DP::Six,  false);
  }
  if(col.funcKey  != E::NoType && myEvent.get(col.funcKey)) {
    lp.setPin(AP::Nine, AnalogReadout::connectToVcc());
    rp.setPin(DP::Four, false);
  }

  // Returns true if any non-Enter character key is currently pressed
  const auto anyCharPressed = [&]() -> bool {
    return std::ranges::any_of(columns, [&](const auto& cc) {
      return (cc.lp6     != E::NoType && myEvent.get(cc.lp6))
          || (cc.rp3     != E::NoType && myEvent.get(cc.rp3))
          || (cc.rp6     != E::NoType && cc.rp6 != E::CompuMateEnter && myEvent.get(cc.rp6))
          || (cc.rp4     != E::NoType && myEvent.get(cc.rp4))
          || (cc.shiftKey != E::NoType && myEvent.get(cc.shiftKey));
    });
  };

  // Detect Func+J (LOAD command) followed by Enter to start cassette playback
  if(!myCassette.isPlaying())
  {
    if(myEvent.get(E::CompuMateFunc) && myEvent.get(E::CompuMateJ) && !myLoadArm.seen)
    {
      myLoadArm.seen = true;
      myLoadArm.bufLen = 1;  // LOAD is a single token
      myLoadArm.waitingForRelease = true;
      // TODO: open file-browser dialog here; for now use default cassette path
      myPendingLoadPath = myCassette.defaultCassettePath();
    }

    // Suppress character tracking until J is released so the J from Func+J
    // isn't counted as a new character press
    if(myLoadArm.seen && myLoadArm.waitingForRelease && !myEvent.get(E::CompuMateJ))
    {
      myLoadArm.waitingForRelease = false;
      myLoadArm.prevAnyChar = false;
    }

    // Backspace removes the last token; if the buffer empties, load is cancelled
    const bool backspaceNow = myEvent.get(E::CompuMateBackspace) != 0;
    if(myLoadArm.seen && backspaceNow && !myLoadArm.prevBackspace)
      if(--myLoadArm.bufLen == 0)
        myLoadArm.seen = false;
    myLoadArm.prevBackspace = backspaceNow;

    // Track character keypresses to keep buffer length in sync with the ROM
    if(myLoadArm.seen && !myLoadArm.waitingForRelease)
    {
      const bool anyCharNow = anyCharPressed();
      if(anyCharNow && !myLoadArm.prevAnyChar)
        ++myLoadArm.bufLen;
      myLoadArm.prevAnyChar = anyCharNow;
    }

    const bool enterNow = myEvent.get(E::CompuMateEnter) != 0;
    if(enterNow && !myLoadArm.prevEnter && myLoadArm.seen)
    {
      myCassette.load(myPendingLoadPath, myConsole.timing());
      myCassette.startPlayback(mySystem.cycles());
      myLoadArm.seen = false;
    }
    myLoadArm.prevEnter = enterNow;
  }

  // Detect Func+H (SAVE command) followed by Enter to start cassette save
  if(!myCassette.isRecording())
  {
    if(myEvent.get(E::CompuMateFunc) && myEvent.get(E::CompuMateH) && !mySaveArm.seen)
    {
      mySaveArm.seen = true;
      mySaveArm.bufLen = 1;  // SAVE is a single token
      mySaveArm.waitingForRelease = true;
      // TODO: open file-browser dialog here; for now fall back to sibling path
      myCassette.setSavePath(myCassette.defaultCassettePath());
    }

    if(mySaveArm.seen && mySaveArm.waitingForRelease && !myEvent.get(E::CompuMateH))
    {
      mySaveArm.waitingForRelease = false;
      mySaveArm.prevAnyChar = false;
    }

    const bool backspaceNow = myEvent.get(E::CompuMateBackspace) != 0;
    if(mySaveArm.seen && backspaceNow && !mySaveArm.prevBackspace)
      if(--mySaveArm.bufLen == 0)
        mySaveArm.seen = false;
    mySaveArm.prevBackspace = backspaceNow;

    if(mySaveArm.seen && !mySaveArm.waitingForRelease)
    {
      const bool anyCharNow = anyCharPressed();
      if(anyCharNow && !mySaveArm.prevAnyChar)
        ++mySaveArm.bufLen;
      mySaveArm.prevAnyChar = anyCharNow;
    }

    const bool enterNow = myEvent.get(E::CompuMateEnter) != 0;
    if(enterNow && !mySaveArm.prevEnter && mySaveArm.seen)
    {
      myCassette.startRecord();
      mySaveArm.seen = false;
    }
    mySaveArm.prevEnter = enterNow;
  }
  else
    myCassette.checkTimeout(mySystem.cycles());
}
