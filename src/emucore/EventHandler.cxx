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

#include <sstream>
#include <map>

#include "bspf.hxx"

#include "Base.hxx"
#include "CommandMenu.hxx"
#include "Console.hxx"
#include "DialogContainer.hxx"
#include "Event.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "Launcher.hxx"
#include "TimeMachine.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PJoystickHandler.hxx"
#include "PointingDevice.hxx"
#include "PropsSet.hxx"
#include "ListWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "Switches.hxx"
#include "M6532.hxx"
#include "MouseControl.hxx"
#include "PNGLibrary.hxx"

#include "EventHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
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
  for(int i = 0; i < kComboSize; ++i)
    for(int j = 0; j < kEventsPerCombo; ++j)
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
  Paddles::setDigitalSensitivity(myOSystem.settings().getInt("dsense"));
  Paddles::setMouseSensitivity(myOSystem.settings().getInt("msense"));
  PointingDevice::setSensitivity(myOSystem.settings().getInt("tsense"));

  // Set quick select delay when typing characters in listwidgets
  ListWidget::setQuickSelectDelay(myOSystem.settings().getInt("listdelay"));

  // Set number of lines a mousewheel will scroll
  ScrollBarWidget::setWheelLines(myOSystem.settings().getInt("mwheel"));

  // Integer to string conversions (for HEX) use upper or lower-case
  Common::Base::setHexUppercase(myOSystem.settings().getBool("dbg.uhex"));

  // Default phosphor blend
  Properties::setDefault(Display_PPBlend,
                         myOSystem.settings().getString("tv.phosblend"));

  // Toggle 7800 mode
  set7800Mode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(EventHandlerState state)
{
  setState(state);
  myOSystem.state().reset();
  myOSystem.png().setContinuousSnapInterval(0);
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
  myOSystem.logMessage(buf.str(), 1);
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
    myIs7800 = myOSystem.console().switches().toggle7800Mode(myOSystem.settings());
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

    // Handle continuous snapshots
    if(myOSystem.png().continuousSnapEnabled())
      myOSystem.png().updateTime(time);
  }
  else if(myOverlay)
  {
    // Update the current dialog container at regular intervals
    // Used to implement continuous events
    myOverlay->updateTime(time);
  }

  // Turn off all mouse-related items; if they haven't been taken care of
  // in the previous ::update() methods, they're now invalid
  myEvent.set(Event::MouseAxisXValue, 0);
  myEvent.set(Event::MouseAxisYValue, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleTextEvent(char text)
{
  // Text events are only used in GUI mode
  if(myOverlay)
    myOverlay->handleTextEvent(text);
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
  else if(myOverlay)
    myOverlay->handleMouseMotionEvent(x, y);
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
  else if(myOverlay)
    myOverlay->handleMouseButtonEvent(b, pressed, x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleSystemEvent(SystemEvent e, int, int)
{
  switch(e)
  {
    case SystemEvent::WINDOW_EXPOSED:
      myOSystem.frameBuffer().update();
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
void EventHandler::handleEvent(Event::Type event, Int32 state)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  switch(event)
  {
    ////////////////////////////////////////////////////////////////////////
    // If enabled, make sure 'impossible' joystick directions aren't allowed
    case Event::JoystickZeroUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneLeft, 0);
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::Fry:
      if(myPKeyHandler->useCtrlKey()) myFryingFlag = bool(state);
      return;

    case Event::VolumeDecrease:
      if(state) myOSystem.sound().adjustVolume(-1);
      return;

    case Event::VolumeIncrease:
      if(state) myOSystem.sound().adjustVolume(+1);
      return;

    case Event::SaveState:
      if(state) myOSystem.state().saveState();
      return;

    case Event::ChangeState:
      if(state) myOSystem.state().changeState();
      return;

    case Event::LoadState:
      if(state) myOSystem.state().loadState();
      return;

    case Event::TakeSnapshot:
      if(state) myOSystem.png().takeSnapshot();
      return;

    case Event::LauncherMode:
      if((myState == EventHandlerState::EMULATION || myState == EventHandlerState::CMDMENU ||
          myState == EventHandlerState::DEBUGGER) && state)
      {
        // Go back to the launcher, or immediately quit
        if(myOSystem.settings().getBool("exitlauncher") ||
           myOSystem.launcherUsed())
          myOSystem.createLauncher();
        else
          handleEvent(Event::Quit, 1);
      }
      return;

    case Event::Quit:
      if(state)
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
      for(int i = 0, combo = event - Event::Combo1; i < kEventsPerCombo; ++i)
        if(myComboTable[combo][i] != Event::NoType)
          handleEvent(myComboTable[combo][i], state);
      return;
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // Events which relate to switches()
    case Event::ConsoleColor:
      if(state && !myIs7800)
      {
        myEvent.set(Event::ConsoleBlackWhite, 0);
        myOSystem.frameBuffer().showMessage("Color Mode");
      }
      break;
    case Event::ConsoleBlackWhite:
      if(state && !myIs7800)
      {
        myEvent.set(Event::ConsoleColor, 0);
        myOSystem.frameBuffer().showMessage("BW Mode");
      }
      break;
    case Event::ConsoleColorToggle:
      if(state && !myIs7800)
      {
        if(myOSystem.console().switches().tvColor())
        {
          myEvent.set(Event::ConsoleBlackWhite, 1);
          myEvent.set(Event::ConsoleColor, 0);
          myOSystem.frameBuffer().showMessage("BW Mode");
        }
        else
        {
          myEvent.set(Event::ConsoleBlackWhite, 0);
          myEvent.set(Event::ConsoleColor, 1);
          myOSystem.frameBuffer().showMessage("Color Mode");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleLeftDiffA:
      if(state)
      {
        myEvent.set(Event::ConsoleLeftDiffB, 0);
        myOSystem.frameBuffer().showMessage("Left Difficulty A");
      }
      break;
    case Event::ConsoleLeftDiffB:
      if(state)
      {
        myEvent.set(Event::ConsoleLeftDiffA, 0);
        myOSystem.frameBuffer().showMessage("Left Difficulty B");
      }
      break;
    case Event::ConsoleLeftDiffToggle:
      if(state)
      {
        if(myOSystem.console().switches().leftDifficultyA())
        {
          myEvent.set(Event::ConsoleLeftDiffA, 0);
          myEvent.set(Event::ConsoleLeftDiffB, 1);
          myOSystem.frameBuffer().showMessage("Left Difficulty B");
        }
        else
        {
          myEvent.set(Event::ConsoleLeftDiffA, 1);
          myEvent.set(Event::ConsoleLeftDiffB, 0);
          myOSystem.frameBuffer().showMessage("Left Difficulty A");
        }
        myOSystem.console().switches().update();
      }
      return;

    case Event::ConsoleRightDiffA:
      if(state)
      {
        myEvent.set(Event::ConsoleRightDiffB, 0);
        myOSystem.frameBuffer().showMessage("Right Difficulty A");
      }
      break;
    case Event::ConsoleRightDiffB:
      if(state)
      {
        myEvent.set(Event::ConsoleRightDiffA, 0);
        myOSystem.frameBuffer().showMessage("Right Difficulty B");
      }
      break;
    case Event::ConsoleRightDiffToggle:
      if(state)
      {
        if(myOSystem.console().switches().rightDifficultyA())
        {
          myEvent.set(Event::ConsoleRightDiffA, 0);
          myEvent.set(Event::ConsoleRightDiffB, 1);
          myOSystem.frameBuffer().showMessage("Right Difficulty B");
        }
        else
        {
          myEvent.set(Event::ConsoleRightDiffA, 1);
          myEvent.set(Event::ConsoleRightDiffB, 0);
          myOSystem.frameBuffer().showMessage("Right Difficulty A");
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
  myEvent.set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleConsoleStartupEvents()
{
  bool update = false;
  if(myOSystem.settings().getBool("holdreset"))
  {
    handleEvent(Event::ConsoleReset, 1);
    update = true;
  }
  if(myOSystem.settings().getBool("holdselect"))
  {
    handleEvent(Event::ConsoleSelect, 1);
    update = true;
  }

  const string& holdjoy0 = myOSystem.settings().getString("holdjoy0");
  update = update || holdjoy0 != "";
  if(BSPF::containsIgnoreCase(holdjoy0, "U"))
    handleEvent(Event::JoystickZeroUp, 1);
  if(BSPF::containsIgnoreCase(holdjoy0, "D"))
    handleEvent(Event::JoystickZeroDown, 1);
  if(BSPF::containsIgnoreCase(holdjoy0, "L"))
    handleEvent(Event::JoystickZeroLeft, 1);
  if(BSPF::containsIgnoreCase(holdjoy0, "R"))
    handleEvent(Event::JoystickZeroRight, 1);
  if(BSPF::containsIgnoreCase(holdjoy0, "F"))
    handleEvent(Event::JoystickZeroFire, 1);

  const string& holdjoy1 = myOSystem.settings().getString("holdjoy1");
  update = update || holdjoy1 != "";
  if(BSPF::containsIgnoreCase(holdjoy1, "U"))
    handleEvent(Event::JoystickOneUp, 1);
  if(BSPF::containsIgnoreCase(holdjoy1, "D"))
    handleEvent(Event::JoystickOneDown, 1);
  if(BSPF::containsIgnoreCase(holdjoy1, "L"))
    handleEvent(Event::JoystickOneLeft, 1);
  if(BSPF::containsIgnoreCase(holdjoy1, "R"))
    handleEvent(Event::JoystickOneRight, 1);
  if(BSPF::containsIgnoreCase(holdjoy1, "F"))
    handleEvent(Event::JoystickOneFire, 1);

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
      else if(myState == EventHandlerState::CMDMENU)
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
      listsize = kEmulActionListSize;
      list     = ourEmulActionList;
      break;
    case kMenuMode:
      listsize = kMenuActionListSize;
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

    // There are some keys which are hardcoded.  These should be represented too.
    string prepend = "";
    if(event == Event::Quit)
#ifndef BSPF_MAC_OSX
      prepend = "Ctrl Q";
#else
      prepend = "Cmd Q";
#endif
    else if(event == Event::UINavNext)
      prepend = "TAB";
    else if(event == Event::UINavPrev)
      prepend = "Shift-TAB";
    // else if ...

    if(key == "")
      key = prepend;
    else if(prepend != "")
      key = prepend + ", " + key;

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
    for(int i = 0; i < kComboSize; ++i)
      for(int j = 0; j < kEventsPerCombo; ++j)
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
    if(atoi(key.c_str()) == kComboSize)
    {
      // Fill the combomap table with events for as long as they exist
      int combocount = 0;
      while(buf >> key && combocount < kComboSize)
      {
        // Each event in a comboevent is separated by a comma
        replace(key.begin(), key.end(), ',', ' ');
        istringstream buf2(key);

        int eventcount = 0;
        while(buf2 >> key && eventcount < kEventsPerCombo)
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
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, StellaKey key)
{
  bool mapped = myPKeyHandler->addMapping(event, mode, key);
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
  buf << kComboSize;
  for(int i = 0; i < kComboSize; ++i)
  {
    buf << ":" << myComboTable[i][0];
    for(int j = 1; j < kEventsPerCombo; ++j)
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
      for(uInt32 i = 0; i < kEmulActionListSize; ++i)
        l.push_back(EventHandler::ourEmulActionList[i].action);
      break;
    case kMenuMode:
      for(uInt32 i = 0; i < kMenuActionListSize; ++i)
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
  for(uInt32 i = 0; i < kEmulActionListSize; ++i)
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
    for(uInt32 i = 0; i < kEventsPerCombo; ++i)
    {
      Event::Type e = myComboTable[combo][i];
      for(uInt32 j = 0; j < kEmulActionListSize; ++j)
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
      if(idx >=0 && idx < kEmulActionListSize)
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
      if(idx < 0 || idx >= kEmulActionListSize)
        return Event::NoType;
      else
        return ourEmulActionList[idx].event;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
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
      if(idx < 0 || idx >= kEmulActionListSize)
        return EmptyString;
      else
        return ourEmulActionList[idx].action;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
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
      if(idx < 0 || idx >= kEmulActionListSize)
        return EmptyString;
      else
        return ourEmulActionList[idx].key;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
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
      usemouse = myOSystem.console().controller(Controller::Left).isAnalog() ||
                 myOSystem.console().controller(Controller::Right).isAnalog();
    }

    const string& control = usemouse ?
      myOSystem.console().properties().get(Controller_MouseAxis) : "none";

    myMouseControl = make_unique<MouseControl>(myOSystem.console(), control);
    myMouseControl->next();  // set first available mode
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(EventHandlerState state)
{
  setState(state);
  myOverlay->reStack();
  myOSystem.sound().mute(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
  setState(EventHandlerState::EMULATION);
  myOSystem.sound().mute(false);
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
  // add one extra state if we are in Time Machine mode
  // TODO: maybe remove this state if we leave the menu at this new state
  myOSystem.state().addExtraState("enter Time Machine dialog"); // force new state

  if(numWinds)
    // hande winds and display wind message (numWinds != 0) in time machine dialog
    myOSystem.timeMachine().setEnterWinds(unwind ? numWinds : -numWinds);

  enterMenuMode(EventHandlerState::TIMEMACHINE);
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
      if(myOSystem.console().leftController().type() == Controller::CompuMate)
        myPKeyHandler->useCtrlKey() = false;
      break;

    case EventHandlerState::PAUSE:
      myOSystem.sound().mute(true);
      enableTextEvents(false);
      break;

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
      myEvent.clear();
      break;

    case EventHandlerState::DEBUGGER:
  #ifdef DEBUGGER_SUPPORT
      myOverlay = &myOSystem.debugger();
      enableTextEvents(true);
  #endif
      break;

    case EventHandlerState::NONE:
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourEmulActionList[kEmulActionListSize] = {
  { Event::ConsoleSelect,          "Select",                   "", true  },
  { Event::ConsoleReset,           "Reset",                    "", true  },
  { Event::ConsoleColor,           "Color TV",                 "", true  },
  { Event::ConsoleBlackWhite,      "Black & White TV",         "", true  },
  { Event::ConsoleColorToggle,     "Swap Color / B&W TV",      "", true  },
  { Event::Console7800Pause,       "7800 Pause Key",           "", true  },
  { Event::ConsoleLeftDiffA,       "P0 Difficulty A",          "", true  },
  { Event::ConsoleLeftDiffB,       "P0 Difficulty B",          "", true  },
  { Event::ConsoleLeftDiffToggle,  "P0 Swap Difficulty",       "", true  },
  { Event::ConsoleRightDiffA,      "P1 Difficulty A",          "", true  },
  { Event::ConsoleRightDiffB,      "P1 Difficulty B",          "", true  },
  { Event::ConsoleRightDiffToggle, "P1 Swap Difficulty",       "", true  },
  { Event::SaveState,              "Save State",               "", false },
  { Event::ChangeState,            "Change State",             "", false },
  { Event::LoadState,              "Load State",               "", false },
  { Event::TakeSnapshot,           "Snapshot",                 "", false },
  { Event::Fry,                    "Fry cartridge",            "", false },
  { Event::VolumeDecrease,         "Decrease volume",          "", false },
  { Event::VolumeIncrease,         "Increase volume",          "", false },
  { Event::PauseMode,              "Pause",                    "", false },
  { Event::OptionsMenuMode,        "Enter options menu UI",    "", false },
  { Event::CmdMenuMode,            "Toggle command menu UI",   "", false },
  { Event::TimeMachineMode,        "Toggle time machine UI",   "", false },
  { Event::DebuggerMode,           "Toggle debugger mode",     "", false },
  { Event::LauncherMode,           "Enter ROM launcher",       "", false },
  { Event::Quit,                   "Quit",                     "", false },

  { Event::JoystickZeroUp,      "P0 Joystick Up",              "", true  },
  { Event::JoystickZeroDown,    "P0 Joystick Down",            "", true  },
  { Event::JoystickZeroLeft,    "P0 Joystick Left",            "", true  },
  { Event::JoystickZeroRight,   "P0 Joystick Right",           "", true  },
  { Event::JoystickZeroFire,    "P0 Joystick Fire",            "", true  },
  { Event::JoystickZeroFire5,   "P0 Booster Top Trigger",      "", true  },
  { Event::JoystickZeroFire9,   "P0 Booster Handle Grip",      "", true  },

  { Event::JoystickOneUp,       "P1 Joystick Up",              "", true  },
  { Event::JoystickOneDown,     "P1 Joystick Down",            "", true  },
  { Event::JoystickOneLeft,     "P1 Joystick Left",            "", true  },
  { Event::JoystickOneRight,    "P1 Joystick Right",           "", true  },
  { Event::JoystickOneFire,     "P1 Joystick Fire",            "", true  },
  { Event::JoystickOneFire5,    "P1 Booster Top Trigger",      "", true  },
  { Event::JoystickOneFire9,    "P1 Booster Handle Grip",      "", true  },

  { Event::PaddleZeroAnalog,    "Paddle 0 Analog",             "", true  },
  { Event::PaddleZeroDecrease,  "Paddle 0 Decrease",           "", true  },
  { Event::PaddleZeroIncrease,  "Paddle 0 Increase",           "", true  },
  { Event::PaddleZeroFire,      "Paddle 0 Fire",               "", true  },

  { Event::PaddleOneAnalog,     "Paddle 1 Analog",             "", true  },
  { Event::PaddleOneDecrease,   "Paddle 1 Decrease",           "", true  },
  { Event::PaddleOneIncrease,   "Paddle 1 Increase",           "", true  },
  { Event::PaddleOneFire,       "Paddle 1 Fire",               "", true  },

  { Event::PaddleTwoAnalog,     "Paddle 2 Analog",             "", true  },
  { Event::PaddleTwoDecrease,   "Paddle 2 Decrease",           "", true  },
  { Event::PaddleTwoIncrease,   "Paddle 2 Increase",           "", true  },
  { Event::PaddleTwoFire,       "Paddle 2 Fire",               "", true  },

  { Event::PaddleThreeAnalog,   "Paddle 3 Analog",             "", true  },
  { Event::PaddleThreeDecrease, "Paddle 3 Decrease",           "", true  },
  { Event::PaddleThreeIncrease, "Paddle 3 Increase",           "", true  },
  { Event::PaddleThreeFire,     "Paddle 3 Fire",               "", true  },

  { Event::KeyboardZero1,       "P0 Keyboard 1",               "", true  },
  { Event::KeyboardZero2,       "P0 Keyboard 2",               "", true  },
  { Event::KeyboardZero3,       "P0 Keyboard 3",               "", true  },
  { Event::KeyboardZero4,       "P0 Keyboard 4",               "", true  },
  { Event::KeyboardZero5,       "P0 Keyboard 5",               "", true  },
  { Event::KeyboardZero6,       "P0 Keyboard 6",               "", true  },
  { Event::KeyboardZero7,       "P0 Keyboard 7",               "", true  },
  { Event::KeyboardZero8,       "P0 Keyboard 8",               "", true  },
  { Event::KeyboardZero9,       "P0 Keyboard 9",               "", true  },
  { Event::KeyboardZeroStar,    "P0 Keyboard *",               "", true  },
  { Event::KeyboardZero0,       "P0 Keyboard 0",               "", true  },
  { Event::KeyboardZeroPound,   "P0 Keyboard #",               "", true  },

  { Event::KeyboardOne1,        "P1 Keyboard 1",               "", true  },
  { Event::KeyboardOne2,        "P1 Keyboard 2",               "", true  },
  { Event::KeyboardOne3,        "P1 Keyboard 3",               "", true  },
  { Event::KeyboardOne4,        "P1 Keyboard 4",               "", true  },
  { Event::KeyboardOne5,        "P1 Keyboard 5",               "", true  },
  { Event::KeyboardOne6,        "P1 Keyboard 6",               "", true  },
  { Event::KeyboardOne7,        "P1 Keyboard 7",               "", true  },
  { Event::KeyboardOne8,        "P1 Keyboard 8",               "", true  },
  { Event::KeyboardOne9,        "P1 Keyboard 9",               "", true  },
  { Event::KeyboardOneStar,     "P1 Keyboard *",               "", true  },
  { Event::KeyboardOne0,        "P1 Keyboard 0",               "", true  },
  { Event::KeyboardOnePound,    "P1 Keyboard #",               "", true  },

  { Event::Combo1,              "Combo 1",                     "", false },
  { Event::Combo2,              "Combo 2",                     "", false },
  { Event::Combo3,              "Combo 3",                     "", false },
  { Event::Combo4,              "Combo 4",                     "", false },
  { Event::Combo5,              "Combo 5",                     "", false },
  { Event::Combo6,              "Combo 6",                     "", false },
  { Event::Combo7,              "Combo 7",                     "", false },
  { Event::Combo8,              "Combo 8",                     "", false },
  { Event::Combo9,              "Combo 9",                     "", false },
  { Event::Combo10,             "Combo 10",                    "", false },
  { Event::Combo11,             "Combo 11",                    "", false },
  { Event::Combo12,             "Combo 12",                    "", false },
  { Event::Combo13,             "Combo 13",                    "", false },
  { Event::Combo14,             "Combo 14",                    "", false },
  { Event::Combo15,             "Combo 15",                    "", false },
  { Event::Combo16,             "Combo 16",                    "", false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourMenuActionList[kMenuActionListSize] = {
  { Event::UIUp,        "Move Up",              "", false },
  { Event::UIDown,      "Move Down",            "", false },
  { Event::UILeft,      "Move Left",            "", false },
  { Event::UIRight,     "Move Right",           "", false },

  { Event::UIHome,      "Home",                 "", false },
  { Event::UIEnd,       "End",                  "", false },
  { Event::UIPgUp,      "Page Up",              "", false },
  { Event::UIPgDown,    "Page Down",            "", false },

  { Event::UIOK,        "OK",                   "", false },
  { Event::UICancel,    "Cancel",               "", false },
  { Event::UISelect,    "Select item",          "", false },

  { Event::UINavPrev,   "Previous object",      "", false },
  { Event::UINavNext,   "Next object",          "", false },

  { Event::UIPrevDir,   "Parent directory",     "", false }
};
