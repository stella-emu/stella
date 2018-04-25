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

#include "OSystem.hxx"
#include "Console.hxx"
#include "Settings.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "StellaKeys.hxx"
#include "TIASurface.hxx"
#include "PNGLibrary.hxx"
#include "DialogContainer.hxx"
#include "PKeyboardHandler.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::PhysicalKeyboardHandler(
      OSystem& system, EventHandler& handler, Event& event)
  : myOSystem(system),
    myHandler(handler),
    myEvent(event),
    myAltKeyCounter(0),
    myUseCtrlKeyFlag(myOSystem.settings().getBool("ctrlcombo"))
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem.settings().getString("keymap");
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  IntArray map;
  int value;
  Event::Type e;

  // Get event count, which should be the first int in the list
  buf >> value;
  e = Event::Type(value);
  if(e == Event::LastType)
    while(buf >> value)
      map.push_back(value);

  // Only fill the key mapping array if the data is valid
  if(e == Event::LastType && map.size() == KBDK_LAST * kNumModes)
  {
    // Fill the keymap table with events
    auto ev = map.cbegin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < KBDK_LAST; ++i)
        myKeyTable[i][mode] = Event::Type(*ev++);
  }
  else
  {
    setDefaultMapping(Event::NoType, kEmulationMode);
    setDefaultMapping(Event::NoType, kMenuMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  bool eraseAll = (event == Event::NoType);
  if(eraseAll)
  {
    // Erase all mappings
    for(int i = 0; i < KBDK_LAST; ++i)
      myKeyTable[i][mode] = Event::NoType;
  }

  auto setDefaultKey = [&](StellaKey key, Event::Type k_event)
  {
    if(eraseAll || k_event == event)
      myKeyTable[key][mode] = k_event;
  };

  switch(mode)
  {
    case kEmulationMode:
      setDefaultKey( KBDK_1,         Event::KeyboardZero1     );
      setDefaultKey( KBDK_2,         Event::KeyboardZero2     );
      setDefaultKey( KBDK_3,         Event::KeyboardZero3     );
      setDefaultKey( KBDK_Q,         Event::KeyboardZero4     );
      setDefaultKey( KBDK_W,         Event::KeyboardZero5     );
      setDefaultKey( KBDK_E,         Event::KeyboardZero6     );
      setDefaultKey( KBDK_A,         Event::KeyboardZero7     );
      setDefaultKey( KBDK_S,         Event::KeyboardZero8     );
      setDefaultKey( KBDK_D,         Event::KeyboardZero9     );
      setDefaultKey( KBDK_Z,         Event::KeyboardZeroStar  );
      setDefaultKey( KBDK_X,         Event::KeyboardZero0     );
      setDefaultKey( KBDK_C,         Event::KeyboardZeroPound );

      setDefaultKey( KBDK_8,         Event::KeyboardOne1      );
      setDefaultKey( KBDK_9,         Event::KeyboardOne2      );
      setDefaultKey( KBDK_0,         Event::KeyboardOne3      );
      setDefaultKey( KBDK_I,         Event::KeyboardOne4      );
      setDefaultKey( KBDK_O,         Event::KeyboardOne5      );
      setDefaultKey( KBDK_P,         Event::KeyboardOne6      );
      setDefaultKey( KBDK_K,         Event::KeyboardOne7      );
      setDefaultKey( KBDK_L,         Event::KeyboardOne8      );
      setDefaultKey( KBDK_SEMICOLON, Event::KeyboardOne9      );
      setDefaultKey( KBDK_COMMA,     Event::KeyboardOneStar   );
      setDefaultKey( KBDK_PERIOD,    Event::KeyboardOne0      );
      setDefaultKey( KBDK_SLASH,     Event::KeyboardOnePound  );

      setDefaultKey( KBDK_UP,        Event::JoystickZeroUp    );
      setDefaultKey( KBDK_DOWN,      Event::JoystickZeroDown  );
      setDefaultKey( KBDK_LEFT,      Event::JoystickZeroLeft  );
      setDefaultKey( KBDK_RIGHT,     Event::JoystickZeroRight );
      setDefaultKey( KBDK_SPACE,     Event::JoystickZeroFire  );
      setDefaultKey( KBDK_LCTRL,     Event::JoystickZeroFire  );
      setDefaultKey( KBDK_4,         Event::JoystickZeroFire5 );
      setDefaultKey( KBDK_5,         Event::JoystickZeroFire9 );

      setDefaultKey( KBDK_Y,         Event::JoystickOneUp     );
      setDefaultKey( KBDK_H,         Event::JoystickOneDown   );
      setDefaultKey( KBDK_G,         Event::JoystickOneLeft   );
      setDefaultKey( KBDK_J,         Event::JoystickOneRight  );
      setDefaultKey( KBDK_F,         Event::JoystickOneFire   );
      setDefaultKey( KBDK_6,         Event::JoystickOneFire5  );
      setDefaultKey( KBDK_7,         Event::JoystickOneFire9  );


      setDefaultKey( KBDK_F1,        Event::ConsoleSelect     );
      setDefaultKey( KBDK_F2,        Event::ConsoleReset      );
      setDefaultKey( KBDK_F3,        Event::ConsoleColor      );
      setDefaultKey( KBDK_F4,        Event::ConsoleBlackWhite );
      setDefaultKey( KBDK_F5,        Event::ConsoleLeftDiffA  );
      setDefaultKey( KBDK_F6,        Event::ConsoleLeftDiffB  );
      setDefaultKey( KBDK_F7,        Event::ConsoleRightDiffA );
      setDefaultKey( KBDK_F8,        Event::ConsoleRightDiffB );
      setDefaultKey( KBDK_F9,        Event::SaveState         );
      setDefaultKey( KBDK_F10,       Event::ChangeState       );
      setDefaultKey( KBDK_F11,       Event::LoadState         );
      setDefaultKey( KBDK_F12,       Event::TakeSnapshot      );
      setDefaultKey( KBDK_BACKSPACE, Event::Fry               );
      setDefaultKey( KBDK_PAUSE,     Event::PauseMode         );
      setDefaultKey( KBDK_TAB,       Event::OptionsMenuMode   );
      setDefaultKey( KBDK_BACKSLASH, Event::CmdMenuMode       );
      setDefaultKey( KBDK_T,         Event::TimeMachineMode   );
      setDefaultKey( KBDK_GRAVE,     Event::DebuggerMode      );
      setDefaultKey( KBDK_ESCAPE,    Event::LauncherMode      );
      break;

    case kMenuMode:
      setDefaultKey( KBDK_UP,        Event::UIUp      );
      setDefaultKey( KBDK_DOWN,      Event::UIDown    );
      setDefaultKey( KBDK_LEFT,      Event::UILeft    );
      setDefaultKey( KBDK_RIGHT,     Event::UIRight   );

      setDefaultKey( KBDK_HOME,      Event::UIHome    );
      setDefaultKey( KBDK_END,       Event::UIEnd     );
      setDefaultKey( KBDK_PAGEUP,    Event::UIPgUp    );
      setDefaultKey( KBDK_PAGEDOWN,  Event::UIPgDown  );

      setDefaultKey( KBDK_RETURN,    Event::UISelect  );
      setDefaultKey( KBDK_ESCAPE,    Event::UICancel  );

      setDefaultKey( KBDK_BACKSPACE, Event::UIPrevDir );
      break;

    default:
      return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::eraseMapping(Event::Type event, EventMode mode)
{
  for(int i = 0; i < KBDK_LAST; ++i)
    if(myKeyTable[i][mode] == event && i != KBDK_TAB)
      myKeyTable[i][mode] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::saveMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream keybuf;
  keybuf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < KBDK_LAST; ++i)
      keybuf << ":" << myKeyTable[i][mode];

  myOSystem.settings().setValue("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalKeyboardHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  ostringstream buf;

  for(int k = 0; k < KBDK_LAST; ++k)
  {
    if(myKeyTable[k][mode] == event)
    {
      if(buf.str() != "")
        buf << ", ";
      buf << StellaKeyName::forKey(StellaKey(k));
    }
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::addMapping(Event::Type event, EventMode mode,
                                         StellaKey key)
{
  // These keys cannot be remapped
  if(key == KBDK_TAB || Event::isAnalog(event))
    return false;
  else
    myKeyTable[key][mode] = event;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::handleEvent(StellaKey key, StellaMod mod, bool state)
{
  // Swallow KBDK_TAB under certain conditions
  // See commments on 'myAltKeyCounter' for more information
#ifdef BSPF_UNIX
  if(myAltKeyCounter > 1 && key == KBDK_TAB)
  {
    myAltKeyCounter = 0;
    return;
  }
#endif

  bool handled = true;
  EventHandlerState estate = myHandler.state();

  // Immediately store the key state
  myEvent.setKey(key, state);

  // An attempt to speed up event processing; we quickly check for
  // Control or Alt/Cmd combos first
  if(StellaModTest::isAlt(mod) && state)
  {
#ifdef BSPF_MAC_OSX
    // These keys work in all states
    if(key == KBDK_Q)
    {
      myHandler.handleEvent(Event::Quit, 1);
    }
    else
#endif
    if(key == KBDK_TAB)
    {
      // Swallow Alt-Tab, but remember that it happened
      myAltKeyCounter = 1;
      return;
    }
    else if(key == KBDK_RETURN)
    {
      myOSystem.frameBuffer().toggleFullscreen();
    }
    // State rewinding must work in pause mode too
    else if(estate == EventHandlerState::EMULATION || estate == EventHandlerState::PAUSE)
    {
      switch(key)
      {
        case KBDK_LEFT:  // Alt-left(-shift) rewinds 1(10) states
          myHandler.enterTimeMachineMenuMode((StellaModTest::isShift(mod) && state) ? 10 : 1, false);
          break;

        case KBDK_RIGHT:  // Alt-right(-shift) unwinds 1(10) states
          myHandler.enterTimeMachineMenuMode((StellaModTest::isShift(mod) && state) ? 10 : 1, true);
          break;

        case KBDK_DOWN:  // Alt-down rewinds to start of list
          myHandler.enterTimeMachineMenuMode(1000, false);
          break;

        case KBDK_UP:  // Alt-up rewinds to end of list
          myHandler.enterTimeMachineMenuMode(1000, true);
          break;

        // These can work in pause mode too
        case KBDK_EQUALS:
          myOSystem.frameBuffer().changeWindowedVidMode(+1);
          break;

        case KBDK_MINUS:
          myOSystem.frameBuffer().changeWindowedVidMode(-1);
          break;

        case KBDK_LEFTBRACKET:
          myOSystem.sound().adjustVolume(-1);
          break;

        case KBDK_RIGHTBRACKET:
          myOSystem.sound().adjustVolume(+1);
          break;

        case KBDK_PAGEUP:    // Alt-PageUp increases YStart
          myOSystem.console().changeYStart(+1);
          break;

        case KBDK_PAGEDOWN:  // Alt-PageDown decreases YStart
          myOSystem.console().changeYStart(-1);
          break;

        case KBDK_1:  // Alt-1 turns off NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_OFF);
          break;

        case KBDK_2:  // Alt-2 turns on 'composite' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_COMPOSITE);
          break;

        case KBDK_3:  // Alt-3 turns on 'svideo' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_SVIDEO);
          break;

        case KBDK_4:  // Alt-4 turns on 'rgb' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_RGB);
          break;

        case KBDK_5:  // Alt-5 turns on 'bad' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_BAD);
          break;

        case KBDK_6:  // Alt-6 turns on 'custom' NTSC filtering
          myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::PRESET_CUSTOM);
          break;

        case KBDK_7:  // Alt-7 changes scanline intensity for NTSC filtering
          if(StellaModTest::isShift(mod))
            myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(-5);
          else
            myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(+5);
          break;

        case KBDK_8:  // Alt-8 turns toggles scanline interpolation
          myOSystem.frameBuffer().tiaSurface().toggleScanlineInterpolation();
          break;

        case KBDK_9:  // Alt-9 selects various custom adjustables for NTSC filtering
          if(myOSystem.frameBuffer().tiaSurface().ntscEnabled())
          {
            if(StellaModTest::isShift(mod))
              myOSystem.frameBuffer().showMessage(
                myOSystem.frameBuffer().tiaSurface().ntsc().setPreviousAdjustable());
            else
              myOSystem.frameBuffer().showMessage(
                myOSystem.frameBuffer().tiaSurface().ntsc().setNextAdjustable());
          }
          break;

        case KBDK_0:  // Alt-0 changes custom adjustables for NTSC filtering
          if(myOSystem.frameBuffer().tiaSurface().ntscEnabled())
          {
            if(StellaModTest::isShift(mod))
              myOSystem.frameBuffer().showMessage(
                myOSystem.frameBuffer().tiaSurface().ntsc().decreaseAdjustable());
            else
              myOSystem.frameBuffer().showMessage(
                myOSystem.frameBuffer().tiaSurface().ntsc().increaseAdjustable());
          }
          break;

        case KBDK_Z:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleP0Collision();
          else
            myOSystem.console().toggleP0Bit();
          break;

        case KBDK_X:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleP1Collision();
          else
            myOSystem.console().toggleP1Bit();
          break;

        case KBDK_C:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleM0Collision();
          else
            myOSystem.console().toggleM0Bit();
          break;

        case KBDK_V:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleM1Collision();
          else
            myOSystem.console().toggleM1Bit();
          break;

        case KBDK_B:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleBLCollision();
          else
            myOSystem.console().toggleBLBit();
          break;

        case KBDK_N:
          if(StellaModTest::isShift(mod))
            myOSystem.console().togglePFCollision();
          else
            myOSystem.console().togglePFBit();
          break;

        case KBDK_COMMA:
          myOSystem.console().toggleFixedColors();
          break;

        case KBDK_PERIOD:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleCollisions();
          else
            myOSystem.console().toggleBits();
          break;

        case KBDK_I:  // Alt-i decreases phosphor blend
          myOSystem.console().changePhosphor(-1);
          break;

        case KBDK_O:  // Alt-o increases phosphor blend
          myOSystem.console().changePhosphor(+1);
          break;

        case KBDK_P:  // Alt-p toggles phosphor effect
          myOSystem.console().togglePhosphor();
          break;

        case KBDK_J:  // Alt-j toggles scanline jitter
          myOSystem.console().toggleJitter();
          break;

        case KBDK_L:
          myOSystem.frameBuffer().toggleFrameStats();
          break;

        case KBDK_T:  // Alt-t toggles Time Machine
          myOSystem.state().toggleTimeMachine();
          break;

        case KBDK_S:
          myOSystem.png().toggleContinuousSnapshots(StellaModTest::isShift(mod));
          break;

        default:
          handled = false;
          break;
      }
    }
    else
      handled = false;
  }
  else if(StellaModTest::isControl(mod) && state && myUseCtrlKeyFlag)
  {
    // These keys work in all states
    if(key == KBDK_Q)
    {
      myHandler.handleEvent(Event::Quit, 1);
    }
    // These only work when in emulation mode
    else if(estate == EventHandlerState::EMULATION || estate == EventHandlerState::PAUSE)
    {
      switch(key)
      {
        case KBDK_0:  // Ctrl-0 switches between mouse control modes
          myHandler.handleMouseControl();
          break;

        case KBDK_1:  // Ctrl-1 swaps Stelladaptor/2600-daptor ports
          myHandler.toggleSAPortOrder();
          break;

        case KBDK_F:  // (Shift) Ctrl-f toggles NTSC/PAL/SECAM mode
          myOSystem.console().toggleFormat(StellaModTest::isShift(mod) ? -1 : 1);
          break;

        case KBDK_G:  // Ctrl-g (un)grabs mouse
          if(!myOSystem.frameBuffer().fullScreen())
          {
            myOSystem.frameBuffer().toggleGrabMouse();
            myOSystem.frameBuffer().showMessage(myOSystem.frameBuffer().grabMouseEnabled()
                                                ? "Grab mouse enabled" : "Grab mouse disabled");
          }
          break;

        case KBDK_L:  // Ctrl-l toggles PAL color-loss effect
          myOSystem.console().toggleColorLoss();
          break;

        case KBDK_P:  // Ctrl-p toggles different palettes
          myOSystem.console().togglePalette();
          break;

        case KBDK_R:  // Ctrl-r reloads the currently loaded ROM
          myOSystem.reloadConsole();
          break;

        case KBDK_PAGEUP:    // Ctrl-PageUp increases Height
          myOSystem.console().changeHeight(+1);
          break;

        case KBDK_PAGEDOWN:  // Ctrl-PageDown decreases Height
          myOSystem.console().changeHeight(-1);
          break;

        case KBDK_S:         // Ctrl-s saves properties to a file
        {
          string filename = myOSystem.baseDir() +
              myOSystem.console().properties().get(Cartridge_Name) + ".pro";
          ofstream out(filename);
          if(out)
          {
            out << myOSystem.console().properties();
            myOSystem.frameBuffer().showMessage("Properties saved");
          }
          else
            myOSystem.frameBuffer().showMessage("Error saving properties");
          break;
        }

        default:
          handled = false;
          break;
      }
    }
    else
      handled = false;
  }
  else
    handled = false;

  // Don't pass the key on if we've already taken care of it
  if(handled) return;

  // Arrange the logic to take advantage of short-circuit evaluation
  if(!(StellaModTest::isControl(mod) || StellaModTest::isShift(mod) || StellaModTest::isAlt(mod)))
  {
    // Special handling for Escape key
    // Basically, exit whichever mode we're currently in
    if(state && key == KBDK_ESCAPE)
    {
      switch(estate)
      {
        case EventHandlerState::PAUSE:
          myHandler.changeStateByEvent(Event::PauseMode);
          return;
        case EventHandlerState::CMDMENU:
          myHandler.changeStateByEvent(Event::CmdMenuMode);
          return;
        case EventHandlerState::TIMEMACHINE:
          myHandler.changeStateByEvent(Event::TimeMachineMode);
          return;
#if 0 // FIXME - exits ROM too, when it should just go back to ROM
        case EventHandlerState::DEBUGGER:
          myHandler.changeStateByEvent(Event::DebuggerMode);
          return;
#endif
        default:
          break;
      }
    }

    // Handle keys which switch eventhandler state
    if(!state && myHandler.changeStateByEvent(myKeyTable[key][kEmulationMode]))
      return;
  }

  // Otherwise, let the event handler deal with it
  switch(estate)
  {
    case EventHandlerState::EMULATION:
      myHandler.handleEvent(myKeyTable[key][kEmulationMode], state);
      break;

    case EventHandlerState::PAUSE:
      switch(myKeyTable[key][kEmulationMode])
      {
        case Event::TakeSnapshot:
        case Event::DebuggerMode:
          myHandler.handleEvent(myKeyTable[key][kEmulationMode], state);
          break;

        default:
          break;
      }
      break;

    default:
      if(myHandler.hasOverlay())
        myHandler.overlay().handleKeyEvent(key, mod, state);
      break;
  }
}
