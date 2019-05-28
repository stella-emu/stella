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

#include <sstream>
#include <map>

#include "bspf.hxx"
#include "Logger.hxx"

#include "Base.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PJoystickHandler.hxx"
#include "PointingDevice.hxx"
#include "PropsSet.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "TimerManager.hxx"
#include "Switches.hxx"
#include "M6532.hxx"
#include "MouseControl.hxx"
#include "PNGLibrary.hxx"
#include "TIASurface.hxx"

#include "EventHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#ifdef GUI_SUPPORT
  #include "Menu.hxx"
  #include "CommandMenu.hxx"
  #include "DialogContainer.hxx"
  #include "Launcher.hxx"
  #include "TimeMachine.hxx"
  #include "ListWidget.hxx"
  #include "ScrollBarWidget.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem& osystem)
  : myOSystem(osystem),
    myOverlay(nullptr),
    myState(EventHandlerState::NONE),
    myAllowAllDirectionsFlag(false),
    myFryingFlag(false),
    mySkipMouseMotion(true),
    myIs7800(false)
{
  // Create keyboard handler (to handle all physical keyboard functionality)
  myPKeyHandler = make_unique<PhysicalKeyboardHandler>(osystem, *this, myEvent);

  // Create joystick handler (to handle all physical joystick functionality)
  myPJoyHandler = make_unique<PhysicalJoystickHandler>(osystem, *this, myEvent);

  // Erase the 'combo' array
  for(int i = 0; i < COMBO_SIZE; ++i)
    for(int j = 0; j < EVENTS_PER_COMBO; ++j)
      myComboTable[i][j] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::initialize()
{
  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setComboMap();
  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  Joystick::setDeadZone(myOSystem.settings().getInt("joydeadzone"));
  Paddles::setDejitterBase(myOSystem.settings().getInt("dejitter.base"));
  Paddles::setDejitterDiff(myOSystem.settings().getInt("dejitter.diff"));
  Paddles::setDigitalSensitivity(myOSystem.settings().getInt("dsense"));
  Paddles::setMouseSensitivity(myOSystem.settings().getInt("msense"));
  PointingDevice::setSensitivity(myOSystem.settings().getInt("tsense"));

#ifdef GUI_SUPPORT
  // Set quick select delay when typing characters in listwidgets
  ListWidget::setQuickSelectDelay(myOSystem.settings().getInt("listdelay"));

  // Set number of lines a mousewheel will scroll
  ScrollBarWidget::setWheelLines(myOSystem.settings().getInt("mwheel"));
#endif

  // Integer to string conversions (for HEX) use upper or lower-case
  Common::Base::setHexUppercase(myOSystem.settings().getBool("dbg.uhex"));

  // Default phosphor blend
  Properties::setDefault(PropType::Display_PPBlend,
                         myOSystem.settings().getString("tv.phosblend"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(EventHandlerState state)
{
  setState(state);
  myOSystem.state().reset();
#ifdef PNG_SUPPORT
  myOSystem.png().setContinuousSnapInterval(0);
#endif
  myFryingFlag = false;

  // Reset events almost immediately after starting emulation mode
  // We wait a little while (0.5s), since 'hold' events may be present,
  // and we want time for the ROM to process them
  if(state == EventHandlerState::EMULATION)
    myOSystem.timer().setTimeout([&ev = myEvent]() { ev.clear(); }, 500);
  // Toggle 7800 mode
  set7800Mode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::addPhysicalJoystick(PhysicalJoystickPtr joy)
{
#ifdef JOYSTICK_SUPPORT
  int ID = myPJoyHandler->add(joy);
  if(ID < 0)
    return;

  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  ostringstream buf;
  buf << "Added joystick " << ID << ":" << endl
      << "  " << joy->about() << endl;
  Logger::log(buf.str(), 1);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::removePhysicalJoystick(int id)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->remove(id);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapStelladaptors(const string& saport)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->mapStelladaptors(saport);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::toggleSAPortOrder()
{
#ifdef JOYSTICK_SUPPORT
  const string& saport = myOSystem.settings().getString("saport");
  if(saport == "lr")
  {
    mapStelladaptors("rl");
    myOSystem.frameBuffer().showMessage("Stelladaptor ports right/left");
  }
  else
  {
    mapStelladaptors("lr");
    myOSystem.frameBuffer().showMessage("Stelladaptor ports left/right");
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::set7800Mode()
{
  if(myOSystem.hasConsole())
    myIs7800 = myOSystem.console().switches().check7800Mode(myOSystem.settings());
  else
    myIs7800 = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseControl()
{
  if(myMouseControl)
    myOSystem.frameBuffer().showMessage(myMouseControl->next());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt64 time)
{
  // Process events from the underlying hardware
  pollEvent();

  // Update controllers and console switches, and in general all other things
  // related to emulation
  if(myState == EventHandlerState::EMULATION)
  {
    myOSystem.console().riot().update();

    // Now check if the StateManager should be saving or loading state
    // (for rewind and/or movies
    if(myOSystem.state().mode() != StateManager::Mode::Off)
      myOSystem.state().update();

  #ifdef CHEATCODE_SUPPORT
    for(auto& cheat: myOSystem.cheat().perFrame())
      cheat->evaluate();
  #endif

  #ifdef PNG_SUPPORT
    // Handle continuous snapshots
    if(myOSystem.png().continuousSnapEnabled())
      myOSystem.png().updateTime(time);
  #endif
  }
  else if(myOverlay)
  {
  #ifdef GUI_SUPPORT
    // Update the current dialog container at regular intervals
    // Used to implement continuous events
    myOverlay->updateTime(time);
  #endif
  }

  // Turn off all mouse-related items; if they haven't been taken care of
  // in the previous ::update() methods, they're now invalid
  myEvent.set(Event::MouseAxisXValue, 0);
  myEvent.set(Event::MouseAxisYValue, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleTextEvent(char text)
{
#ifdef GUI_SUPPORT
  // Text events are only used in GUI mode
  if(myOverlay)
    myOverlay->handleTextEvent(text);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseMotionEvent(int x, int y, int xrel, int yrel)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == EventHandlerState::EMULATION)
  {
    if(!mySkipMouseMotion)
    {
      myEvent.set(Event::MouseAxisXValue, xrel);
      myEvent.set(Event::MouseAxisYValue, yrel);
    }
    mySkipMouseMotion = false;
  }
#ifdef GUI_SUPPORT
  else if(myOverlay)
    myOverlay->handleMouseMotionEvent(x, y);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseButtonEvent(MouseButton b, bool pressed,
                                          int x, int y)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == EventHandlerState::EMULATION)
  {
    switch(b)
    {
      case MouseButton::LEFT:
        myEvent.set(Event::MouseButtonLeftValue, int(pressed));
        break;
      case MouseButton::RIGHT:
        myEvent.set(Event::MouseButtonRightValue, int(pressed));
        break;
      default:
        return;
    }
  }
#ifdef GUI_SUPPORT
  else if(myOverlay)
    myOverlay->handleMouseButtonEvent(b, pressed, x, y);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleSystemEvent(SystemEvent e, int, int)
{
  switch(e)
  {
    case SystemEvent::WINDOW_EXPOSED:
    case SystemEvent::WINDOW_RESIZED:
      myOSystem.frameBuffer().update(true); // force full update
      break;

    case SystemEvent::WINDOW_FOCUS_GAINED:
      // Used to handle Alt-x key combos; sometimes the key associated with
      // Alt gets 'stuck'  and is passed to the core for processing
      if(myPKeyHandler->altKeyCount() > 0)
        myPKeyHandler->altKeyCount() = 2;
      break;
#if 0
    case SystemEvent::WINDOW_MINIMIZED:
      if(myState == EventHandlerState::EMULATION) enterMenuMode(EventHandlerState::OPTIONSMENU);
        break;
#endif
    default:  // handle other events as testing requires
      // cerr << "handleSystemEvent: " << e << endl;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, bool pressed)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  switch(event)
  {
    ////////////////////////////////////////////////////////////////////////
    // If enabled, make sure 'impossible' joystick directions aren't allowed
    case Event::JoystickZeroUp:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(!myAllowAllDirectionsFlag && pressed)
        myEvent.set(Event::JoystickOneLeft, 0);
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::Fry:
      if(myPKeyHandler->useCtrlKey()) myFryingFlag = pressed;
      return;

    case Event::ReloadConsole:
      if (pressed) myOSystem.reloadConsole();
      return;

    case Event::VolumeDecrease:
      if(pressed) myOSystem.sound().adjustVolume(-1);
      return;

    case Event::VolumeIncrease:
      if(pressed) myOSystem.sound().adjustVolume(+1);
      return;

    case Event::SoundToggle:
      if(pressed) myOSystem.sound().toggleMute();
      return;

    case Event::VidmodeDecrease:
      if(pressed) myOSystem.frameBuffer().changeVidMode(-1);
      return;

    case Event::VidmodeIncrease:
      if(pressed) myOSystem.frameBuffer().changeVidMode(+1);
      return;

    case Event::ToggleFullScreen:
      if (pressed) myOSystem.frameBuffer().toggleFullscreen();
      return;

    case Event::VidmodeStd:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::OFF);
      return;

    case Event::VidmodeRGB:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::RGB);
      return;

    case Event::VidmodeSVideo:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::SVIDEO);
      return;

    case Event::VidModeComposite:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::COMPOSITE);
      return;

    case Event::VidModeBad:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::BAD);
      return;

    case Event::VidModeCustom:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::CUSTOM);
      return;

    case Event::ScanlinesDecrease:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(-5);
      return;

    case Event::ScanlinesIncrease:
      if (pressed) myOSystem.frameBuffer().tiaSurface().setScanlineIntensity(+5);
      return;

    case Event::PreviousAttribute:
      if (pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::CUSTOM);
        myOSystem.frameBuffer().showMessage(
          myOSystem.frameBuffer().tiaSurface().ntsc().setPreviousAdjustable());
      }
      return;

    case Event::NextAttribute:
      if (pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::CUSTOM);
        myOSystem.frameBuffer().showMessage(
          myOSystem.frameBuffer().tiaSurface().ntsc().setNextAdjustable());
      }
      return;

    case Event::DecreaseAttribute:
      if (pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::CUSTOM);
        myOSystem.frameBuffer().showMessage(
          myOSystem.frameBuffer().tiaSurface().ntsc().decreaseAdjustable());
      }
      return;

    case Event::IncreaseAttribute:
      if (pressed)
      {
        myOSystem.frameBuffer().tiaSurface().setNTSC(NTSCFilter::Preset::CUSTOM);
        myOSystem.frameBuffer().showMessage(
          myOSystem.frameBuffer().tiaSurface().ntsc().increaseAdjustable());
      }
      return;

    case Event::DecreasePhosphor:
      if (pressed) myOSystem.console().changePhosphor(-1);
      return;

    case Event::IncreasePhosphor:
      if (pressed) myOSystem.console().changePhosphor(1);
      return;

    case Event::TogglePhosphor:
      if (pressed) myOSystem.console().togglePhosphor();
      return;

    case Event::ToggleColorLoss:
      if (pressed) myOSystem.console().toggleColorLoss();
      return;

    case Event::TogglePalette:
      if (pressed) myOSystem.console().togglePalette();
      return;

    case Event::ToggleJitter:
      if (pressed) myOSystem.console().toggleJitter();
      return;

    case Event::ToggleFrameStats:
      if (pressed) myOSystem.frameBuffer().toggleFrameStats();
      return;

    case Event::ToggleTimeMachine:
      if (pressed) myOSystem.state().toggleTimeMachine();
      return;

  #ifdef PNG_SUPPORT
    case Event::ToggleContSnapshots:
      if (pressed) myOSystem.png().toggleContinuousSnapshots(false);
      return;

    case Event::ToggleContSnapshotsFrame:
      if (pressed) myOSystem.png().toggleContinuousSnapshots(true);
      return;
  #endif

    case Event::HandleMouseControl:
      if (pressed) handleMouseControl();
      return;

    case Event::ToggleSAPortOrder:
      if (pressed) toggleSAPortOrder();
      return;

    case Event::DecreaseFormat:
      if (pressed) myOSystem.console().toggleFormat(-1);
      return;

    case Event::IncreaseFormat:
      if (pressed) myOSystem.console().toggleFormat(1);
      return;

    case Event::ToggleGrabMouse:
      if (pressed && !myOSystem.frameBuffer().fullScreen())
      {
        myOSystem.frameBuffer().toggleGrabMouse();
        myOSystem.frameBuffer().showMessage(myOSystem.frameBuffer().grabMouseEnabled()
                                            ? "Grab mouse enabled" : "Grab mouse disabled");
      }
      return;

    case Event::ToggleP0Collision:
      if (pressed) myOSystem.console().toggleP0Collision();
      return;

    case Event::ToggleP0Bit:
      if (pressed) myOSystem.console().toggleP0Bit();
      return;

    case Event::ToggleP1Collision:
      if (pressed) myOSystem.console().toggleP1Collision();
      return;

    case Event::ToggleP1Bit:
      if (pressed) myOSystem.console().toggleP1Bit();
      return;

    case Event::ToggleM0Collision:
      if (pressed) myOSystem.console().toggleM0Collision();
      return;

    case Event::ToggleM0Bit:
      if (pressed) myOSystem.console().toggleM0Bit();
      return;

    case Event::ToggleM1Collision:
      if (pressed) myOSystem.console().toggleM1Collision();
      return;

    case Event::ToggleM1Bit:
      if (pressed) myOSystem.console().toggleM1Bit();
      return;

    case Event::ToggleBLCollision:
      if (pressed) myOSystem.console().toggleBLCollision();
      return;

    case Event::ToggleBLBit:
      if (pressed) myOSystem.console().toggleBLBit();
      return;

    case Event::TogglePFCollision:
      if (pressed) myOSystem.console().togglePFCollision();
      return;

    case Event::TogglePFBit:
      if (pressed) myOSystem.console().togglePFBit();
      return;

    case Event::ToggleFixedColors:
      if (pressed) myOSystem.console().toggleFixedColors();
      return;

    case Event::ToggleCollisions:
      if (pressed) myOSystem.console().toggleCollisions();
      return;

    case Event::ToggleBits:
      if (pressed) myOSystem.console().toggleBits();
      return;

    case Event::SaveState:
      if(pressed) myOSystem.state().saveState();
      return;

    case Event::ChangeState:
      if(pressed) myOSystem.state().changeState();
      return;

    case Event::LoadState:
      if(pressed) myOSystem.state().loadState();
      return;

    case Event::Rewind:
      if (pressed) myOSystem.state().rewindStates();
      return;

    case Event::Unwind:
      if (pressed) myOSystem.state().unwindStates();
      return;

    case Event::TakeSnapshot:
      if(pressed) myOSystem.frameBuffer().tiaSurface().saveSnapShot();
      return;

    case Event::ExitMode:
      // Special handling for Escape key
      // Basically, exit whichever mode we're currently in
      switch (myState)
      {
        case EventHandlerState::PAUSE:
          if (pressed) changeStateByEvent(Event::PauseMode);
          return;

        case EventHandlerState::CMDMENU:
          if (pressed) changeStateByEvent(Event::CmdMenuMode);
          return;

        case EventHandlerState::TIMEMACHINE:
          if (pressed) changeStateByEvent(Event::TimeMachineMode);
          return;

        #if 0 // FIXME - exits ROM too, when it should just go back to ROM
        case EventHandlerState::DEBUGGER:
          if (pressed) changeStateByEvent(Event::DebuggerMode);
          return;
        #endif

        case EventHandlerState::EMULATION:
          if (pressed)
          {
            // Go back to the launcher, or immediately quit
            if (myOSystem.settings().getBool("exitlauncher") ||
              myOSystem.launcherUsed())
              myOSystem.createLauncher();
            else
              handleEvent(Event::Quit);
          }
          return;

        default:
          return;
      }

    case Event::Quit:
      if(pressed)
      {
        saveKeyMapping();
        saveJoyMapping();
        myOSystem.quit();
      }
      return;

    ////////////////////////////////////////////////////////////////////////
    // A combo event is simply multiple calls to handleEvent, once for
    // each event it contains
    case Event::Combo1:
    case Event::Combo2:
    case Event::Combo3:
    case Event::Combo4:
    case Event::Combo5:
    case Event::Combo6:
    case Event::Combo7:
    case Event::Combo8:
    case Event::Combo9:
    case Event::Combo10:
    case Event::Combo11:
    case Event::Combo12:
    case Event::Combo13:
    case Event::Combo14:
    case Event::Combo15:
    case Event::Combo16:
      for(int i = 0, combo = event - Event::Combo1; i < EVENTS_PER_COMBO; ++i)
        if(myComboTable[combo][i] != Event::NoType)
          handleEvent(myComboTable[combo][i], pressed);
      return;
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // Events which relate to switches()
    case Event::ConsoleColor:
      if(pressed)
      {
        myEvent.set(Event::ConsoleBlackWhite, 0);
        myOSystem.frameBuffer().showMessage(myIs7800 ? "Pause released" : "Color Mode");
      }
      break;
    case Event::ConsoleBlackWhite:
      if(pressed)
      {
        myEvent.set(Event::ConsoleColor, 0);
        myOSystem.frameBuffer().showMessage(myIs7800 ? "Pause pushed" : "B/W Mode");
      }
      break;
    case Event::ConsoleColorToggle:
      if(pressed)
      {
        if(myOSystem.console().switches().tvColor())
        {
          myEvent.set(Event::ConsoleBlackWhite, 1);
          myEvent.set(Event::ConsoleColor, 0);
          myOSystem.frameBuffer().showMessage(myIs7800 ? "Pause pushed" : "B/W Mode");
        }
        else
        {
          myEvent.set(Event::ConsoleBlackWhite, 0);
          myEvent.set(Event::ConsoleColor, 1);
          myOSystem.frameBuffer().showMessage(myIs7800 ? "Pause released" : "Color Mode");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::Console7800Pause:
      if(pressed)
      {
        myEvent.set(Event::ConsoleBlackWhite, 0);
        myEvent.set(Event::ConsoleColor, 0);
        if (myIs7800)
          myOSystem.frameBuffer().showMessage("Pause pressed");
      }
      break;

    case Event::ConsoleLeftDiffA:
      if(pressed)
      {
        myEvent.set(Event::ConsoleLeftDiffB, 0);
        myOSystem.frameBuffer().showMessage(GUI::LEFT_DIFFICULTY + " A");
      }
      break;
    case Event::ConsoleLeftDiffB:
      if(pressed)
      {
        myEvent.set(Event::ConsoleLeftDiffA, 0);
        myOSystem.frameBuffer().showMessage(GUI::LEFT_DIFFICULTY + " B");
      }
      break;
    case Event::ConsoleLeftDiffToggle:
      if(pressed)
      {
        if(myOSystem.console().switches().leftDifficultyA())
        {
          myEvent.set(Event::ConsoleLeftDiffA, 0);
          myEvent.set(Event::ConsoleLeftDiffB, 1);
          myOSystem.frameBuffer().showMessage(GUI::LEFT_DIFFICULTY + " B");
        }
        else
        {
          myEvent.set(Event::ConsoleLeftDiffA, 1);
          myEvent.set(Event::ConsoleLeftDiffB, 0);
          myOSystem.frameBuffer().showMessage(GUI::LEFT_DIFFICULTY + " A");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleRightDiffA:
      if(pressed)
      {
        myEvent.set(Event::ConsoleRightDiffB, 0);
        myOSystem.frameBuffer().showMessage(GUI::RIGHT_DIFFICULTY + " A");
      }
      break;
    case Event::ConsoleRightDiffB:
      if(pressed)
      {
        myEvent.set(Event::ConsoleRightDiffA, 0);
        myOSystem.frameBuffer().showMessage(GUI::RIGHT_DIFFICULTY + " B");
      }
      break;
    case Event::ConsoleRightDiffToggle:
      if(pressed)
      {
        if(myOSystem.console().switches().rightDifficultyA())
        {
          myEvent.set(Event::ConsoleRightDiffA, 0);
          myEvent.set(Event::ConsoleRightDiffB, 1);
          myOSystem.frameBuffer().showMessage(GUI::RIGHT_DIFFICULTY + " B");
        }
        else
        {
          myEvent.set(Event::ConsoleRightDiffA, 1);
          myEvent.set(Event::ConsoleRightDiffB, 0);
          myOSystem.frameBuffer().showMessage(GUI::RIGHT_DIFFICULTY + " A");
        }
        myOSystem.console().switches().update();
      }
      return;
    ////////////////////////////////////////////////////////////////////////

    case Event::NoType:  // Ignore unmapped events
      return;

    default:
      break;
  }

  // Otherwise, pass it to the emulation core
  myEvent.set(event, pressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleConsoleStartupEvents()
{
  bool update = false;
  if(myOSystem.settings().getBool("holdreset"))
  {
    handleEvent(Event::ConsoleReset);
    update = true;
  }
  if(myOSystem.settings().getBool("holdselect"))
  {
    handleEvent(Event::ConsoleSelect);
    update = true;
  }

  const string& holdjoy0 = myOSystem.settings().getString("holdjoy0");
  update = update || holdjoy0 != "";
  if(BSPF::containsIgnoreCase(holdjoy0, "U"))
    handleEvent(Event::JoystickZeroUp);
  if(BSPF::containsIgnoreCase(holdjoy0, "D"))
    handleEvent(Event::JoystickZeroDown);
  if(BSPF::containsIgnoreCase(holdjoy0, "L"))
    handleEvent(Event::JoystickZeroLeft);
  if(BSPF::containsIgnoreCase(holdjoy0, "R"))
    handleEvent(Event::JoystickZeroRight);
  if(BSPF::containsIgnoreCase(holdjoy0, "F"))
    handleEvent(Event::JoystickZeroFire);

  const string& holdjoy1 = myOSystem.settings().getString("holdjoy1");
  update = update || holdjoy1 != "";
  if(BSPF::containsIgnoreCase(holdjoy1, "U"))
    handleEvent(Event::JoystickOneUp);
  if(BSPF::containsIgnoreCase(holdjoy1, "D"))
    handleEvent(Event::JoystickOneDown);
  if(BSPF::containsIgnoreCase(holdjoy1, "L"))
    handleEvent(Event::JoystickOneLeft);
  if(BSPF::containsIgnoreCase(holdjoy1, "R"))
    handleEvent(Event::JoystickOneRight);
  if(BSPF::containsIgnoreCase(holdjoy1, "F"))
    handleEvent(Event::JoystickOneFire);

  if(update)
    myOSystem.console().riot().update();

#ifdef DEBUGGER_SUPPORT
  if(myOSystem.settings().getBool("debug"))
    enterDebugMode();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::changeStateByEvent(Event::Type type)
{
  bool handled = true;

  switch(type)
  {
    case Event::PauseMode:
      if(myState == EventHandlerState::EMULATION)
        setState(EventHandlerState::PAUSE);
      else if(myState == EventHandlerState::PAUSE)
        setState(EventHandlerState::EMULATION);
      else
        handled = false;
      break;

    case Event::OptionsMenuMode:
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE)
        enterMenuMode(EventHandlerState::OPTIONSMENU);
      else
        handled = false;
      break;

    case Event::CmdMenuMode:
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE)
        enterMenuMode(EventHandlerState::CMDMENU);
      else if(myState == EventHandlerState::CMDMENU && !myOSystem.settings().getBool("minimal_ui"))
        // The extra check for "minimal_ui" allows mapping e.g. right joystick fire
        //  to open the command dialog and navigate there using that fire button
        leaveMenuMode();
      else
        handled = false;
      break;

    case Event::TimeMachineMode:
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE)
        enterTimeMachineMenuMode(0, false);
      else if(myState == EventHandlerState::TIMEMACHINE)
        leaveMenuMode();
      else
        handled = false;
      break;

    case Event::DebuggerMode:
  #ifdef DEBUGGER_SUPPORT
      if(myState == EventHandlerState::EMULATION || myState == EventHandlerState::PAUSE
         || myState == EventHandlerState::TIMEMACHINE)
        enterDebugMode();
      else if(myState == EventHandlerState::DEBUGGER && myOSystem.debugger().canExit())
        leaveDebugMode();
      else
        handled = false;
  #endif
      break;

    default:
      handled = false;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setActionMappings(EventMode mode)
{
  int listsize = 0;
  ActionList* list = nullptr;

  switch(mode)
  {
    case kEmulationMode:
      listsize = EMUL_ACTIONLIST_SIZE;
      list     = ourEmulActionList;
      break;
    case kMenuMode:
      listsize = MENU_ACTIONLIST_SIZE;
      list     = ourMenuActionList;
      break;
    default:
      return;
  }

  // Fill the ActionList with the current key and joystick mappings
  for(int i = 0; i < listsize; ++i)
  {
    Event::Type event = list[i].event;
    list[i].key = "None";
    string key = myPKeyHandler->getMappingDesc(event, mode);

#ifdef JOYSTICK_SUPPORT
    string joydesc = myPJoyHandler->getMappingDesc(event, mode);
    if(joydesc != "")
    {
      if(key != "")
        key += ", ";
      key += joydesc;
    }
#endif

    if(key != "")
      list[i].key = key;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboMap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem.settings().getString("combomap");
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  // Erase the 'combo' array
  auto ERASE_ALL = [&]() {
    for(int i = 0; i < COMBO_SIZE; ++i)
      for(int j = 0; j < EVENTS_PER_COMBO; ++j)
        myComboTable[i][j] = Event::NoType;
  };

  // Get combo count, which should be the first int in the list
  // If it isn't, then we treat the entire list as invalid
  if(!buf.good())
    ERASE_ALL();
  else
  {
    string key;
    buf >> key;
    if(atoi(key.c_str()) == COMBO_SIZE)
    {
      // Fill the combomap table with events for as long as they exist
      int combocount = 0;
      while(buf >> key && combocount < COMBO_SIZE)
      {
        // Each event in a comboevent is separated by a comma
        replace(key.begin(), key.end(), ',', ' ');
        istringstream buf2(key);

        int eventcount = 0;
        while(buf2 >> key && eventcount < EVENTS_PER_COMBO)
        {
          myComboTable[combocount][eventcount] = Event::Type(atoi(key.c_str()));
          ++eventcount;
        }
        ++combocount;
      }
    }
    else
      ERASE_ALL();
  }

  saveComboMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::removePhysicalJoystickFromDatabase(const string& name)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->remove(name);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, StellaKey key, StellaMod mod)
{
  bool mapped = myPKeyHandler->addMapping(event, mode, key, mod);
  if(mapped)
    setActionMappings(mode);

  return mapped;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyAxisMapping(Event::Type event, EventMode mode,
                                     int stick, int axis, int value,
                                     bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  bool mapped = myPJoyHandler->addAxisMapping(event, mode, stick, axis, value);
  if(mapped && updateMenus)
    setActionMappings(mode);

  return mapped;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyButtonMapping(Event::Type event, EventMode mode,
                                       int stick, int button,
                                       bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  bool mapped = myPJoyHandler->addBtnMapping(event, mode, stick, button);
  if(mapped && updateMenus)
    setActionMappings(mode);

  return mapped;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyHatMapping(Event::Type event, EventMode mode,
                                    int stick, int hat, JoyHat value,
                                    bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  bool mapped = myPJoyHandler->addHatMapping(event, mode, stick, hat, value);
  if(mapped && updateMenus)
    setActionMappings(mode);

  return mapped;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // Erase the KeyEvent array
  myPKeyHandler->eraseMapping(event, mode);

#ifdef JOYSTICK_SUPPORT
  // Erase the joystick mapping arrays
  myPJoyHandler->eraseMapping(event, mode);
#endif

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  setDefaultKeymap(event, mode);
  setDefaultJoymap(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap(Event::Type event, EventMode mode)
{
  myPKeyHandler->setDefaultMapping(event, mode);
  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap(Event::Type event, EventMode mode)
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->setDefaultMapping(event, mode);
  setActionMappings(mode);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  myPKeyHandler->saveMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
#ifdef JOYSTICK_SUPPORT
  myPJoyHandler->saveMapping();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveComboMapping()
{
  // Iterate through the combomap table and create a colon-separated list
  // For each combo event, create a comma-separated list of its events
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << COMBO_SIZE;
  for(int i = 0; i < COMBO_SIZE; ++i)
  {
    buf << ":" << myComboTable[i][0];
    for(int j = 1; j < EVENTS_PER_COMBO; ++j)
      buf << "," << myComboTable[i][j];
  }
  myOSystem.settings().setValue("combomap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getActionList(EventMode mode) const
{
  StringList l;
  switch(mode)
  {
    case kEmulationMode:
      for(uInt32 i = 0; i < EMUL_ACTIONLIST_SIZE; ++i)
        l.push_back(EventHandler::ourEmulActionList[i].action);
      break;
    case kMenuMode:
      for(uInt32 i = 0; i < MENU_ACTIONLIST_SIZE; ++i)
        l.push_back(EventHandler::ourMenuActionList[i].action);
      break;
    default:
      break;
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VariantList EventHandler::getComboList(EventMode /**/) const
{
  // For now, this only works in emulation mode
  VariantList l;
  ostringstream buf;

  VarList::push_back(l, "None", "-1");
  for(uInt32 i = 0; i < EMUL_ACTIONLIST_SIZE; ++i)
  {
    if(EventHandler::ourEmulActionList[i].allow_combo)
    {
      buf << i;
      VarList::push_back(l, EventHandler::ourEmulActionList[i].action, buf.str());
      buf.str("");
    }
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getComboListForEvent(Event::Type event) const
{
  StringList l;
  ostringstream buf;
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    int combo = event - Event::Combo1;
    for(uInt32 i = 0; i < EVENTS_PER_COMBO; ++i)
    {
      Event::Type e = myComboTable[combo][i];
      for(uInt32 j = 0; j < EMUL_ACTIONLIST_SIZE; ++j)
      {
        if(EventHandler::ourEmulActionList[j].event == e &&
           EventHandler::ourEmulActionList[j].allow_combo)
        {
          buf << j;
          l.push_back(buf.str());
          buf.str("");
        }
      }
      // Make sure entries are 1-to-1, using '-1' to indicate Event::NoType
      if(i == l.size())
        l.push_back("-1");
    }
  }
  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboListForEvent(Event::Type event, const StringList& events)
{
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    assert(events.size() == 8);
    int combo = event - Event::Combo1;
    for(int i = 0; i < 8; ++i)
    {
      int idx = atoi(events[i].c_str());
      if(idx >= 0 && idx < EMUL_ACTIONLIST_SIZE)
        myComboTable[combo][i] = EventHandler::ourEmulActionList[idx].event;
      else
        myComboTable[combo][i] = Event::NoType;
    }
    saveComboMapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type EventHandler::eventAtIndex(int idx, EventMode mode) const
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= EMUL_ACTIONLIST_SIZE)
        return Event::NoType;
      else
        return ourEmulActionList[idx].event;
    case kMenuMode:
      if(idx < 0 || idx >= MENU_ACTIONLIST_SIZE)
        return Event::NoType;
      else
        return ourMenuActionList[idx].event;
    default:
      return Event::NoType;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::actionAtIndex(int idx, EventMode mode) const
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= EMUL_ACTIONLIST_SIZE)
        return EmptyString;
      else
        return ourEmulActionList[idx].action;
    case kMenuMode:
      if(idx < 0 || idx >= MENU_ACTIONLIST_SIZE)
        return EmptyString;
      else
        return ourMenuActionList[idx].action;
    default:
      return EmptyString;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::keyAtIndex(int idx, EventMode mode) const
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= EMUL_ACTIONLIST_SIZE)
        return EmptyString;
      else
        return ourEmulActionList[idx].key;
    case kMenuMode:
      if(idx < 0 || idx >= MENU_ACTIONLIST_SIZE)
        return EmptyString;
      else
        return ourMenuActionList[idx].key;
    default:
      return EmptyString;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setMouseControllerMode(const string& enable)
{
  if(myOSystem.hasConsole())
  {
    bool usemouse = false;
    if(BSPF::equalsIgnoreCase(enable, "always"))
      usemouse = true;
    else if(BSPF::equalsIgnoreCase(enable, "never"))
      usemouse = false;
    else  // 'analog'
    {
      usemouse = myOSystem.console().leftController().isAnalog() ||
                 myOSystem.console().rightController().isAnalog();
    }

    const string& control = usemouse ?
      myOSystem.console().properties().get(PropType::Controller_MouseAxis) : "none";

    myMouseControl = make_unique<MouseControl>(myOSystem.console(), control);
    myMouseControl->next();  // set first available mode
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(EventHandlerState state)
{
#ifdef GUI_SUPPORT
  setState(state);
  myOverlay->reStack();
  myOSystem.sound().mute(true);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
#ifdef GUI_SUPPORT
  setState(EventHandlerState::EMULATION);
  myOSystem.sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::enterDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  if(myState == EventHandlerState::DEBUGGER || !myOSystem.hasConsole())
    return false;

  // Make sure debugger starts in a consistent state
  // This absolutely *has* to come before we actually change to debugger
  // mode, since it takes care of locking the debugger state, which will
  // probably be modified below
  myOSystem.debugger().setStartState();
  setState(EventHandlerState::DEBUGGER);

  FBInitStatus fbstatus = myOSystem.createFrameBuffer();
  if(fbstatus != FBInitStatus::Success)
  {
    myOSystem.debugger().setQuitState();
    setState(EventHandlerState::EMULATION);
    if(fbstatus == FBInitStatus::FailTooLarge)
      myOSystem.frameBuffer().showMessage("Debugger window too large for screen",
                                          MessagePosition::BottomCenter, true);
    return false;
  }
  myOverlay->reStack();
  myOSystem.sound().mute(true);
#else
  myOSystem.frameBuffer().showMessage("Debugger support not included",
                                      MessagePosition::BottomCenter, true);
#endif

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  // paranoia: this should never happen:
  if(myState != EventHandlerState::DEBUGGER)
    return;

  // Make sure debugger quits in a consistent state
  myOSystem.debugger().setQuitState();

  setState(EventHandlerState::EMULATION);
  myOSystem.createFrameBuffer();
  myOSystem.sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterTimeMachineMenuMode(uInt32 numWinds, bool unwind)
{
#ifdef GUI_SUPPORT
  // add one extra state if we are in Time Machine mode
  // TODO: maybe remove this state if we leave the menu at this new state
  myOSystem.state().addExtraState("enter Time Machine dialog"); // force new state

  if(numWinds)
    // hande winds and display wind message (numWinds != 0) in time machine dialog
    myOSystem.timeMachine().setEnterWinds(unwind ? numWinds : -numWinds);

  enterMenuMode(EventHandlerState::TIMEMACHINE);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setState(EventHandlerState state)
{
  myState = state;

  // Normally, the usage of Control key is determined by 'ctrlcombo'
  // For certain ROMs it may be forced off, whatever the setting
  myPKeyHandler->useCtrlKey() = myOSystem.settings().getBool("ctrlcombo");

  // Only enable text input in GUI modes, since in emulation mode the
  // keyboard acts as one large joystick with many (single) buttons
  myOverlay = nullptr;
  switch(myState)
  {
    case EventHandlerState::EMULATION:
      myOSystem.sound().mute(false);
      enableTextEvents(false);
      if(myOSystem.console().leftController().type() == Controller::Type::CompuMate)
        myPKeyHandler->useCtrlKey() = false;
      break;

    case EventHandlerState::PAUSE:
      myOSystem.sound().mute(true);
      enableTextEvents(false);
      break;

  #ifdef GUI_SUPPORT
    case EventHandlerState::OPTIONSMENU:
      myOverlay = &myOSystem.menu();
      enableTextEvents(true);
      break;

    case EventHandlerState::CMDMENU:
      myOverlay = &myOSystem.commandMenu();
      enableTextEvents(true);
      break;

    case EventHandlerState::TIMEMACHINE:
      myOSystem.timeMachine().requestResize();
      myOverlay = &myOSystem.timeMachine();
      enableTextEvents(true);
      break;

    case EventHandlerState::LAUNCHER:
      myOverlay = &myOSystem.launcher();
      enableTextEvents(true);
      break;
  #endif

  #ifdef DEBUGGER_SUPPORT
    case EventHandlerState::DEBUGGER:
      myOverlay = &myOSystem.debugger();
      enableTextEvents(true);
      break;
  #endif

    case EventHandlerState::NONE:
    default:
      break;
  }

  // Inform various subsystems about the new state
  myOSystem.stateChanged(myState);
  myOSystem.frameBuffer().stateChanged(myState);
  myOSystem.frameBuffer().setCursorState();
  if(myOSystem.hasConsole())
    myOSystem.console().stateChanged(myState);

  // Sometimes an extraneous mouse motion event is generated
  // after a state change, which should be supressed
  mySkipMouseMotion = true;

  // Erase any previously set events, since a state change implies
  // that old events are now invalid
  myEvent.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourEmulActionList[EMUL_ACTIONLIST_SIZE] = {
  { Event::ConsoleSelect,           "Select",                                "", true  },
  { Event::ConsoleReset,            "Reset",                                 "", true  },
  { Event::ConsoleColor,            "Color TV",                              "", true  },
  { Event::ConsoleBlackWhite,       "Black & White TV",                      "", true  },
  { Event::ConsoleColorToggle,      "Swap Color / B&W TV",                   "", true  },
  { Event::Console7800Pause,        "7800 Pause Key",                        "", true  },
  { Event::ConsoleLeftDiffA,        "P0 Difficulty A",                       "", true  },
  { Event::ConsoleLeftDiffB,        "P0 Difficulty B",                       "", true  },
  { Event::ConsoleLeftDiffToggle,   "P0 swap Difficulty",                    "", true  },
  { Event::ConsoleRightDiffA,       "P1 Difficulty A",                       "", true  },
  { Event::ConsoleRightDiffB,       "P1 Difficulty B",                       "", true  },
  { Event::ConsoleRightDiffToggle,  "P1 swap Difficulty",                    "", true  },
  { Event::SaveState,               "Save state",                            "", false },
  { Event::ChangeState,             "Change state",                          "", false },
  { Event::LoadState,               "Load state",                            "", false },
  { Event::TakeSnapshot,            "Snapshot",                              "", false },
  { Event::Fry,                     "Fry cartridge",                         "", false },
  { Event::VidmodeDecrease,         "Previous zoom level",                   "", false },
  { Event::VidmodeIncrease,         "Next zoom level",                       "", false },
  { Event::ToggleFullScreen,        "Toggle fullscreen",                     "", false },

  { Event::VidmodeStd,              "Disable TV effects",                    "", false },
  { Event::VidmodeRGB,              "Select 'RGB' preset",                   "", false },
  { Event::VidmodeSVideo,           "Select 'S-Video' preset",               "", false },
  { Event::VidModeComposite,        "Select 'Composite' preset",             "", false },
  { Event::VidModeBad,              "Select 'Badly adjusted' preset",        "", false },
  { Event::VidModeCustom,           "Select 'Custom' preset",                "", false },
  { Event::PreviousAttribute,       "Select previous 'Custom' attribute",    "", false },
  { Event::NextAttribute,           "Select next 'Custom' attribute",        "", false },
  { Event::DecreaseAttribute,       "Decrease selected 'Custom' attribute",  "", false },
  { Event::IncreaseAttribute,       "Increase selected 'Custom' attribute",  "", false },
  { Event::ScanlinesDecrease,       "Decrease scanlines",                    "", false },
  { Event::ScanlinesIncrease,       "Increase scanlines",                    "", false },
  { Event::TogglePhosphor,          "Toggle 'phosphor' effect",              "", false },
  { Event::DecreasePhosphor,        "Decrease 'phosphor' blend",             "", false },
  { Event::IncreasePhosphor,        "Increase 'phosphor' blend",             "", false },
  { Event::DecreaseFormat,          "Decrease display format",               "", false },
  { Event::IncreaseFormat,          "Increase display format",               "", false },
  { Event::TogglePalette,           "Switch palette (Standard/Z26/User)",    "", false },
#ifdef PNG_SUPPORT
  { Event::ToggleContSnapshots,     "Save cont. PNG snapsh. (as defined)",   "", false },
  { Event::ToggleContSnapshotsFrame,"Save cont. PNG snapsh. (every frame)",  "", false },
#endif
  { Event::ToggleTimeMachine,       "Toggle 'Time Machine' mode",            "", false },

  { Event::VolumeDecrease,          "Decrease volume",                       "", false },
  { Event::VolumeIncrease,          "Increase volume",                       "", false },
  { Event::SoundToggle,             "Toggle sound",                          "", false },
  { Event::PauseMode,               "Pause",                                 "", false },
  { Event::OptionsMenuMode,         "Enter options menu UI",                 "", false },
  { Event::CmdMenuMode,             "Toggle command menu UI",                "", false },
  { Event::TimeMachineMode,         "Toggle time machine UI",                "", false },
  { Event::Rewind,                  "Rewind game",                           "", false },
  { Event::Unwind,                  "Unwind game",                           "", false },
  { Event::DebuggerMode,            "Toggle debugger mode",                  "", false },
  { Event::ReloadConsole,           "Reload current ROM/load next game",     "", false },
  { Event::ExitMode,                "Exit current Stella mode",              "", false },
  { Event::Quit,                    "Quit",                                  "", false },

  { Event::HandleMouseControl,      "Disable TV effects",                    "", false },
  { Event::ToggleGrabMouse,         "Select 'RGB' preset",                   "", false },
  { Event::ToggleSAPortOrder,       "Select 'S-Video' preset",               "", false },

  { Event::JoystickZeroUp,          "P0 Joystick Up",                        "", true  },
  { Event::JoystickZeroDown,        "P0 Joystick Down",                      "", true  },
  { Event::JoystickZeroLeft,        "P0 Joystick Left",                      "", true  },
  { Event::JoystickZeroRight,       "P0 Joystick Right",                     "", true  },
  { Event::JoystickZeroFire,        "P0 Joystick Fire",                      "", true  },
  { Event::JoystickZeroFire5,       "P0 Booster Top Booster Button",         "", true  },
  { Event::JoystickZeroFire9,       "P0 Booster Handle Grip Trigger",        "", true  },

  { Event::JoystickOneUp,           "P1 Joystick Up",                        "", true  },
  { Event::JoystickOneDown,         "P1 Joystick Down",                      "", true  },
  { Event::JoystickOneLeft,         "P1 Joystick Left",                      "", true  },
  { Event::JoystickOneRight,        "P1 Joystick Right",                     "", true  },
  { Event::JoystickOneFire,         "P1 Joystick Fire",                      "", true  },
  { Event::JoystickOneFire5,        "P1 Booster Top Booster Button",         "", true  },
  { Event::JoystickOneFire9,        "P1 Booster Handle Grip Trigger",        "", true  },

  { Event::PaddleZeroAnalog,        "Paddle 0 Analog",                       "", true  },
  { Event::PaddleZeroDecrease,      "Paddle 0 Decrease",                     "", true  },
  { Event::PaddleZeroIncrease,      "Paddle 0 Increase",                     "", true  },
  { Event::PaddleZeroFire,          "Paddle 0 Fire",                         "", true  },

  { Event::PaddleOneAnalog,         "Paddle 1 Analog",                       "", true  },
  { Event::PaddleOneDecrease,       "Paddle 1 Decrease",                     "", true  },
  { Event::PaddleOneIncrease,       "Paddle 1 Increase",                     "", true  },
  { Event::PaddleOneFire,           "Paddle 1 Fire",                         "", true  },

  { Event::PaddleTwoAnalog,         "Paddle 2 Analog",                       "", true  },
  { Event::PaddleTwoDecrease,       "Paddle 2 Decrease",                     "", true  },
  { Event::PaddleTwoIncrease,       "Paddle 2 Increase",                     "", true  },
  { Event::PaddleTwoFire,           "Paddle 2 Fire",                         "", true  },

  { Event::PaddleThreeAnalog,       "Paddle 3 Analog",                       "", true  },
  { Event::PaddleThreeDecrease,     "Paddle 3 Decrease",                     "", true  },
  { Event::PaddleThreeIncrease,     "Paddle 3 Increase",                     "", true  },
  { Event::PaddleThreeFire,         "Paddle 3 Fire",                         "", true  },

  { Event::KeyboardZero1,           "P0 Keyboard 1",                         "", true  },
  { Event::KeyboardZero2,           "P0 Keyboard 2",                         "", true  },
  { Event::KeyboardZero3,           "P0 Keyboard 3",                         "", true  },
  { Event::KeyboardZero4,           "P0 Keyboard 4",                         "", true  },
  { Event::KeyboardZero5,           "P0 Keyboard 5",                         "", true  },
  { Event::KeyboardZero6,           "P0 Keyboard 6",                         "", true  },
  { Event::KeyboardZero7,           "P0 Keyboard 7",                         "", true  },
  { Event::KeyboardZero8,           "P0 Keyboard 8",                         "", true  },
  { Event::KeyboardZero9,           "P0 Keyboard 9",                         "", true  },
  { Event::KeyboardZeroStar,        "P0 Keyboard *",                         "", true  },
  { Event::KeyboardZero0,           "P0 Keyboard 0",                         "", true  },
  { Event::KeyboardZeroPound,       "P0 Keyboard #",                         "", true  },

  { Event::KeyboardOne1,            "P1 Keyboard 1",                         "", true  },
  { Event::KeyboardOne2,            "P1 Keyboard 2",                         "", true  },
  { Event::KeyboardOne3,            "P1 Keyboard 3",                         "", true  },
  { Event::KeyboardOne4,            "P1 Keyboard 4",                         "", true  },
  { Event::KeyboardOne5,            "P1 Keyboard 5",                         "", true  },
  { Event::KeyboardOne6,            "P1 Keyboard 6",                         "", true  },
  { Event::KeyboardOne7,            "P1 Keyboard 7",                         "", true  },
  { Event::KeyboardOne8,            "P1 Keyboard 8",                         "", true  },
  { Event::KeyboardOne9,            "P1 Keyboard 9",                         "", true  },
  { Event::KeyboardOneStar,         "P1 Keyboard *",                         "", true  },
  { Event::KeyboardOne0,            "P1 Keyboard 0",                         "", true  },
  { Event::KeyboardOnePound,        "P1 Keyboard #",                         "", true  },

  { Event::Combo1,                  "Combo 1",                               "", false },
  { Event::Combo2,                  "Combo 2",                               "", false },
  { Event::Combo3,                  "Combo 3",                               "", false },
  { Event::Combo4,                  "Combo 4",                               "", false },
  { Event::Combo5,                  "Combo 5",                               "", false },
  { Event::Combo6,                  "Combo 6",                               "", false },
  { Event::Combo7,                  "Combo 7",                               "", false },
  { Event::Combo8,                  "Combo 8",                               "", false },
  { Event::Combo9,                  "Combo 9",                               "", false },
  { Event::Combo10,                 "Combo 10",                              "", false },
  { Event::Combo11,                 "Combo 11",                              "", false },
  { Event::Combo12,                 "Combo 12",                              "", false },
  { Event::Combo13,                 "Combo 13",                              "", false },
  { Event::Combo14,                 "Combo 14",                              "", false },
  { Event::Combo15,                 "Combo 15",                              "", false },
  { Event::Combo16,                 "Combo 16",                              "", false },

  { Event::ToggleFrameStats,        "Toggle frame stats",                    "", false },
  { Event::ToggleP0Bit,             "Toggle TIA Player0 object",             "", false },
  { Event::ToggleP0Collision,       "Toggle TIA Player0 collisions",         "", false },
  { Event::ToggleP1Bit,             "Toggle TIA Player1 object",             "", false },
  { Event::ToggleP1Collision,       "Toggle TIA Player1 collisions",         "", false },
  { Event::ToggleM0Bit,             "Toggle TIA Missile0 object",            "", false },
  { Event::ToggleM0Collision,       "Toggle TIA Missile0 collisions",        "", false },
  { Event::ToggleM1Bit,             "Toggle TIA Missile1 object",            "", false },
  { Event::ToggleM1Collision,       "Toggle TIA Missile1 collisions",        "", false },
  { Event::ToggleBLBit,             "Toggle TIA Ball object",                "", false },
  { Event::ToggleBLCollision,       "Toggle TIA Ball collisions",            "", false },
  { Event::TogglePFBit,             "Toggle TIA Playfield object",           "", false },
  { Event::TogglePFCollision,       "Toggle TIA Playfield collisions",       "", false },
  { Event::ToggleFixedColors,       "Toggle TIA 'Fixed Debug Colors' mode",  "", false },
  { Event::ToggleBits,              "Toggle all TIA objects",                "", false },
  { Event::ToggleCollisions,        "Toggle all TIA collisions",             "", false },
  { Event::ToggleColorLoss,         "Toggle PAL color-loss effect",          "", false },
  { Event::ToggleJitter,            "Toggle TV 'Jitter' effect",             "", false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourMenuActionList[MENU_ACTIONLIST_SIZE] = {
  { Event::UIUp,              "Move Up",              "", false },
  { Event::UIDown,            "Move Down",            "", false },
  { Event::UILeft,            "Move Left",            "", false },
  { Event::UIRight,           "Move Right",           "", false },

  { Event::UIHome,            "Home",                 "", false },
  { Event::UIEnd,             "End",                  "", false },
  { Event::UIPgUp,            "Page Up",              "", false },
  { Event::UIPgDown,          "Page Down",            "", false },

  { Event::UIOK,              "OK",                   "", false },
  { Event::UICancel,          "Cancel",               "", false },
  { Event::UISelect,          "Select item",          "", false },

  { Event::UINavPrev,         "Previous object",      "", false },
  { Event::UINavNext,         "Next object",          "", false },
  { Event::UITabPrev,         "Previous tab",         "", false },
  { Event::UITabNext,         "Next tab",             "", false },

  { Event::UIPrevDir,         "Parent directory",     "", false },
  { Event::ToggleFullScreen,  "Toggle fullscreen",    "", false },
  { Event::Quit,              "Quit",                 "", false }
};
