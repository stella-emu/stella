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

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Console.hxx"
#include "Cart.hxx"
#include "Switches.hxx"
#include "RewindManager.hxx"

#include "StateManager.hxx"

// #define MOVIE_HEADER "03030000movie"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::StateManager(OSystem& osystem)
  : myOSystem{osystem},
    myRewindManager{std::make_unique<RewindManager>(osystem, *this)}
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::~StateManager() = default;

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::toggleRecordMode()
{
  if(myActiveMode != kMovieRecordMode)  // Turn on movie record mode
  {
    myActiveMode = kOffMode;

    string moviefile = /*myOSystem.baseDir() +*/ "test.inp";
    if(myMovieWriter.isOpen())
      myMovieWriter.close();
    if(!myMovieWriter.open(moviefile))
      return false;

    // Prepend the ROM md5 so this state file only works with that ROM
    myMovieWriter.putString(myOSystem.console().properties().get(Cartridge_MD5));

    if(!myOSystem.console().save(myMovieWriter))
      return false;

    // Save controller types for this ROM
    // We need to check this, since some controllers save more state than
    // normal, and those states files wouldn't be compatible with normal
    // controllers.
    myMovieWriter.putString(
      myOSystem.console().controller(Controller::Jack::Left).name());
    myMovieWriter.putString(
      myOSystem.console().controller(Controller::Jack::Right).name());

    // If we get this far, we're really in movie record mode
    myActiveMode = kMovieRecordMode;
  }
  else  // Turn off movie record mode
  {
    myActiveMode = kOffMode;
    myMovieWriter.close();
    return false;
  }

  return myActiveMode == kMovieRecordMode;
////////////////////////////////////////////////////////
// FIXME - For now, I'm going to use this to activate movie playback
  // Close the writer, since we're about to re-open in read mode
  myMovieWriter.close();

  if(myActiveMode != kMoviePlaybackMode)  // Turn on movie playback mode
  {
    myActiveMode = kOffMode;

    string moviefile = /*myOSystem.baseDir() + */ "test.inp";
    if(myMovieReader.isOpen())
      myMovieReader.close();
    if(!myMovieReader.open(moviefile))
      return false;

    // Check the ROM md5
    if(myMovieReader.getString() !=
       myOSystem.console().properties().get(Cartridge_MD5))
      return false;

    if(!myOSystem.console().load(myMovieReader))
      return false;

    // Check controller types
    const string& left  = myMovieReader.getString();
    const string& right = myMovieReader.getString();

    if(left != myOSystem.console().controller(Controller::Jack::Left).name() ||
       right != myOSystem.console().controller(Controller::Jack::Right).name())
      return false;

    // If we get this far, we're really in movie record mode
    myActiveMode = kMoviePlaybackMode;
  }
  else  // Turn off movie playback mode
  {
    myActiveMode = kOffMode;
    myMovieReader.close();
    return false;
  }
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::toggleTimeMachine()
{
  const bool devSettings = myOSystem.settings().getBool("dev.settings");

  myActiveMode = (myActiveMode == Mode::TimeMachine)
    ? Mode::Off
    : Mode::TimeMachine;
  const bool enabled = (myActiveMode == Mode::TimeMachine);

  myOSystem.frameBuffer().showTextMessage(
    enabled ? "Time Machine enabled" : "Time Machine disabled");

  myOSystem.settings().setValue(
    devSettings ? "dev.timemachine" : "plr.timemachine", enabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::addExtraState(string_view message)
{
  if(myActiveMode == Mode::TimeMachine)
    return myRewindManager->addState(message);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::rewindStates(uInt32 numStates)
{
  return myRewindManager->rewindStates(numStates);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::unwindStates(uInt32 numStates)
{
  return myRewindManager->unwindStates(numStates);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::windStates(uInt32 numStates, bool unwind)
{
  return myRewindManager->windStates(numStates, unwind);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::update()
{
  if(myActiveMode == Mode::TimeMachine)
    myRewindManager->addState("Time Machine", true);

#if 0
  switch(myActiveMode)
  {
    case Mode::TimeMachine:
      myRewindManager->addState("Time Machine", true);
      break;

    case Mode::MovieRecord:
      myOSystem.console().controller(Controller::Jack::Left).save(myMovieWriter);
      myOSystem.console().controller(Controller::Jack::Right).save(myMovieWriter);
      myOSystem.console().switches().save(myMovieWriter);
      break;

    case Mode::MoviePlayback:
      myOSystem.console().controller(Controller::Jack::Left).load(myMovieReader);
      myOSystem.console().controller(Controller::Jack::Right).load(myMovieReader);
      myOSystem.console().switches().load(myMovieReader);
      break;
    default:
      break;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::loadState(int slot)
{
  if(!myOSystem.hasConsole())
    return;

  if(slot < 0)
    slot = myCurrentSlot;

  const auto path = std::format("{}{}.st{}",
    myOSystem.stateDir().getPath(),
    myOSystem.console().properties().get(PropType::Cart_Name),
    slot);

  Serializer in(path, Serializer::FileMode::ReadOnly);
  if(!in)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Can't open/load from state file {}", slot));
    return;
  }

  try
  {
    // First test if we have a valid header
    // If so, do a complete state load using the Console
    const auto header = in.getString();
    if(header != STATE_HEADER)
      myOSystem.frameBuffer().showTextMessage(
        std::format("Incompatible state {} file", slot));
    else if(myOSystem.console().load(in))
      myOSystem.frameBuffer().showTextMessage(
        std::format("State {} loaded", slot));
    else
      myOSystem.frameBuffer().showTextMessage(
        std::format("Invalid data in state {} file", slot));
  }
  catch(...)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Invalid data in state {} file", slot));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::saveState(int slot)
{
  if(!myOSystem.hasConsole())
    return;

  if(slot < 0)
    slot = myCurrentSlot;

  const auto path = std::format("{}{}.st{}",
    myOSystem.stateDir().getPath(),
    myOSystem.console().properties().get(PropType::Cart_Name),
    slot);

  Serializer out(path, Serializer::FileMode::ReadWriteTrunc);
  if(!out)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Can't open/save to state file {}", slot));
    return;
  }

  try
  {
    // Add header so that if the state format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(STATE_HEADER);
  }
  catch(...)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Error saving state {}", slot));
    return;
  }

  // Do a complete state save using the Console
  if(myOSystem.console().save(out))
  {
    if(myOSystem.settings().getBool("autoslot"))
    {
      myCurrentSlot = (slot + 1) % 10;
      myOSystem.frameBuffer().showTextMessage(
        std::format("State {} saved, switching to slot {}", slot, myCurrentSlot));
    }
    else
    {
      myOSystem.frameBuffer().showTextMessage(
        std::format("State {} saved", slot));
    }
  }
  else
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Error saving state {}", slot));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::changeState(int direction)
{
  myCurrentSlot = BSPF::clampw(myCurrentSlot + direction, 0, 9);

  myOSystem.frameBuffer().showTextMessage(
    direction != 0
      ? std::format("Changed to state slot {}", myCurrentSlot)
      : std::format("State slot {}", myCurrentSlot));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::toggleAutoSlot()
{
  const bool autoSlot = !myOSystem.settings().getBool("autoslot");

  myOSystem.frameBuffer().showTextMessage(
    std::format("Automatic slot change {}", autoSlot ? "enabled" : "disabled"));

  myOSystem.settings().setValue("autoslot", autoSlot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::loadState(Serializer& in)
{
  if(!myOSystem.hasConsole() || !in)
    return false;

  try
  {
    // First test if we have a valid header
    // If so, do a complete state load using the Console
    return in.getString() == STATE_HEADER && myOSystem.console().load(in);
  }
  catch(...)
  {
    cerr << "ERROR: StateManager::loadState(Serializer&)\n";
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::saveState(Serializer& out)
{
  if(!myOSystem.hasConsole() || !out)
    return false;

  try
  {
    // Add header so that if the state format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(STATE_HEADER);

    // Do a complete state save using the Console
    return myOSystem.console().save(out);
  }
  catch(...)
  {
    cerr << "ERROR: StateManager::saveState(Serializer&)\n";
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::reset()
{
  myCurrentSlot = 0;
  myRewindManager->clear();

  const bool devSettings = myOSystem.settings().getBool("dev.settings");
  const bool timeMachine = myOSystem.settings().getBool(
    devSettings ? "dev.timemachine" : "plr.timemachine");

  myActiveMode = timeMachine ? Mode::TimeMachine : Mode::Off;

#if 0
  myCurrentSlot = 0;

  switch(myActiveMode)
  {
    case kMovieRecordMode:
      myMovieWriter.close();
      break;

    case kMoviePlaybackMode:
      myMovieReader.close();
      break;

    default:
      break;
  }
  myActiveMode = kOffMode;
#endif
}
